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

#include "../ATracker.hpp"
#include "../MallocInfo.hpp"

/**
 * This class represents a thread local allocation tracker.
 */
class ThreadAllocInfo: public ATracker {
    /** The mutex used to protect the allocation infos.                    */
    std::recursive_mutex                     infosMutex;
    /** The map containing the allocation infos.                           */
    std::map<const void * const, MallocInfo> infos;
    
public:
    /** The reference wrapper type.          */
    using  Ref = std::reference_wrapper<ThreadAllocInfo>;
    /** The constant reference wrapper type. */
    using CRef = std::reference_wrapper<const ThreadAllocInfo>;
    
    ThreadAllocInfo();
   ~ThreadAllocInfo();
    
    ThreadAllocInfo(const ThreadAllocInfo &) = delete;
    ThreadAllocInfo(ThreadAllocInfo &&)      = delete;
    
    ThreadAllocInfo & operator=(const ThreadAllocInfo &) = delete;
    ThreadAllocInfo & operator=(ThreadAllocInfo &&)      = delete;
    
    virtual void addMalloc(MallocInfo && info) override;
    
    /**
     * Exchanges the MallocInfo associated with the pointer of the given info
     * by the given MallocInfo.
     *
     * @param info the info to be exchanged
     * @param search indicates whether to search globally if not found here
     * @return whether the info was exchanged
     */
    auto changeMalloc(const MallocInfo & info, bool search) -> bool;
    /**
     * Removes the MallocInfo associated with the given pointer.
     *
     * @param pointer the deallocated pointer
     * @param search indicates whether to search globally if not found here
     * @return whether the info was removed
     */
    auto removeMalloc(const void * pointer, bool search) -> bool;
    
    virtual inline auto changeMalloc(const MallocInfo & info) -> bool override {
        return changeMalloc(info, true);
    }
    
    virtual inline auto removeMalloc(const void * pointer) -> bool override {
        return removeMalloc(pointer, true);
    }
    
    /**
     * Returns a reference to the stored allocation infos.
     *
     * @return the stored allocation infos
     */
    constexpr inline auto getInfos() -> std::map<const void * const, MallocInfo> & {
        return infos;
    }
    
    /**
     * Returns a reference to the stored allocation infos.
     *
     * @return the stored allocation infos
     */
    constexpr inline auto getInfos() const -> const std::map<const void * const, MallocInfo> & {
        return infos;
    }
};

#endif /* ThreadAllocInfo_hpp */
