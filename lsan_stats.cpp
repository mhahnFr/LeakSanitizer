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
    LSan::setIgnoreMalloc(true);
    
    LSan::setIgnoreMalloc(false);
}
