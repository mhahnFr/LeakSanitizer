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

#include "../Formatter.hpp"

/**
 * Prints the given message and a callstack up to the provided omit address.
 *
 * @param message the message to be printed
 * @param omitAddress the address beyond which frames are omitted in the callstack
 * @tparam Warning whether to use warning formatting
 */
template<bool Warning>
static inline void printer(const std::string & message, void * omitAddress) {
    using Formatter::Style;
    
    const auto colour = Warning ? Style::MAGENTA : Style::RED;

    std::cerr << Formatter::format<Style::BOLD, colour>((Warning ? "Warning: " : "") + message + "!") << std::endl;
    MallocInfo::printCallstack(lcs::callstack(omitAddress), std::cerr);
    std::cerr << std::endl;
}

/**
 * Prints the given message, the file with line number and a callstack up to+
 * the provided omit address.
 *
 * @param message the message to be printed
 * @param file the file name
 * @param line the line number
 * @param omitAddress the address beyond which frames are omitted in the callstack
 * @tparam Warning whether to use warning formatting
 */
template<bool Warning>
static inline void printer(const std::string & message,
                           const std::string & file,
                           const int           line,
                           const void *        omitAddress) {
    using Formatter::Style;
    
    const auto colour = Warning ? Style::MAGENTA : Style::RED;
    
    std::cerr << Formatter::get<Style::BOLD>
              << Formatter::format<colour>((Warning ? "Warning: " : "") + message) << ", at "
              << Formatter::get<Style::UNDERLINED> << file << ":" << line
              << Formatter::clear<Style::BOLD, Style::UNDERLINED>
              << std::endl;
    MallocInfo::printCallstack(lcs::callstack(omitAddress), std::cerr);
    std::cerr << std::endl;
}

/**
 * Prints the given message, the allocation information found in the
 * optionally provided allocation record and a callstack up to the given
 * omit address.
 *
 * @param message the message to be printed
 * @param info the optional allocation record
 * @param omitAddress the address beyond which frames are omitted in the callstack
 * @tparam Warning whether to use warning formatting
 */
template<bool Warning>
static inline void printer(const std::string & message,
                           std::optional<std::reference_wrapper<const MallocInfo>> info,
                           void * omitAddress) {
    using Formatter::Style;
    
    printer<Warning>(message, omitAddress);
    
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

void warn(const std::string & message, void * omitAddress) {
    printer<true>(message, omitAddress);
}

void warn(const std::string & message, const std::string & file, int line, void * omitAddress) {
    printer<true>(message, file, line, omitAddress);
}

void warn(const std::string & message, std::optional<std::reference_wrapper<const MallocInfo>> info, void * omitAddress) {
    printer<true>(message, info, omitAddress);
}

[[ noreturn ]] void crash(const std::string & message, void * omitAddress) {
    printer<false>(message, omitAddress);
    std::terminate();
}

[[ noreturn ]] void crash(const std::string & message, const std::string & file, int line, void * omitAddress) {
    printer<false>(message, file, line, omitAddress);
    std::terminate();
}

[[ noreturn ]] void crash(const std::string & message, std::optional<std::reference_wrapper<const MallocInfo>> info, void * omitAddress) {
    printer<false>(message, info, omitAddress);
    std::terminate();
}
