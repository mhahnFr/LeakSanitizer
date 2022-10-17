/*
 * LeakSanitizer - A small library showing informations about lost memory.
 *
 * Copyright (C) 2022  mhahnFr
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
#include "warn.hpp"
#include "MallocInfo.hpp"
#include "LeakSani.hpp"
#include "Formatter.hpp"

static inline void warnShared(void * omitAddress = __builtin_return_address(0)) {
    std::cerr << std::endl;
    MallocInfo::printCallstack(lcs::callstack(omitAddress), std::cerr);
    std::cerr << std::endl;
}

void warn(const std::string & message, const char * file, int line, void * omitAddress) {
    using Formatter::Style;
    std::cerr << Formatter::get(Style::BOLD) << Formatter::get(Style::MAGENTA)
              << "Warning: " << message << Formatter::clear(Style::MAGENTA) << ", at "
              << Formatter::get(Style::UNDERLINED) << file << ":" << line
              << Formatter::clear(Style::BOLD) << Formatter::clear(Style::UNDERLINED);
    warnShared(omitAddress);
}

void warn(const std::string & message, void * omitAddress) {
    using Formatter::Style;
    std::cerr << Formatter::get(Style::BOLD) << Formatter::get(Style::MAGENTA)
              << "Warning: " << message << "!"
              << Formatter::clear(Style::BOLD) << Formatter::clear(Style::MAGENTA);
    warnShared(omitAddress);
}
