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

#include "MallocInfo.hpp"
#include <execinfo.h>
#include <dlfcn.h>
#include <cxxabi.h>
#include "LeakSani.hpp"

MallocInfo::MallocInfo(const void * const pointer, size_t size, const std::string & file, const int line, int omitCount, bool createdSet)
    : pointer(pointer), size(size), createdInFile(file), createdOnLine(line), createdSet(createdSet), createdCallstack(createCallstack(omitCount)), deletedOnLine() {}

MallocInfo::MallocInfo(const void * const pointer, size_t size, int omitCount)
    : MallocInfo(pointer, size, "<Unknown>", 1, omitCount, false) {}

MallocInfo::MallocInfo(const void * const pointer, size_t size, const std::string & file, const int line, int omitCount)
    : MallocInfo(pointer, size, file, line, omitCount, true) {}

MallocInfo::~MallocInfo() {
    LSan::setIgnoreMalloc(true);
    if (!LSan::ignoreMalloc()) abort();
}

const std::vector<std::string> MallocInfo::createCallstack(int omitCount) {
    LSan::setIgnoreMalloc(true);
    if (!LSan::ignoreMalloc()) abort();
    std::vector<std::string> ret;
    void * callstack[128];
    int frames = backtrace(callstack, 128);

    ret.reserve(static_cast<std::vector<std::string>::size_type>(frames - omitCount));
    for (int i = omitCount; i < frames; ++i) {
        Dl_info info;
        if (dladdr(callstack[i], &info)) {
            char * demangled;
            int status;
            if (info.dli_sname == nullptr) {
                ret.emplace_back("<Unknown>");
            } else if ((demangled = abi::__cxa_demangle(info.dli_sname, nullptr, nullptr, &status)) != nullptr) {
                ret.emplace_back(demangled);
                free(demangled);
            } else {
                ret.push_back(info.dli_sname + (" + " + std::to_string(static_cast<char *>(callstack[i]) - static_cast<char *>(info.dli_saddr))));
            }
        }
    }
    return ret;
}

const void * MallocInfo::getPointer() const {
    return pointer;
}

const std::string & MallocInfo::getCreatedInFile() const {
    return createdInFile;
}

int MallocInfo::getCreatedOnLine() const {
    return createdOnLine;
}

size_t MallocInfo::getSize() const {
    return size;
}

const std::string & MallocInfo::getDeletedInFile() const {
    return deletedInFile;
}

int MallocInfo::getDeletedOnLine() const {
    return deletedOnLine;
}

void MallocInfo::setDeletedInFile(const std::string & file) {
    deletedInFile = file;
}

void MallocInfo::setDeletedOnLine(int line) {
    deletedOnLine = line;
}

void MallocInfo::generateDeletedCallstack() {
    deletedCallstack = createCallstack();
}

void MallocInfo::printCallstack(const std::vector<std::string> & callstack, std::ostream & stream) {
    for (const auto & frame : callstack) {
        stream << (frame == callstack.front() ? "In: \033[23;1m" : "\033[22;3mat: \033[23m") << frame << std::endl;
    }
}

void MallocInfo::printCreatedCallstack(std::ostream & stream) const {
    printCallstack(createdCallstack, stream);
}

void MallocInfo::printDeletedCallstack(std::ostream & stream) const {
    printCallstack(deletedCallstack, stream);
}

const std::vector<std::string> & MallocInfo::getCreatedCallstack() const {
    return createdCallstack;
}

const std::vector<std::string> & MallocInfo::getDeletedCallstack() const {
    return deletedCallstack;
}

bool operator==(const MallocInfo & lhs, const MallocInfo & rhs) {
    return lhs.getPointer() == rhs.getPointer();
}

bool operator<(const MallocInfo & lhs, const MallocInfo & rhs) {
    return lhs.getPointer() < rhs.getPointer();
}

std::ostream & operator<<(std::ostream & stream, const MallocInfo & self) {
    stream << "\033[1;3;31mLeak\033[22;39m of size " << self.size << ", ";
    if (self.createdSet) {
        stream << "allocated at \033[4m" << self.createdInFile << ":" << self.createdOnLine << "\033[24m";
    } else {
        stream << "allocation stacktrace:";
    }
    stream << std::endl;
    self.printCreatedCallstack(stream);
    return stream;
}
