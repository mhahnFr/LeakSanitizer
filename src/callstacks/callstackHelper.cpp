/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2023  mhahnFr
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

#include <optional>

#include "callstackHelper.hpp"

#include "../Formatter.hpp"
#include "../LeakSani.hpp"

#include <dlfcn.h>

namespace callstackHelper {
static inline auto isInLSan(const std::string & name) -> bool {
    return LSan::getInstance().getLibName() == name;
}

auto isFirstParty(const std::string & file) -> bool {
    // TODO: Platforms?
    
    return file.rfind("/usr/lib", 0) != std::string::npos
        || file.rfind("/System", 0) != std::string::npos;
}

auto isCallstackFirstParty(lcs::callstack & callstack) -> bool {
    const auto frames = callstack_getBinaries(callstack);
    
    const auto frameCount = callstack_getFrameCount(callstack);
    for (std::size_t i = 0; i < frameCount; ++i) {
        if (!isInLSan(frames[i]->binaryFile) && !isFirstParty(frames[i]->binaryFile)) {
            return false;
        }
    }
    return true;
}

auto originatesInFirstParty(lcs::callstack & callstack) -> bool {
    const auto frames = callstack_getBinaries(callstack);
    
    const auto frameCount = callstack_getFrameCount(callstack);
    std::size_t firstPartyCount = 0;
    for (std::size_t i = 0; i < frameCount; ++i) {
        if (isInLSan(frames[i]->binaryFile)) {
            continue;
        } else if (isFirstParty(frames[i]->binaryFile)) {
            if (++firstPartyCount >= 5) {
                return true;
            }
        } else {
            break;
        }
    }
    return false;
}

template<Formatter::Style S>
static inline void formatShared(const struct callstack_frame & frame, std::ostream & out) {
    using Formatter::Style;
    
    out << (frame.function == nullptr ? "<< Unknown >>" : frame.function);
    if (frame.sourceFile != nullptr) {
        out << " (" << Formatter::get<Style::GREYED, Style::UNDERLINED> << frame.sourceFile
            << ":" << frame.sourceLine << Formatter::clear<Style::GREYED, Style::UNDERLINED>;
        if (S == Style::GREYED || S == Style::BOLD) {
            out << Formatter::get<S>;
        }
        out << ")";
    }
    out << Formatter::clear<S> << std::endl;
}

void format(lcs::callstack & callstack, std::ostream & stream) {
    using Formatter::Style;
    
    const auto frames = callstack_toArray(callstack);
    const auto size   = callstack_getFrameCount(callstack);
    
    bool firstHit   = true,
         firstPrint = true;
    std::size_t i;
    for (i = 0; i < size && i < __lsan_callstackSize; ++i) {
        if (isInLSan(frames[i]->binaryFile)) {
            continue;
        } else if (firstHit && isFirstParty(frames[i]->binaryFile)) {
            stream << Formatter::get<Style::GREYED>
                   << Formatter::format<Style::ITALIC>(firstPrint ? "At: " : "at: ");
            formatShared<Style::GREYED>(*frames[i], stream);
        } else if (firstHit) {
            firstHit = false;
            stream << Formatter::get<Style::BOLD>
                   << Formatter::format<Style::ITALIC>(firstPrint ? "In: " : "in: ");
            formatShared<Style::BOLD>(*frames[i], stream);
        } else {
            stream << Formatter::format<Style::ITALIC>(firstPrint ? "At: " : "at: ");
            formatShared<Style::NONE>(*frames[i], stream);
        }
        firstPrint = false;
    }
    if (i < size) {
        stream << std::endl << Formatter::format<Style::UNDERLINED, Style::ITALIC>("And " + std::to_string(size - i) + " more lines...") << std::endl;
        LSan::getInstance().setCallstackSizeExceeded(true);
    }
}
}
