//
//  MallocInfo.cpp
//  LeakSanitizer
//
//  Created by Manuel Hahn on 16.07.22.
//  Copyright © 2022 mhahnFr. All rights reserved.
//

#include "MallocInfo.hpp"
#include <execinfo.h>
#include <dlfcn.h>
#include <cxxabi.h>

MallocInfo::MallocInfo(const void * const pointer, size_t size, const std::string & file, const int line)
    : pointer(pointer), size(size), createdInFile(file), createdOnLine(line), createdCallstack(createCallstack()) {}

const std::vector<std::string> MallocInfo::createCallstack() {
    std::vector<std::string> ret;
    void * callstack[128];
    int frames = backtrace(callstack, 128);
    char ** symbols = backtrace_symbols(callstack, frames);

    for (int i = 3; i < 128; ++i) {
        Dl_info info;
        if (dladdr(callstack[i], &info)) {
            char * demangled;
            int status;
            if ((demangled = abi::__cxa_demangle(info.dli_sname, nullptr, 0, &status)) != nullptr) {
                ret.push_back(demangled);
                free(demangled);
            } else {
                ret.push_back(info.dli_sname);// + (" + " + std::to_string(static_cast<uintptr_t>(callstack[i] - info.dli_saddr))));
            }
        }
    }
    free(symbols);
    return ret;
}

const void * const MallocInfo::getPointer() const {
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
        stream << (frame == callstack.front() ? "In: " : "çat: ") << frame << std::endl;
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
