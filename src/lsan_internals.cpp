/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2022 - 2024  mhahnFr and contributors
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

#include <charconv>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <optional>

#include "../include/lsan_internals.h"

/**
 * Retrieves the environment variable with the given name.
 *
 * @param name the name of the environment variable to retrieve
 * @return an optional with a pointer to the content of the retrieved variable
 */
static inline auto getVariable(const char * name) -> std::optional<const char *> {
    const char * var = getenv(name);
    
    if (var == nullptr) {
        return std::nullopt;
    }
    return var;
}

/**
 * Converts and returns the given value to a `std::size_t`.
 *
 * @param value the value to be converted
 * @return an optional with the converted result
 */
static inline auto getSize_tFrom(const char * value) -> std::optional<std::size_t> {
    if (value == nullptr) {
        return std::nullopt;
    }
    
    std::size_t i = 0;
    auto [_, err] = std::from_chars(value, value + strlen(value), i);
    
    if (err == std::errc()) {
        return i;
    }
    return std::nullopt;
}

/**
 * Retrieves a `std::size_t` from the environment.
 *
 * @param name the name of the variable to be retrieved
 * @return an optional with the value of the variable
 */
static inline auto getSize_t(const char * name) -> std::optional<std::size_t> {
    auto var = getVariable(name);
    
    return var.has_value() ? getSize_tFrom(var.value()) : std::nullopt;
}

/**
 * Compares the two given strings lowercased.
 *
 * @param string1 the first string
 * @param string2 the second string
 * @return whether the two given strings are lowercased equal
 */
static inline auto lowerCompare(const char * string1, const char * string2) -> bool {
    const std::size_t len1 = strlen(string1),
                      len2 = strlen(string2);
    
    if (len1 != len2) {
        return false;
    }
    for (std::size_t i = 0; i < len1; ++i) {
        if (tolower(string1[i]) != tolower(string2[i])) {
            return false;
        }
    }
    return true;
}

/**
 * Retrieves a boolean value from the environment.
 *
 * @param name the name of the variable to be retrieved
 * @return an optional with the value of the retrieved variable
 */
static inline auto getBool(const char * name) -> std::optional<bool> {
    auto var = getVariable(name);
    if (!var.has_value()) {
        return std::nullopt;
    }
    
    auto s = var.value();
    if (lowerCompare(s, "true")) {
        return true;
    } else if (lowerCompare(s, "false")) {
        return false;
    }
    
    auto i = getSize_tFrom(s);
    if (!i.has_value()) {
        return std::nullopt;
    }
    return i.value() != 0;
}

bool __lsan_humanPrint       = getBool("LSAN_HUMAN_PRINT")    .value_or(true);
bool __lsan_printCout        = getBool("LSAN_PRINT_COUT")     .value_or(false);
bool __lsan_printFormatted   = getBool("LSAN_PRINT_FORMATTED").value_or(true);
bool __lsan_printLicense     = getBool("LSAN_PRINT_LICENSE")  .value_or(true);
bool __lsan_printWebsite     = getBool("LSAN_PRINT_WEBSITE")  .value_or(true);

bool __lsan_invalidCrash     = getBool("LSAN_INVALID_CRASH")  .value_or(true);
bool __lsan_invalidFree      = getBool("LSAN_INVALID_FREE")   .value_or(false);
bool __lsan_freeNull         = getBool("LSAN_FREE_NULL")      .value_or(false);
bool __lsan_zeroAllocation   = getBool("LSAN_ZERO_ALLOCATION").value_or(false);

bool __lsan_trackMemory      = false; // Deprecated: No need to further allow usage. - mhahnFr
bool __lsan_statsActive      = getBool("LSAN_STATS_ACTIVE").value_or(false);

bool __lsan_printStatsOnExit = getBool("LSAN_PRINT_STATS_ON_EXIT").value_or(false);
bool __lsan_printExitPoint   = getBool("LSAN_PRINT_EXIT_POINT")   .value_or(false);

bool __lsan_printBinaries    = getBool("LSAN_PRINT_BINARIES") .value_or(true);
bool __lsan_printFunctions   = getBool("LSAN_PRINT_FUNCTIONS").value_or(true);
bool __lsan_relativePaths    = getBool("LSAN_RELATIVE_PATHS") .value_or(true);

std::size_t __lsan_leakCount = getSize_t("LSAN_LEAK_COUNT").value_or(100);

std::size_t __lsan_callstackSize = getSize_t("LSAN_CALLSTACK_SIZE").value_or(20);

std::size_t __lsan_firstPartyThreshold = getSize_t("LSAN_FIRST_PARTY_THRESHOLD").value_or(3);

const char * __lsan_firstPartyRegex = getVariable("LSAN_FIRST_PARTY_REGEX").value_or(nullptr);
