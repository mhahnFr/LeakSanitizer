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

using namespace std::rel_ops;

class MallocInfo {
    constexpr static int CALLSTACK_SIZE = 128;
    
    MallocInfo(const void * const, size_t, const std::string &, int, int, bool);
    
    const void * const pointer;
    const size_t       size;
    
    const std::string createdInFile;
    const int         createdOnLine;
    const bool        createdSet;
    void *            createdCallstack[128];
    int               createdCallstackFrames;

    std::string       deletedInFile;
    int               deletedOnLine;
    void *            deletedCallstack[128];
    int               deletedCallstackFrames;
        
public:
    MallocInfo(const void * const, size_t, int = 4);
    MallocInfo(const void * const, size_t, const std::string &, int, int = 4);
    
    auto getPointer()        const -> const void *;
    auto getCreatedInFile()  const -> const std::string &;
    auto getCreatedOnLine()  const -> int;
    auto getSize()           const -> size_t;
    
    void setDeletedInFile(const std::string &);
    auto getDeletedInFile()  const -> const std::string &;
    
    void setDeletedOnLine(int);
    void generateDeletedCallstack();
    auto getDeletedOnLine()  const -> int;
    
    void printCreatedCallstack(std::ostream &) const;
    void printDeletedCallstack(std::ostream &) const;
    
    auto getDeletedCallstack() const -> const void * const *;
    auto getCreatedCallstack() const -> const void * const *;

    static void printCallstack(void * const *, int, std::ostream &);
    static auto createCallstack(void *[], int, int = 1) -> int;
    
    friend auto operator==(const MallocInfo &, const MallocInfo &) -> bool;
    friend auto operator<(const MallocInfo &, const MallocInfo &)  -> bool;
    friend auto operator<<(std::ostream &, const MallocInfo &)     -> std::ostream &;
};

#endif /* MallocInfo_hpp */
