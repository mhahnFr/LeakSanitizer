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
auto getBundle() -> CFBundleRef {
    static auto bundle = getTracker().withIgnorationResult(true, [] {
        return CFBundleGetBundleWithIdentifier(CFSTR("fr.mhahn.LeakSanitizer"));
    });
    return bundle;
}

void killBundle() {
    getTracker().withIgnoration(false, [] {
        CFRelease(getBundle());
    });
}

auto getCrashHandlerPath() -> std::string {
    return getTracker().withIgnorationResult(false, [] {
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
        return toReturn;
    });
}

/** The default version string in case loading fails. */
constexpr inline auto DEFAULT_VERSION = std::string("CLEAN BUILD");

auto getVersion() -> std::string {
    return getTracker().withIgnorationResult(false, [] -> std::string {
        const auto value = CFBundleGetValueForInfoDictionaryKey(getBundle(), kCFBundleVersionKey);
        [[unlikely]] if (value == nil) {
            return DEFAULT_VERSION;
        }
        return convertCFString(CFStringRef(value));
    });
}

auto convertCFString(const CFStringRef str) -> std::string {
    [[unlikely]] if (str == nil) return {};

    auto& tracker = getTracker();
    const auto cStr = tracker.withIgnorationResult(false, [str] {
        return CFStringGetCStringPtr(str, kCFStringEncodingUTF8);
    });
    [[likely]] if (cStr != nullptr) {
        return cStr;
    }

    auto toReturn = std::string(std::string::size_type(tracker.withIgnorationResult(false, [str] {
        return CFStringGetLength(str);
    })), '\0');
    return tracker.withIgnorationResult(false, [&toReturn, str] {
        return CFStringGetCString(str, toReturn.data(), CFIndex(toReturn.capacity()), kCFStringEncodingUTF8);
    }) ? toReturn : std::string {};
}
}
