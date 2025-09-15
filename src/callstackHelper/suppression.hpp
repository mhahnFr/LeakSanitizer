/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2025  mhahnFr
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

#ifndef callstackHelper_suppression_hpp
#define callstackHelper_suppression_hpp

#include <callstack.h>

#include "../suppression/Suppression.hpp"

namespace lsan::callstackHelper {
/**
 * Returns whether the given callstack is suppressed by the given suppression.
 *
 * @param suppression the suppression
 * @param callstack the callstack to be checked
 * @return whether the callstack is suppressed by the given suppression
 */
auto isSuppressed(const suppression::Suppression& suppression, lcs::callstack& callstack) -> bool;

/**
 * @brief Returns whether the given callstack is suppressed by at least one
 * suppression of the given range.
 *
 * A callstack is never matched by an empty range of suppressions.
 *
 * @param suppBegin the beginning of the suppressions range
 * @param suppEnd the end of the suppressions range
 * @param callstack the callstack to be checked
 * @return whether the callstack was matched
 */
template<typename It>
constexpr inline auto isSuppressed(It suppBegin, It suppEnd, lcs::callstack& callstack) -> bool {
    for (; suppBegin != suppEnd; ++suppBegin) {
        if (isSuppressed(*suppBegin, callstack)) {
            return true;
        }
    }
    return false;
}
}

#endif /* callstackHelper_suppression_hpp */
