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
 * LeakSanitizer, see the file LICENSE.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef crash_hpp
#define crash_hpp

#include <optional>
#include <string>

#include "../MallocInfo.hpp"

namespace lsan {
/**
 * @brief Terminates the linked program and prints the given message and a callstack.
 *
 * This function does nothing if the generated callstack is not user relevant.
 *
 * @param message the message to be printed
 */
void crash(const std::string & message);

/**
 * @brief Terminates the linked program and prints the given message and a callstack.
 *
 * This function performs the termination in any case.
 *
 * @param message the message to be printed
 */
[[ noreturn ]] void crashForce(const std::string & message);

/**
 * @brief Terminates the linked program and prints the given message, the optionally given
 * reason and the given callstack.
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

/**
 * @brief Terminates the linked program and prints the given message, the information
 * provided by the optional allocation record and a callstack.
 *
 * This function does nothing if the generated callstack is not user relevant.
 *
 * @param message the message to be printed
 * @param info the optional allocation record
 */
void crash(const std::string& message,
           const std::optional<MallocInfo::CRef>& info);

/**
 * This function resets the signal handler for `SIGABRT` and performs the abort.
 */
[[ noreturn ]] void abort();
}

#endif /* crash_hpp */
