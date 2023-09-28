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

#include "callstackHelper.hpp"

namespace callstackHelper {
static inline auto isInLSan(const void * frame) -> bool {
    // TODO: Implement
    return false;
}

auto isFirstParty(const std::string & file) -> bool {
    // TODO: Implement
    return false;
}

auto isCallstackFirstParty(lcs::callstack & callstack) -> bool {
    const auto frames = callstack_getBinaries(callstack);
    
    const auto frameCount = callstack_getFrameCount(callstack);
    for (size_t i = 0; i < frameCount; ++i) {
        if (!isFirstParty(frames[i]->binaryFile)) {
            return false;
        }
    }
    return true;
}

auto originatesInFirstParty(lcs::callstack & callstack) -> bool {
    // TODO: Implement
    return false;
}

void format(lcs::callstack & callstack, std::ostream & stream) {
    // TODO: Implement
}
}
