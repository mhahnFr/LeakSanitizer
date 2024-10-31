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

#ifndef Object_hpp
#define Object_hpp

#include <map>
#include <optional>

#include "Value.hpp"

namespace lsan::json {
struct Object {
    ObjectContent content;

    inline Object(const ObjectContent& content): content(content) {}

    template<typename T>
    constexpr inline auto get(const std::string& name) -> std::optional<T> {
        const auto& it = content.find(name);
        if (it != content.end()) {
            return std::get<T>(it->second.value);
        }
        return std::nullopt;
    }

    template<ValueType T>
    constexpr inline auto get(const std::string& name) {
        return get<typename Trait<T>::Type>(name);
    }

    inline auto getObject(const std::string& name) -> std::optional<Object> {
        if (auto object = get<ValueType::Object>(name)) {
            return Object { *object };
        }
        return std::nullopt;
    }
};
}

#endif /* Object_hpp */
