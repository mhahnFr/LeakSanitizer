/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2023 - 2025  mhahnFr
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
#include <chrono>
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
static inline auto getVariable(const char* name) -> std::optional<const char*> {
    const char* var = getenv(name);

    return var == nullptr ? std::nullopt : std::optional(var);
}

/**
 * Retrieves a value of the given type from the given string.
 *
 * @param value the string to retrieve the value from
 * @return the optionally deducted value
 * @tparam T the type of the value to be deducted
 */
template<typename T>
constexpr auto getFrom(const char* value) -> std::optional<T>;

/**
 * Retrieves the value set in the environment variable of the given name.
 *
 * @param name the name of the environment variable
 * @return the optionally deducted value stored in the variable
 * @tparam T the type of the value to be deducted
 */
template<typename T>
constexpr auto get(const char* name) -> std::optional<T> {
    if (const auto var = getVariable(name)) {
        return getFrom<T>(*var);
    }
    return std::nullopt;
}

template<>
constexpr auto getFrom(const char* value) -> std::optional<std::size_t> {
    if (value == nullptr) {
        return std::nullopt;
    }

    std::size_t toReturn = 0;
    auto [_, err] = std::from_chars(value, value + strlen(value), toReturn);

    return err == std::errc() ? std::optional(toReturn) : std::nullopt;
}

/**
 * Compares the two given strings lowercased.
 *
 * @param string1 the first string
 * @param string2 the second string
 * @return whether the two given strings are lowercased equal
 */
static auto lowerCompare(const char * string1, const char * string2) -> bool {
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

template<>
constexpr auto getFrom(const char* value) -> std::optional<bool> {
    if (value == nullptr) {
        return std::nullopt;
    }

    if (lowerCompare(value, "true")) {
        return true;
    } else if (lowerCompare(value, "false")) {
        return false;
    }

    if (const auto number = getFrom<std::size_t>(value)) {
        return number != 0;
    }
    return std::nullopt;
}

template<>
inline auto getFrom(const char* value) -> std::optional<std::chrono::nanoseconds> {
    unsigned long count = 0;
    auto [ptr, err] = std::from_chars(value, value + strlen(value), count);

    if (err != std::errc()) {
        return std::nullopt;
    }

    if (strcmp(ptr, "ns") == 0) {
        return std::chrono::nanoseconds(count);
    } else if (strcmp(ptr, "us") == 0) {
        return std::chrono::microseconds(count);
    } else if (strcmp(ptr, "ms") == 0) {
        return std::chrono::milliseconds(count);
    } else if (strcmp(ptr, "s") == 0 || *ptr == '\0') {
        return std::chrono::seconds(count);
    } else if (strcmp(ptr, "m") == 0) {
        return std::chrono::minutes(count);
    } else if (strcmp(ptr, "h") == 0) {
        return std::chrono::hours(count);
    }

    return std::nullopt;
}

template<>
constexpr auto getFrom(const char* value) -> std::optional<const char*> {
    return value == nullptr ? std::nullopt : std::optional(value);
}
}

#endif /* helper_hpp */
