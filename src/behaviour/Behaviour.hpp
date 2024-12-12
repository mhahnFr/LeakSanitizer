/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2024  mhahnFr
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

#ifndef Behaviour_hpp
#define Behaviour_hpp

#include <chrono>

#include <lsan_internals.h>

#include "helper.hpp"

namespace lsan::behaviour {
/**
 * Represents the settings of the behaviours this sanitizer supports.
 */
class Behaviour {
    /** Whether to print human-readable.                                 */
    const std::optional<bool> _humanPrint     = get<bool>("LSAN_HUMAN_PRINT"),
    /** Whether to print to the standard output stream.                  */
                              _printCout      = get<bool>("LSAN_PRINT_COUT"),
    /** Whether to print using ANSI escape codes.                        */
                              _printFormatted = get<bool>("LSAN_PRINT_FORMATTED"),
    /** Whether to halt the program when invalid behaviour is detected.  */
                              _invalidCrash   = get<bool>("LSAN_INVALID_CRASH"),
    /** Whether to check for invalid `free`s.                            */
                              _invalidFree    = get<bool>("LSAN_INVALID_FREE"),
    /** Whether to check for `free`ing `NULL`.                           */
                              _freeNull       = get<bool>("LSAN_FREE_NULL"),
    /** Whether to check for zero allocations.                           */
                              _zeroAllocation = get<bool>("LSAN_ZERO_ALLOCATION"),
    /** Whether to activate the statistical bookkeeping.                 */
                              _statsActive    = get<bool>("LSAN_STATS_ACTIVE"),
    /** Whether to print the callstack of the exit point.                */
                              _printExitPoint = get<bool>("LSAN_PRINT_EXIT_POINT"),
    /** Whether to print the names of the binary files.                  */
                              _printBinaries  = get<bool>("LSAN_PRINT_BINARIES"),
    /** Whether to print function names when line numbers are available. */
                              _printFunctions = get<bool>("LSAN_PRINT_FUNCTIONS"),
    /** Whether to relativate paths.                                     */
                              _relativePaths  = get<bool>("LSAN_RELATIVE_PATHS");

    /** The amount of leaks to print.                                    */
    const std::optional<std::size_t> _leakCount           = get<std::size_t>("LSAN_LEAK_COUNT"),
    /** The amount of frames to print in callstacks.                     */
                                     _callstackSize       = get<std::size_t>("LSAN_CALLSTACK_SIZE"),
    /** The threshold for callstacks to be treated as user-relevant.     */
                                     _firstPartyThreshold = get<std::size_t>("LSAN_FIRST_PARTY_THRESHOLD");

    /** The regex to detect first party binary names.                    */
    const std::optional<const char*> _firstPartyRegex = getVariable("LSAN_FIRST_PARTY_REGEX");

    /** The time interval between the automatical statistics printing.   */
    const std::optional<std::chrono::nanoseconds> _autoStats = get<std::chrono::nanoseconds>("LSAN_AUTO_STATS");

    /**
     * Returns whether the stats have been activated using an environment
     * variable or by the the C API.
     *
     * @return whether the stats should be active
     */
    inline auto statsActiveInternal() const -> bool {
        return _statsActive ? *_statsActive : __lsan_statsActive;
    }

#define ENV_OR_API(name)                       \
inline auto name() const {                     \
    return _##name ? *_##name : __lsan_##name; \
}

public:
    ENV_OR_API(humanPrint)
    ENV_OR_API(printCout)
    ENV_OR_API(printFormatted)
    ENV_OR_API(invalidCrash)
    ENV_OR_API(invalidFree)
    ENV_OR_API(freeNull)
    ENV_OR_API(zeroAllocation)
    ENV_OR_API(printExitPoint)
    ENV_OR_API(printBinaries)
    ENV_OR_API(printFunctions)
    ENV_OR_API(relativePaths)

    ENV_OR_API(leakCount)
    ENV_OR_API(callstackSize)
    ENV_OR_API(firstPartyThreshold)

    ENV_OR_API(firstPartyRegex)

    /**
     * Returns whether the stats should be active.
     *
     * @return whether to activate the statistical book-keeping.
     */
    inline auto statsActive() const -> bool {
        return statsActiveInternal() || _autoStats;
    }

    /**
     * Returns the optionally set time interval between automatical stats printing.
     *
     * @return the optional time interval
     */
    constexpr inline auto autoStats() const {
        return _autoStats;
    }

#undef ENV_OR_API
};
}

#endif /* Behaviour_hpp */
