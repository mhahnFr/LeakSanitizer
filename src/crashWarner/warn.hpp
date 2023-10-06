/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2023  mhahnFr
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

#ifndef warn_hpp
#define warn_hpp

#include <functional>
#include <optional>
#include <string>
#include <utility>

#include "../MallocInfo.hpp"

namespace lsan {
/**
 * @brief Prints the given message, the file name, the line number and a callstack up
 * to the given omitting address.
 *
 * This function does nothing if the generated callstack is not user relevant.
 *
 * @param message the message to be printed
 * @param file the file name
 * @param line the line number
 * @param omitAddress the address beyond which frames are omitted from the generated callstack
 */
void warn(const std::string & message,
          const std::string & file,
          const int           line,
                void *        omitAddress = __builtin_return_address(0));

/**
 * @brief Prints the given message and a callstack up to the given omitting address.
 *
 * This function does nothing if the generated callstack is not user relevant.
 *
 * @param message the message to be printed
 * @param omitAddress the address beyond which frames are omitted from the generated callstack
 */
void warn(const std::string & message, void * omitAddress = __builtin_return_address(0));

/**
 * @brief Prints the given message, the information provided by the optional allocation
 * record and a callstack up to the given omitting address.
 *
 * This function does nothing if the generated callstack is not user relevant.
 *
 * @param message the message to be printed
 * @param info the optional allocation record
 * @param omitAddress the address beyond which frames are omitted from the generated callstack
 */
void warn(const std::string & message,
          std::optional<std::reference_wrapper<const MallocInfo>> info,
          void * omitAddress = __builtin_return_address(0));
}

#endif /* warn_hpp */
