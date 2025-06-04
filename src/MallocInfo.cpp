/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2022 - 2025  mhahnFr
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

#include <limits>

#include "MallocInfo.hpp"

#include "bytePrinter.hpp"
#include "formatter.hpp"

namespace lsan {
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
constexpr void MallocInfo::forEachIndirect(const bool mark, F func, Args... args) const {
    for (const auto& leak : viaMeRecords) {
        leak.get().flag = false;
    }
    for (const auto& leak : viaMeRecords) {
        if (auto& record = leak.get(); isIndirect(record.leakType) && isConsideredGreater(record.leakType, leakType) && !record.printedInRoot && !record.suppressed && !record.flag) {
            func(record, std::forward<Args>(args)...);
            record.printedInRoot = mark;
            record.flag = true;
        }
    }
}

void MallocInfo::markSuppressed() {
    suppressed = true;
}

auto MallocInfo::enumerate() -> std::pair<std::size_t, std::size_t> {
    std::size_t count { 0 },
                bytes { 0 };

    for (const auto& leak : viaMeRecords) {
        if (auto& record = leak.get(); isIndirect(record.leakType) && isConsideredGreater(record.leakType, leakType) && !record.suppressed && !record.enumerated) {
            ++count;
            bytes += record.size;
            record.enumerated = true;
        }
    }
    enumerated = true;

    return std::make_pair(count, bytes);
}

auto operator<<(std::ostream& stream, const MallocInfo& self) -> std::ostream& {
    self.print(stream);
    return stream;
}

/**
 * Returns the best of the given paths.
 *
 * @param path the pair with the relative and absolute path
 * @return the path to be used
 */
static inline auto maybeRelativate(const std::pair<const char*, const char*>& path) -> const char* {
    if (!getBehaviour().relativePaths()) return path.first;

    const auto& s1 = path.first == nullptr ? std::numeric_limits<std::size_t>::max() : std::strlen(path.first);
    const auto& s2 = path.second == nullptr ? std::numeric_limits<std::size_t>::max() : std::strlen(path.second);
    return s2 < s1 ? path.second : path.first;
}

void MallocInfo::print(std::ostream& stream, unsigned long indent, unsigned long number, const unsigned long indent2) const {
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
           << clear<Style::ITALIC> << bytesToString(size) << get<Style::ITALIC>;
    auto print = true;
    if (getInstance().getIsThreaded()) {
        const auto& desc = getInstance().getThreadDescription(threadId);

        print = desc.c_str() != imageName.first;
        stream << ", " << clear<Style::ITALIC> << desc << get<Style::ITALIC>;
    }
    stream << ", " << leakType;
    if (print && imageName.first != nullptr) {
        stream << " in " << format<Style::BLUE>(maybeRelativate(imageName));
    }

    std::size_t count { 0 },
                bytes { 0 };
    forEachIndirect(!getBehaviour().showIndirects(), [&](const auto& record) {
        ++count;
        bytes += record.size;
    });
    if (count > 0) {
        getInstance().hadIndirects = true;
        stream << ", " << count << " leak" << (count > 1 ? "s" : "") << " (" << bytesToString(bytes) << ") indirect";
    }
    stream << std::endl;
    printCreatedCallstack(stream, indentString);

    if (getBehaviour().showIndirects() && count > 0) {
        stream << std::endl << indentString << get<Style::AMBER> << "Indirect leak" << (count > 1 ? "s" : "") << ":" << clear<Style::AMBER>;
        const auto& shouldPrint = count > 1;
        const auto& newIndent = indent + (shouldPrint ? std::to_string(count).size() : 0) + 3;
        forEachIndirect(true, [&](const auto& record) {
            stream << std::endl;
            record.print(stream, newIndent, shouldPrint ? ++number : 0, indent);
        });
        stream << indentString << format<Style::AMBER>("---------------") << std::endl;
    }
}

auto MallocInfo::getThreadId() -> unsigned long {
    return getInstance().getThreadId();
}
}
