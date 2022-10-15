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
#include <cstring>
#include "MallocInfo.hpp"
#include "LeakSani.hpp"
#include "Formatter.hpp"
#include "bytePrinter.hpp"
#include "../include/lsan_internals.h"

MallocInfo::MallocInfo(void * const pointer, size_t size, const std::string & file, const int line, void * omitAddress, bool createdSet)
    : pointer(pointer), size(size), createdInFile(file), createdOnLine(line), createdSet(createdSet), createdCallstack(false), deletedOnLine(0), deleted(false), deletedCallstack(false) {
    void * trace[CALLSTACK_SIZE];
    int length = createCallstack(trace, CALLSTACK_SIZE, omitAddress);
    callstack_emplaceWithBacktrace(createdCallstack, trace, static_cast<size_t>(length));
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

const lcs::callstack & MallocInfo::getCreatedCallstack() const { return createdCallstack; }
const lcs::callstack & MallocInfo::getDeletedCallstack() const { return deletedCallstack; }

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
    void * trace[CALLSTACK_SIZE];
    int length = createCallstack(trace, CALLSTACK_SIZE);
    callstack_emplaceWithBacktrace(deletedCallstack, trace, length);
}

void MallocInfo::printCallstack(lcs::callstack && callstack, std::ostream & stream) {
    printCallstack(callstack, stream);
}

void MallocInfo::printCallstack(lcs::callstack & callstack, std::ostream & stream) {
    using Formatter::Style;
    char ** strings = callstack_toArray(callstack);
    const size_t size = callstack_getFrameCount(callstack);
    size_t i;
    for (i = 0; i < size && i < __lsan_callstackSize; ++i) {
        stream << (i == 0 ? ("In: " + Formatter::clear(Style::ITALIC) + Formatter::get(Style::BOLD))
                          : (Formatter::clear(Style::BOLD) + Formatter::get(Style::ITALIC) + "at: " + Formatter::clear(Style::ITALIC)));
        stream << (strings[i] == nullptr ? "<Unknown>" : strings[i]);
        if (i == 0) {
            stream << Formatter::clear(Style::BOLD);
        }
        stream << std::endl;
    }
    if (i < size) {
        stream << std::endl << Formatter::get(Style::UNDERLINED) << Formatter::get(Style::ITALIC)
               << "And " << size - i << " more lines..."
               << Formatter::clear(Style::UNDERLINED) << Formatter::clear(Style::ITALIC) << std::endl;
        LSan::getInstance().setCallstackSizeExceeded(true);
    }
}

void MallocInfo::printCreatedCallstack(std::ostream & stream) const {
    printCallstack(createdCallstack, stream);
}

void MallocInfo::printDeletedCallstack(std::ostream & stream) const {
    printCallstack(deletedCallstack, stream);
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
