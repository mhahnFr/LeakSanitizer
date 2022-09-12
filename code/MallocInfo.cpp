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

#include <execinfo.h>
#include <dlfcn.h>
#include <cxxabi.h>
#include <cstring>
#include "MallocInfo.hpp"
#include "LeakSani.hpp"
#include "Formatter.hpp"
#include "bytePrinter.hpp"
#include "../include/lsan_internals.h"

MallocInfo::MallocInfo(void * const pointer, size_t size, const std::string & file, const int line, void * omitAddress, bool createdSet)
    : pointer(pointer), size(size), createdInFile(file), createdOnLine(line), createdSet(createdSet), createdCallstack(), createdCallstackFrames(), deletedOnLine(0), deleted(false), deletedCallstack(), deletedCallstackFrames(0) {
    createdCallstackFrames = createCallstack(createdCallstack, CALLSTACK_SIZE, omitAddress);
}

MallocInfo::MallocInfo(void * const pointer, size_t size, void * omitAddress)
    : MallocInfo(pointer, size, "<Unknown>", 1, omitAddress, false) {}

MallocInfo::MallocInfo(void * const pointer, size_t size, const std::string & file, const int line, void * omitAddress)
    : MallocInfo(pointer, size, file, line, omitAddress, true) {}

int MallocInfo::createCallstack(void * buffer[], int bufferSize, void * omitAddress) {
    int i,
        frames = backtrace(buffer, bufferSize);
    
    for (i = 0; buffer[i] != omitAddress && i < bufferSize; ++i);
    memmove(buffer, buffer + i, static_cast<size_t>(bufferSize - i));
    return frames - i;
}

const void * MallocInfo::getPointer() const { return pointer; }

const std::string & MallocInfo::getCreatedInFile() const { return createdInFile; }
int                 MallocInfo::getCreatedOnLine() const { return createdOnLine; }

size_t MallocInfo::getSize() const { return size; }

const std::string & MallocInfo::getDeletedInFile() const { return deletedInFile; }
int                 MallocInfo::getDeletedOnLine() const { return deletedOnLine; }

bool MallocInfo::isDeleted() const { return deleted; }

const void * const * MallocInfo::getCreatedCallstack() const { return createdCallstack; }
const void * const * MallocInfo::getDeletedCallstack() const { return deletedCallstack; }

void MallocInfo::setDeletedInFile(const std::string & file) {
    deletedInFile = file;
}

void MallocInfo::setDeletedOnLine(int line) {
    deletedOnLine = line;
}

void MallocInfo::setDeleted(bool del) {
    deleted = del;
}

void MallocInfo::generateDeletedCallstack() {
    deletedCallstackFrames = createCallstack(deletedCallstack, CALLSTACK_SIZE);
}

void MallocInfo::printCallstack(void * const * callstack, int size, std::ostream & stream) {
    using Formatter::Style;
    char ** strings = backtrace_symbols(callstack, size);
    int i;
    for (i = 0; i < size && i < __lsan_callstackSize; ++i) {
        Dl_info info;
        if (dladdr(callstack[i], &info)) {
            stream << (i == 0 ? ("In: " + Formatter::clear(Style::ITALIC) + Formatter::get(Style::BOLD))
                              : (Formatter::clear(Style::BOLD) + Formatter::get(Style::ITALIC) + "at: " + Formatter::clear(Style::ITALIC)));
            char * demangled;
            int status;
            if (info.dli_sname == nullptr) {
                stream << strings[i];
            } else if ((demangled = abi::__cxa_demangle(info.dli_sname, nullptr, nullptr, &status)) != nullptr) {
                stream << demangled;
                free(demangled);
            } else {
                stream << info.dli_sname + (" + " + std::to_string(static_cast<char *>(callstack[i]) - static_cast<char *>(info.dli_saddr)));
            }
            stream << std::endl;
        }
    }
    free(strings);
    if (i < size) {
        stream << "And " << size - i << " more lines..." << std::endl
               << "To see more, increase the value of __lsan_callstackSize (currently " << __lsan_callstackSize << ")." << std::endl;
    }
}

void MallocInfo::printCreatedCallstack(std::ostream & stream) const {
    printCallstack(createdCallstack, createdCallstackFrames, stream);
}

void MallocInfo::printDeletedCallstack(std::ostream & stream) const {
    printCallstack(deletedCallstack, deletedCallstackFrames, stream);
}

bool operator==(const MallocInfo & lhs, const MallocInfo & rhs) {
    return lhs.getPointer() == rhs.getPointer();
}

bool operator<(const MallocInfo & lhs, const MallocInfo & rhs) {
    return lhs.getPointer() < rhs.getPointer();
}

std::ostream & operator<<(std::ostream & stream, const MallocInfo & self) {
    using Formatter::Style;
    stream << Formatter::get(Style::BOLD) << Formatter::get(Style::ITALIC) << Formatter::get(Style::RED)
           << "Leak" << Formatter::clear(Style::RED) << Formatter::clear(Style::BOLD)
           << " of size " << Formatter::clear(Style::ITALIC)
           << bytesToString(self.size) << Formatter::get(Style::ITALIC) << ", ";
    if (self.createdSet) {
        stream << "allocated at " << Formatter::get(Style::UNDERLINED)
               << self.createdInFile << ":" << self.createdOnLine << Formatter::clear(Style::UNDERLINED);
    } else {
        stream << "allocation stacktrace:";
    }
    stream << std::endl;
    self.printCreatedCallstack(stream);
    return stream;
}
