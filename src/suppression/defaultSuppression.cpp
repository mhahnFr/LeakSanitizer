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

#include "../utils/definitions.hpp"

#ifndef LSAN_OS_DEFINED
# error Unknown operating system
#endif

#ifdef LSAN_OS_MACOS
# include <macos/AppKit.hpp>
# include <macos/core.hpp>
# include <macos/systemLibraries.hpp>

# include "../macos/bundle.hpp"

using namespace lsan::macos::bundle;

#elifdef LSAN_OS_LINUX
# include <linux/core.hpp>
# include <linux/systemLibraries.hpp>
#endif

namespace lsan::suppression {
auto getDefaultSuppression() -> std::vector<std::string> {
    auto toReturn = std::vector<std::string>();

    toReturn.insert(toReturn.cend(), {
        std::string(core_json),
#ifdef LSAN_OS_MACOS
        std::string(AppKit_json),
#endif
    });

    return toReturn;
}

auto getSystemLibraryFiles() -> std::vector<std::string> {
    auto toReturn = std::vector<std::string>();

    toReturn.insert(toReturn.cend(), {
        std::string(systemLibraries_json),
    });

    return toReturn;
}

auto getDefaultTLVSuppressions() -> std::vector<std::string> {
    auto toReturn = std::vector<std::string>();

    toReturn.insert(toReturn.cend(), {
#ifdef LSAN_OS_MACOS
        shared::loadResource(CFBundleCopyResourceURL(getBundle(), CFSTR("tlv"), CFSTR("json"), nil)),
#endif
    });

    return toReturn;
}
}
