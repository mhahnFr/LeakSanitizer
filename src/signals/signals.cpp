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

#include <cstdlib>
#include <csignal>

#include "signals.hpp"

namespace lsan::signals {
auto registerFunction(void (*function)(int), int signalCode) -> bool {
    return signal(signalCode, function) != SIG_ERR;
}

static bool hasAlternativeStack = false;
auto createAlternativeStack() -> void* {
    const std::size_t stackSize = SIGSTKSZ;
    
    auto toReturn = std::malloc(stackSize);
    if (toReturn == nullptr) {
        return nullptr;
    }
    stack_t s;
    s.ss_flags = 0;
    s.ss_size  = stackSize;
    s.ss_sp    = toReturn;
    if (sigaltstack(&s, nullptr) != 0) {
        std::free(toReturn);
        return nullptr;
    }
    hasAlternativeStack = true;
    return toReturn;
}

auto registerFunction(void* function, int signalCode, bool forCrash) -> bool {
    struct sigaction s{};
    s.sa_sigaction = reinterpret_cast<void (*)(int, siginfo_t*, void*)>(function);
    s.sa_flags = SA_SIGINFO;
    if (forCrash) {
        s.sa_flags |= SA_RESETHAND;
        if (hasAlternativeStack) {
            s.sa_flags |= SA_ONSTACK;
        }
    } else {
        s.sa_flags |= SA_RESTART;
    }
    return sigaction(signalCode, &s, nullptr);
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
        case SIGTRAP:   return "Trapping instruction";
        case SIGBUS:    return "Bus error";
        case SIGSYS:    return "Non-existent system call";
        case SIGXCPU:   return "CPU time limit exceeded";
        case SIGXFSZ:   return "File size limit exceeded";
        case SIGVTALRM: return "Virtual time alarm";
        case SIGPROF:   return "Profiling timer alarm";
            
#if defined(__APPLE__) || defined(SIGEMT)
        case SIGEMT:    return "Emulate instruction executed";
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
        case SIGTRAP:   return "SIGTRAP";
        case SIGBUS:    return "SIGBUS";
        case SIGSYS:    return "SIGSYS";
        case SIGXCPU:   return "SIGXCPU";
        case SIGXFSZ:   return "SIGXFSZ";
        case SIGVTALRM: return "SIGVTALRM";
        case SIGPROF:   return "SIGPROF";
            
#if defined(__APPLE__) || defined(SIGEMT)
        case SIGEMT:    return "SIGEMT";
#endif
    }
    return "Unkown";
}

auto hasAddress(int signal) noexcept -> bool {
    switch (signal) {
        case SIGBUS:
        case SIGFPE:
        case SIGILL:
        case SIGSEGV: return true;
    }
    return false;
}
}
