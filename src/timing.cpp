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

#include "timing.hpp"

#ifdef BENCHMARK

#include <tuple>

#include "formatter.hpp"

namespace lsan::timing {
/**
 * Returns the timing mapping.
 *
 * @return the mapping with the timings
 */
static inline auto getTimingMap() -> std::map<AllocType, Timings>& {
    return getInstance().getTimingMap();
}

void addSystemTime(const std::chrono::nanoseconds duration, const AllocType type) {
    std::lock_guard lock { getInstance().mutex };
    getTimingMap()[type].system.push_back(duration);
}

void addLockingTime(const std::chrono::nanoseconds duration, const AllocType type) {
    std::lock_guard lock { getInstance().mutex };
    getTimingMap()[type].locking.push_back(duration);
}

void addTrackingTime(const std::chrono::nanoseconds duration, const AllocType type) {
    std::lock_guard lock { getInstance().mutex };
    getTimingMap()[type].tracking.push_back(duration);
}

void addTotalTime(const std::chrono::nanoseconds duration, const AllocType type) {
    std::lock_guard lock { getInstance().mutex };
    getTimingMap()[type].total.push_back(duration);
}

/**
 * Returns the minimal, maximal and average value of the given nanoseconds.
 *
 * @param values the values to examine
 * @return the minimal, maximal and average values
 */
static inline auto getMinMaxAvg(std::deque<std::chrono::nanoseconds> values) -> std::tuple<std::chrono::nanoseconds,std::chrono::nanoseconds, double, double> {
    std::sort(values.begin(), values.end());
    std::chrono::nanoseconds total { 0 }, min { std::chrono::nanoseconds::max() }, max { 0 };
    for (const auto& value : values) {
        total += value;
        if (value < min) {
            min = value;
        }
        if (value > max) {
            max = value;
        }
    }
    double median { 0 };
    if (values.size() % 2 == 0) {
        const auto& value1 = values[values.size() / 2];
        const auto& value2 = values[values.size() / 2 + 1];
        median = (value1.count() + value2.count()) / 2;
    } else {
        median = values[values.size() / 2].count();
    }
    return std::make_tuple(min, max, static_cast<double>(total.count()) / values.size(), median);
}

static inline auto operator<<(std::ostream& out, const Timings& timings) -> std::ostream& {
    using namespace formatter;

    if (timings.tracking.empty() || timings.locking.empty() || timings.system.empty() || timings.total.empty()) {
        return out << format<Style::ITALIC>("(Not available)") << std::endl;
    }
    
    const auto& [systemMin, systemMax, systemAvg, systemMed] = getMinMaxAvg(timings.system);
    const auto& [lockingMin, lockingMax, lockingAvg, lockingMed] = getMinMaxAvg(timings.locking);
    const auto& [trackingMin, trackingMax, trackingAvg, trackingMed] = getMinMaxAvg(timings.tracking);
    const auto& [totalMin, totalMax, totalAvg, totalMed] = getMinMaxAvg(timings.total);

    out << "  System time (" << format<Style::GREEN>("min") << ", " << format<Style::RED>("max") << ", "
        << format<Style::MAGENTA>("avg") << ", " << format<Style::BOLD>("med") << "): "

        << get<Style::GREEN>   << systemMin.count() << " ns, " << clear<Style::GREEN>
        << get<Style::RED>     << systemMax.count() << " ns, " << clear<Style::RED>
        << get<Style::MAGENTA> << systemAvg         << " ns, " << clear<Style::MAGENTA>
        << get<Style::BOLD>    << systemMed         << " ns" << clear<Style::BOLD> << std::endl

        << " Locking time (" << format<Style::GREEN>("min") << ", " << format<Style::RED>("max") << ", "
        << format<Style::MAGENTA>("avg") << ", " << format<Style::BOLD>("med") << "): "

        << get<Style::GREEN>   << lockingMin.count() << " ns, " << clear<Style::GREEN>
        << get<Style::RED>     << lockingMax.count() << " ns, " << clear<Style::RED>
        << get<Style::MAGENTA> << lockingAvg         << " ns, " << clear<Style::MAGENTA>
        << get<Style::BOLD>    << lockingMed         << " ns" << clear<Style::BOLD> << std::endl

        << "Tracking time (" << format<Style::GREEN>("min") << ", " << format<Style::RED>("max") << ", "
        << format<Style::MAGENTA>("avg") << ", " << format<Style::BOLD>("med") << "): "

        << get<Style::GREEN>   << trackingMin.count() << " ns, " << clear<Style::GREEN>
        << get<Style::RED>     << trackingMax.count() << " ns, " << clear<Style::RED>
        << get<Style::MAGENTA> << trackingAvg         << " ns, " << clear<Style::MAGENTA>
        << get<Style::BOLD>    << trackingMed         << " ns" << clear<Style::BOLD> << std::endl

        << "   Total time (" << format<Style::GREEN>("min") << ", " << format<Style::RED>("max") << ", "
        << format<Style::MAGENTA>("avg") << ", " << format<Style::BOLD>("med") << "): "

        << get<Style::GREEN>   << totalMin.count() << " ns, " << clear<Style::GREEN>
        << get<Style::RED>     << totalMax.count() << " ns, " << clear<Style::RED>
        << get<Style::MAGENTA> << totalAvg         << " ns, " << clear<Style::MAGENTA>
        << get<Style::BOLD>    << totalMed         << " ns" << clear<Style::BOLD> << std::endl;

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
