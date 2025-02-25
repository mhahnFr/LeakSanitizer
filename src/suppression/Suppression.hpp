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
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "json/Object.hpp"

#include "../LeakType.hpp"

namespace lsan {
struct MallocInfo;
}

namespace lsan::suppression {
struct Suppression {
    enum class Type {
        regex, range
    };

    std::string name;
    std::optional<std::size_t> size;
    std::optional<LeakType> leakType;
    std::optional<std::regex> imageName;
    bool hasRegexes = false;

    template<Type T>
    auto getTopCallstack(unsigned long i) const -> const auto&;

    template<>
    inline constexpr auto getTopCallstack<Type::regex>(unsigned long i) const -> const auto& {
        return std::get<std::vector<std::regex>>(topCallstack[i].second);
    }

    template<>
    inline constexpr auto getTopCallstack<Type::range>(unsigned long i) const -> const auto& {
        return std::get<std::pair<uintptr_t, std::size_t>>(topCallstack[i].second);
    }

    std::vector<std::pair<Type, std::variant<std::vector<std::regex>, std::pair<uintptr_t, std::size_t>>>> topCallstack;

    Suppression(const json::Object& object);

    auto match(const MallocInfo& info) const -> bool;
};
}

#endif /* Suppression_hpp */
