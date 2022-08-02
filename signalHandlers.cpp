/*
 * LeakSanitizer - A small library showing informations about lost memory.
 *
 * Copyright (C) 2022  mhahnFr
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see https://www.gnu.org/licenses/.
 */

#include <iostream>
#include "signalHandlers.hpp"
#include "crash.hpp"

#if __cplusplus >= 202002L
 #include <format>
#else
 #include <sstream>
#endif

static std::string signalString(int signal) {
    switch (signal) {
        case SIGBUS:  return "Bus error";
        case SIGABRT: return "Abort";
        case SIGTRAP: return "Trapping instruction";
        case SIGSEGV: return "Segmentation fault";
    }
    return "<Unknown>";
}

[[ noreturn ]] void crashHandler(int signal, siginfo_t * info, void * constext) {
#if __cplusplus >= 202002L
    std::string address = std::format("{:#x}", info->si_addr);
#else
    std::stringstream s;
    s << info->si_addr;
    std::string address = s.str();
#endif
    crash("\033[31;1m" + signalString(signal) + "\033[39;22m on address \033[1m" + address + "\033[22m", 4);
}
