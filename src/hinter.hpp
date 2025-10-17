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

#ifndef hinter_hpp
#define hinter_hpp

#include <ostream>

namespace lsan::hinter {
/**
 * Prints a hint about how to influence the callstack size onto the given
 * output stream if the callstack size has been exceeded.
 *
 * @param out the output stream to print onto
 * @param exceeded whether to print the hint
 */
void maybeHintCallstackSize(std::ostream& out, bool exceeded);

/**
 * Prints an indicator for a hint.
 *
 * @param out the output stream to print onto
 * @return the given output stream
 */
static inline auto hintBegin(std::ostream& out) -> std::ostream& {
    return out << "  --   ";
}
}

#endif /* hinter_hpp */
