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

#include "defaultSuppression.hpp"

#ifdef __APPLE__
# define LSAN_APPLE
#elif defined(__linux__)
# define LSAN_LINUX
#endif

#ifdef LSAN_APPLE
# include <macos/systemLibraries.hpp>
# include <macos/tlv.hpp>

#elif defined(LSAN_LINUX)
# include <linux/core.hpp>
# include <linux/systemLibraries.hpp>
#endif

#include <fstream>
#include <sstream>
#include <CoreFoundation/CFBundle.h>

namespace lsan::suppression {
static inline auto convertCFString(const CFStringRef str) -> std::string {
    if (str == nil) return {};

    if (const auto cStr = CFStringGetCStringPtr(str, kCFStringEncodingUTF8)) {
        return cStr;
    }
    auto toReturn = std::string();
    toReturn.reserve(std::string::size_type(CFStringGetLength(str) + 1));
    if (!CFStringGetCString(str, toReturn.data(), CFIndex(toReturn.capacity()), kCFStringEncodingUTF8)) {
        return {};
    }
    return toReturn;
}

static inline auto loadResource(CFURLRef url) -> std::string {
    const auto path = CFURLCopyPath(url);
    CFRelease(url);
    const auto& pathStr = convertCFString(path);
    CFRelease(path);
    auto stream = std::ifstream();
    auto strStr = std::ostringstream();
    stream.exceptions(std::ifstream::badbit | std::ifstream::failbit);
    try {
        stream.open(pathStr);
        strStr << stream.rdbuf();
        stream.close();
    } catch (...) {
        if (stream.is_open()) {
            stream.close();
        }
    }
    return strStr.str();
}

template<typename F, typename... Args>
constexpr static inline auto loadWithBundle(F&& f, Args&&... args) {
    const auto bundle = CFBundleGetBundleWithIdentifier(CFSTR("fr.mhahn.LeakSanitizer"));
    if constexpr (std::is_same_v<void, std::invoke_result_t<F, CFBundleRef, Args...>>) {
        f(bundle, std::forward<Args&&>(args)...);
        CFRelease(bundle);
    } else {
        const auto toReturn = f(bundle, std::forward<Args&&>(args)...);
        CFRelease(bundle);
        return toReturn;
    }
}

auto getDefaultSuppression() -> std::vector<std::string> {
    auto toReturn = std::vector<std::string>();

#ifdef LSAN_APPLE
    loadWithBundle([&toReturn](const auto bundle) {
        toReturn.insert(toReturn.cend(), {
            loadResource(CFBundleCopyResourceURL(bundle, CFSTR("AppKit"), CFSTR("json"), nullptr)),
            loadResource(CFBundleCopyResourceURL(bundle, CFSTR("core"), CFSTR("json"), nullptr)),
        });
    });
#elif defined(LSAN_LINUX)
    toReturn.emplace_back(std::string(suppressions_linux_core));
#endif

    return toReturn;
}

auto getSystemLibraryFiles() -> std::vector<std::string> {
    auto toReturn = std::vector<std::string>();

    toReturn.insert(toReturn.cbegin(), {
#ifdef LSAN_APPLE
        std::string(suppressions_macos_systemLibraries),
#elif defined(LSAN_LINUX)
        std::string(suppressions_linux_systemLibraries),
#endif
    });

    return toReturn;
}

auto getDefaultTLVSuppressions() -> std::vector<std::string> {
    auto toReturn = std::vector<std::string>();

    toReturn.insert(toReturn.cbegin(), {
#ifdef LSAN_APPLE
        std::string(suppressions_macos_tlv),
#endif
    });

    return toReturn;
}
}
