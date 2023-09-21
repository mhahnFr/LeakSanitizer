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

#include <iostream>

#include "signalHandlers.hpp"
#include "LeakSani.hpp"
#include "MallocInfo.hpp"
#include "Formatter.hpp"
#include "crashWarner/crash.hpp"

#include "../include/lsan_internals.h"
#include "../include/lsan_stats.h"

#if __cplusplus >= 202002L
 #include <format>
#else
 #include <sstream>
#endif

/**
 * Returns a string representation for the given signal code.
 *
 * @param signal the signal code
 * @return a string representation of the given signal
 */
static std::string signalString(int signal) {
    switch (signal) {
        case SIGBUS:  return "Bus error";
        case SIGABRT: return "Abort";
        case SIGTRAP: return "Trapping instruction";
        case SIGSEGV: return "Segmentation fault";
    }
    return "<Unknown>";
}

[[ noreturn ]] void crashHandler(int signal, siginfo_t * info, void *) {
#if __cplusplus >= 202002L
    std::string address = std::format("{:#x}", info->si_addr);
#else
    std::stringstream s;
    s << info->si_addr;
    std::string address = s.str();
#endif
    using Formatter::Style;
    crash(Formatter::formatString<Style::BOLD, Style::RED>(signalString(signal))
          + " on address " + Formatter::formatString<Style::BOLD>(address), __builtin_return_address(0));
}

void callstackSignal(int) {
    using Formatter::Style;
    
    bool ignore = LSan::getIgnoreMalloc();
    LSan::setIgnoreMalloc(true);
    
    std::ostream & out = __lsan_printCout ? std::cout : std::cerr;
    out << Formatter::format<Style::ITALIC>("The current callstack:") << std::endl;
    
    MallocInfo::printCallstack(lcs::callstack(), out);
    out << std::endl;
    if (!ignore) {
        LSan::setIgnoreMalloc(false);
    }
}

void statsSignal(int) {
    __lsan_printStats();
}
