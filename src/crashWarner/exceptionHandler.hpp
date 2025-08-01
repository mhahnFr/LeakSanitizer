/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2023 - 2025  mhahnFr
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

#ifndef exceptionHandler_hpp
#define exceptionHandler_hpp

namespace lsan {
/**
 * @brief Handles the current exception pointer that is available.
 *
 * Terminates the linked application.
 */
[[ noreturn ]] void exceptionHandler() noexcept;

/**
 * @brief Handles the terminating exception created by @c mh_tryCatch .
 *
 * Terminates the linked application.
 */
[[ noreturn ]] void mhExceptionHandler() noexcept;
}

#endif /* exceptionHandler_hpp */
