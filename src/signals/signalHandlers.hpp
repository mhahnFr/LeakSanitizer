/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2022 - 2024  mhahnFr
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

#ifndef signalHandlers_hpp
#define signalHandlers_hpp

#include <csignal>

/**
 * This namespace contains the available signal handlers.
 */
namespace lsan::signals::handlers {
/**
 * A signal handler that terminates the program and prints out a trace created from
 * the passed signal and execution context.
 *
 * @param signalCode the signal code
 * @param signalContext the signal context
 * @param executionContext the execution context where the signal was received
 */
[[ noreturn ]] void crashWithTrace(int signalCode, siginfo_t* signalContext, void* executionContext);

/**
 * This signal handler prints the statistics.
 *
 * @param signalCode the signal code, ignored
 */
void stats(int signalCode);

/**
 * This signal handler prints the callstack where the signal was received.
 *
 * @param signalCode the signal code, ignored
 * @param signalContext the signal context, ignored
 * @param executionContext the execution context where the signal was received
 */
void callstack(int signalCode, siginfo_t* signalContext, void* executionContext);
}

#endif /* signalHandlers_hpp */
