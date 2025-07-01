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

#ifndef firstPartyLibrary_hpp
#define firstPartyLibrary_hpp

namespace lsan::suppression {
/**
 * Returns whether the given binary file name is considered first party.
 *
 * @param binaryName the binary file name in question
 * @param useCache whether to use the cache
 * @return whether the given binary file is considered first party
 */
auto isFirstParty(const char* binaryName, bool useCache) -> bool;
}

#endif /* firstPartyLibrary_hpp */
