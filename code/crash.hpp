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

#ifndef crash_hpp
#define crash_hpp

#include <string>

/**
 * @brief Prints the given message as an error and terminates the program.
 *
 * A callstack is created and printed along with the file and line.
 *
 * @param message the message to display
 * @param file the filename
 * @param line the line inside the file
 * @param omitAddress the return address upon which function calls are ignored
 */
[[ noreturn ]] void crash(const std::string & message, const char * file, int line, void * omitAddress = __builtin_return_address(0));
/**
 * @brief Prints the given message as an error and terminates the program.
 *
 * A callstack is created and printed.
 *
 * @param message the message to display
 * @param omitAddress the return address upon which function calls are ignored
 */
[[ noreturn ]] void crash(const std::string & message, void * omitAddress = __builtin_return_address(0));

#endif /* crash_hpp */
