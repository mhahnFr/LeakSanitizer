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

#include <iostream>

#include "crash.hpp"
#include "warn.hpp"
#include "crashWarner.h"

#include "../Formatter.hpp"
#include "../callstacks/callstackHelper.hpp"

/**
 * Prints the given message and a callstack up to the provided omit address.
 *
 * @param message the message to be printed
 * @param callstack the callstack to be printed
 * @tparam Warning whether to use warning formatting
 */
template<bool Warning>
static inline void printer(const std::string & message, lcs::callstack & callstack) {
    using Formatter::Style;
    
    const auto colour = Warning ? Style::MAGENTA : Style::RED;

    std::cerr << Formatter::format<Style::BOLD, colour>((Warning ? "Warning: " : "") + message + "!") << std::endl;
    callstackHelper::format(callstack, std::cerr);
    std::cerr << std::endl;
}

/**
 * Prints the given message, the file with line number and a callstack up to+
 * the provided omit address.
 *
 * @param message the message to be printed
 * @param file the file name
 * @param line the line number
 * @param callstack the callstack to be printed
 * @tparam Warning whether to use warning formatting
 */
template<bool Warning>
static inline void printer(const std::string & message,
                           const std::string & file,
                           const int           line,
                           lcs::callstack &    callstack) {
    using Formatter::Style;
    
    const auto colour = Warning ? Style::MAGENTA : Style::RED;
    
    std::cerr << Formatter::get<Style::BOLD>
              << Formatter::format<colour>((Warning ? "Warning: " : "") + message) << ", at "
              << Formatter::get<Style::UNDERLINED> << file << ":" << line
              << Formatter::clear<Style::BOLD, Style::UNDERLINED>
              << std::endl;
    callstackHelper::format(callstack, std::cerr);
    std::cerr << std::endl;
}

/**
 * Prints the given message, the allocation information found in the
 * optionally provided allocation record and a callstack up to the given
 * omit address.
 *
 * @param message the message to be printed
 * @param info the optional allocation record
 * @param callstack the callstack to be printed
 * @tparam Warning whether to use warning formatting
 */
template<bool Warning>
static inline void printer(const std::string &                                     message,
                           std::optional<std::reference_wrapper<const MallocInfo>> info,
                           lcs::callstack &                                        callstack) {
    using Formatter::Style;
    
    printer<Warning>(message, callstack);
    
    if (info.has_value()) {
        const auto   colour = Warning ? Style::MAGENTA : Style::RED;
        const auto & record = info.value().get();
        
        std::cerr << Formatter::format<Style::ITALIC, colour>("Previously allocated here:") << std::endl;
        record.printCreatedCallstack(std::cerr);
        std::cerr << std::endl;
        if (record.getDeletedCallstack().has_value()) {
            std::cerr << std::endl << Formatter::format<Style::ITALIC, colour>("Previously freed here:") << std::endl;
            record.printDeletedCallstack(std::cerr);
            std::cerr << std::endl;
        }
    }
}

template<typename F>
static inline void withCallstack(void * omitAddress, const F & function) {
    auto callstack = lcs::callstack(omitAddress);
    if (!callstackHelper::originatesInFirstParty(callstack)) {
        function(callstack);
    }
}

void warn(const std::string & message, void * omitAddress) {
    withCallstack(omitAddress, [&] (auto & callstack) {
        printer<true>(message, callstack);
    });
}

void warn(const std::string & message, const std::string & file, int line, void * omitAddress) {
    withCallstack(omitAddress, [&] (auto & callstack) {
        printer<true>(message, file, line, callstack);
    });
}

void warn(const std::string &                                     message,
          std::optional<std::reference_wrapper<const MallocInfo>> info,
          void *                                                  omitAddress) {
    withCallstack(omitAddress, [&] (auto & callstack) {
        printer<true>(message, info, callstack);
    });
}

void crash(const std::string & message, void * omitAddress) {
    withCallstack(omitAddress, [&] (auto & callstack) {
        printer<false>(message, callstack);
        std::terminate();
    });
}

void crash(const std::string & message, const std::string & file, int line, void * omitAddress) {
    withCallstack(omitAddress, [&] (auto & callstack) {
        printer<false>(message, file, line, callstack);
        std::terminate();
    });
}

void crash(const std::string &                                     message,
           std::optional<std::reference_wrapper<const MallocInfo>> info,
           void *                                                  omitAddress) {
    withCallstack(omitAddress, [&] (auto & callstack) {
        printer<false>(message, info, callstack);
        std::terminate();
    });
}

void __lsan_warn(const char * message) {
    warn(message, __builtin_return_address(0));
}

void __lsan_crash(const char * message) {
    crash(message, __builtin_return_address(0));
    std::terminate();
}
