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

#include "crash.hpp"
#include "MallocInfo.hpp"
#include "Formatter.hpp"
#include <iostream>

[[ noreturn ]] static inline void crashShared(void * omitAddress = __builtin_return_address(0)) {
    std::cerr << std::endl;
    void * callstack[128];
    int frames = MallocInfo::createCallstack(callstack, 128, omitAddress);
    MallocInfo::printCallstack(lcs::callstack(callstack, static_cast<size_t>(frames)), std::cerr);
    std::cerr << std::endl;
    std::terminate();
}

[[ noreturn ]] void crash(const std::string & reason, const char * file, int line, void * omitAddress) {
    using Formatter::Style;
    std::cerr << Formatter::get(Style::BOLD)
              << Formatter::get(Style::RED) << reason << Formatter::clear(Style::RED)
              << ", at "
              << Formatter::get(Style::UNDERLINED) << file << ":" << line
              << Formatter::clear(Style::UNDERLINED) << Formatter::clear(Style::BOLD);
    crashShared(omitAddress);
}

[[ noreturn ]] void crash(const std::string & reason, void * omitAddress) {
    using Formatter::Style;
    std::cerr << Formatter::get(Style::BOLD) << Formatter::get(Style::RED)
              << reason << "!"
              << Formatter::clear(Style::BOLD) << Formatter::clear(Style::RED);
    crashShared(omitAddress);
}
