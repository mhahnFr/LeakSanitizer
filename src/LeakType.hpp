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

#ifndef LeakType_hpp
#define LeakType_hpp

#include <ostream>

namespace lsan {
enum class LeakType {
    reachableDirect,
    reachableIndirect,
    
    globalDirect,
    globalIndirect,
    
    tlvDirect,
    tlvIndirect,

    unreachableIndirect,
    unreachableDirect,
    
    unclassified
};

static inline auto isIndirect(const LeakType& type) -> bool {
    return type == LeakType::tlvIndirect
        || type == LeakType::globalIndirect
        || type == LeakType::reachableIndirect
        || type == LeakType::unreachableIndirect;
}

static inline auto operator<<(std::ostream& out, const LeakType& type) -> std::ostream& {
    switch (type) {
        case LeakType::unclassified:        out << "unclassified";           break;
        case LeakType::reachableDirect:     out << "stack";                  break;
        case LeakType::reachableIndirect:   out << "via stack";              break;
        case LeakType::unreachableDirect:   out << "lost";                   break;
        case LeakType::unreachableIndirect: out << "via lost";               break;
        case LeakType::globalDirect:        out << "global";                 break;
        case LeakType::globalIndirect:      out << "via global";             break;
        case LeakType::tlvDirect:           out << "thread-local value";     break;
        case LeakType::tlvIndirect:         out << "via thread-local value"; break;
    }
    return out;
}

constexpr static inline auto debugString(const LeakType& type) -> const char* {
    switch (type) {
        case LeakType::unclassified:        return "unclassified";
        case LeakType::reachableDirect:     return "reachableDirect";
        case LeakType::reachableIndirect:   return "reachableIndirect";
        case LeakType::unreachableDirect:   return "unreachableDirect";
        case LeakType::unreachableIndirect: return "unreachableIndirect";
        case LeakType::globalDirect:        return "globalDirect";
        case LeakType::globalIndirect:      return "globalIndirect";
        case LeakType::tlvDirect:           return "tlvDirect";
        case LeakType::tlvIndirect:         return "tlvIndirect";

        default: return "Unknown";
    }
}
}

#endif /* LeakType_hpp */
