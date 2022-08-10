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
#include "LeakSani.hpp"
#include "lsan_stats.h"
#include "bytePrinter.hpp"

size_t __lsan_getTotalMallocs() {
    return LSan::getStats().getTotalMallocCount();
}

size_t __lsan_getTotalBytes() {
    return LSan::getStats().getTotalBytes();
}

size_t __lsan_getTotalFrees() {
    return LSan::getStats().getTotalFreeCount();
}

size_t __lsan_getCurrentMallocCount() {
    return LSan::getStats().getCurrentMallocCount();
}

size_t __lsan_getCurrentByteCount() {
    return LSan::getStats().getCurrentBytes();
}

size_t __lsan_getMallocPeek() {
    return LSan::getStats().getMallocPeek();
}

size_t __lsan_getBytePeek() {
    return LSan::getStats().getBytePeek();
}

void __lsan_printStats() {
    __lsan_printStatsWithWidth(100);
}

void __lsan_printStatsWithWidth(size_t width) {
    bool ignore = LSan::ignoreMalloc();
    LSan::setIgnoreMalloc(true);
    std::cout << "\033[3mStats of the memory usage so far:\033[23m" << std::endl
              << "\033[0m" << __lsan_getCurrentMallocCount() << " objects in the heap, peek " << __lsan_getMallocPeek() << ", " << __lsan_getTotalFrees() << " deleted objects.\033[23m" << std::endl << std::endl
              << "\033[1m" << bytesToString(__lsan_getCurrentByteCount()) << "\033[22m currently used, peek " << bytesToString(__lsan_getBytePeek()) << "." << std::endl
              << "\033[1m[\033[22;4;2m";
    size_t i;
    for (i = 0; i < (static_cast<float>(__lsan_getCurrentByteCount()) / __lsan_getBytePeek()) * width; ++i) {
        std::cout << '*';
    }
    for (; i < width; ++i) {
        std::cout << ' ';
    }
    std::cout << "\033[22;24;1m]\033[22m of \033[1m" << bytesToString(__lsan_getBytePeek()) << "\033[22m peek" << std::endl << std::endl
              << "\033[1m" << __lsan_getCurrentMallocCount() << " objects\033[22m currently in the heap, peek " << __lsan_getMallocPeek() << " objects." << std::endl
              << "\033[1m[\033[22;4;2m";
    for (i = 0; i < (static_cast<float>(__lsan_getCurrentMallocCount()) / __lsan_getMallocPeek()) * width; ++i) {
        std::cout << '*';
    }
    for (; i < width; ++i) {
        std::cout << ' ';
    }
    std::cout << "\033[22;24;1m]\033[22m of \033[1m" << __lsan_getMallocPeek() << " objects\033[22m peek" << std::endl << std::endl;
    if (!ignore) {
        LSan::setIgnoreMalloc(false);
    }
}
