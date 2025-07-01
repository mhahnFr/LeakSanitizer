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
/**
 * Represents a memory leak suppression.
 */
struct Suppression {
    /** An enumeration of possible types for the callstack entries. */
    enum class Type {
        /** A regular expression. */
        regex,
        /** A memory range.       */
        range
    };

    /** The type used for memory ranges.                            */
    using RangeType = std::pair<uintptr_t, std::size_t>;
    /** The type used for regular expressions.                      */
    using RegexType = std::vector<std::regex>;
    /** Helper type containing both of the regex and range types.   */
    using RangeOrRegexType = std::pair<Type, std::variant<RegexType, RangeType>>;

    /** The name of this suppression.                               */
    std::string name;
    /** The size of the memory leak in question.                    */
    std::optional<std::size_t> size;
    /** The leak type of the memory leak t be suppressed.           */
    std::optional<LeakType> leakType;
    /**
     * The regular expression matching the runtime image name the memory leak
     * originates in.
     */
    std::optional<std::regex> imageName;
    /** Whether this suppression contains regular expressions.      */
    bool hasRegexes = false;
    /** The top callstack to be matched.                            */
    std::vector<RangeOrRegexType> topCallstack;

    /**
     * Constructs a suppression from the given JSON object.
     *
     * @param object the JSON object to deduct
     */
    explicit Suppression(const simple_json::Object& object);

    /**
     * Returns the suppression callstack entry at the given index of the given type.
     *
     * @tparam T the type of the suppression callstack entry
     * @param i the index into the callstack array
     * @return the suppression callstack entry
     */
    template<Type T>
    auto getTopCallstack(unsigned long i) const -> const auto&;

    /**
     * Returns whether the given allocation record is matched by this suppression.
     *
     * @param info the allocation record to be checked
     * @return whether the given allocation record is matched by this suppression
     */
    auto match(const MallocInfo& info) const -> bool;
};

template<>
inline auto Suppression::getTopCallstack<Suppression::Type::regex>(const unsigned long i) const -> const auto& {
    return std::get<RegexType>(topCallstack[i].second);
}

template<>
inline auto Suppression::getTopCallstack<Suppression::Type::range>(const unsigned long i) const -> const auto& {
    return std::get<RangeType>(topCallstack[i].second);
}
}

#endif /* Suppression_hpp */
