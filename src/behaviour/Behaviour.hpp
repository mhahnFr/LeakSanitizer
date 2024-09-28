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

#ifndef Behaviour_hpp
#define Behaviour_hpp

#include "helper.hpp"

namespace lsan::behaviour {
class Behaviour {
    bool _autoStatsActive;

public:
    inline Behaviour():
        _autoStatsActive(getBool("LSAN_AUTO_STATS"))
    {}

    // TODO: Combine with __lsan_statsActive!

    constexpr inline auto autoStatsActive() const -> bool {
        return _autoStatsActive;
    }
};
}

#endif /* Behaviour_hpp */
