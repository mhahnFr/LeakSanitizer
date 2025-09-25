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

#include "../../behaviour/getBehaviour.hpp"
#include "../../crashWarner/crashForce.hpp"
#include "../../signals/SignalInfo.hpp"
#include "../../suppression/defaultSuppression.hpp"
#include "../../suppression/systemLibraryLoader.hpp"

using namespace lsan;

static std::filesystem::path executablePath;

auto suppression::getSystemLibraries() -> const std::vector<std::regex>& {
    static auto supps = loadSystemLibraries();
    return supps;
}

auto suppression::getSystemLibraryFiles() -> std::vector<std::string> {
    return {
        readFile(executablePath.remove_filename() / "systemLibraries.json"),
    };
}

auto behaviour::getBehaviour() -> const Behaviour& {
    static Behaviour instance;
    return instance;
}

auto main(int argc, const char** argv) -> int {
    using namespace signals;

    executablePath = argv[0];

    // TODO: Safety!!!
    auto info = SignalInfo::fromBinary(argv[2], sizeof(SignalInfo), argv[1][0]);
    const auto _ = info.callstack.absolutize(argv + 3);
    const auto& [message, reason] = createCrashMessage(info.code, info.siCode, info.faultAddress);
    crashWarner::crashForce(message, reason, std::move(info.callstack));
}
