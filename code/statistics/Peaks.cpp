/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2023  mhahnFr
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

#include "Peaks.hpp"

Peaks::Peaks(Peaks && other):
    mutex(),
    peakMallocCount(std::move(other.peakByteCount)),
    peakByteCount(std::move(other.peakByteCount))
{}

Peaks::Peaks(const Peaks & other):
    mutex(),
    peakMallocCount(other.peakMallocCount),
    peakByteCount(other.peakByteCount)
{}

auto Peaks::operator=(Peaks && other) -> Peaks & {
    if (std::addressof(other) != this) {
        std::lock_guard lock1(mutex),
                        lock2(other.mutex);
        peakMallocCount = std::move(other.peakMallocCount);
        peakByteCount   = std::move(other.peakByteCount);
    }
    return *this;
}

auto Peaks::operator=(const Peaks & other) -> Peaks & {
    if (std::addressof(other) != this) {
        std::lock_guard lock1(mutex),
                        lock2(other.mutex);
        peakMallocCount = other.peakMallocCount;
        peakByteCount   = other.peakByteCount;
    }
    return *this;
}

void Peaks::setPeakMallocCount(const std::size_t peakMallocCount) {
    std::lock_guard lock(mutex);
    
    this->peakMallocCount = peakMallocCount;
}

void Peaks::setPeakByteCount(const std::size_t peakByteCount) {
    std::lock_guard lock(mutex);
    
    this->peakByteCount = peakByteCount;
}

auto Peaks::getPeakMallocCount() const -> std::size_t {
    std::lock_guard lock(mutex);
    
    return peakMallocCount;
}

auto Peaks::getPeakByteCount() const -> std::size_t {
    std::lock_guard lock(mutex);
    
    return peakByteCount;
}
