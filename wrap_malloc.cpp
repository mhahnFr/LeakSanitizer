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

#include "wrap_malloc.hpp"
#include "crash.hpp"
#include "warn.hpp"
#include "LeakSani.hpp"
#include <cstdio>
#include <iostream>

void * __wrap_malloc(size_t size, const char * file, int line) {
    if (size == 0) {
        crash("Invalid allocation of size 0", file, line, 4);
    }
    void * ret = LSan::getInstance().malloc(size);
    if (ret != nullptr) {
        LSan::getInstance().addMalloc(MallocInfo(ret, size, file, line, 5));
    }
    return ret;
}

void __wrap_free(void * pointer, const char * file, int line) {
    if (pointer == nullptr) {
        warn("Free of NULL", file, line, 4);
    }
    LSan::getInstance().removeMalloc(MallocInfo(pointer, 0, file, line, 5));
    LSan::getInstance().free(pointer);
}

[[ noreturn ]] void __wrap_exit(int code, const char * file, int line) {
    std::cout << std::endl
              << "\033[32mExiting\033[39m at \033[4m" << file << ":" << line << "\033[24m" << std::endl << std::endl
              << LSan::getInstance() << std::endl;
    auto quit = LSan::getInstance().exit;
    internalCleanUp();
    quit(code);
    __builtin_unreachable();
}

void * malloc(size_t size) {
    if (size == 0) {
        crash("Invalid allocation of size 0", 4);
    }
    void * ptr = LSan::getInstance().malloc(size);
    if (ptr != nullptr) {
        LSan::getInstance().addMalloc(MallocInfo(ptr, size, 5));
    }
    return ptr;
}

void free(void * pointer) {
    if (pointer == nullptr) {
        warn("Free of NULL", 4);
    }
    LSan::getInstance().removeMalloc(MallocInfo(pointer, 0, 5));
    LSan::getInstance().free(pointer);
}
//
//#define DYLD_INTERPOSE(_replacement,_replacee) \
//   __attribute__((used)) static struct{ const void* replacement; const void* replacee; } _interpose_##_replacee \
//            __attribute__ ((section ("__DATA,__interpose"))) = { (const void*)(unsigned long)&_replacement, (const void*)(unsigned long)&_replacee };
//
//DYLD_INTERPOSE(Aexit, exit)
//DYLD_INTERPOSE(Amalloc, malloc)
//DYLD_INTERPOSE(Afree, free)
