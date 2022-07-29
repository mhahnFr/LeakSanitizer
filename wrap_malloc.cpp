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
#include <execinfo.h>

void * __wrap_malloc(size_t size, const char * file, int line) {
    if (size == 0) {
        crash("Invalid allocation of size 0", file, line);
    }
    void * ret = LSan::getInstance().malloc(size);
    if (ret != nullptr) {
        LSan::getInstance().addMalloc(MallocInfo(ret, size, file, line));
    }
    return ret;
}

void __wrap_free(void * pointer, const char * file, int line) {
    if (pointer == nullptr || !LSan::getInstance().removeMalloc(MallocInfo(pointer, 0, file, line))) {
        crash("Invalid free", file, line);
    } else {
        LSan::getInstance().free(pointer);
    }
}

[[ noreturn ]] void __wrap_exit(int code, const char * file, int line) {
    std::cout << "\033[32mExiting\033[39m at \033[4m" << file << ":" << line << "\033[24m" << std::endl
              << LSan::getInstance() << std::endl;
    auto quit = LSan::getInstance().exit;
    internalCleanUp();
    quit(code);
}

/*void * malloc(size_t size) {
    if (size == 0) {
        crash("Invalid allocation of size 0");
    }
    char ** buf = new char*[1000]();
    backtrace((void**) buf, 1000);
    char ** bt = backtrace_symbols((void**) buf, 1000);
    void * ptr = LSan::malloc(size);
    LSan::getInstance().addMalloc(MallocInfo(ptr, size, std::string(bt[0]), 1));
    return ptr;
}

void free(void * pointer) {
    if (pointer == nullptr || !LSan::getInstance().removeMalloc(MallocInfo(pointer, 0))) {
        crash("Invalid free");
    } else {
        LSan::free(pointer);
    }
}

void exit(int code) {
    std::cout << "\033[32mExiting\033[39m" << std::endl
              << LSan::getInstance() << std::endl;
    LSan::exit(code);
}*/
//
//#define DYLD_INTERPOSE(_replacement,_replacee) \
//   __attribute__((used)) static struct{ const void* replacement; const void* replacee; } _interpose_##_replacee \
//            __attribute__ ((section ("__DATA,__interpose"))) = { (const void*)(unsigned long)&_replacement, (const void*)(unsigned long)&_replacee };
//
//DYLD_INTERPOSE(Aexit, exit)
//DYLD_INTERPOSE(Amalloc, malloc)
//DYLD_INTERPOSE(Afree, free)
