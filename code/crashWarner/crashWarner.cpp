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

template<bool Warning>
static inline void printer(const std::string & message, void * omitAddress) {
    using Formatter::Style;
    
    const auto colour = Warning ? Style::MAGENTA : Style::RED;
    
    std::cerr << Formatter::get(Style::BOLD) << Formatter::get(colour)
              << (Warning ? "Warning: " : "") << message << "!"
              << Formatter::clear(Style::BOLD) << Formatter::clear(colour)
              << std::endl;
    MallocInfo::printCallstack(lcs::callstack(omitAddress), std::cerr);
    std::cerr << std::endl;
}

template<bool Warning>
static inline void printer(const std::string & message, const std::string & file, int line, void * omitAddress) {
    using Formatter::Style;
    
    const auto colour = Warning ? Style::MAGENTA : Style::RED;
    
    std::cerr << Formatter::get(Style::BOLD) << Formatter::get(colour)
              << (Warning ? "Warning: " : "") << message << Formatter::clear(colour) << ", at "
              << Formatter::get(Style::UNDERLINED) << file << ":" << line
              << Formatter::clear(Style::BOLD) << Formatter::clear(Style::UNDERLINED)
              << std::endl;
    MallocInfo::printCallstack(lcs::callstack(omitAddress), std::cerr);
    std::cerr << std::endl;
}

template<bool Warning>
static inline void printer(const std::string & message, std::optional<std::reference_wrapper<const MallocInfo>> info, void * omitAddress) {
    using Formatter::Style;
    
    const auto colour = Warning ? Style::MAGENTA : Style::RED;
    
    std::cerr << Formatter::get(Style::BOLD) << Formatter::get(colour)
              << (Warning ? "Warning: " : "") << message << "!"
              << Formatter::clear(Style::BOLD) << Formatter::clear(colour)
              << std::endl;
    MallocInfo::printCallstack(lcs::callstack(omitAddress), std::cerr);
    std::cerr << std::endl;
    
    if (info.has_value()) {
        const auto & record = info.value().get();
        std::cerr << Formatter::get(Style::ITALIC) << Formatter::get(colour)
                  << "Previously allocated here:"
                  << Formatter::clear(Style::ITALIC) << Formatter::clear(colour)
                  << std::endl;
        record.printCreatedCallstack(std::cerr);
        std::cerr << std::endl;
        if (record.getDeletedCallstack().has_value()) {
            std::cerr << std::endl
                      << Formatter::get(Style::ITALIC) << Formatter::get(colour)
                      << "Previously freed here:"
                      << Formatter::clear(Style::ITALIC) << Formatter::clear(colour)
                      << std::endl;
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
