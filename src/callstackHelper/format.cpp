/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2023 - 2025  mhahnFr
 *
 * This file is part of the LeakSanitizer.
 *
 * The LeakSanitizer is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * The LeakSanitizer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with the
 * LeakSanitizer, see the file LICENSE.  If not, see <https://www.gnu.org/licenses/>.
 */

#define LCS_USE_UNSAFE_OPTIMIZATION 1

#include "format.hpp"

#include <callstack_internals.h>

#include "../formatter/formatter.hpp"
#include "../suppression/firstPartyLibrary.hpp"

namespace lsan::callstackHelper {
/**
 * Creates and indent string if the given indentation is bigger than zero.
 *
 * @param indent the amount of indentation
 * @param indentChar the character to indent with
 * @return a suitable indentation string
 */
static inline auto getIndent(const std::string::size_type indent, const char indentChar = ' ') -> std::string {
    return indent > 0 ? std::string(indent, indentChar) : std::string();
}

auto format(lcs::callstack& callstack, std::ostream& stream, const std::string& indent) -> bool {
    using formatter::Style;

    if (!callstack_autoClearCaches) {
        // Make sure to use the cached values and
        // potentially fail early.
        //
        //                          - mhahnFr
        [[unlikely]] if (callstack_getBinariesCached(callstack) == nullptr) {
            stream << indent << formatter::format<Style::RED>("LSan: Error: Failed to translate the callstack.") << std::endl;
            return false;
        }
    }
    const auto& frames = callstack_toArray(callstack);
    const auto& size   = callstack_getFrameCount(callstack);

    [[unlikely]] if (frames == nullptr) {
        stream << indent << formatter::format<Style::RED>("LSan: Error: Failed to translate the callstack.") << std::endl;
        return false;
    }

    bool firstHit   = true,
         firstPrint = true;
    std::size_t i, printed, maxCount = 1;

    if (behaviour::getBehaviour().callstackSize() > 9) {
        std::size_t toSkip = 0;
        if (size > 9) {
            for (; toSkip < size && (frames[toSkip].binaryFile == nullptr || frames[toSkip].binaryFileIsSelf); ++toSkip);
        }
        if (size - toSkip > 9) {
            if (size - toSkip < 100) {
                maxCount = 2;
            } else {
                maxCount = std::to_string(size - toSkip).size();
            }
        }
    }

    for (i = printed = 0; i < size && printed < behaviour::getBehaviour().callstackSize(); ++i) {
        if (const auto& binaryFile = frames[i].binaryFile; binaryFile == nullptr || (firstPrint && frames[i].binaryFileIsSelf)) {
            continue;
        } else if (firstHit && (suppression::isFirstParty(binaryFile, !callstack_autoClearCaches) || frames[i].binaryFileIsSelf)) {
            const auto& number = std::to_string(printed + 1);
            stream << indent << formatter::get<Style::GREYED>
                   << formatter::format<Style::ITALIC>("# " + getIndent(maxCount - number.size()) + number + ": ");
            formatFrame<Style::GREYED>(frames[i], stream);
        } else if (firstHit) {
            firstHit = false;
            stream << indent << formatter::get<Style::BOLD>
                   << formatter::format<Style::ITALIC>(getIndent(maxCount - 1) + " ->  ");
            formatFrame<Style::BOLD>(frames[i], stream);
        } else {
            const auto& number = std::to_string(printed + 1);
            stream << indent << formatter::format<Style::ITALIC>("# " + getIndent(maxCount - number.size()) + number + ": ");
            formatFrame<Style::NONE>(frames[i], stream);
        }
        firstPrint = false;
        ++printed;
    }
    auto toReturn = false;
    if (i < size) {
        stream << std::endl << indent << formatter::format<Style::UNDERLINED, Style::ITALIC>("And " + std::to_string(size - i) + " more line" + (size - i > 1 ? "s" : "") + "...") << std::endl;
        toReturn = true;
    }
    return toReturn;
}
}
