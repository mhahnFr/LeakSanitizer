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

#include "exceptionHandler.hpp"

#include <callstack_exception.hpp>
#include <cxxabi.h>
#include <exception>
#include <sstream>
#include <typeinfo>

#ifdef __APPLE__
# include <CoreFoundation/CFNumber.h>
# include <CoreFoundation/CFString.h>
# include <objc/runtime.h>

# define OBJC_SUPPORT_EXTRA 1
# include "../objcSupport.hpp"
# include "../macos/bundle.hpp"
#endif

#include "crashForce.hpp"
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
    
    crashWarner::crashForce(stream.str());
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
    
    crashWarner::crashForce(stream.str());
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
            crashWarner::crashForce("Unknown uncaught exception");
        }
    }
    crashWarner::crashForce("Terminating without active exception");
}

[[ noreturn ]] void mhExceptionHandler() noexcept {
    getTracker().ignoreMalloc = true;

    if (LOAD_FUNC(void*(*)(), tryCatch_getException); tryCatch_getException != nullptr) {
        [[likely]] if (const auto exception = tryCatch_getException(); exception != nullptr) {
            const auto exceptionType = *reinterpret_cast<const char**>(uintptr_t(exception) - sizeof(char*));
            crashWarner::crashForce("Uncaught exception of type " + std::string(exceptionType));
        }
    }
    crashWarner::crashForce("Terminating with unknown exception");
}

#ifdef __APPLE__
void objcExceptionHandler(id exception) noexcept {
    auto stream = std::ostringstream();
    const auto cls = object_getClass(exception);
    stream << "Uncaught exception of type " << class_getName(cls);

    std::optional<lcs::callstack> callstack;
    if (const auto nsExCls = objc_getClass("NSException"); _1(exception, isKindOfClass:, nsExCls) || cls == nsExCls) {
        if (const auto name = macos::bundle::convertCFString(CFStringRef(_1(exception, name)))) {
            stream << ", name: \"" << *name << "\"";
        }
        if (const auto reason = macos::bundle::convertCFString(CFStringRef(_1(exception, reason)))) {
            stream << ", reason: \"" << *reason << "\"";
        }
        if (const auto cs = CFArrayRef(_1(exception, callStackReturnAddresses)); cs != nil) {
            callstack = lcs::callstack(false);
            const auto size = CFArrayGetCount(cs);
            for (CFIndex i = 0; i < size; ++i) {
                const auto value = CFNumberRef(CFArrayGetValueAtIndex(cs, i));
                unsigned long number;
                CFNumberGetValue(value, CFNumberGetType(value), &number);
                (*callstack)->backtrace[i] = reinterpret_cast<void*>(number);
            }
            (*callstack)->backtraceSize = static_cast<unsigned long>(size);
        }
    }
    if (callstack) {
        crashWarner::crashForce(stream.str(), std::nullopt, std::move(*callstack));
    }
    crashWarner::crashForce(stream.str());
}
#endif
}
