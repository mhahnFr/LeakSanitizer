//
//  crash.cpp
//  LeakSanitizer
//
//  Created by Manuel Hahn on 16.07.22.
//  Copyright © 2022 mhahnFr. All rights reserved.
//

#include "crash.hpp"
#include "LeakSani.hpp"
#include <iostream>

[[ noreturn ]] static void crashShared() {
    std::cerr << "Terminating...\033[22m" << std::endl
              << "Callstack: "            << std::endl;
    for (const auto & frame : MallocInfo::createCallstack()) {
        std::cerr << frame << std::endl;
    }
    std::terminate();
}

[[ noreturn ]] void crash(const std::string & reason, const char * file, int line) {
    std::cerr << "\033[1;31m" << reason << "\033[39m, at \033[4m" << file << ":" << line << "\033[24m" << std::endl;
    crashShared();
}

[[ noreturn ]] void crash(const std::string & reason) {
    std::cerr << "\033[1;31m" << reason << "!\033[39m ";
    crashShared();
}
