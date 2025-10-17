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

#include <iostream>

#include "../../crashWarner/crashForce.hpp"
#include "../../macos/bundle.hpp"
#include "../../signals/SignalInfo.hpp"
#include "../../suppression/defaultSuppression.hpp"
#include "../../suppression/systemLibraryLoader.hpp"

using namespace lsan;

auto suppression::getSystemLibraries() -> const std::vector<std::regex>& {
    static auto supps = loadSystemLibraries();
    return supps;
}

auto macos::bundle::convertCFString(CFStringRef str) -> std::string {
    [[unlikely]] if (str == nil) return {};

    const auto cStr = CFStringGetCStringPtr(str, kCFStringEncodingUTF8);
    [[likely]] if (cStr != nullptr) {
        return cStr;
    }
    auto toReturn = std::string(std::string::size_type(CFStringGetLength(str)), '\0');
    return CFStringGetCString(str, toReturn.data(), CFIndex(toReturn.capacity()), kCFStringEncodingUTF8) ? toReturn : std::string {};
}

auto macos::bundle::getBundle() -> CFBundleRef {
    static auto bundle = CFBundleGetBundleWithIdentifier(CFSTR("fr.mhahn.LeakSanitizer"));
    return bundle;
}

auto main(int argc, const char** argv) -> int {
    using namespace signals;

    // TODO: Safety!!!
    auto info = SignalInfo::fromBinary(argv[2], sizeof(SignalInfo), argv[1][0]);
    const auto _ = info.callstack.absolutize(argv + 3);
    const auto& [message, reason] = createCrashMessage(info.code, info.siCode, info.faultAddress);
    crashWarner::crashForce(message, reason, std::move(info.callstack));
}
