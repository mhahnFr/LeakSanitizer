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

[[ noreturn ]] static void crashShared(int omitCaller = 2) {
    std::cerr << std::endl;
    MallocInfo::printCallstack(MallocInfo::createCallstack(omitCaller), std::cerr);
    std::cerr << std::endl;
    std::terminate();
}

[[ noreturn ]] void crash(const std::string & reason, const char * file, int line, int omitCaller) {
    std::cerr << "\033[1;31m" << reason << "\033[39m, at \033[4m" << file << ":" << line << "\033[24;22m";
    crashShared(omitCaller);
}

[[ noreturn ]] void crash(const std::string & reason, int omitCaller) {
    std::cerr << "\033[1;31m" << reason << "!\033[39;22m";
    crashShared(omitCaller);
}
