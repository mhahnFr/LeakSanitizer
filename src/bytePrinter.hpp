/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2022 - 2025  mhahnFr
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

#ifndef bytePrinter_hpp
#define bytePrinter_hpp

#include <string>

namespace lsan {
/**
 * @brief Returns a human-readable representation of the given byte amount.
 *
 * Depending on @c Behaviour::humanPrint() the amount may be represented in a
 * higher entity.
 *
 * @param amount the amount to create a string representation of
 * @return a string representation of the given byte amount
 */
auto bytesToString(unsigned long long amount) -> std::string;
}

#endif /* bytePrinter_hpp */
