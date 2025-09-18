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

#ifndef systemLibraryLoader_hpp
#define systemLibraryLoader_hpp

#include <regex>
#include <vector>

namespace lsan::suppression {
/**
 * Returns the system library regexes.
 *
 * @return the system library regexes
 */
auto getSystemLibraries() -> const std::vector<std::regex>&;

/**
 * Loads the system library regexes.
 *
 * @return the loaded regexes
 */
auto loadSystemLibraries() -> std::vector<std::regex>;
}

#endif /* systemLibraryLoader_hpp */
