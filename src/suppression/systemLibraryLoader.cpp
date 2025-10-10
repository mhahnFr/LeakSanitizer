/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2025  mhahnFr
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

#include "systemLibraryLoader.hpp"

#include <fstream>
#include <SimpleJSON/SimpleJSON.hpp>

#include "defaultSuppression.hpp"
#include "../formatter/formatter.hpp"

namespace lsan::suppression {
/**
 * Loads the regexes found in the given JSON value into the given vector.
 *
 * @param content the vector to store the deducted regexes in
 * @param object the JSON value to deduct the regexes from
 */
static inline void loadSystemLibraryFile(std::vector<std::regex>& content, const simple_json::Value& object) {
    using namespace simple_json;
    [[unlikely]] if (!object.is(ValueType::Array)) {
        throw std::runtime_error("System libraries should be defined as a top level string array");
    }
    for (const auto& value : object.as<ValueType::Array>()) {
        [[unlikely]] if (!value.is(ValueType::String)) {
            throw std::runtime_error("System library regex was not a string");
        }

        content.emplace_back(value.as<ValueType::String>());
    }
}

auto loadSystemLibraries() -> std::vector<std::regex> {
    auto toReturn = std::vector<std::regex>();

    for (const auto& file : getSystemLibraryFiles()) {
        try {
            loadSystemLibraryFile(toReturn, simple_json::parse(std::istringstream(file)));
        } catch (const std::exception& e) {
            using namespace formatter;
            using namespace std::string_literals;

            getOutputStream() << format<Style::RED, Style::BOLD>("LSan: Failed to load default system library file: "s + e.what()) << std::endl << std::endl;
        }
    }

    for (const auto& file : behaviour::getFiles(behaviour::getBehaviour().systemLibraryFiles())) {
        auto stream = std::ifstream();
        stream.exceptions(std::ifstream::badbit | std::ifstream::failbit);

        try {
            stream.open(file);
            loadSystemLibraryFile(toReturn, simple_json::parse(stream));
        } catch (const std::exception& e) {
            using namespace formatter;

            getOutputStream() << format<Style::RED, Style::BOLD>("LSan: Failed to load system library file \"" + file.string() + "\": " + e.what()) << std::endl << std::endl;
        }
        [[likely]] if (stream.is_open()) {
            stream.close();
        }
    }

    return toReturn;
}
}
