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

#include "Stats.hpp"

size_t Stats::getCurrentMallocCount() const { return currentMallocCount; }
size_t Stats::getTotalMallocCount()   const { return totalMallocCount;   }
size_t Stats::getMallocPeek()         const { return peekMallocCount;    }

size_t Stats::getCurrentBytes() const { return currentBytes; }
size_t Stats::getTotalBytes()   const { return totalBytes;   }
size_t Stats::getBytePeek()     const { return peekBytes;    }

size_t Stats::getTotalFreeCount() const { return freeCount; }

void Stats::addMalloc(size_t size) {
    ++currentMallocCount;
    ++totalMallocCount;
    if (peekMallocCount < currentMallocCount) {
        peekMallocCount = currentMallocCount;
    }
    
    currentBytes += size;
    totalBytes   += size;
    if (peekBytes < currentBytes) {
        peekBytes = currentBytes;
    }
}

void Stats::addMalloc(const MallocInfo & mInfo) {
    addMalloc(mInfo.getSize());
}

void Stats::addFree(size_t size) {
    ++freeCount;
    
    --currentMallocCount;
    currentBytes -= size;
}

void Stats::addFree(const MallocInfo & mInfo) {
    addFree(mInfo.getSize());
}

Stats Stats::operator+(const MallocInfo & minfo) {
    Stats old = *this;
    addMalloc(minfo);
    return old;
}

Stats & Stats::operator+=(const MallocInfo & mInfo) {
    addMalloc(mInfo);
    return *this;
}

Stats Stats::operator-(const MallocInfo & mInfo) {
    Stats old = *this;
    addFree(mInfo);
    return old;
}

Stats & Stats::operator-=(const MallocInfo & mInfo) {
    addFree(mInfo);
    return *this;
}
