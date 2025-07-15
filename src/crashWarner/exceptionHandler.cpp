/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2023 - 2025  mhahnFr
 *
 * This file is part of the LeakSanitizer.
 *
 * The LeakSanitizer is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * The LeakSanitizer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with the
 * LeakSanitizer, see the file LICENSE.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <cxxabi.h>
#include <exception>
#include <sstream>
#include <typeinfo>

#include <callstack_exception.hpp>

#include "crash.hpp"
#include "exceptionHandler.hpp"

#include "../lsanMisc.hpp"
#include "../utils.hpp"

namespace lsan {
/**
 * Demangles the given C string.
 *
 * @param string the string to be demangled
 * @return the possibly demangled string
 */
static inline auto demangle(const char * string) noexcept -> std::string {
    int status;
    const char * result = abi::__cxa_demangle(string, nullptr, nullptr, &status);
    if (result == nullptr || result == string) {
        return string;
    }
    std::string toReturn = result;
    std::free(const_cast<char *>(result));
    return toReturn;
}

/**
 * Handles the given standard exception.
 *
 * @param exception the exception to be handled
 */
[[ noreturn ]] static inline void handleException(std::exception & exception) noexcept {
    std::stringstream stream;
    stream << "Uncaught exception of type " << demangle(typeid(exception).name()) << ": \"" << exception.what() << "\"";
    
    crashForce(stream.str());
}

/**
 * Handles the given exception from the CallstackLibrary.
 *
 * @param exception the exception to be handled
 */
[[ noreturn ]] static inline void handleException(lcs::exception & exception) noexcept {
    exception.setPrintStacktrace(false);
    
    std::stringstream stream;
    stream << "Uncaught exception of type " << exception.what();
    
    crashForce(stream.str());
}

[[ noreturn ]] void exceptionHandler() noexcept {
    getTracker().ignoreMalloc = true;

    if (const auto exception = std::current_exception()) {
        try {
            std::rethrow_exception(exception);
        } catch (lcs::exception& e) {
            handleException(e);
        } catch (std::exception& e) {
            handleException(e);
        } catch (...) {
            crashForce("Unknown uncaught exception");
        }
    }
    crashForce("Terminating without active exception");
}

[[ noreturn ]] void mhExceptionHandler() noexcept {
    getTracker().ignoreMalloc = true;

    if (LOAD_FUNC(void*(*)(), tryCatch_getException); tryCatch_getException != nullptr) {
        if (const auto exception = tryCatch_getException(); exception != nullptr) {
            const auto exceptionType = *reinterpret_cast<const char**>(uintptr_t(exception) - sizeof(char*));
            crashForce("Uncaught exception of type " + std::string(exceptionType));
        }
    }
    crashForce("Terminating with unknown exception");
}
}
