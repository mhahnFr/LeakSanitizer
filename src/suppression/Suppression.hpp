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

#ifndef Suppression_hpp
#define Suppression_hpp

#include <cstdint>
#include <optional>
#include <regex>
#include <utility>
#include <variant>
#include <vector>

#include <SimpleJSON/Object.hpp>

#include "../LeakType.hpp"

namespace lsan {
struct MallocInfo;
}

namespace lsan::suppression {
struct Suppression {
    enum class Type {
        regex, range
    };

    using RangeType = std::pair<uintptr_t, std::size_t>;
    using RegexType = std::vector<std::regex>;
    using RangeOrRegexType = std::pair<Type, std::variant<RegexType, RangeType>>;

    std::string name;
    std::optional<std::size_t> size;
    std::optional<LeakType> leakType;
    std::optional<std::regex> imageName;
    bool hasRegexes = false;

    template<Type T>
    auto getTopCallstack(unsigned long i) const -> const auto&;

    template<>
    inline constexpr auto getTopCallstack<Type::regex>(unsigned long i) const -> const auto& {
        return std::get<RegexType>(topCallstack[i].second);
    }

    template<>
    inline constexpr auto getTopCallstack<Type::range>(unsigned long i) const -> const auto& {
        return std::get<RangeType>(topCallstack[i].second);
    }

    std::vector<RangeOrRegexType> topCallstack;

    Suppression(const simple_json::Object& object);

    auto match(const MallocInfo& info) const -> bool;
};
}

#endif /* Suppression_hpp */
