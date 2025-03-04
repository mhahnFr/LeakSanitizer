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
# include <macos/AppKit.hpp>
# include <macos/core.hpp>

# include <macos/systemLibraries.hpp>

#elif defined(LSAN_LINUX)
# include <linux/core.hpp>

#endif

namespace lsan::suppression {
auto getDefaultSuppression() -> std::vector<std::string> {
    auto toReturn = std::vector<std::string>();

    toReturn.insert(toReturn.cbegin(), {
#ifdef LSAN_APPLE
        std::string(suppressions_macos_AppKit),
        std::string(suppressions_macos_core),
        // TODO: Swift, AppKit, ...

#elif defined(LSAN_LINUX)
        std::string(suppressions_linux_core),
#endif
    });

    return toReturn;
}

auto getSystemLibraryFiles() -> std::vector<std::string> {
    auto toReturn = std::vector<std::string>();

    toReturn.insert(toReturn.cbegin(), {
#ifdef LSAN_APPLE
        std::string(suppressions_macos_systemLibraries),
#elif defined(LSAN_LINUX)
#endif
    });

    return toReturn;
}
}
