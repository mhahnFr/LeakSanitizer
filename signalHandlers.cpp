//
//  signalHandlers.cpp
//  LeakSanitizer
//
//  Created by Manuel Hahn on 8/2/22.
//  Copyright © 2022 mhahnFr. All rights reserved.
//

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
