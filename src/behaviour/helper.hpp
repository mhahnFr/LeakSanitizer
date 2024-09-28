/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2023 - 2024  mhahnFr
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

#ifndef helper_hpp
#define helper_hpp

#include <cctype>
#include <charconv>
#include <cstdlib>
#include <cstring>
#include <optional>

namespace lsan::behaviour {
/**
 * Retrieves the environment variable with the given name.
 *
 * @param name the name of the environment variable to retrieve
 * @return an optional with a pointer to the content of the retrieved variable
 */
static inline auto getVariable(const char * name) -> std::optional<const char *> {
    const char * var = getenv(name);

    if (var == nullptr) {
        return std::nullopt;
    }
    return var;
}

/**
 * Converts and returns the given value to a `std::size_t`.
 *
 * @param value the value to be converted
 * @return an optional with the converted result
 */
static inline auto getSize_tFrom(const char * value) -> std::optional<std::size_t> {
    if (value == nullptr) {
        return std::nullopt;
    }

    std::size_t i = 0;
    auto [_, err] = std::from_chars(value, value + strlen(value), i);

    if (err == std::errc()) {
        return i;
    }
    return std::nullopt;
}

/**
 * Retrieves a `std::size_t` from the environment.
 *
 * @param name the name of the variable to be retrieved
 * @return an optional with the value of the variable
 */
static inline auto getSize_t(const char * name) -> std::optional<std::size_t> {
    auto var = getVariable(name);

    return var.has_value() ? getSize_tFrom(var.value()) : std::nullopt;
}

/**
 * Compares the two given strings lowercased.
 *
 * @param string1 the first string
 * @param string2 the second string
 * @return whether the two given strings are lowercased equal
 */
static inline auto lowerCompare(const char * string1, const char * string2) -> bool {
    const std::size_t len1 = strlen(string1),
                      len2 = strlen(string2);

    if (len1 != len2) {
        return false;
    }
    for (std::size_t i = 0; i < len1; ++i) {
        if (tolower(string1[i]) != tolower(string2[i])) {
            return false;
        }
    }
    return true;
}

/**
 * Retrieves a boolean value from the environment.
 *
 * @param name the name of the variable to be retrieved
 * @return an optional with the value of the retrieved variable
 */
static inline auto getBool(const char * name) -> std::optional<bool> {
    auto var = getVariable(name);
    if (!var.has_value()) {
        return std::nullopt;
    }

    auto s = var.value();
    if (lowerCompare(s, "true")) {
        return true;
    } else if (lowerCompare(s, "false")) {
        return false;
    }

    auto i = getSize_tFrom(s);
    if (!i.has_value()) {
        return std::nullopt;
    }
    return i.value() != 0;
}
}

#endif /* helper_hpp */
