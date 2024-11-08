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

#include "Suppression.hpp"

#include <cassert>
#include <dlfcn.h>

namespace lsan::suppression {
using namespace json;

static inline auto getFunctionPair(const std::string& name, const std::optional<long>& offset) -> std::pair<uintptr_t, uintptr_t> {
    // TODO: FIXME: Properly implement

    const auto& addr = dlsym(RTLD_DEFAULT, name.c_str());
    assert(addr != nullptr);
    if (offset) {
        const auto& rangeAddr = uintptr_t(addr) + *offset;
        return std::make_pair(rangeAddr, rangeAddr);
    }
    const auto& begin = uintptr_t(addr);
    return std::make_pair(begin, begin + 200); // FIXME: Length
}

Suppression::Suppression(const Object& object):
    name(object.get<ValueType::String>("name").value_or("<unnamed>")),
    size(object.get<ValueType::Int>("size"))
{
    const auto& functionArray = object.get<ValueType::Array>("functions");
    topCallstack.reserve(functionArray->size());
    for (const auto& functionObject : *functionArray) {
        std::string         name;
        std::optional<long> offset;

        if (functionObject.type == ValueType::Object) {
            const auto& theObject = Object(functionObject.as<ValueType::Object>());
            name = *theObject.get<ValueType::String>("name");
            offset = theObject.get<ValueType::Int>("offset");
        } else {
            name = functionObject.as<ValueType::String>();
        }
        topCallstack.push_back(getFunctionPair(name, offset));
    }
}
}
