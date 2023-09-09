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

#ifndef warn_hpp
#define warn_hpp

#include <optional>
#include <string>
#include <utility>

#include "../MallocInfo.hpp"

void warn(const std::string & message, const std::string & file, int line, void * omitAddress = __builtin_return_address(0));
void warn(const std::string & message, void * omitAddress = __builtin_return_address(0));
void warn(const std::string & message, std::optional<std::reference_wrapper<const MallocInfo>> info, void * omitAddress = __builtin_return_address(0));

#endif /* warn_hpp */
