/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2023  mhahnFr
 *
 * This file is part of the LeakSanitizer. This library is free software:
 * you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this library, see the file LICENSE.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <cxxabi.h>
#include <exception>
#include <sstream>
#include <typeinfo>

#include "exceptionHandler.hpp"

#include "crash.hpp"
#include "../lsanMisc.hpp"

#include "../../CallstackLibrary/include/callstack_exception.hpp"

namespace lsan {
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

[[ noreturn ]] static inline void handleException(std::exception & exception) noexcept {
    std::stringstream stream;
    stream << "Uncaught exception of type " << demangle(typeid(exception).name()) << ": \"" << exception.what() << "\"";
    
    crashForce(stream.str());
}

[[ noreturn ]] static inline void handleException(lcs::exception & exception) noexcept {
    exception.setPrintStacktrace(false);
    
    std::stringstream stream;
    stream << "Uncaught exception of type " << exception.what();
    
    crashForce(stream.str());
}

[[ noreturn ]] void exceptionHandler() noexcept {
    setIgnoreMalloc(true);
    try {
        std::rethrow_exception(std::current_exception());
    } catch (lcs::exception & exception) {
        handleException(exception);
    } catch (std::exception & exception) {
        handleException(exception);
    } catch (...) {
        // Unknown type...
    }
    crashForce("Unknown uncaught exception");
}
}
