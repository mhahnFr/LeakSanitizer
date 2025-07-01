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

#include "Suppression.hpp"
#include "FunctionNotFoundException.hpp"

#include "../MallocInfo.hpp"

namespace lsan::suppression {
using namespace simple_json;

static inline auto getFunctionPair(const std::string& name,
                                   const std::optional<long>& offset,
                                   const std::optional<std::string>& library,
                                   const std::string& suppName) -> std::pair<uintptr_t, std::size_t> {
    const auto& [begin, length, found] = functionInfo_loadHint(name.c_str(), library ? library->c_str() : nullptr);
    if (!found) {
        throw FunctionNotFoundException(name, suppName);
    }
    if (offset) {
        return std::make_pair(begin + *offset, 0);
    }
    return std::make_pair(begin, length);
}

static inline constexpr auto asLeakType(const std::optional<unsigned long>& number) -> std::optional<LeakType> {
    if (number && *number > 10) {
        throw std::runtime_error("Not a leak type: " + std::to_string(*number));
    }
    return number ? std::optional(LeakType(*number)) : std::nullopt;
}

static inline auto getCallstackObject(const Value& object, const std::string& suppName) -> decltype(Suppression::topCallstack)::value_type {
    if (object.type == ValueType::String) {
        return {
            Suppression::Type::range,
            getFunctionPair(object.as<ValueType::String>(),
                            std::nullopt, std::nullopt, suppName)
        };
    } else if (object.type == ValueType::Object) {
        const auto& theObject = Object(object);
        if (const auto& name = theObject.get<ValueType::String>("name")) {
            return {
                Suppression::Type::range,
                getFunctionPair(*name,
                                theObject.get<ValueType::Int>("offset"),
                                theObject.get<ValueType::String>("library"),
                                suppName)
            };
        } else {
            const auto& libraryRegex = theObject.content.at("libraryRegex");
            auto regexes = std::vector<std::regex>();
            if (libraryRegex.type == ValueType::String) {
                regexes.emplace_back(libraryRegex.as<ValueType::String>());
            } else if (libraryRegex.type == ValueType::Array) {
                const auto& array = libraryRegex.as<ValueType::Array>();
                regexes.reserve(array.size());
                for (const auto& regex : array) {
                    regexes.emplace_back(regex.as<ValueType::String>());
                }
            } else {
                throw std::runtime_error("Library regex value is neither an array nor a (regex) string");
            }
            return { Suppression::Type::regex, regexes };
        }
    }
    throw std::runtime_error("Unsupported value in function array");
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
        if (functionArray->empty()) {
            throw std::runtime_error("Function array empty");
        }
        topCallstack.reserve(functionArray->size());
        for (const auto& functionObject : *functionArray) {
            auto&& frame = getCallstackObject(functionObject, name);
            hasRegexes = hasRegexes || frame.first == Type::regex;
            topCallstack.push_back(std::move(frame));
        }
    }
}

auto Suppression::match(const MallocInfo& info) const -> bool {
    if (size && info.getSize() != *size) return false;
    if (leakType && info.leakType != *leakType) return false;

    if (imageName && info.imageName.first != nullptr && !std::regex_match(info.imageName.first, *imageName)) return false;

    if (topCallstack.empty() && info.imageName.first == nullptr) return false;

    return topCallstack.empty() ? true : callstackHelper::isSuppressed(*this, info.getAllocationCallstack());
}
}
