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

#ifndef Value_hpp
#define Value_hpp

#include <variant>

#include "Trait.hpp"

namespace lsan::json {
struct Value {
    ValueType type;
    std::variant<
        Trait<ValueType::Int>::Type,
        Trait<ValueType::Array>::Type,
        Trait<ValueType::String>::Type,
        Trait<ValueType::Bool>::Type,
        Trait<ValueType::Object>::Type
    > value;

    template<ValueType T>
    constexpr inline auto as() const {
        return std::get<typename Trait<T>::Type>(value);
    }

    constexpr inline auto is(ValueType type) const -> bool {
        return Value::type == type;
    }
};
}

#endif /* Value_hpp */
