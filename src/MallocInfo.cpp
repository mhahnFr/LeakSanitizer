/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2022 - 2024  mhahnFr
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

#include "MallocInfo.hpp"

#include "formatter.hpp"
#include "bytePrinter.hpp"

namespace lsan {
static bool printIndirects = false; // TODO: Make this configurable

/**
 * Returns whether the first given leak type is greater than the other one.
 *
 * @param lhs the first leak type
 * @param rhs the second leak type
 * @return whether the first leak type is considered to be greater
 */
static inline auto isConsideredGreater(const LeakType& lhs, const LeakType& rhs) -> bool {
    if (lhs == LeakType::unreachableIndirect && rhs == LeakType::unreachableDirect) {
        return true;
    }
    if (lhs == LeakType::unreachableDirect && rhs == LeakType::unreachableIndirect) {
        return false;
    }

    return lhs > rhs;
}

template<typename F, typename... Args>
static inline void forEachIndirect(bool mark, const MallocInfo& info, F func, Args... args) {
    for (const auto& record : info.viaMeRecords) {
        if (isIndirect(record->leakType) && isConsideredGreater(record->leakType, info.leakType) && !record->printedInRoot) {
            func(*record, std::forward<Args>(args)...);
            record->printedInRoot = mark;
        }
    }
}

auto operator<<(std::ostream& stream, const MallocInfo& self) -> std::ostream& {
    self.print(stream);
    return stream;
}

void MallocInfo::print(std::ostream& stream, unsigned long indent, unsigned long number, unsigned long indent2) const {
    using namespace formatter;

    const auto& indentString = std::string(indent, ' ');
    if (number > 0) {
        const auto& numberString = std::to_string(number);
        stream << std::string(indent2, ' ') << get<Style::AMBER>
               << "#" << std::string(indent - numberString.size() - 2, ' ') << numberString
               << clear<Style::AMBER> << ' ';
    } else {
        stream << indentString;
    }
    stream << get<Style::ITALIC>
           << format<Style::BOLD, Style::RED>("Leak") << " of size "
           << clear<Style::ITALIC>
           << bytesToString(size) << get<Style::ITALIC> << ", " << leakType;
    if (imageName) {
        stream << " in " << format<Style::BLUE>(*imageName); // TODO: Relativate?
    }

    std::size_t count { 0 },
                bytes { 0 };
    forEachIndirect(!printIndirects, *this, [&](const auto& record) {
        ++count;
        bytes += record.size;
    });
    if (count > 0) {
        stream << ", " << count << " leak" << (count > 1 ? "s" : "") << " (" << bytesToString(bytes) << ") indirect";
    }
    stream << std::endl;
    printCreatedCallstack(stream, indentString);

    if (printIndirects && count > 0) {
        stream << std::endl << indentString << get<Style::AMBER> << "Indirect leak" << (count > 1 ? "s" : "") << ":" << clear<Style::AMBER>;
        const auto& print = count > 1;
        const auto& newIndent = indent + (print ? std::to_string(count).size() : 0) + 3;
        forEachIndirect(true, *this, [&](const auto& record) {
            stream << std::endl;
            record.print(stream, newIndent, print ? ++number : 0, indent);
        });
        stream << indentString << format<Style::AMBER>("---------------") << std::endl;
    }
}
}
