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

#ifndef Stats_hpp
#define Stats_hpp

#include <cstddef>
#include "MallocInfo.hpp"

class Stats {
    size_t currentMallocCount,
             totalMallocCount,
              peekMallocCount;

    size_t currentBytes,
             totalBytes,
              peekBytes;
    
    size_t freeCount;
    
public:
    auto getCurrentMallocCount() const -> size_t;
    auto getTotalMallocCount()   const -> size_t;
    auto getMallocPeek()         const -> size_t;
    
    auto getCurrentBytes() const -> size_t;
    auto getTotalBytes()   const -> size_t;
    auto getBytePeek()     const -> size_t;
    
    auto getTotalFreeCount() const -> size_t;
    
    void addMalloc(size_t size);
    void addMalloc(const MallocInfo &);
    
    void addFree(size_t size);
    void addFree(const MallocInfo &);
    
    auto operator+ (const MallocInfo &) -> Stats;
    auto operator+=(const MallocInfo &) -> Stats &;
    
    auto operator- (const MallocInfo &) -> Stats;
    auto operator-=(const MallocInfo &) -> Stats &;
};

#endif /* Stats_hpp */
