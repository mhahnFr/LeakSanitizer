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

#ifndef Trait_hpp
#define Trait_hpp

#include <map>
#include <string>
#include <vector>

#include "ValueType.hpp"

namespace lsan::json {
struct Value;
using ObjectContent = std::map<std::string, Value>;

template<ValueType T>
struct Trait;

template<>
struct Trait<ValueType::Int> {
    using Type = long;
};

template<>
struct Trait<ValueType::Array> {
    using Type = std::vector<Value>;
};

template<>
struct Trait<ValueType::String> {
    using Type = std::string;
};

template<>
struct Trait<ValueType::Bool> {
    using Type = bool;
};

template<>
struct Trait<ValueType::Object> {
    using Type = ObjectContent;
};
}

#endif /* Trait_hpp */
