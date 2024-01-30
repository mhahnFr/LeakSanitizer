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

#include <iostream>

#include "signalHandlers.hpp"

#include "formatter.hpp"
#include "lsanMisc.hpp"
#include "MallocInfo.hpp"
#include "callstacks/callstackHelper.hpp"
#include "crashWarner/crash.hpp"

#include "../include/lsan_internals.h"
#include "../include/lsan_stats.h"

#if __cplusplus >= 202002L
 #include <format>
#else
 #include <sstream>
#endif

namespace lsan {
/**
 * Returns a string representation for the given signal code.
 *
 * @param signal the signal code
 * @return a string representation of the given signal
 */
constexpr static inline auto signalDescription(int signal) -> const char* {
    switch (signal) {
        case SIGHUP:    return "Terminal line hangup";
        case SIGINT:    return "Interrupt";
        case SIGQUIT:   return "Quit";
        case SIGILL:    return "Illegal instruction";
        case SIGTRAP:   return "Trapping instruction";
        case SIGABRT:   return "Abort";
        case SIGEMT:    return "Emulate instruction executed";
        case SIGFPE:    return "Floating-point exception";
        case SIGKILL:   return "Killed";
        case SIGBUS:    return "Bus error";
        case SIGSEGV:   return "Segmentation fault";
        case SIGSYS:    return "Non-existent system call";
        case SIGPIPE:   return "Broken pipe";
        case SIGALRM:   return "Timer expired";
        case SIGTERM:   return "Terminated";
        case SIGXCPU:   return "CPU time limit exceeded";
        case SIGXFSZ:   return "File size limit exceeded";
        case SIGVTALRM: return "Virtual time alarm";
        case SIGPROF:   return "Profiling timer alarm";
    }
    return "Unknown signal";
}

constexpr static inline auto signalString(int signal) -> const char* {
    switch (signal) {
        case SIGHUP:    return "SIGHUP";
        case SIGINT:    return "SIGINT";
        case SIGQUIT:   return "SIGQUIT";
        case SIGILL:    return "SIGILL";
        case SIGTRAP:   return "SIGTRAP";
        case SIGABRT:   return "SIGABRT";
        case SIGEMT:    return "SIGEMT";
        case SIGFPE:    return "SIGFPE";
        case SIGKILL:   return "SIGKILL";
        case SIGBUS:    return "SIGBUS";
        case SIGSEGV:   return "SIGSEGV";
        case SIGSYS:    return "SIGSYS";
        case SIGPIPE:   return "SIGPIPE";
        case SIGALRM:   return "SIGALRM";
        case SIGTERM:   return "SIGTERM";
        case SIGXCPU:   return "SIGXCPU";
        case SIGXFSZ:   return "SIGXFSZ";
        case SIGVTALRM: return "SIGVTALRM";
        case SIGPROF:   return "SIGPROF";
    }
    return "Unkown";
}

[[ noreturn ]] static inline void aborter(int) {
    abort();
}

[[ noreturn ]] void crashHandler(int signalCode, siginfo_t * info, void *) {
    signal(signalCode, aborter);
    
#if __cplusplus >= 202002L
    std::string address = std::format("{:#x}", info->si_addr);
#else
    std::stringstream s;
    s << info->si_addr;
    std::string address = s.str();
#endif
    using formatter::Style;
    crashForce(formatter::formatString<Style::BOLD, Style::RED>(signalDescription(signalCode)) + " (" + signalString(signalCode) + ")"
               + " on address " + formatter::formatString<Style::BOLD>(address), __builtin_return_address(0));
}

void callstackSignal(int) {
    using formatter::Style;
    
    bool ignore = getIgnoreMalloc();
    setIgnoreMalloc(true);
    
    auto & out = getOutputStream();
    out << formatter::format<Style::ITALIC>("The current callstack:") << std::endl;
    
    callstackHelper::format(lcs::callstack(), out);
    out << std::endl;
    if (!ignore) {
        setIgnoreMalloc(false);
    }
}

void statsSignal(int) {
    __lsan_printStats();
}
}
