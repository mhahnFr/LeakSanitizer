/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2024  mhahnFr
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

#ifndef utils_hpp
#define utils_hpp

#include <sstream>

namespace lsan::utils {
/**
 * Converts the given pointer to a string.
 *
 * @param pointer the pointer to create a string representation
 * @return the string representation
 */
static inline auto toString(const void* pointer) -> std::string {
    auto stream = std::ostringstream();
    stream << pointer;
    return stream.str();
}
}

#endif /* utils_hpp */
