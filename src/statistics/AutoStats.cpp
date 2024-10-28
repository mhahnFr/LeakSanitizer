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
#include <condition_variable>
#include <mutex>
#include <thread>

#include <lsan_stats.h>

#include "../lsanMisc.hpp"

namespace lsan {
namespace {
class AutoStats {
    std::atomic_bool run = true;
    std::chrono::nanoseconds interval;

    std::thread statsThread;
    std::mutex mutex;
    std::condition_variable cv;
    bool threadRunning = false;

    inline void printer() {
        std::chrono::nanoseconds sleepTime { 0 };
        while (true) {
            std::unique_lock lock { mutex };
            cv.wait_for(lock, sleepTime);
            if (!run) {
                return;
            }
            const auto& begin = std::chrono::system_clock::now();
            __lsan_printStats();
            __lsan_printFStats();
            const auto& elapsed = std::chrono::system_clock::now() - begin;
            sleepTime = elapsed < interval ? interval - elapsed : std::chrono::nanoseconds { 0 };
        }
    }

public:
    inline AutoStats() {
        using namespace std::chrono_literals;

        if (auto duration = getBehaviour().autoStats()) {
            interval = *duration;
            statsThread = std::thread(&AutoStats::printer, this);
            threadRunning = true;
        }
    }

    inline ~AutoStats() {
        run = false;
        cv.notify_all();
        if (threadRunning) {
            statsThread.join();
        }
    }
};

static AutoStats autoStats;
}
}
