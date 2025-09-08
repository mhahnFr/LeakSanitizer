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

#ifndef crashForce_hpp
#define crashForce_hpp

#include <optional>
#include <string>

#include <callstack.h>

namespace lsan::crashWarner {
/**
 * @brief Terminates the linked program and prints the given message and a callstack.
 *
 * This function performs the termination in any case.
 *
 * @param message the message to be printed
 */
[[ noreturn ]] void crashForce(const std::string& message);

/**
 * @brief Terminates the linked program and prints the given message, the
 * optionally given reason and the given callstack.
 *
 * This function performs the termination in any case.
 *
 * @param message the message to be printed
 * @param reason the optional reason
 * @param callstack the callstack
 */
[[ noreturn ]] void crashForce(const std::string&                message,
                               const std::optional<std::string>& reason,
                                     lcs::callstack&&            callstack);
}

#endif /* crashForce_hpp */
