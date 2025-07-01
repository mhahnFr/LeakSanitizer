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

#include "firstPartyLibrary.hpp"

#include <map>

#include "../lsanMisc.hpp"

namespace lsan::suppression {
/**
 * An enumeration containing the currently known classifications of a binary
 * file path.
 */
enum class Classification {
    /** Indicates the file path is first party.    */
    firstParty,
    /** Indicates the file path is user-defined.   */
    none
};

/** Caches the classifications of the file paths. */
static std::map<const char*, Classification> cache; // TODO: Use a pool map?

/**
 * Returns whether the given binary file name represents a first party
 * (system) binary.
 *
 * @param file the binary file name to be checked
 * @return whether the given binary file name is first party
 */
static inline auto isFirstPartyCore(const char* file) -> bool {
    if (file == nullptr) {
        return false;
    }

    const auto& systemLibraries = getSystemLibraries();
    return std::any_of(systemLibraries.cbegin(), systemLibraries.cend(), [file](const auto& regex) {
        return std::regex_match(file, regex);
    });
}

/**
 * Classifies the given binary file name.
 *
 * @param file the binary file name to be checked
 * @return the classification of the file name
 */
static inline auto classify(const char* file) -> Classification {
    if (isFirstPartyCore(file)) {
        return Classification::firstParty;
    }
    return Classification::none;
}

/**
 * Classifies and caches the given binary file name.
 *
 * @param file the binary file name to be classified
 * @return the classification of the file name
 */
static inline auto classifyAndCache(const char* file) -> Classification {
    const auto& toReturn = classify(file);
    cache.emplace(file, toReturn);
    return toReturn;
}

/**
 * @brief Returns whether the given binary file name is first party.
 *
 * Uses the cache.
 *
 * @param file the binary file name to be checked
 * @return whether the given binary file name is first party
 */
static inline auto isFirstPartyCached(const char* file) -> bool {
    if (const auto& it = cache.find(file); it != cache.end()) {
        return it->second == Classification::firstParty;
    }
    return classifyAndCache(file) == Classification::firstParty;
}

auto isFirstParty(const char* binaryName, const bool useCache) -> bool {
    return useCache ? isFirstPartyCached(binaryName) : isFirstPartyCore(binaryName);
}
}
