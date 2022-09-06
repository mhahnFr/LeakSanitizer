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

#ifndef LeakSani_hpp
#define LeakSani_hpp

#include <map>
#include <ostream>
#include <mutex>
#include "MallocInfo.hpp"
#include "Stats.hpp"

class LSan {
    static Stats * stats;
    
    static bool & getIgnoreMalloc();
    
    std::map<const void * const, MallocInfo> infos;
    std::recursive_mutex infoMutex;
    Stats                realStats;
    
public:
    LSan();
    ~LSan() = default;
    
    LSan(const LSan &)              = delete;
    LSan(const LSan &&)             = delete;
    LSan & operator=(const LSan &)  = delete;
    LSan & operator=(const LSan &&) = delete;
    
    void addMalloc(MallocInfo &&);
    void changeMalloc(const MallocInfo &);
    auto removeMalloc(const MallocInfo &) -> bool;
    auto removeMalloc(const void *)       -> bool;
    
    auto getTotalAllocatedBytes() -> size_t;
    auto getTotalLeakedBytes()    -> size_t;
    auto getLeakCount()           -> size_t;
    
    static void * (*malloc) (size_t);
    static void * (*calloc) (size_t, size_t);
    static void * (*realloc)(void *, size_t);
    static void   (*free)   (void *);
    static void   (*exit)   (int);
    
    static auto getInstance()  -> LSan &;
    static auto getStats()     -> Stats &;
    static auto hasStats()     -> bool;
    static auto ignoreMalloc() -> bool;
    
    static void setIgnoreMalloc(bool);
    static void printInformations();
    static void printLicense();
    static void __exit_hook();

    friend void           internalCleanUp();
    friend std::ostream & operator<<(std::ostream &, LSan &);
};

void internalCleanUp();

#endif /* LeakSani_hpp */
