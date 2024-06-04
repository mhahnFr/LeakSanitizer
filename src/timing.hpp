/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2024  mhahnFr
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

#ifndef timing_hpp
#define timing_hpp

#ifdef BENCHMARK
#include <chrono>
#include <deque>
#include <map>
#include <ostream>

namespace lsan::timing {
/**
 * This structure contatins the different timings.
 */
struct Timings {
    /** The amount of time the system took.               */
    std::deque<std::chrono::nanoseconds> system;
    /** The amount of time the mutex locking took.        */
    std::deque<std::chrono::nanoseconds> locking;
    /** The amount of time the rest of the tracking took. */
    std::deque<std::chrono::nanoseconds> tracking;
    /** The total time.                                   */
    std::deque<std::chrono::nanoseconds> total;
};

/**
 * This enumeration contains the possible allocation types.
 */
enum class AllocType {
    malloc, calloc, realloc, free
};

void addSystemTime(std::chrono::nanoseconds duration, AllocType type);
void addLockingTime(std::chrono::nanoseconds duration, AllocType type);
void addTrackingTime(std::chrono::nanoseconds duration, AllocType type);
void addTotalTime(std::chrono::nanoseconds duration, AllocType type);
auto printTimings(std::ostream& out) -> std::ostream&;
}

#define BENCH(expr, type, varName)                              \
const auto __now##varName { std::chrono::steady_clock::now() }; \
expr                                                            \
const auto __end##varName { std::chrono::steady_clock::now() }; \
type varName = std::chrono::duration_cast<type>(__end##varName - __now##varName)

#define BENCH_ONLY(block) block
#else
#define BENCH(expr, type, varName) expr
#define BENCH_ONLY(block)
#endif

#endif /* timing_hpp */
