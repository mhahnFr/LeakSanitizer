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

#ifndef signals_hpp
#define signals_hpp

/**
 * This namespace contains the signal functions.
 */
namespace lsan::signals {
/**
 * Casts the given function to a signal handler for the POSIX signal registration.
 *
 * @param function the function to be cast
 * @return the cast function pointer
 * @tparam F the type of the function
 */
template<typename F>
static inline auto asHandler(F function) noexcept -> void* {
    return reinterpret_cast<void*>(function);
}

/**
 * @brief Registers the given function using the POSIX signal handler
 * registration.
 *
 * If the handler is intended as crash handler, upon receipt of the signal the
 * handler is removed.
 *
 * @param function the function to be registered
 * @param signal the signal for which the function should be registered
 * @param forCrash whether the handler is intended as crash handler
 * @return whether the function was registered successfully
 */
auto registerFunction(void* function, int signal, bool forCrash = true) -> bool;

/**
 * Registers the given function using the C standard signal handler registration.
 *
 * @param function the function to be registered
 * @param signal the signal for which the function should be registered
 * @return whether the function was registered successfully
 */
auto registerFunction(void (*function)(int), int signal) -> bool;

/**
 * Returns a string description for the given signal code.
 *
 * @param signal the signal code
 * @return a string description of the given signal
 */
auto getDescriptionFor(int signal) noexcept -> const char*;

/**
 * Returns the stringified signal code.
 *
 * @param signal the signal code
 * @return the stringified signal
 */
auto stringify(int signal) noexcept -> const char*;

/**
 * Returns whether the given signal usually has a faulty address in its
 * signal context.
 *
 * @param signal the signal
 * @return whether the signal has a crashing address
 */
auto hasAddress(int signal) noexcept -> bool;
}

#endif /* signals_hpp */
