//
//  wrap_malloc.cpp
//  LeakSanitizer
//
//  Created by Manuel Hahn on 16.07.22.
//  Copyright © 2022 mhahnFr. All rights reserved.
//

#include "wrap_malloc.hpp"
#include "LeakSani.hpp"
#include <cstdio>
#include <iostream>

void * __wrap_malloc(size_t size, const char * file, int line) {
    if (size == 0) {
        crash("Invalid allocation of size 0", file, line);
    }
    void * ret = std::malloc(size);
    if (ret != nullptr) {
        LSan::getInstance().addMalloc(MallocInfo(ret, size, file, line));
    }
    return ret;
}

void __wrap_free(void * pointer, const char * file, int line) {
    if (pointer == nullptr || !LSan::getInstance().removeMalloc(MallocInfo(pointer, 0, file, line))) {
        crash("Invalid free", file, line);
    } else {
        std::free(pointer);
    }
}

[[ noreturn ]] void __wrap_exit(int code, const char * file, int line) {
    std::cout << "\033[32mExiting\033[39m at \033[4m" << file << ":" << line << "\033[24m" << std::endl
              << LSan::getInstance() << std::endl;
    std::exit(code);
}
