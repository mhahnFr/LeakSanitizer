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

bool   __lsan_humanPrint       = true;
bool   __lsan_printCout        = false;
bool   __lsan_printFormatted   = true;

#ifdef NO_LICENSE
bool   __lsan_printLicense     = false;
#else
bool   __lsan_printLicense     = true;
#endif

#ifdef NO_WEBSITE
bool   __lsan_printWebsite     = false;
#else
bool   __lsan_printWebsite     = true;
#endif

bool   __lsan_invalidCrash     = true;

bool   __lsan_invalidFree      = false;
bool   __lsan_freeNull         = false;

bool   __lsan_trackMemory      = false;
bool   __lsan_statsActive      = false;

bool   __lsan_printStatsOnExit = false;

size_t __lsan_leakCount        = 100;

size_t __lsan_callstackSize    = 20;


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

void initInternals() {
    // TODO: Implement
}
