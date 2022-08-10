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

#include <cmath>
#include <iomanip>
#include "bytePrinter.hpp"
#include "include/lsan_internals.h"

std::string bytesToString(size_t amount) {
    std::stringstream s;
    if (!__lsan_humanPrint) {
        s << amount << " B";
    } else {
        const std::string sizes[] {"EiB", "PiB", "TiB", "GiB", "MiB", "KiB", "B"};
        size_t multiplier = pow(1024, 6);
        for (size_t i = 0; i < std::size(sizes); ++i, multiplier /= 1024) {
            if (multiplier < amount) {
                const unsigned short preDot = amount / multiplier;
                const unsigned char digitCount = preDot < 10 ? 1 :
                                                (preDot < 100 ? 2 :
                                                (preDot < 1000 ? 3 : 4));
                s << std::setprecision(digitCount + 2) << static_cast<double>(amount) / multiplier << " " << sizes[i];
                break;
            }
        }
    }
    return s.str();
}
