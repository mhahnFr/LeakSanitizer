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

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

#include <lsan_stats.h>

#include "../lsanMisc.hpp"

namespace lsan {
namespace {
/**
 * Cares about the automatic statistics printing.
 */
class AutoStats {
    /** Whether the printing thread is allowed to run.             */
    bool run = true;
    /** The interval between the prints.                           */
    std::chrono::nanoseconds interval;
    /** The printing thread.                                       */
    std::thread statsThread;
    /** The mutex to synchronize with the printing thread.         */
    std::mutex mutex;
    /** The condition variable for the printing thread to wait on. */
    std::condition_variable cv;

    /**
     * The loop of the printing thread.
     */
    inline void printer() {
        std::chrono::nanoseconds sleepTime { 0 };
        while (true) {
            {
                std::unique_lock lock { mutex };
                cv.wait_for(lock, sleepTime, [this] {
                    return !run;
                });
                if (!run) {
                    return;
                }
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
        }
    }

    inline ~AutoStats() {
        {
            std::lock_guard lock { mutex };
            run = false;
        }
        cv.notify_all();
        if (statsThread.joinable()) {
            statsThread.join();
        }
    }
};

/** The hidden global variable of the auto stats printer. */
static AutoStats autoStats;
}
}
