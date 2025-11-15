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

#include <vector>

#include "LeakSani.hpp"
#include "callstackHelper/format.hpp"
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
 * Loads and returns the suppressions to match thread-local memory leaks.
 *
 * @return the suppressions
 */
auto createTLVSuppression() -> std::vector<suppression::Suppression>;

auto shouldActivate() -> bool;

namespace callstack {
static inline void format(lcs::callstack& callstack, std::ostream& out, const std::string& indent = "") {
    if (callstackHelper::format(callstack, out, indent)) {
        getInstance().setCallstackSizeExceeded(true);
    }
}

static inline void format(lcs::callstack&& callstack, std::ostream& out, const std::string& indent = "") {
    format(callstack, out, indent);
}
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
 * Returns the suppressions.
 *
 * @return the suppressions
 */
static inline auto getSuppressions() -> const std::vector<suppression::Suppression>& {
    return getInstance().getSuppressions();
}
}

#endif /* lsanMisc_hpp */
