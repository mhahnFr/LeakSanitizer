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

#include <functionInfo/functionInfo.h>

#include "FunctionNotFoundException.hpp"
#include "Suppression.hpp"

namespace lsan::suppression {
using namespace json;

auto Suppression::getFunctionPair(const std::string& name,
                                  const std::optional<long>& offset,
                                  const std::optional<std::string>& library) -> std::pair<uintptr_t, std::size_t> {
    const auto& result = functionInfo_loadHint(name.c_str(), library ? library->c_str() : nullptr);
    if (!result.found) {
        throw FunctionNotFoundException(name, Suppression::name);
    }
    if (offset) {
        return std::make_pair(result.begin + *offset, 0);
    }
    return std::make_pair(result.begin, result.length);
}

Suppression::Suppression(const Object& object):
    name(object.get<ValueType::String>("name").value_or("<unnamed>")),
    size(object.get<ValueType::Int>("size"))
{
    const auto functionArray = object.get<ValueType::Array>("functions").value();
    if (functionArray.size() < 1) {
        throw std::runtime_error("Function array empty");
    }
    topCallstack.reserve(functionArray.size());
    for (const auto& functionObject : functionArray) {
        std::string         name;
        std::optional<long> offset;
        std::optional<std::string> libraryName;

        if (functionObject.type == ValueType::Object) {
            const auto& theObject = Object(functionObject);
            name = theObject.get<ValueType::String>("name").value();
            offset = theObject.get<ValueType::Int>("offset");
            libraryName = theObject.get<ValueType::String>("library");
        } else {
            name = functionObject.as<ValueType::String>();
        }
        topCallstack.push_back(getFunctionPair(name, offset, libraryName));
    }
}
}
