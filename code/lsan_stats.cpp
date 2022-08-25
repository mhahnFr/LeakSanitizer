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

#include <iostream>
#include "LeakSani.hpp"
#include "Formatter.hpp"
#include "bytePrinter.hpp"
#include "../include/lsan_internals.h"
#include "../include/lsan_stats.h"

size_t __lsan_getTotalMallocs() {
    return LSan::getStats().getTotalMallocCount();
}

size_t __lsan_getTotalBytes() {
    return LSan::getStats().getTotalBytes();
}

size_t __lsan_getTotalFrees() {
    return LSan::getStats().getTotalFreeCount();
}

size_t __lsan_getCurrentMallocCount() {
    return LSan::getStats().getCurrentMallocCount();
}

size_t __lsan_getCurrentByteCount() {
    return LSan::getStats().getCurrentBytes();
}

size_t __lsan_getMallocPeek() {
    return LSan::getStats().getMallocPeek();
}

size_t __lsan_getBytePeek() {
    return LSan::getStats().getBytePeek();
}

bool __lsan_statsAvailable() {
    return LSan::hasStats();
}

void __lsan_printStats() {
    __lsan_printStatsWithWidth(100);
}

void __lsan_printStatsWithWidth(size_t width) {
    using Formatter::Style;
    bool ignore = LSan::ignoreMalloc();
    LSan::setIgnoreMalloc(true);
    std::ostream & out = __lsan_printCout ? std::cout : std::cerr;
    if (__lsan_statsAvailable()) {
        out << Formatter::get(Style::ITALIC)
            << "Stats of the memory usage so far:"
            << Formatter::clear(Style::ITALIC)
            << std::endl;
        
        out << Formatter::clearAll()
            << __lsan_getCurrentMallocCount() << " objects in the heap, peek " << __lsan_getMallocPeek() << ", " << __lsan_getTotalFrees() << " deleted objects."
            << Formatter::clear(Style::ITALIC)
            << std::endl << std::endl;
        
        out << Formatter::get(Style::BOLD)
            << bytesToString(__lsan_getCurrentByteCount())
            << Formatter::clear(Style::BOLD)
            << " currently used, peek " << bytesToString(__lsan_getBytePeek()) << "." << std::endl;
        
        out << Formatter::get(Style::BOLD)
            << "["
            << Formatter::clear(Style::BOLD)
            << Formatter::get(Style::GREYED) << Formatter::get(Style::UNDERLINED);
        
        size_t i;
        for (i = 0; i < (static_cast<float>(__lsan_getCurrentByteCount()) / __lsan_getBytePeek()) * width; ++i) {
            out << (__lsan_printFormatted ? '*' : '=');
        }
        for (; i < width; ++i) {
            out << (__lsan_printFormatted ? ' ' : '.');
        }
        out << Formatter::clear(Style::BOLD) << Formatter::clear(Style::GREYED) << Formatter::clear(Style::UNDERLINED)
            << Formatter::get(Style::BOLD) << "]" << Formatter::clear(Style::BOLD)
            << " of "
            << Formatter::get(Style::BOLD) << bytesToString(__lsan_getBytePeek()) << Formatter::clear(Style::BOLD)
            << " peek" << std::endl << std::endl;
        
        out << Formatter::get(Style::BOLD)
            << __lsan_getCurrentMallocCount() << " objects"
            << Formatter::clear(Style::BOLD)
            << " currently in the heap, peek " << __lsan_getMallocPeek() << " objects." << std::endl;
    
        out << Formatter::get(Style::BOLD)
            << "["
            << Formatter::clear(Style::BOLD)
            << Formatter::get(Style::UNDERLINED) << Formatter::get(Style::GREYED);
        
        for (i = 0; i < (static_cast<float>(__lsan_getCurrentMallocCount()) / __lsan_getMallocPeek()) * width; ++i) {
            out << (__lsan_printFormatted ? '*' : '=');
        }
        for (; i < width; ++i) {
            out << (__lsan_printFormatted ? ' ' : '.');
        }
        out << Formatter::clear(Style::GREYED) << Formatter::clear(Style::UNDERLINED)
            << Formatter::get(Style::BOLD) << "]" << Formatter::clear(Style::BOLD)
            << " of "
            << Formatter::get(Style::BOLD)
            << __lsan_getMallocPeek() << " objects"
            << Formatter::clear(Style::BOLD)
            << " peek" << std::endl << std::endl;
        
    } else {
        out << Formatter::get(Style::ITALIC) << Formatter::get(Style::RED)
            << "No memory statistics available at the moment!"
            << Formatter::clear(Style::ITALIC) << Formatter::clear(Style::RED)
            << std::endl;
    }
    if (!ignore) {
        LSan::setIgnoreMalloc(false);
    }
}
