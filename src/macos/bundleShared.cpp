/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2025 - 2026  mhahnFr
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

#include "bundle.hpp"

#include <fstream>
#include <sstream>

namespace lsan::macos::bundle::shared {
auto readFile(const std::filesystem::path& path) -> std::string {
    auto stream = std::ifstream();
    auto strStr = std::ostringstream();
    stream.exceptions(std::ifstream::badbit | std::ifstream::failbit);
    try {
        stream.open(path);
        strStr << stream.rdbuf();
        stream.close();
    } catch (...) {
        if (stream.is_open()) {
            stream.close();
        }
    }
    return strStr.str();
}

auto loadResource(const CFURLRef url) -> std::string {
    if (url == nil) [[unlikely]] {
        throw std::runtime_error("Resource URL is nil");
    }
    const auto path = CFURLCopyPath(url);
    CFRelease(url);
    const auto& pathStr = convertCFString(path);
    CFRelease(path);
    return readFile(pathStr.value());
}
}
