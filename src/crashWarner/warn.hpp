/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2023 - 2024  mhahnFr
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
 * LeakSanitzer, see the file LICENSE.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef warn_hpp
#define warn_hpp

#include <functional>
#include <optional>
#include <string>

#include "../MallocInfo.hpp"

namespace lsan {
/**
 * @brief Prints the given message and a callstack up to the given omitting address.
 *
 * This function does nothing if the generated callstack is not user relevant.
 *
 * @param message the message to be printed
 */
void warn(const std::string& message);

/**
 * @brief Prints the given message, the information provided by the optional allocation
 * record and a callstack up to the given omitting address.
 *
 * This function does nothing if the generated callstack is not user relevant.
 *
 * @param message the message to be printed
 * @param info the optional allocation record
 */
void warn(const std::string & message,
          const std::optional<std::reference_wrapper<const MallocInfo>>& info);
}

#endif /* warn_hpp */
