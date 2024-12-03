/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2023 - 2024  mhahnFr
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

#include <ostream>
#include <vector>

#include <callstack.h>

#include "../suppression/Suppression.hpp"

/** This namespace includes the helper functions for the callstacks. */
namespace lsan::callstackHelper {
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

namespace v2 {
using Suppressions = std::vector<suppression::Suppression>;

auto isSuppressed(const Suppressions& suppressions, lcs::callstack& callstack) -> bool;
auto isSuppressed(const suppression::Suppression& suppression, lcs::callstack& callstack) -> bool;
}
}

#endif /* callstackHelper_hpp */
