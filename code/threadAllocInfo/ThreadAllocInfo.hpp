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

#ifndef ThreadAllocInfo_hpp
#define ThreadAllocInfo_hpp

#include <map>
#include <mutex>

#include "../MallocInfo.hpp"
#include "../Stats.hpp"

class ThreadAllocInfo {
    Stats stats;
    std::recursive_mutex statsMutex;
    std::map<const void * const, MallocInfo> infos;
    
public:
    ThreadAllocInfo();
   ~ThreadAllocInfo();
    
    ThreadAllocInfo(const ThreadAllocInfo &) = delete;
    ThreadAllocInfo(ThreadAllocInfo &&)      = delete;
    
    ThreadAllocInfo & operator=(const ThreadAllocInfo &) = delete;
    ThreadAllocInfo & operator=(ThreadAllocInfo &&)      = delete;
    
    void addMalloc(MallocInfo && info);
    void changeMalloc(const MallocInfo & info);
    auto removeMalloc(const void * pointer) -> bool;
};

#endif /* ThreadAllocInfo_hpp */
