/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2023 - 2024  mhahnFr
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

#include "../LeakSani.hpp"
#include "../lsanMisc.hpp"
#include "../formatter.hpp"
#include "../callstacks/callstackHelper.hpp"

namespace lsan {
/**
 * Prints the given message and the given callstack.
 *
 * @param message the message to be printed
 * @param callstack the callstack to be printed
 * @tparam Warning whether to use warning formatting
 */
template<bool Warning>
static inline void printer(const std::string & message, lcs::callstack & callstack) {
    using formatter::Style;
    
    const auto colour = Warning ? Style::MAGENTA : Style::RED;
    
    std::cerr << formatter::format<Style::BOLD, colour>((Warning ? "Warning: " : "") + message + "!") << std::endl;
    callstackHelper::format(callstack, std::cerr);
    std::cerr << std::endl;
    
    if (!Warning) {
        getInstance().maybeHintCallstackSize(std::cerr);
        std::cerr << maybeHintRelativePaths;
    }
}

/**
 * Prints the given message and the given callstack.
 *
 * @param message the message to be printed
 * @param callstack the callstack to be printed
 * @tparam Warning whether to use warning formatting
 */
template<bool Warning>
static inline void printer(const std::string & message, lcs::callstack && callstack) {
    printer<Warning>(message, callstack);
}

/**
 * Prints the given message, the file with line number and the given callstack.
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
    using formatter::Style;
    
    const auto colour = Warning ? Style::MAGENTA : Style::RED;
    
    std::cerr << formatter::get<Style::BOLD>
              << formatter::format<colour>((Warning ? "Warning: " : "") + message) << ", at "
              << formatter::get<Style::UNDERLINED> << file << ":" << line
              << formatter::clear<Style::BOLD, Style::UNDERLINED>
              << std::endl;
    callstackHelper::format(callstack, std::cerr);
    std::cerr << std::endl;
}

/**
 * Prints the given message, the allocation information found in the
 * optionally provided allocation record and the given callstack.
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
    using formatter::Style;
    
    printer<Warning>(message, callstack);
    
    if (info.has_value()) {
        const auto   colour = Warning ? Style::MAGENTA : Style::RED;
        const auto & record = info.value().get();
        
        std::cerr << formatter::format<Style::ITALIC, colour>("Previously allocated here:") << std::endl;
        record.printCreatedCallstack(std::cerr);
        std::cerr << std::endl;
        if (record.getDeletedCallstack().has_value()) {
            std::cerr << std::endl << formatter::format<Style::ITALIC, colour>("Previously freed here:") << std::endl;
            record.printDeletedCallstack(std::cerr);
            std::cerr << std::endl;
        }
    }
}

/**
 * Executes the given function with a callstack up to the given omit address
 * if the generated callstack is user relevant.
 *
 * @param omitAddress the callstack delimiter
 * @param function the function to be executed
 * @tparam F the function's type - it will get a lcs::callstack as the only argument
 */
template<typename F>
static inline void withCallstack(void * omitAddress, const F & function) {
    auto callstack = lcs::callstack(omitAddress);
    if (callstackHelper::getCallstackType(callstack) == callstackHelper::CallstackType::USER) {
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
        std::abort();
    });
}

void crashForce(const std::string & message, void * omitAddress) {
    printer<false>(message, lcs::callstack(omitAddress));
    std::abort();
}

void crash(const std::string & message, const std::string & file, int line, void * omitAddress) {
    withCallstack(omitAddress, [&] (auto & callstack) {
        printer<false>(message, file, line, callstack);
        std::abort();
    });
}

void crash(const std::string &                                     message,
           std::optional<std::reference_wrapper<const MallocInfo>> info,
           void *                                                  omitAddress) {
    withCallstack(omitAddress, [&] (auto & callstack) {
        printer<false>(message, info, callstack);
        std::abort();
    });
}
}
