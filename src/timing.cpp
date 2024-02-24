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

#include "timing.hpp"

#include "formatter.hpp"

namespace lsan::timing {
auto getTimingMap() -> std::map<AllocType, Timings>& {
    return getInstance().getTimingMap();
}

void addSystemTime(std::chrono::nanoseconds duration, AllocType type) {
    getTimingMap()[type].system.push_back(duration);
}

void addLockingTime(std::chrono::nanoseconds duration, AllocType type) {
    getTimingMap()[type].locking.push_back(duration);
}

void addTrackingTime(std::chrono::nanoseconds duration, AllocType type) {
    getTimingMap()[type].tracking.push_back(duration);
}

void addTotalTime(std::chrono::nanoseconds duration, AllocType type) {
    getTimingMap()[type].total.push_back(duration);
}

static inline auto operator<<(std::ostream& out, const std::deque<std::chrono::nanoseconds>& values) -> std::ostream& {
    if (values.empty()) return out << formatter::format<formatter::Style::ITALIC>("(Not available)");
    
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
    out << "(min, max, avg): " << min.count() << " ns, " << max.count() << " ns, " << (static_cast<double>(total.count()) / values.size()) << " ns";
    return out;
}

static inline auto operator<<(std::ostream& out, const Timings& timings) -> std::ostream& {
    out << "  System time " << timings.system   << std::endl
        << " Locking time " << timings.locking  << std::endl
        << "Tracking time " << timings.tracking << std::endl
        << "   Total time " << timings.total    << std::endl;
    
    return out;
}

auto printTimings(std::ostream& out) -> std::ostream& {
    using formatter::Style;
    
    out << formatter::format<Style::BOLD>("Malloc timings")  << std::endl << getTimingMap()[AllocType::malloc]  << std::endl
        << formatter::format<Style::BOLD>("Calloc timings")  << std::endl << getTimingMap()[AllocType::calloc]  << std::endl
        << formatter::format<Style::BOLD>("Realloc timings") << std::endl << getTimingMap()[AllocType::realloc] << std::endl
        << formatter::format<Style::BOLD>("Free timings")    << std::endl << getTimingMap()[AllocType::free]    << std::endl;
    
    return out;
}
}
