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
 */
void printInformation();

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
 */
void maybeHintRelativePaths(std::ostream & out);

/**
 * Prints the hint about the relative paths, including the current working directory.
 *
 * @param out the output stream to print to
 */
void printWorkingDirectory(std::ostream & out);

auto isATTY() -> bool;

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
}

#endif /* lsanMisc_hpp */
