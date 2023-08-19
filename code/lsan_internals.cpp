/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2022 - 2023  mhahnFr and contributors
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

#include <cstdlib>
#include <optional>
#include <string>

#include "../include/lsan_internals.h"

static inline std::optional<std::string> getVariable(const std::string & name) {
    const char * var = getenv(name.c_str());
    
    if (var == nullptr) {
        return std::nullopt;
    }
    std::string s = var;
    for (auto it = s.begin(); it != s.end(); ++it) {
        *it = tolower(*it);
    }
    return s;
}

static inline std::optional<bool> getBool(const std::string & name) {
    auto var = getVariable(name);
    if (!var.has_value()) {
        return std::nullopt;
    }
    
    auto s = var.value();
    if (s == "true" || s == "1") {
        return true;
    } else if (s == "false" || s == "0") {
        return false;
    }
    return std::nullopt;
}

static inline std::optional<std::size_t> getSize_t(const std::string & name) {
    auto var = getVariable(name);
    
    if (!var.has_value()) {
        return std::nullopt;
    }
    
    auto s = var.value();
    try {
        return stoll(s);
    } catch (const std::exception &) {
        return std::nullopt;
    }
}

bool __lsan_humanPrint       = getBool("LSAN_HUMAN_PRINT")    .value_or(true);
bool __lsan_printCout        = getBool("LSAN_PRINT_COUT")     .value_or(false);
bool __lsan_printFormatted   = getBool("LSAN_PRINT_FORMATTED").value_or(true);

#ifdef NO_LICENSE
bool __lsan_printLicense     = getBool("LSAN_PRINT_LICENSE").value_or(false);
#else
bool __lsan_printLicense     = getBool("LSAN_PRINT_LICENSE").value_or(true);
#endif

#ifdef NO_WEBSITE
bool __lsan_printWebsite     = getBool("LSAN_PRINT_WEBSITE").value_or(false);
#else
bool __lsan_printWebsite     = getBool("LSAN_PRINT_WEBSITE").value_or(true);
#endif

bool __lsan_invalidCrash     = getBool("LSAN_INVALID_CRASH").value_or(true);

bool __lsan_invalidFree      = getBool("LSAN_INVALID_FREE").value_or(false);
bool __lsan_freeNull         = getBool("LSAN_FREE_NULL")   .value_or(false);

bool __lsan_trackMemory      = false; // Deprecated: No need to further allow usage. - mhahnFr
bool __lsan_statsActive      = getBool("LSAN_STATS_ACTIVE").value_or(false);

bool __lsan_printStatsOnExit = getBool("LSAN_PRINT_STATS_ON_EXIT").value_or(false);

std::size_t __lsan_leakCount     = getSize_t("LSAN_LEAK_COUNT").value_or(100);

std::size_t __lsan_callstackSize = getSize_t("LSAN_CALLSTACK_SIZE").value_or(20);
