/*
 * LeakSanitizer - A small library showing informations about lost memory.
 *
 * Copyright (C) 2022  mhahnFr
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see https://www.gnu.org/licenses/.
 */

#include <iostream>
#include "LeakSani.hpp"
#include "lsan_stats.h"

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
    LSan::setIgnoreMalloc(true);
    std::cout << "Stats of the memory usage so far: " << std::endl
              << __lsan_getCurrentMallocCount() << " objects in the heap, peek " << __lsan_getMallocPeek() << ", " << __lsan_getTotalFrees() << " deleted objects." << std::endl << std::endl
              << __lsan_getCurrentByteCount() << " bytes currently used, peek " << __lsan_getBytePeek() << " bytes." << std::endl;
    std::cout << "[";
    size_t i;
    for (i = 0; i < (static_cast<float>(__lsan_getCurrentByteCount()) / __lsan_getBytePeek()) * width; ++i) {
        std::cout << '*';
    }
    for (; i < width; ++i) {
        std::cout << ' ';
    }
    std::cout << "] of " << __lsan_getBytePeek() << " peek bytes" << std::endl;
    std::cout << "Objects (" << __lsan_getCurrentMallocCount() << "):" << std::endl << "[";
    for (i = 0; i < (static_cast<float>(__lsan_getCurrentMallocCount()) / __lsan_getMallocPeek()) * width; ++i) {
        std::cout << '*';
    }
    for (; i < width; ++i) {
        std::cout << ' ';
    }
    std::cout << "] of " << __lsan_getMallocPeek() << " peek object count" << std::endl;
    LSan::setIgnoreMalloc(false);
}
