/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2022 - 2023, 2025  mhahnFr
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
 * LeakSanitizer<, see the file LICENSE.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "Stats.hpp"

namespace lsan {
Stats::Stats(const Stats & other)
    : currentMallocCount(other.currentMallocCount),
      totalMallocCount(other.totalMallocCount),
      peekMallocCount(other.peekMallocCount),
      currentBytes(other.currentBytes),
      totalBytes(other.totalBytes),
      peekBytes(other.peekBytes),
      freeCount(other.freeCount)
{}

Stats::Stats(Stats && other) noexcept
    : currentMallocCount(other.currentMallocCount),
      totalMallocCount(other.totalMallocCount),
      peekMallocCount(other.peekMallocCount),
      currentBytes(other.currentBytes),
      totalBytes(other.totalBytes),
      peekBytes(other.peekBytes),
      freeCount(other.freeCount)
{}

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

auto Stats::operator=(Stats && other) noexcept -> Stats& {
    currentMallocCount = other.totalMallocCount;
    totalMallocCount   = other.totalMallocCount;
    peekMallocCount    = other.peekMallocCount;

    currentBytes = other.currentBytes;
    totalBytes   = other.totalBytes;
    peekBytes    = other.peekBytes;

    freeCount = other.freeCount;

    return *this;
}

auto Stats::getCurrentMallocCount() const -> std::size_t {
    std::lock_guard lock(mutex);
    
    return currentMallocCount;
}

auto Stats::getTotalMallocCount() const -> std::size_t {
    std::lock_guard lock(mutex);
    
    return totalMallocCount;
}

auto Stats::getMallocPeek() const -> std::size_t {
    std::lock_guard lock(mutex);
    
    return peekMallocCount;
}

auto Stats::getCurrentBytes() const -> std::size_t {
    std::lock_guard lock(mutex);
    
    return currentBytes;
}

auto Stats::getTotalBytes() const -> std::size_t {
    std::lock_guard lock(mutex);
    
    return totalBytes;
}

auto Stats::getBytePeek() const -> std::size_t {
    std::lock_guard lock(mutex);
    
    return peekBytes;
}

auto Stats::getTotalFreeCount() const -> std::size_t {
    std::lock_guard lock(mutex);
    
    return freeCount;
}

void Stats::addMalloc(const std::size_t size) {
    std::lock_guard lock(mutex);
    
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

void Stats::replaceMalloc(const std::size_t oldSize, const std::size_t newSize) {
    std::lock_guard lock(mutex);
    
    currentBytes -= oldSize;
    currentBytes += newSize;
    if (peekBytes < currentBytes) {
        peekBytes = currentBytes;
    }
    if (newSize > oldSize) {
        totalBytes += newSize - oldSize;
    }
}

void Stats::addFree(const std::size_t size) {
    std::lock_guard lock(mutex);
    
    ++freeCount;
    
    --currentMallocCount;
    currentBytes -= size;
}
}
