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

#include "helper.hpp"

namespace lsan::behaviour {
/**
 * Represents the settings of the behaviours this sanitizer supports.
 */
class Behaviour {
#define FROM_ENV(type, name, envName, def)                           \
private:                                                             \
    const std::optional<type> _##name = get<type>("LSAN_" #envName); \
public:                                                              \
    [[nodiscard]] constexpr inline auto name() const {               \
        return _##name.value_or(def);                                \
    }                                                                \
private:

    FROM_ENV(const char*, suppressionFiles, SUPPRESSION_FILES, nullptr)
    FROM_ENV(const char*, systemLibraryFiles, SYSTEM_LIBRARY_FILES, nullptr)
    FROM_ENV(std::size_t, callstackSize, CALLSTACK_SIZE, 20)
    FROM_ENV(bool, suppressionDevelopersMode, SUPPRESSION_DEVELOPER, false)
    FROM_ENV(bool, showIndirects, INDIRECT_LEAKS, false)
    FROM_ENV(bool, showReachables, REACHABLE_LEAKS, true)
    FROM_ENV(bool, humanPrint, HUMAN_PRINT, true)
    FROM_ENV(bool, printCout, PRINT_COUT, false)
    FROM_ENV(bool, printFormatted, PRINT_FORMATTED, true)
    FROM_ENV(bool, invalidCrash, INVALID_CRASH, true)
    FROM_ENV(bool, invalidFree, INVALID_FREE, true)
    FROM_ENV(bool, freeNull, FREE_NULL, false)
    FROM_ENV(bool, zeroAllocation, ZERO_ALLOCATION, false)
    FROM_ENV(bool, printExitPoint, PRINT_EXIT_POINT, false)
    FROM_ENV(bool, printBinaries, PRINT_BINARIES, true)
    FROM_ENV(bool, printFunctions, PRINT_FUNCTIONS, true)
    FROM_ENV(bool, relativePaths, RELATIVE_PATHS, true)

    /** Whether to activate the statistical bookkeeping.                 */
    const std::optional<bool> _statsActive = get<bool>("LSAN_STATS_ACTIVE");

    /** The time interval between the automatic statistics printing.     */
    const std::optional<std::chrono::nanoseconds> _autoStats = get<std::chrono::nanoseconds>("LSAN_AUTO_STATS");

    /**
     * Returns whether the stats have been activated using an environment
     * variable or by the C API.
     *
     * @return whether the stats should be active
     */
    [[nodiscard]] auto statsActiveInternal() const -> bool {
        return _statsActive ? *_statsActive : false;
    }

public:
    /**
     * Returns whether the stats should be active.
     *
     * @return whether to activate the statistical bookkeeping.
     */
    [[nodiscard]] auto statsActive() const -> bool {
        return statsActiveInternal() || _autoStats;
    }

    /**
     * Returns the optionally set time interval between automatically stats printing.
     *
     * @return the optional time interval
     */
    [[nodiscard]] constexpr auto autoStats() const {
        return _autoStats;
    }

#undef FROM_ENV
};
}

#endif /* Behaviour_hpp */
