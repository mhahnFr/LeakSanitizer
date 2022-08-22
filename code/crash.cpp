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
#include <iostream>

[[ noreturn ]] static void crashShared(int omitCaller = 2) {
    std::cerr << std::endl;
    void * callstack[128];
    int frames = MallocInfo::createCallstack(callstack, 128, omitCaller);
    MallocInfo::printCallstack(callstack, frames, std::cerr);
    std::cerr << std::endl;
    std::terminate();
}

[[ noreturn ]] void crash(const std::string & reason, const char * file, int line, int omitCaller) {
    std::cerr << "\033[1;31m" << reason << "\033[39m, at \033[4m" << file << ":" << line << "\033[24;22m";
    crashShared(omitCaller);
}

[[ noreturn ]] void crash(const std::string & reason, int omitCaller) {
    std::cerr << "\033[1;31m" << reason << "!\033[39;22m";
    crashShared(omitCaller);
}