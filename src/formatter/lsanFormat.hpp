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

#ifndef lsanFormat_hpp
#define lsanFormat_hpp

#include <iostream>
#include <string>

#include "../behaviour/getBehaviour.hpp"

namespace lsan {
/**
 * Returns whether the given variable has been set in the environment.
 *
 * @param var the variable to be checked
 * @return whether the variable name is in the environment
 */
auto has(const std::string& var) -> bool;

/**
 * @brief Returns whether the output stream to print to is a TTY.
 *
 * If the POSIX function @c isatty is not available, @c Behaviour::printFormatted()
 * is returned.
 *
 * @return whether the output stream to print to is an interactive terminal
 */
auto isATTY() -> bool;

/**
 * Prints the note about the relative paths if relative paths are allowed by
 * @c Behaviour::relativePaths() on the given output stream.
 *
 * @param out the output stream to print to
 * @return the given output stream
 */
auto maybeHintRelativePaths(std::ostream& out) -> std::ostream&;

/**
 * Prints the hint about the relative paths, including the current working
 * directory.
 *
 * @param out the output stream to print to
 * @return the given output stream
 */
auto printWorkingDirectory(std::ostream& out) -> std::ostream&;

/**
 * @brief Returns whether to print formatted.
 *
 * This condition is met when @c Behaviour::printFormatted() returns @c true
 * and the output stream prints onto an interactive terminal.
 *
 * @return whether to print formatted
 */
static inline auto printFormatted() -> bool {
    if (has("LSAN_PRINT_FORMATTED")) {
        return behaviour::getBehaviour().printFormatted();
    }
    return behaviour::getBehaviour().printFormatted() && isATTY();
}

/**
 * Returns the appropriate output stream to print to.
 *
 * @return the output stream to print to
 */
static inline auto getOutputStream() -> std::ostream& {
    return behaviour::getBehaviour().printCout() ? std::cout : std::clog;
}
}

#endif /* lsanFormat_hpp */
