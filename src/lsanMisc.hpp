/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2023 - 2024  mhahnFr
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

#ifndef lsanMisc_hpp
#define lsanMisc_hpp

#include <iostream>

#include "LeakSani.hpp"

namespace lsan {
/**
 * Returns a reference to a boolean value indicating
 * whether to ignore allocations.
 *
 * @return the ignoration flag
 */
auto _getIgnoreMalloc() -> bool &;

/**
 * Returns the current instance of this sanitizer.
 *
 * @return the current instance
 */
auto getInstance() -> LSan &;

/**
 * Prints the additional information about this sanitizer.
 *
 * @param out the output stream to print to
 * @return the given output stream
 */
auto printInformation(std::ostream & out) -> std::ostream &;

/**
 * @brief The hook to be called on exit.
 *
 * It prints all informations tracked by the sanitizer and performs internal cleaning.
 */
void exitHook();

/**
 * Prints the note about the relative paths if relative paths are
 * allowed by `__lsan_relativePaths` on the given output stream.
 *
 * @param out the output stream to print to
 * @return the given output stream
 */
auto maybeHintRelativePaths(std::ostream & out) -> std::ostream &;

/**
 * Prints the hint about the relative paths, including the current working directory.
 *
 * @param out the output stream to print to
 * @return the given output stream
 */
auto printWorkingDirectory(std::ostream & out) -> std::ostream &;

/**
 * @brief Returns whether the output stream to print to is a TTY.
 *
 * If the POSIX function `isatty` is not available, `__lsan_printFormatted` is returned.
 *
 * @return whether the output stream to print to is an interactive terminal
 */
auto isATTY() -> bool;

/**
 * Returns whether the given variable has been set in the environment.
 *
 * @param var the variable to be checked
 * @return whether the variable name is in the environment
 */
auto has(const std::string & var) -> bool;

/**
 * Sets whether to ignore subsequent allocation management requests.
 *
 * @param ignoreMalloc whether to ignore allocations
 */
static inline void setIgnoreMalloc(const bool ignoreMalloc) {
    _getIgnoreMalloc() = ignoreMalloc;
}

/**
 * Returns whether to ignore subsequent alloction management requests.
 *
 * @return whether to ignore allocations
 */
static inline auto getIgnoreMalloc() -> bool {
    return _getIgnoreMalloc();
}

/**
 * Returns the current instance of the statistics object.
 *
 * @return the current statistics instance
 */
static inline auto getStats() -> const Stats & {
    return getInstance().getStats();
}

/**
 * Deletes the currently active instance of the sanitizer.
 */
static inline void internalCleanUp() {
    delete std::addressof(getInstance());
}

/**
 * Returns whether to print formatted, that is, whether `__lsan_printFormatted` is
 * `true` and the output stream is an interactive terminal.
 *
 * @return whether to print formatted
 */
static inline auto printFormatted() -> bool {
    if (has("LSAN_PRINT_FORMATTED")) {
        return __lsan_printFormatted;
    }
    return __lsan_printFormatted && isATTY();
}

/**
 * Returns the appropriate output stream to print to.
 *
 * @return the output stream to print to
 */
static inline auto getOutputStream() -> std::ostream & {
    return __lsan_printCout ? std::cout : std::clog;
}
}

#endif /* lsanMisc_hpp */
