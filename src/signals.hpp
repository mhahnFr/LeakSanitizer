/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2024  mhahnFr
 *
 * This file is part of the LeakSanitizer. This library is free software:
 * you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this library, see the file LICENSE.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef signals_hpp
#define signals_hpp

namespace lsan::signals {
bool registerFunction(void* function, int signal);

/**
 * Returns a string description for the given signal code.
 *
 * @param signal the signal code
 * @return a string description of the given signal
 */
auto getDescriptionFor(int signal) noexcept -> const char*;
auto stringify(int signal) noexcept -> const char*;
}

#endif /* signals_hpp */
