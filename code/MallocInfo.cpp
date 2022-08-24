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

MallocInfo::MallocInfo(const void * const pointer, size_t size, const std::string & file, const int line, int omitCount, bool createdSet)
    : pointer(pointer), size(size), createdInFile(file), createdOnLine(line), createdSet(createdSet), createdCallstack(), createdCallstackFrames(), deletedOnLine(), deletedCallstack(), deletedCallstackFrames() {
    createdCallstackFrames = createCallstack(createdCallstack, CALLSTACK_SIZE, omitCount);
}

MallocInfo::MallocInfo(const void * const pointer, size_t size, int omitCount)
    : MallocInfo(pointer, size, "<Unknown>", 1, omitCount, false) {}

MallocInfo::MallocInfo(const void * const pointer, size_t size, const std::string & file, const int line, int omitCount)
    : MallocInfo(pointer, size, file, line, omitCount, true) {}

int MallocInfo::createCallstack(void * buffer[], int bufferSize, int omitCount) {
    int frames = backtrace(buffer, bufferSize);
    memmove(buffer, buffer + omitCount, static_cast<size_t>(bufferSize - omitCount));
    return frames - omitCount;
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
    deletedCallstackFrames = createCallstack(deletedCallstack, CALLSTACK_SIZE);
}

void MallocInfo::printCallstack(void * const * callstack, int size, std::ostream & stream) {
    using Formatter::Style;
    char ** strings = backtrace_symbols(callstack, size);
    for (int i = 0; i < size; ++i) {
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
}

void MallocInfo::printCreatedCallstack(std::ostream & stream) const {
    printCallstack(createdCallstack, createdCallstackFrames, stream);
}

void MallocInfo::printDeletedCallstack(std::ostream & stream) const {
    printCallstack(deletedCallstack, deletedCallstackFrames, stream);
}

const void * const * MallocInfo::getCreatedCallstack() const {
    return createdCallstack;
}

const void * const * MallocInfo::getDeletedCallstack() const {
    return deletedCallstack;
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
