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

#include "../../crashWarner/crashForce.hpp"
#include "../../formatter/formatter.hpp"
#include "../../macos/bundle.hpp"
#include "../../signals/SignalInfo.hpp"
#include "../../suppression/systemLibraryLoader.hpp"

using namespace lsan;

auto suppression::getSystemLibraries() -> const std::vector<std::regex>& {
    static auto supps = loadSystemLibraries();
    return supps;
}

auto macos::bundle::convertCFString(CFStringRef str) -> std::optional<std::string> {
    if (str == nil) [[unlikely]] return std::nullopt;

    const auto cStr = CFStringGetCStringPtr(str, kCFStringEncodingUTF8);
    if (cStr != nullptr) [[likely]] {
        return cStr;
    }
    auto toReturn = std::string(std::string::size_type(CFStringGetLength(str)), '\0');
    return CFStringGetCString(str, toReturn.data(), CFIndex(toReturn.capacity()), kCFStringEncodingUTF8) ? std::make_optional(toReturn) : std::nullopt;
}

auto macos::bundle::getBundle() -> CFBundleRef {
    static auto bundle = CFBundleGetBundleWithIdentifier(CFSTR("fr.mhahn.LeakSanitizer"));
    if (bundle == nil) [[unlikely]] {
        throw std::runtime_error("Bundle not loaded");
    }
    return bundle;
}

/**
 * Exits the crash handler and prints the given error message.
 *
 * @param message the message to print prior to failure exiting
 */
[[noreturn]] static inline void error(const std::string& message) {
    using namespace formatter;
    using formatter::Style;

    std::cerr << get<Style::RED> << "CrashHandler of mhahnFr's LeakSanitizer: Error: "
              << format<Style::BOLD>(message) << "!"
              << clear<Style::RED> << std::endl;
    exit(EXIT_FAILURE);
}

[[noreturn]] auto main(int argc, const char** argv) -> int {
    using namespace signals;

    if (argc < 3) [[unlikely]] {
        error("Not enough arguments provided");
    }
    if (strlen(argv[1]) != 1) [[unlikely]] {
        error("Zero replacement is not a single byte");
    }
    if (strlen(argv[2]) != sizeof(SignalInfo)) [[unlikely]] {
        error("Passed wrong number of bytes");
    }

    auto info = SignalInfo::fromBinary(argv[2], sizeof(SignalInfo), argv[1][0]);
    if (argc - 3 != info.callstack->backtraceSize) [[unlikely]] {
        error("Wrong number of binary file paths");
    }
    setenv("\t\n\f\vLSAN_SUPPRESSED_BY_CRASH_HANDLER", "", false);
    const auto _ = info.callstack.absolutize(argv + 3);
    const auto& [message, reason] = createCrashMessage(info.code, info.siCode, info.faultAddress);
    crashWarner::crashForce(message, reason, std::move(info.callstack));
}
