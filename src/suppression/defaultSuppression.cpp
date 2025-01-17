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
# include <macos/core.hpp>
# include <macos/misc.hpp>
#endif

namespace lsan::suppression {
auto getDefaultSuppression() -> std::vector<std::string> {
    auto toReturn = std::vector<std::string>();

#ifdef LSAN_APPLE
    toReturn.insert(toReturn.cbegin(), {
        std::string(suppressions_macos_core),
        std::string(suppressions_macos_misc),
    });
    // TODO: Swift, AppKit, ...
#endif

    return toReturn;
}
}
