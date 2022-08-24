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

#include "Formatter.hpp"
#include "../include/lsan_internals.h"

std::string Formatter::get(Style style) {
    if (!__lsan_printFormatted) {
        return "";
    }
    switch (style) {
        case Style::BOLD:       return "\033[1m";
        case Style::GREEN:      return "\033[32m";
        case Style::GREYED:     return "\033[2m";
        case Style::ITALIC:     return "\033[3m";
        case Style::MAGENTA:    return "\033[95m";
        case Style::RED:        return "\033[31m";
        case Style::UNDERLINED: return "\033[4m";
        default:
            return "";
    }
}

std::string Formatter::clear(Style style) {
    if (!__lsan_printFormatted) {
        return "";
    }
    switch (style) {
        case Style::RED:
        case Style::GREEN:
        case Style::MAGENTA:    return "\033[39m";
            
        case Style::BOLD:
        case Style::GREYED:     return "\033[22m";
            
        case Style::ITALIC:     return "\033[23m";
            
        case Style::UNDERLINED: return "\033[24m";
            
        default:
            return "";
    }
}

std::string Formatter::clearAll() {
    return "\033[0m";
}
