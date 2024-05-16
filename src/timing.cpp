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
    
    const auto system   = getMinMaxAvg(timings.system);
    const auto locking  = getMinMaxAvg(timings.locking);
    const auto tracking = getMinMaxAvg(timings.tracking);
    const auto total    = getMinMaxAvg(timings.total);
    
    const double totalMin = static_cast<double>(std::get<0>(total).count()),
                 totalMax = static_cast<double>(std::get<1>(total).count()),
                 totalAvg = std::get<2>(total);
    
    const double sysPartMin   = (std::get<0>(system).count() / totalMin) * 100,
                 sysPartMax   = (std::get<1>(system).count() / totalMax) * 100,
                 sysPartAvg   = (std::get<2>(system) / totalAvg) * 100,
                 lockPartMin  = (std::get<0>(locking).count() / totalMin) * 100,
                 lockPartMax  = (std::get<1>(locking).count() / totalMax) * 100,
                 lockPartAvg  = (std::get<2>(locking) / totalAvg) * 100,
                 trackPartMin = (std::get<0>(tracking).count() / totalMin) * 100,
                 trackPartMax = (std::get<1>(tracking).count() / totalMax) * 100,
                 trackPartAvg = (std::get<2>(tracking) / totalAvg) * 100;
    
    out << "  System time (min, max, avg): "
        << std::get<0>(system).count() << " ns (" << sysPartMin << " %), "
        << std::get<1>(system).count() << " ns (" << sysPartMax << " %), "
        << std::get<2>(system)         << " ns (" << sysPartAvg << " %)" << std::endl
    
        << " Locking time (min, max, avg): " 
        << std::get<0>(locking).count() << " ns (" << lockPartMin << " %), "
        << std::get<1>(locking).count() << " ns (" << lockPartMax << " %), "
        << std::get<2>(locking)         << " ns (" << lockPartAvg << " %)" << std::endl
    
        << "Tracking time (min, max, avg): " 
        << std::get<0>(tracking).count() << " ns (" << trackPartMin << " %), "
        << std::get<1>(tracking).count() << " ns (" << trackPartMax << " %), "
        << std::get<2>(tracking)         << " ns (" << trackPartAvg << " %)" << std::endl
    
        << "   Total time (min, max, avg): " 
        << std::get<0>(total).count() << " ns, "
        << std::get<1>(total).count() << " ns, "
        << std::get<2>(total)         << " ns" << std::endl;
    
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
