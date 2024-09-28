/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2022 - 2024  mhahnFr and contributors
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

#include <lsan_internals.h>

#include "behaviour/helper.hpp"

using namespace lsan::behaviour;

bool __lsan_humanPrint       = getBool("LSAN_HUMAN_PRINT")    .value_or(true);
bool __lsan_printCout        = getBool("LSAN_PRINT_COUT")     .value_or(false);
bool __lsan_printFormatted   = getBool("LSAN_PRINT_FORMATTED").value_or(true);
bool __lsan_printLicense     = getBool("LSAN_PRINT_LICENSE")  .value_or(true);
bool __lsan_printWebsite     = getBool("LSAN_PRINT_WEBSITE")  .value_or(true);

bool __lsan_invalidCrash     = getBool("LSAN_INVALID_CRASH")  .value_or(true);
bool __lsan_invalidFree      = getBool("LSAN_INVALID_FREE")   .value_or(true);
bool __lsan_freeNull         = getBool("LSAN_FREE_NULL")      .value_or(false);
bool __lsan_zeroAllocation   = getBool("LSAN_ZERO_ALLOCATION").value_or(false);

bool __lsan_trackMemory      = false; // Deprecated: No need to further allow usage. - mhahnFr
bool __lsan_statsActive      = getBool("LSAN_STATS_ACTIVE").value_or(false);

bool __lsan_printStatsOnExit = getBool("LSAN_PRINT_STATS_ON_EXIT").value_or(false);
bool __lsan_printExitPoint   = getBool("LSAN_PRINT_EXIT_POINT")   .value_or(false);

bool __lsan_printBinaries    = getBool("LSAN_PRINT_BINARIES") .value_or(true);
bool __lsan_printFunctions   = getBool("LSAN_PRINT_FUNCTIONS").value_or(true);
bool __lsan_relativePaths    = getBool("LSAN_RELATIVE_PATHS") .value_or(true);

std::size_t __lsan_leakCount           = getSize_t("LSAN_LEAK_COUNT")           .value_or(100);
std::size_t __lsan_callstackSize       = getSize_t("LSAN_CALLSTACK_SIZE")       .value_or(20);
std::size_t __lsan_firstPartyThreshold = getSize_t("LSAN_FIRST_PARTY_THRESHOLD").value_or(3);

const char * __lsan_firstPartyRegex = getVariable("LSAN_FIRST_PARTY_REGEX").value_or(nullptr);
