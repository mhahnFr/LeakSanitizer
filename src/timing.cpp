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

#include "timing.hpp"

#ifdef BENCHMARK

#include <tuple>
#include <optional>

#include "formatter.hpp"

namespace lsan::timing {
auto getTimingMap() -> std::map<AllocType, Timings>& {
    return getInstance().getTimingMap();
}

void addSystemTime(std::chrono::nanoseconds duration, AllocType type) {
    std::lock_guard lock { getInstance().mutex };
    getTimingMap()[type].system.push_back(duration);
}

void addLockingTime(std::chrono::nanoseconds duration, AllocType type) {
    std::lock_guard lock { getInstance().mutex };
    getTimingMap()[type].locking.push_back(duration);
}

void addTrackingTime(std::chrono::nanoseconds duration, AllocType type) {
    std::lock_guard lock { getInstance().mutex };
    getTimingMap()[type].tracking.push_back(duration);
}

void addTotalTime(std::chrono::nanoseconds duration, AllocType type) {
    std::lock_guard lock { getInstance().mutex };
    getTimingMap()[type].total.push_back(duration);
}

/**
 * Returns the minimal, maximal and average value of the given nanoseconds.
 *
 * @param values the values to examine
 * @return the minimal, maximal and average values
 */
static inline auto getMinMaxAvg(const std::deque<std::chrono::nanoseconds>& values) -> std::tuple<std::chrono::nanoseconds, std::chrono::nanoseconds, double> {
    std::chrono::nanoseconds total{0}, min{std::chrono::nanoseconds::max()}, max{0};
    for (const auto value : values) {
        total += value;
        if (value < min) {
            min = value;
        }
        if (value > max) {
            max = value;
        }
    }
    return std::make_tuple(min, max, static_cast<double>(total.count()) / values.size());
}

static inline auto operator<<(std::ostream& out, const Timings& timings) -> std::ostream& {
    if (timings.tracking.empty() || timings.locking.empty() || timings.system.empty() || timings.total.empty()) {
        return out << formatter::format<formatter::Style::ITALIC>("(Not available)") << std::endl;
    }
    
    const auto& [systemMin, systemMax, systemAvg, systemMed] = getMinMaxAvg(timings.system);
    const auto& [lockingMin, lockingMax, lockingAvg, lockingMed] = getMinMaxAvg(timings.locking);
    const auto& [trackingMin, trackingMax, trackingAvg, trackingMed] = getMinMaxAvg(timings.tracking);
    const auto& [totalMin, totalMax, totalAvg, totalMed] = getMinMaxAvg(timings.total);

    const double totalMinD = static_cast<double>(totalMin.count()),
                 totalMaxD = static_cast<double>(totalMax.count());

    const double sysPartMin   = (systemMin.count() / totalMinD) * 100,
                 sysPartMax   = (systemMax.count() / totalMaxD) * 100,
                 sysPartAvg   = (systemAvg / totalAvg) * 100,
                 lockPartMin  = (lockingMin.count() / totalMinD) * 100,
                 lockPartMax  = (lockingMax.count() / totalMaxD) * 100,
                 lockPartAvg  = (lockingAvg / totalAvg) * 100,
                 trackPartMin = (trackingMin.count() / totalMinD) * 100,
                 trackPartMax = (trackingMax.count() / totalMaxD) * 100,
                 trackPartAvg = (trackingAvg / totalAvg) * 100;

    out << "  System time (min, max, avg): "
        << systemMin.count() << " ns (" << sysPartMin << " %), "
        << systemMax.count() << " ns (" << sysPartMax << " %), "
        << systemAvg         << " ns (" << sysPartAvg << " %)" << std::endl

        << " Locking time (min, max, avg): " 
        << lockingMin.count() << " ns (" << lockPartMin << " %), "
        << lockingMax.count() << " ns (" << lockPartMax << " %), "
        << lockingAvg         << " ns (" << lockPartAvg << " %)" << std::endl

        << "Tracking time (min, max, avg): " 
        << trackingMin.count() << " ns (" << trackPartMin << " %), "
        << trackingMax.count() << " ns (" << trackPartMax << " %), "
        << trackingAvg         << " ns (" << trackPartAvg << " %)" << std::endl

        << "   Total time (min, max, avg): " 
        << totalMin.count() << " ns, "
        << totalMax.count() << " ns, "
        << totalAvg         << " ns" << std::endl;
    
    return out;
}

auto printTimings(std::ostream& out) -> std::ostream& {
    using formatter::Style;

    std::lock_guard lock { getInstance().mutex };

    out << formatter::format<Style::BOLD>("Malloc timings")  << std::endl << getTimingMap()[AllocType::malloc]  << std::endl
        << formatter::format<Style::BOLD>("Calloc timings")  << std::endl << getTimingMap()[AllocType::calloc]  << std::endl
        << formatter::format<Style::BOLD>("Realloc timings") << std::endl << getTimingMap()[AllocType::realloc] << std::endl
        << formatter::format<Style::BOLD>("Free timings")    << std::endl << getTimingMap()[AllocType::free]    << std::endl;
    
    return out;
}
}

#endif
