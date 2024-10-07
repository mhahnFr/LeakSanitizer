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

#include <atomic>
#include <chrono>
#include <thread>

#include <lsan_stats.h>

#include "../lsanMisc.hpp"

namespace lsan {
namespace {
class AutoStats {
    static std::atomic_bool run;
    static std::chrono::microseconds interval;

    std::thread statsThread;
    bool threadRunning = false;

    static inline void printer() {
        while (run) {
            const auto& begin = std::chrono::system_clock::now();
            __lsan_printStats();
            __lsan_printFStats();
            const auto& elapsed = std::chrono::system_clock::now() - begin;
            if (elapsed < interval) {
                std::this_thread::sleep_for(interval - elapsed);
            }
        }
    }

public:
    inline AutoStats() {
        using namespace std::chrono_literals;

        if (getBehaviour().autoStatsActive()) {
            interval = 1s;
            statsThread = std::thread(printer);
            threadRunning = true;
        }
    }

    inline ~AutoStats() {
        run = false;
        if (threadRunning) {
            statsThread.join();
        }
    }
};

std::atomic_bool AutoStats::run = true;
std::chrono::microseconds AutoStats::interval;

static AutoStats autoStats;
}
}
