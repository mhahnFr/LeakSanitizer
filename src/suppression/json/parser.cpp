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

#include "parser.hpp"

namespace lsan::json {
using Exception = std::runtime_error;

static inline void skipWhitespaces(std::istream& in) {
    while (std::isspace(in.peek())) {
        in.get();
    }
}

static inline void expect(std::istream& in, char expected, bool skipWhite = true) {
    if (skipWhite) {
        skipWhitespaces(in);
    }
    if (in.peek() != expected) {
        throw Exception("Expected different character!");
    }
}

static inline void expectConsume(std::istream& in, char expected, bool skipWhite = true) {
    expect(in, expected, skipWhite);
    in.get();
}

static inline auto readString(std::istream& in) -> Value {
    expectConsume(in, '"');

    std::string buffer;
    while (in.peek() != '"') {
        if (in.peek() == '\\') in.get();
        buffer += static_cast<char>(in.get());
    }
    expectConsume(in, '"');
    return Value { Value::Type::String, buffer };
}

static inline auto readArray(std::istream& in) -> Value;
static inline auto readPrimitive(std::istream& in) -> Value;

static inline auto readObject(std::istream& in) -> Object {
    expectConsume(in, '{');

    auto toReturn = std::map<std::string, Value>();
    skipWhitespaces(in);
    while (in.peek() != '}') {
        auto name = readString(in);
        expectConsume(in, ':');
        skipWhitespaces(in);
        Value value;
        switch (in.peek()) {
            case '"': value = readString(in); break;
            case '[': value = readArray(in);  break;
            case '{': throw Exception("Nested objects are not supported.");

            default: value = readPrimitive(in); break;
        }
        toReturn.emplace(std::make_pair(name, value));
        skipWhitespaces(in);
        if (in.peek() == ',') {
            in.get();
            skipWhitespaces(in);
        }
    }
    expectConsume(in, '}');
    return Object { toReturn };
}

auto parse(std::istream& stream) -> Object {
    throw std::runtime_error("Unimplemented");
}
}
