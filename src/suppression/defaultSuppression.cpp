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

#ifdef LSAN_APPLE
// TODO: Generate these headers properly
# include "../../suppressions/macos/AppKit.hpp"
# include "../../suppressions/macos/core.hpp"
# include "../../suppressions/macos/systemLibraries.hpp"
# include "../macos/bundle.hpp"

using namespace lsan::macos::bundle;

#elif defined(LSAN_LINUX)
# include <linux/core.hpp>
# include <linux/systemLibraries.hpp>
#endif

namespace lsan::suppression {
auto getDefaultSuppression() -> std::vector<std::string> {
    auto toReturn = std::vector<std::string>();

    toReturn.insert(toReturn.cend(), {
#ifdef LSAN_APPLE
        std::string(suppressions_macos_core),
        std::string(suppressions_macos_AppKit),
#elif defined(LSAN_LINUX)
        std::string(suppressions_linux_core),
#endif
    });

    return toReturn;
}

auto getSystemLibraryFiles() -> std::vector<std::string> {
    auto toReturn = std::vector<std::string>();

    toReturn.insert(toReturn.cend(), {
#ifdef LSAN_APPLE
        std::string(suppressions_macos_systemLibraries)
#elif defined(LSAN_LINUX)
        std::string(suppressions_linux_systemLibraries)
#endif
    });

    return toReturn;
}

auto getDefaultTLVSuppressions() -> std::vector<std::string> {
    auto toReturn = std::vector<std::string>();

    toReturn.insert(toReturn.cend(), {
#ifdef LSAN_APPLE
        shared::loadResource(CFBundleCopyResourceURL(macos::bundle::getBundle(), CFSTR("tlv"), CFSTR("json"), nullptr)),
#endif
    });

    return toReturn;
}
}
