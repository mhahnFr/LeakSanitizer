//
//  warn.cpp
//  LeakSanitizer
//
//  Created by Manuel Hahn on 8/3/22.
//  Copyright © 2022 mhahnFr. All rights reserved.
//

#include <iostream>
#include "warn.hpp"
#include "MallocInfo.hpp"

static void warnShared(int omitCaller = 2) {
    std::cerr << std::endl;
    MallocInfo::printCallstack(MallocInfo::createCallstack(omitCaller), std::cerr);
    std::cerr << std::endl;
}

void warn(const std::string & message, const char * file, int line, int omitCaller) {
    std::cerr << "\033[1;95mWarning: " << message << "\033[39m, at \033[4m" << file << ":" << line << "\033[24;22m";
    warnShared(omitCaller);
}

void warn(const std::string & message, int omitCaller) {
    std::cerr << "\033[1;95mWarning: " << message << "!\033[39;22m";
    warnShared(omitCaller);
}
