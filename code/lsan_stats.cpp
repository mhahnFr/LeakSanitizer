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
#include <functional>
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

bool __lsan_fragmentationStatsAvailable() {
    return __lsan_trackMemory;
}

static inline void __lsan_printBar(size_t current, size_t peek, size_t width, const std::string & peekText, std::ostream & out) {
    using Formatter::Style;
    out << Formatter::get(Style::BOLD)
        << "["
        << Formatter::clear(Style::BOLD)
        << Formatter::get(Style::GREYED) << Formatter::get(Style::UNDERLINED);
    
    size_t i;
    for (i = 0; i < (static_cast<float>(current) / peek) * width; ++i) {
        out << Formatter::get(Style::BAR_FILLED);
    }
    for (; i < width; ++i) {
        out << Formatter::get(Style::BAR_EMPTY);
    }
    out << Formatter::clear(Style::BOLD) << Formatter::clear(Style::GREYED) << Formatter::clear(Style::UNDERLINED)
        << Formatter::get(Style::BOLD) << "]" << Formatter::clear(Style::BOLD)
        << " of "
        << Formatter::get(Style::BOLD) << peekText << Formatter::clear(Style::BOLD)
        << " peek" << std::endl << std::endl;

}

static inline void __lsan_printStatsCore(const std::string & statsName, size_t width, std::ostream & out, std::function<void (size_t, std::ostream &)> printBarBytes, std::function<void (size_t, std::ostream &)> printBarObjects) {
    using Formatter::Style;
    out << Formatter::get(Style::ITALIC)
        << "Stats of the " << statsName << " so far:"
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
    printBarBytes(width, out);
    
    out << Formatter::get(Style::BOLD)
        << __lsan_getCurrentMallocCount() << " objects"
        << Formatter::clear(Style::BOLD)
        << " currently in the heap, peek " << __lsan_getMallocPeek() << " objects." << std::endl;
     printBarObjects(width, out);
}

void __lsan_printFragmentationStats() {
    __lsan_printFragmentationStatsWidth(100);
}

static inline void __lsan_printFragmentationObjectBar(size_t width, std::ostream & out) {
    using Formatter::Style;
    out << Formatter::get(Style::BOLD)
        << "["
        << Formatter::clear(Style::BOLD)
        << Formatter::get(Style::GREYED) << Formatter::get(Style::UNDERLINED);
    
    const auto & infos = LSan::getInstance().getInfos();
    const float step = infos.size() / static_cast<float>(width);
    auto it = infos.cbegin();
    for (size_t i = 0; i < width; ++i) {
        const auto e = std::next(it, step);
        size_t fs = 0;
        for (; it != e; ++it) {
            if (it->second.isDeleted()) {
                ++fs;
            }
        }
        out << ((fs >= step / 2.0f) ? Formatter::get(Style::BAR_EMPTY) : Formatter::get(Style::BAR_FILLED));
    }
    out << Formatter::clear(Style::GREYED) << Formatter::clear(Style::UNDERLINED)
        << Formatter::get(Style::BOLD) << "]" << Formatter::clear(Style::BOLD)
        << " of "
        << Formatter::get(Style::BOLD) << __lsan_getMallocPeek() << " objects"
        << Formatter::clear(Style::BOLD) << " peek" << std::endl << std::endl;
}

static inline void __lsan_printFragmentationByteBar(size_t width, std::ostream & out) {
    using Formatter::Style;
    out << Formatter::get(Style::BOLD)
        << "["
        << Formatter::clear(Style::BOLD)
        << Formatter::get(Style::GREYED) << Formatter::get(Style::UNDERLINED);
    
    const auto & infos = LSan::getInstance().getInfos();
    auto it = infos.cbegin();
    size_t currentBlockBegin = 0,
           currentBlockEnd   = it->second.getSize(),
           b                 = 0;
    
    const float step = __lsan_getBytePeek() / static_cast<float>(width);
    for (size_t i = 0; i < width; ++i) {
        size_t fs = 0;
        for (size_t j = 0; j < step; ++j, ++b) {
            if (b - currentBlockBegin > currentBlockEnd) {
                ++it;
                currentBlockBegin = b;
                currentBlockEnd   = currentBlockBegin + it->second.getSize();
            }
            if (it->second.isDeleted()) {
                ++fs;
            }
        }
        out << ((fs >= step / 2.0f) ? Formatter::get(Style::BAR_EMPTY) : Formatter::get(Style::BAR_FILLED));
    }
    out << Formatter::clear(Style::GREYED) << Formatter::clear(Style::UNDERLINED)
        << Formatter::get(Style::BOLD) << "]" << Formatter::clear(Style::BOLD)
        << " of "
        << Formatter::get(Style::BOLD) << bytesToString(__lsan_getBytePeek())
        << Formatter::clear(Style::BOLD) << " peek" << std::endl << std::endl;
}

void __lsan_printFragmentationStatsWidth(size_t width) {
    using Formatter::Style;
    bool ignore = LSan::ignoreMalloc();
    std::ostream & out = __lsan_printCout ? std::cout : std::cerr;
    if (__lsan_fragmentationStatsAvailable()) {
        __lsan_printStatsCore("memory fragmentation", width, out,
                              __lsan_printFragmentationByteBar,
                              __lsan_printFragmentationObjectBar);
    } else {
        out << Formatter::get(Style::BOLD) << Formatter::get(Style::RED)
            << "No memory fragmentation stats available at the moment!" << std::endl
            << Formatter::clear(Style::BOLD) << Formatter::get(Style::ITALIC)
            << "Hint: Did you set "
            << Formatter::clear(Style::RED) << Formatter::clear(Style::ITALIC)
            << "__lsan_trackMemory"
            << Formatter::get(Style::ITALIC) << Formatter::get(Style::RED) << " to "
            << Formatter::clear(Style::RED) << Formatter::clear(Style::ITALIC)
            << "true" << Formatter::get(Style::RED) << Formatter::get(Style::ITALIC) << "?"
            << Formatter::clearAll() << std::endl;
    }
    if (!ignore) {
        LSan::setIgnoreMalloc(false);
    }
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
        __lsan_printStatsCore("memory usage", width, out,
                              std::bind(__lsan_printBar, __lsan_getCurrentByteCount(), __lsan_getBytePeek(), std::placeholders::_1, bytesToString(__lsan_getBytePeek()), std::placeholders::_2),
                              std::bind(__lsan_printBar, __lsan_getCurrentMallocCount(), __lsan_getMallocPeek(), std::placeholders::_1, std::to_string(__lsan_getMallocPeek()) + " objects", std::placeholders::_2));
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
