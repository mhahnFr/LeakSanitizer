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

#ifndef defaultSuppression_hpp
#define defaultSuppression_hpp

#include <string>
#include <vector>

namespace lsan::suppression {
/**
 * Loads and returns the contents of the default suppression files.
 *
 * @return the contents of the default suppression files
 */
auto getDefaultSuppression() -> std::vector<std::string>;

/**
 * Loads and returns the contents of the default system library expression files.
 *
 * @return the contents of the system library regular expression files
 */
auto getSystemLibraryFiles() -> std::vector<std::string>;

/**
 * Loads and returns the contents of the default thread-local value suppression files.
 *
 * @return the contents of the default thread-local value suppression files
 */
auto getDefaultTLVSuppressions() -> std::vector<std::string>;
}

#endif /* defaultSuppression_hpp */
