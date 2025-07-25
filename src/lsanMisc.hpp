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

#ifndef lsanMisc_hpp
#define lsanMisc_hpp

#include <iostream>
#include <string>
#include <vector>

#include "LeakSani.hpp"
#include "suppression/Suppression.hpp"
#include "trackers/ATracker.hpp"

namespace lsan {
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
 * It prints all information tracked by the sanitizer and performs internal
 * cleaning.
 */
void exitHook();

/**
 * Prints the note about the relative paths if relative paths are allowed by
 * @c Behaviour::relativePaths() on the given output stream.
 *
 * @param out the output stream to print to
 * @return the given output stream
 */
auto maybeHintRelativePaths(std::ostream & out) -> std::ostream &;

/**
 * Prints the hint about the relative paths, including the current working
 * directory.
 *
 * @param out the output stream to print to
 * @return the given output stream
 */
auto printWorkingDirectory(std::ostream & out) -> std::ostream &;

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
 * Returns whether the given variable has been set in the environment.
 *
 * @param var the variable to be checked
 * @return whether the variable name is in the environment
 */
auto has(const std::string & var) -> bool;

/**
 * Prints the stacktrace of the exit point if requested.
 *
 * @param out the output stream to print to
 * @return the given output stream
 */
auto maybePrintExitPoint(std::ostream& out) -> std::ostream&;

/**
 * Returns the tracker instance to be used to track allocations.
 *
 * @return the tracker to be used
 */
auto getTracker() -> trackers::ATracker&;

/**
 * Loads the suppressions.
 *
 * @return the loaded suppressions
 */
auto loadSuppressions() -> std::vector<suppression::Suppression>;

/**
 * Loads the system library regexes.
 *
 * @return the loaded regexes
 */
auto loadSystemLibraries() -> std::vector<std::regex>;

/**
 * Loads and returns the suppressions to match thread-local memory leaks.
 *
 * @return the suppressions
 */
auto createTLVSuppression() -> std::vector<suppression::Suppression>;

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
 * Returns the behaviour object of the main class.
 *
 * @return the behaviour object of the main class
 */
static inline auto getBehaviour() -> const behaviour::Behaviour& {
    return getInstance().getBehaviour();
}

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
        return getBehaviour().printFormatted();
    }
    return getBehaviour().printFormatted() && isATTY();
}

/**
 * Returns the appropriate output stream to print to.
 *
 * @return the output stream to print to
 */
static inline auto getOutputStream() -> std::ostream & {
    return getBehaviour().printCout() ? std::cout : std::clog;
}

/**
 * Returns the suppressions.
 *
 * @return the suppressions
 */
static inline auto getSuppressions() -> const std::vector<suppression::Suppression>& {
    return getInstance().getSuppressions();
}

/**
 * Returns the system library regexes.
 *
 * @return the system library regexes
 */
static inline auto getSystemLibraries() -> const std::vector<std::regex>& {
    return getInstance().getSystemLibraries();
}

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

#endif /* lsanMisc_hpp */
