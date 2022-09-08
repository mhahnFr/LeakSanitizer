/*
 * LeakSanitizer - A small library showing informations about lost memory.
 *
 * Copyright (C) 2022  mhahnFr and contributors
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

#ifndef Formatter_hpp
#define Formatter_hpp

#include <string>

namespace Formatter {
    enum class Style {
        GREEN, RED, MAGENTA, ITALIC, UNDERLINED, GREYED, BOLD, BAR_FILLED, BAR_EMPTY
    };
    
    auto get(Style)   -> std::string;
    auto clear(Style) -> std::string;
    auto clearAll()   -> std::string;
}

#endif /* Formatter_hpp */
