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

Stats::Stats(const Stats & other)
    : mutex(), currentMallocCount(other.currentMallocCount), totalMallocCount(other.totalMallocCount), peekMallocCount(other.peekMallocCount), currentBytes(other.currentBytes), totalBytes(other.totalBytes), peekBytes(other.peekBytes), freeCount(other.freeCount) {}

Stats::Stats(Stats && other)
    : mutex(), currentMallocCount(std::move(other.currentMallocCount)), totalMallocCount(std::move(other.totalMallocCount)), peekMallocCount(std::move(other.peekMallocCount)), currentBytes(std::move(other.currentBytes)), totalBytes(std::move(other.totalBytes)), peekBytes(std::move(other.peekBytes)), freeCount(std::move(other.freeCount)) {}

Stats & Stats::operator=(const Stats & other) {
    if (&other != this) {
        currentMallocCount = other.currentMallocCount;
        totalMallocCount   = other.totalMallocCount;
        peekMallocCount    = other.peekMallocCount;
        
        currentBytes = other.currentBytes;
        totalBytes   = other.totalBytes;
        peekBytes    = other.peekBytes;
        
        freeCount = other.freeCount;
    }
    return *this;
}

Stats & Stats::operator=(Stats && other) {
    if (&other != this) {
        currentMallocCount = std::move(other.totalMallocCount);
        totalMallocCount   = std::move(other.totalMallocCount);
        peekMallocCount    = std::move(other.peekMallocCount);
        
        currentBytes = std::move(other.currentBytes);
        totalBytes   = std::move(other.totalBytes);
        peekBytes    = std::move(other.peekBytes);
        
        freeCount = std::move(other.freeCount);
    }
    return *this;
}

size_t Stats::getCurrentMallocCount() const { return currentMallocCount; }
size_t Stats::getTotalMallocCount()   const { return totalMallocCount;   }
size_t Stats::getMallocPeek()         const { return peekMallocCount;    }

size_t Stats::getCurrentBytes() const { return currentBytes; }
size_t Stats::getTotalBytes()   const { return totalBytes;   }
size_t Stats::getBytePeek()     const { return peekBytes;    }

size_t Stats::getTotalFreeCount() const { return freeCount; }

void Stats::addFree  (const MallocInfo & mInfo) { addFree(mInfo.getSize());   }
void Stats::addMalloc(const MallocInfo & mInfo) { addMalloc(mInfo.getSize()); }

void Stats::addMalloc(size_t size) {
    std::lock_guard<std::mutex> lock(mutex);
    
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

void Stats::replaceMalloc(size_t oldSize, size_t newSize) {
    std::lock_guard<std::mutex> lock(mutex);
    
    currentBytes -= oldSize;
    currentBytes += newSize;
    if (peekBytes < currentBytes) {
        peekBytes = currentBytes;
    }
    if (newSize > oldSize) {
        totalBytes += newSize - oldSize;
    }
}

void Stats::addFree(size_t size) {
    std::lock_guard<std::mutex> lock(mutex);
    
    ++freeCount;
    
    --currentMallocCount;
    currentBytes -= size;
}

Stats Stats::operator+(const MallocInfo & minfo) {
    Stats old = *this;
    old.addMalloc(minfo);
    return old;
}

Stats & Stats::operator+=(const MallocInfo & mInfo) {
    addMalloc(mInfo);
    return *this;
}

Stats Stats::operator-(const MallocInfo & mInfo) {
    Stats old = *this;
    old.addFree(mInfo);
    return old;
}

Stats & Stats::operator-=(const MallocInfo & mInfo) {
    addFree(mInfo);
    return *this;
}
