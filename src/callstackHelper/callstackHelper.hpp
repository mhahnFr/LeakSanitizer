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

#ifndef callstackHelper_hpp
#define callstackHelper_hpp

#include <callstack.h>
#include <ostream>
#include <string>

/** This namespace includes the helper functions for the callstacks. */
namespace lsan::callstackHelper {
/**
 * Formats the given callstack onto the given output stream.
 *
 * @param callstack the callstack
 * @param stream the stream to print to
 * @param indent the leading indentation to be used
 */
void format(lcs::callstack& callstack, std::ostream& stream, const std::string& indent = "");

/**
 * Formats the given callstack onto the given output stream.
 *
 * @param callstack the callstack
 * @param out the stream to print to
 * @param indent the leading indentation to be used
 */
static inline void format(lcs::callstack&& callstack, std::ostream& out, const std::string& indent = "") {
    format(callstack, out, indent);
}
}

#endif /* callstackHelper_hpp */
