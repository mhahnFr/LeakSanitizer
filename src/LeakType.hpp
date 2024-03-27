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

#ifndef LeakType_hpp
#define LeakType_hpp

#include <ostream>

namespace lsan {
enum class LeakType {
    unclassified,
    
    reachableDirect,
    reachableIndirect,
    
    unreachableDirect,
    unreachableIndirect,
};

static inline auto operator<<(std::ostream& out, LeakType type) -> std::ostream& {
    switch (type) {
        case LeakType::unclassified:        out << "unclassified";        break;
        case LeakType::reachableDirect:     out << "reachableDirect";     break;
        case LeakType::reachableIndirect:   out << "reachableIndirect";   break;
        case LeakType::unreachableDirect:   out << "unreachableDirect";   break;
        case LeakType::unreachableIndirect: out << "unreachableIndirect"; break;
    }
    return out;
}
}

#endif /* LeakType_hpp */
