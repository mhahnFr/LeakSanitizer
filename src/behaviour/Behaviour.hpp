/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2024 - 2025  mhahnFr
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
#define FROM_ENV(type, name, envName, def) \
private:\
    const std::optional<type> _##name = get<type>("LSAN_" #envName);\
public:\
    constexpr inline auto name() const {\
        return _##name.value_or(def);\
    }\
private:

#define FROM_ENV_API(type, name, envName) FROM_ENV(type, name, envName, __lsan_##name)

    FROM_ENV(bool, suppressionDevelopersMode, SUPPRESSION_DEVELOPER, false)
    FROM_ENV(bool, showIndirects, INDIRECT_LEAKS, false)
    FROM_ENV(bool, showReachables, REACHABLE_LEAKS, true)
    FROM_ENV(const char*, suppressionFiles, SUPPRESSION_FILES, nullptr)
    FROM_ENV(const char*, systemLibraryFiles, SYSTEM_LIBRARY_FILES, nullptr)

    FROM_ENV_API(bool, humanPrint,     HUMAN_PRINT)
    FROM_ENV_API(bool, printCout,      PRINT_COUT)
    FROM_ENV_API(bool, printFormatted, PRINT_FORMATTED)
    FROM_ENV_API(bool, invalidCrash,   INVALID_CRASH)
    FROM_ENV_API(bool, invalidFree,    INVALID_FREE)
    FROM_ENV_API(bool, freeNull,       FREE_NULL)
    FROM_ENV_API(bool, zeroAllocation, ZERO_ALLOCATION)
    FROM_ENV_API(bool, printExitPoint, PRINT_EXIT_POINT)
    FROM_ENV_API(bool, printBinaries,  PRINT_BINARIES)
    FROM_ENV_API(bool, printFunctions, PRINT_FUNCTIONS)
    FROM_ENV_API(bool, relativePaths,  RELATIVE_PATHS)

    /** Whether to activate the statistical bookkeeping.                 */
    const std::optional<bool> _statsActive = get<bool>("LSAN_STATS_ACTIVE");

    FROM_ENV_API(std::size_t, leakCount, LEAK_COUNT)
    FROM_ENV_API(std::size_t, callstackSize, CALLSTACK_SIZE)

    /** The time interval between the automatic statistics printing.     */
    const std::optional<std::chrono::nanoseconds> _autoStats = get<std::chrono::nanoseconds>("LSAN_AUTO_STATS");

    /**
     * Returns whether the stats have been activated using an environment
     * variable or by the C API.
     *
     * @return whether the stats should be active
     */
    auto statsActiveInternal() const -> bool {
        return _statsActive ? *_statsActive : __lsan_statsActive;
    }

public:
    /**
     * Returns whether the stats should be active.
     *
     * @return whether to activate the statistical bookkeeping.
     */
    auto statsActive() const -> bool {
        return statsActiveInternal() || _autoStats;
    }

    /**
     * Returns the optionally set time interval between automatically stats printing.
     *
     * @return the optional time interval
     */
    constexpr auto autoStats() const {
        return _autoStats;
    }

#undef FROM_ENV
#undef FROM_ENV_API
};
}

#endif /* Behaviour_hpp */
