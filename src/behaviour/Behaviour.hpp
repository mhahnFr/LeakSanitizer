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
class Behaviour {
    const std::optional<bool> _humanPrint     = get<bool>("LSAN_HUMAN_PRINT"),
                              _printCout      = get<bool>("LSAN_PRINT_COUT"),
                              _printFormatted = get<bool>("LSAN_PRINT_FORMATTED"),
                              _invalidCrash   = get<bool>("LSAN_INVALID_CRASH"),
                              _invalidFree    = get<bool>("LSAN_INVALID_FREE"),
                              _freeNull       = get<bool>("LSAN_FREE_NULL"),
                              _zeroAllocation = get<bool>("LSAN_ZERO_ALLOCATION"),
                              _statsActive    = get<bool>("LSAN_STATS_ACTIVE"),
                              _printExitPoint = get<bool>("LSAN_PRINT_EXIT_POINT"),
                              _printBinaries  = get<bool>("LSAN_PRINT_BINARIES"),
                              _printFunctions = get<bool>("LSAN_PRINT_FUNCTIONS"),
                              _relativePaths  = get<bool>("LSAN_RELATIVE_PATHS");

    const std::optional<std::size_t> _leakCount           = get<std::size_t>("LSAN_LEAK_COUNT"),
                                     _callstackSize       = get<std::size_t>("LSAN_CALLSTACK_SIZE"),
                                     _firstPartyThreshold = get<std::size_t>("LSAN_FIRST_PARTY_THRESHOLD");

    const std::optional<const char*> _firstPartyRegex = getVariable("LSAN_FIRST_PARTY_REGEX");

    const std::optional<std::chrono::nanoseconds> _autoStats = get<std::chrono::nanoseconds>("LSAN_AUTO_STATS");

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

    inline auto statsActive() const -> bool {
        return statsActiveInternal() || _autoStats;
    }

    constexpr inline auto autoStats() const {
        return _autoStats;
    }

#undef ENV_OR_API
};
}

#endif /* Behaviour_hpp */
