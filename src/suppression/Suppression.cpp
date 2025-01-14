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

#include <functionInfo/functionInfo.h>

#include "FunctionNotFoundException.hpp"
#include "Suppression.hpp"

#include "../MallocInfo.hpp"

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

static inline constexpr auto asLeakType(const std::optional<unsigned long>& number) -> std::optional<LeakType> {
    if (number && *number > 10) {
        throw std::runtime_error("Not a leak type: " + std::to_string(*number));
    }
    return number ? std::optional(LeakType(*number)) : std::nullopt;
}

Suppression::Suppression(const Object& object):
    name(object.get<ValueType::String>("name").value_or("<unnamed>")),
    size(object.get<ValueType::Int>("size")),
    leakType(asLeakType(object.get<ValueType::Int>("type"))),
    imageName(object.get<ValueType::String>("imageName"))
{
    const auto& functionArray = object.get<ValueType::Array>("functions");
    if (!imageName && !functionArray) {
        throw std::runtime_error("Suppressions need either 'imageName' or 'functions'");
    }
    if (functionArray) {
        if (functionArray->size() < 1) {
            throw std::runtime_error("Function array empty");
        }
        topCallstack.reserve(functionArray->size());
        for (const auto& functionObject : *functionArray) {
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

auto Suppression::match(const MallocInfo& info) const -> bool {
    if (size && info.size != *size) return false;
    if (leakType && info.leakType != *leakType) return false;

    if (imageName && info.imageName.first != nullptr && !std::regex_match(info.imageName.first, *imageName)) return false;

    if (topCallstack.empty() && info.imageName.first == nullptr) return false;

    return topCallstack.empty() ? true : callstackHelper::isSuppressed(*this, info.createdCallstack);
}
}
