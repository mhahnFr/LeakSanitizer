/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2024  mhahnFr
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

#include <csignal>

#include "signals.hpp"

namespace lsan::signals {
auto registerFunction(void* function, int signalCode) -> bool {
    switch (signalCode) {
#if defined(__APPLE__) || defined(SIGBUS)
        case SIGBUS:
#endif
        case SIGSEGV: {
            struct sigaction s{};
            s.sa_sigaction = reinterpret_cast<void (*)(int, siginfo_t*, void*)>(function);
            return sigaction(signalCode, &s, nullptr) == 0;
        }
            
        default:
            return signal(signalCode, reinterpret_cast<void (*)(int)>(function)) != SIG_ERR;
    }
    return false;
}

auto getDescriptionFor(int signal) noexcept -> const char* {
    switch (signal) {
        case SIGHUP:    return "Terminal line hangup";
        case SIGINT:    return "Interrupt";
        case SIGQUIT:   return "Quit";
        case SIGILL:    return "Illegal instruction";
        case SIGABRT:   return "Abort";
        case SIGFPE:    return "Floating-point exception";
        case SIGKILL:   return "Killed";
        case SIGSEGV:   return "Segmentation fault";
        case SIGPIPE:   return "Broken pipe";
        case SIGALRM:   return "Timer expired";
        case SIGTERM:   return "Terminated";
            
#if defined(__APPLE__) || defined(SIGTRAP)
        case SIGTRAP:   return "Trapping instruction";
#endif
#if defined(__APPLE__) || defined(SIGEMT)
        case SIGEMT:    return "Emulate instruction executed";
#endif
#if defined(__APPLE__) || defined(SIGBUS)
        case SIGBUS:    return "Bus error";
#endif
#if defined(__APPLE__) || defined(SIGSYS)
        case SIGSYS:    return "Non-existent system call";
#endif
#if defined(__APPLE__) || defined(SIGXCPU)
        case SIGXCPU:   return "CPU time limit exceeded";
#endif
#if defined(__APPLE__) || defined(SIGXFSZ)
        case SIGXFSZ:   return "File size limit exceeded";
#endif
#if defined(__APPLE__) || defined(SIGVTALRM)
        case SIGVTALRM: return "Virtual time alarm";
#endif
#if defined(__APPLE__) || defined(SIGPROF)
        case SIGPROF:   return "Profiling timer alarm";
#endif
    }
    return "Unknown signal";
}

auto stringify(int signal) noexcept -> const char* {
    switch (signal) {
        case SIGHUP:    return "SIGHUP";
        case SIGINT:    return "SIGINT";
        case SIGQUIT:   return "SIGQUIT";
        case SIGILL:    return "SIGILL";
        case SIGABRT:   return "SIGABRT";
        case SIGFPE:    return "SIGFPE";
        case SIGKILL:   return "SIGKILL";
        case SIGSEGV:   return "SIGSEGV";
        case SIGPIPE:   return "SIGPIPE";
        case SIGALRM:   return "SIGALRM";
        case SIGTERM:   return "SIGTERM";
            
#if defined(__APPLE__) || defined(SIGTRAP)
        case SIGTRAP:   return "SIGTRAP";
#endif
#if defined(__APPLE__) || defined(SIGEMT)
        case SIGEMT:    return "SIGEMT";
#endif
#if defined(__APPLE__) || defined(SIGBUS)
        case SIGBUS:    return "SIGBUS";
#endif
#if defined(__APPLE__) || defined(SIGSYS)
        case SIGSYS:    return "SIGSYS";
#endif
#if defined(__APPLE__) || defined(SIGXCPU)
        case SIGXCPU:   return "SIGXCPU";
#endif
#if defined(__APPLE__) || defined(SIGXFSZ)
        case SIGXFSZ:   return "SIGXFSZ";
#endif
#if defined(__APPLE__) || defined(SIGVTALRM)
        case SIGVTALRM: return "SIGVTALRM";
#endif
#if defined(__APPLE__) || defined(SIGPROF)
        case SIGPROF:   return "SIGPROF";
#endif
    }
    return "Unkown";
}
}
