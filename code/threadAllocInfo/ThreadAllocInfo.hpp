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

#include <functional>
#include <map>
#include <mutex>

#include "../MallocInfo.hpp"

#include "../statistics/Stats.hpp"

class ThreadAllocInfo {
    Stats stats;
    mutable std::recursive_mutex infosMutex;
    std::map<const void * const, MallocInfo> infos;
    bool ignoreMalloc;
    
public:
    using  Ref = std::reference_wrapper<ThreadAllocInfo>;
    using CRef = std::reference_wrapper<const ThreadAllocInfo>;
    
    ThreadAllocInfo();
   ~ThreadAllocInfo();
    
    ThreadAllocInfo(const ThreadAllocInfo &) = delete;
    ThreadAllocInfo(ThreadAllocInfo &&)      = delete;
    
    ThreadAllocInfo & operator=(const ThreadAllocInfo &) = delete;
    ThreadAllocInfo & operator=(ThreadAllocInfo &&)      = delete;
    
    void addMalloc(MallocInfo && info);
    auto changeMalloc(const MallocInfo & info, bool search = true) -> bool;
    auto removeMalloc(const void * pointer, bool search = true) -> bool;
    
    auto removeMalloc(MallocInfo && info) -> bool {
        return removeMalloc(info.getPointer());
    }
    
    constexpr inline void setIgnoreMalloc(const bool ignoreMalloc) {
        this->ignoreMalloc = ignoreMalloc;
    }
    
    constexpr inline auto getIgnoreMalloc() const -> bool {
        return ignoreMalloc;
    }
    
    constexpr inline auto getStats() -> Stats & {
        return stats;
    }
    
    constexpr inline auto getStats() const -> const Stats & {
        return stats;
    }
    
    constexpr inline auto getInfos() -> std::map<const void * const, MallocInfo> & {
        return infos;
    }
    
    constexpr inline auto getInfos() const -> const std::map<const void * const, MallocInfo> & {
        return infos;
    }
    
    void lockMutex() const;
    void unlockMutex() const;
};

#endif /* ThreadAllocInfo_hpp */
