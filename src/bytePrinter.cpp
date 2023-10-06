/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2022 - 2023  mhahnFr
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

#include <iomanip>
#include <sstream>
#include "bytePrinter.hpp"
#include "../include/lsan_internals.h"

namespace lsan {
/// Represents exactly 1 EiB. Needed for the calculations as starting point.
static constexpr unsigned long long exabyte = 1024ULL * 1024ULL * 1024ULL * 1024ULL * 1024ULL * 1024ULL;

std::string bytesToString(unsigned long long amount) {
    std::stringstream s;
    if (!__lsan_humanPrint || amount == 0) {
        s << amount << " B";
    } else {
        const std::string sizes[] {"EiB", "PiB", "TiB", "GiB", "MiB", "KiB", "B"};
        unsigned long long multiplier = exabyte;
        for (std::size_t i = 0; i < std::size(sizes); ++i, multiplier /= 1024) {
            if (multiplier <= amount) {
                const unsigned short preDot = static_cast<unsigned short>(amount / multiplier);
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
}
