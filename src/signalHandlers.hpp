/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2022 - 2023  mhahnFr
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

namespace lsan {
/// This function acts as a signal handler for access violation signals.
[[ noreturn ]] void crashHandler(int, siginfo_t *, void *);

/// This function acts as a general signal handler. It prints the statistics.
void statsSignal(int);
/// This function acts as a general signal handler. It prints the current callstack.
void callstackSignal(int);
}

#endif /* signalHandlers_hpp */
