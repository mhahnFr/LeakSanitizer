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

[[ noreturn ]] void crash(const std::string & reason, const char * file, int line) {
    std::cerr << reason << ", at " << file << ":" << line << std::endl
              << "Terminating..." << std::endl;
    std::cerr << LSan::getInstance() << std::endl;
    std::terminate();
}

[[ noreturn ]] void crash(const std::string & reason) {
    std::cerr << reason << "! Terminating..." << std::endl;
    std::cerr << LSan::getInstance() << std::endl;
    std::terminate();
}
