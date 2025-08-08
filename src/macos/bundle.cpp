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

#include "bundle.hpp"

#include "../lsanMisc.hpp"

namespace lsan::macos::bundle {
static inline auto getBundleWrapper() -> CFBundleRef {
    return getTracker().withIgnorationResult(true, []() {
        return CFBundleGetBundleWithIdentifier(CFSTR("fr.mhahn.LeakSanitizer"));
    });
}

auto getBundle() -> CFBundleRef {
    static CFBundleRef bundle = getBundleWrapper();
    return bundle;
}

void killBundle() {
    getTracker().withIgnoration(true, []() {
        CFRelease(getBundle());
    });
}

constexpr inline auto DEFAULT_VERSION = "CLEAN BUILD";

auto getVersion() -> std::string {
    return getTracker().withIgnorationResult(true, []() -> std::string {
        const auto value = CFBundleGetValueForInfoDictionaryKey(getBundle(), kCFBundleVersionKey);
        if (value == nil) {
            return DEFAULT_VERSION;
        }
        return convertCFString(CFStringRef(value));
    });
}

auto convertCFString(const CFStringRef str) -> std::string {
    return getTracker().withIgnorationResult(true, [str]() -> std::string {
        if (str == nil) return {};

        if (const auto cStr = CFStringGetCStringPtr(str, kCFStringEncodingUTF8)) {
            return cStr;
        }
        auto toReturn = std::string();
        toReturn.resize(std::string::size_type(CFStringGetLength(str) + 1));
        if (!CFStringGetCString(str, toReturn.data(), CFIndex(toReturn.capacity()), kCFStringEncodingUTF8)) {
            return {};
        }
        return toReturn;
    });
}
}
