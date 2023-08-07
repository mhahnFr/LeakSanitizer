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

#ifndef Peaks_hpp
#define Peaks_hpp

#include <mutex>

class Peaks {
    mutable std::mutex mutex;
    
    std::size_t peakMallocCount;
    std::size_t peakByteCount;
    
public:
    Peaks() = default;
   ~Peaks() = default;
    
    Peaks(Peaks &&);
    Peaks(const Peaks &);
    
    auto operator=(Peaks &&)      -> Peaks &;
    auto operator=(const Peaks &) -> Peaks &;
    
    void setPeakMallocCount(const std::size_t peakMallocCount);
    void setPeakByteCount(const std::size_t peakByteCount);
    
    auto getPeakMallocCount() const -> std::size_t;
    auto getPeakByteCount() const -> std::size_t;
};

#endif /* Peaks_hpp */
