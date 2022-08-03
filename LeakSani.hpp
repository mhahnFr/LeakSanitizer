/*
 * LeakSanitizer - A small library showing informations about lost memory.
 *
 * Copyright (C) 2022  mhahnFr
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see https://www.gnu.org/licenses/.
 */

#ifndef LeakSani_hpp
#define LeakSani_hpp

#include <list>
#include <ostream>
#include <mutex>
#include "MallocInfo.hpp"

class LSan {
    static LSan * instance;
    
    std::list<MallocInfo> infos;
    std::recursive_mutex  infoMutex;
    
public:
    LSan();
    ~LSan() = default;
    
    LSan(const LSan &)              = delete;
    LSan(const LSan &&)             = delete;
    LSan & operator=(const LSan &)  = delete;
    LSan & operator=(const LSan &&) = delete;
    
    void addMalloc(const MallocInfo &&);
    bool removeMalloc(const MallocInfo &);
    
    size_t getTotalAllocatedBytes();
    
    void * (*malloc)(size_t);
    void   (*free)  (void *);
    void   (*exit)  (int);
    
    static LSan & getInstance();
    static void   __exit_hook();

    friend void           internalCleanUp();
    friend std::ostream & operator<<(std::ostream &, LSan &);
};

void internalCleanUp();

#endif /* LeakSani_hpp */
