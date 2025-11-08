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
/** Indicates whether to ignore Objective-C related allocations. */
constexpr static inline auto IGNORE_OBJC = false;

auto getBundle() -> CFBundleRef {
    static auto bundle = getTracker().withIgnorationResult(true, [] {
        return CFBundleGetBundleWithIdentifier(CFSTR("fr.mhahn.LeakSanitizer"));
    });
    return bundle;
}

void killBundle() {
    getTracker().withIgnoration(IGNORE_OBJC, [] {
        CFRelease(getBundle());
    });
}

auto getCrashHandlerPath() -> std::string {
    return getTracker().withIgnorationResult(IGNORE_OBJC, [] {
        const auto result = CFBundleCopyResourceURL(getBundle(), CFSTR("CrashHandler"), nil, nil);
        [[unlikely]] if (result == nil) {
            return std::string {};
        }

        const auto path = CFURLCopyPath(result);
        CFRelease(result);
        [[unlikely]] if (path == nil) {
            return std::string {};
        }

        const auto& toReturn = convertCFString(path);
        CFRelease(path);
        return toReturn.value_or("");
    });
}

/** The default version string in case loading fails. */
constexpr inline auto DEFAULT_VERSION = std::string("CLEAN BUILD");

auto getVersion() -> std::string {
    return getTracker().withIgnorationResult(IGNORE_OBJC, [] -> std::string {
        const auto value = CFBundleGetValueForInfoDictionaryKey(getBundle(), kCFBundleVersionKey);
        [[unlikely]] if (value == nil) {
            return DEFAULT_VERSION;
        }
        return convertCFString(CFStringRef(value)).value_or(DEFAULT_VERSION);
    });
}

auto convertCFString(const CFStringRef str) -> std::optional<std::string> {
    [[unlikely]] if (str == nil) return std::nullopt;

    auto& tracker = getTracker();
    const auto cStr = tracker.withIgnorationResult(IGNORE_OBJC, [str] {
        return CFStringGetCStringPtr(str, kCFStringEncodingUTF8);
    });
    [[likely]] if (cStr != nullptr) {
        return cStr;
    }

    auto toReturn = std::string(std::string::size_type(tracker.withIgnorationResult(IGNORE_OBJC, [str] {
        return CFStringGetLength(str);
    })), '\0');
    return tracker.withIgnorationResult(IGNORE_OBJC, [&toReturn, str] {
        return CFStringGetCString(str, toReturn.data(), CFIndex(toReturn.capacity()), kCFStringEncodingUTF8);
    }) ? std::make_optional(toReturn) : std::nullopt;
}
}
