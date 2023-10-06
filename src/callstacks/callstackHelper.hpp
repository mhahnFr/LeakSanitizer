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

#ifndef callstackHelper_hpp
#define callstackHelper_hpp

#include <ostream>
#include <string>

#include "../../CallstackLibrary/include/callstack.h"

/** This namespace includes the helper functions for the callstacks. */
namespace lsan::callstackHelper {
/** An enumeration containing the possible callstack types.  */
enum class CallstackType {
    /** Indicates the callstack shoud be ignored completely. */
    HARD_IGNORE,
    /** Indicates the callstack is entirely first party.     */
    FIRST_PARTY,
    /** Indicates the callstack originates in first party.   */
    FIRST_PARTY_ORIGIN,
    /** Indicates the callstack is user relevant.            */
    USER
};

/**
 * Returns the type of the given callstack.
 *
 * @param callstack the callstack
 * @return the type of the callstack
 */
auto getCallstackType(lcs::callstack & callstack) -> CallstackType;

/**
 * Formats the given callstack onto the given output stream.
 *
 * @param callstack the callstack
 * @param stream the stream to print to
 */
void format(lcs::callstack & callstack, std::ostream & stream);

/**
 * Formats the given callstack onto the given output stream.
 *
 * @param callstack the callstack
 * @param out the stream to print to
 */
static inline void format(lcs::callstack && callstack, std::ostream & out) {
    format(callstack, out);
}
}

#endif /* callstackHelper_hpp */
