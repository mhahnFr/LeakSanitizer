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

#ifndef MallocInfo_hpp
#define MallocInfo_hpp

#include <vector>
#include <ostream>
#include <string>
#include <utility>

#include "../CallstackLibrary/include/callstack.h"

using namespace std::rel_ops;

class MallocInfo {
    constexpr static int CALLSTACK_SIZE = 128;
    
    MallocInfo(void * const, size_t, const std::string &, int, void *, bool);
    
    void *      pointer;
    size_t      size;
    
    std::string            createdInFile;
    int                    createdOnLine;
    bool                   createdSet;
    mutable lcs::callstack createdCallstack;

    std::string            deletedInFile;
    int                    deletedOnLine;
    bool                   deleted;
    mutable lcs::callstack deletedCallstack;
        
public:
    MallocInfo(void * const, size_t, void * = __builtin_return_address(0));
    MallocInfo(void * const, size_t, const std::string &, int, void * = __builtin_return_address(0));
    
    auto getPointer()        const -> const void *;
    auto getCreatedInFile()  const -> const std::string &;
    auto getCreatedOnLine()  const -> int;
    auto getSize()           const -> size_t;
    
    void setDeletedInFile(const std::string &);
    auto getDeletedInFile()  const -> const std::string &;
    
    void setDeletedOnLine(int);
    auto getDeletedOnLine()  const -> int;
    
    auto isDeleted()         const -> bool;
    void setDeleted(bool);
    
    void generateDeletedCallstack();
    
    void printCreatedCallstack(std::ostream &) const;
    void printDeletedCallstack(std::ostream &) const;
    
    auto getDeletedCallstack() const -> const lcs::callstack &;
    auto getCreatedCallstack() const -> const lcs::callstack &;

    static void printCallstack(lcs::callstack &,  std::ostream &);
    static void printCallstack(lcs::callstack &&, std::ostream &);
    
    friend auto operator==(const MallocInfo &, const MallocInfo &) -> bool;
    friend auto operator<(const MallocInfo &, const MallocInfo &)  -> bool;
    friend auto operator<<(std::ostream &, const MallocInfo &)     -> std::ostream &;
};

#endif /* MallocInfo_hpp */
