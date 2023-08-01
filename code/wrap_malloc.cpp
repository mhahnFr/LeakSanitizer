/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2022 - 2023  mhahnFr and contributors
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

#include <iostream>

#include "wrap_malloc.hpp"

#include "crash.hpp"
#include "warn.hpp"
#include "LeakSani.hpp"
#include "Formatter.hpp"
#include "../include/lsan_stats.h"
#include "../include/lsan_internals.h"

#ifdef __GLIBC__
bool __lsan_glibc = true;
#else
bool __lsan_glibc = false;
#endif

auto __wrap_malloc(std::size_t size, const char * file, int line) -> void * {
    auto & instance = LSan::getLocalInstance();
    auto   ret      = LSan::malloc(size);
    
    if (ret != nullptr && !instance.getIgnoreMalloc()) {
        instance.setIgnoreMalloc(true);
        if (size == 0) {
            if (!__lsan_invalidCrash || __lsan_glibc) {
                warn("Invalid allocation of size 0", file, line, __builtin_return_address(0));
            } else {
                crash("Invalid allocation of size 0", file, line, __builtin_return_address(0));
            }
        }
        instance.addMalloc(MallocInfo(ret, size, file, line, __builtin_return_address(0)));
        instance.setIgnoreMalloc(false);
    }
    return ret;
}

auto __wrap_calloc(std::size_t objectSize, std::size_t count, const char * file, int line) -> void * {
    auto & instance = LSan::getLocalInstance();
    auto   ret      = LSan::calloc(objectSize, count);
    
    if (ret != nullptr && !instance.getIgnoreMalloc()) {
        instance.setIgnoreMalloc(true);
        if (objectSize * count == 0) {
            if (!__lsan_invalidCrash || __lsan_glibc) {
                warn("Invalid allocation of size 0", file, line, __builtin_return_address(0));
            } else {
                crash("Invalid allocation of size 0", file, line, __builtin_return_address(0));
            }
        }
        instance.addMalloc(MallocInfo(ret, objectSize * count, file, line, __builtin_return_address(0)));
        instance.setIgnoreMalloc(false);
    }
    return ret;
}

auto __wrap_realloc(void * pointer, std::size_t size, const char * file, int line) -> void * {
    auto & instance = LSan::getLocalInstance();
    auto   ignored  = instance.getIgnoreMalloc();
    if (!ignored) {
        instance.setIgnoreMalloc(true);
    }
    void * ptr = LSan::realloc(pointer, size);
    if (!ignored) {
        if (ptr != nullptr) {
            if (pointer != ptr) {
                if (pointer != nullptr) {
                    instance.removeMalloc(MallocInfo(pointer, 0, file, line, __builtin_return_address(0)));
                }
                instance.addMalloc(MallocInfo(ptr, size, file, line, __builtin_return_address(0)));
            } else {
                instance.changeMalloc(MallocInfo(ptr, size, file, line, __builtin_return_address(0)));
            }
        }
        instance.setIgnoreMalloc(false);
    }
    return ptr;
}

void __wrap_free(void * pointer, const char * file, int line) {
    auto & instance = LSan::getLocalInstance();
    
    if (!instance.getIgnoreMalloc()) {
        instance.setIgnoreMalloc(true);
        if (pointer == nullptr && __lsan_freeNull) {
            warn("Free of NULL", file, line, __builtin_return_address(0));
        }
        bool removed = instance.removeMalloc(MallocInfo(pointer, 0, file, line, __builtin_return_address(0)));
        if (__lsan_invalidFree && !removed) {
            if (__lsan_invalidCrash) {
                crash("Invalid free", file, line, __builtin_return_address(0));
            } else {
                warn("Invalid free", file, line, __builtin_return_address(0));
            }
        }
        instance.setIgnoreMalloc(false);
    }
    LSan::free(pointer);
}

[[ noreturn ]] void __wrap_exit(int code, const char * file, int line) {
    using Formatter::Style;
    
    auto & instance = LSan::getLocalInstance();
    
    instance.setIgnoreMalloc(true);
    std::ostream & out = __lsan_printCout ? std::cout : std::cerr;
    out << std::endl
        << Formatter::get(Style::GREEN) << "Exiting" << Formatter::clear(Style::GREEN) << " at "
        << Formatter::get(Style::UNDERLINED) << file << ":" << line << Formatter::clear(Style::UNDERLINED)
        << std::endl << std::endl
        << LSan::getInstance() << std::endl;
    if (__lsan_printStatsOnExit) {
        __lsan_printStats();
    }
    LSan::printInformations();
    auto quit = LSan::exit;
    internalCleanUp();
    quit(code);
    __builtin_unreachable();
}

auto malloc(std::size_t size) -> void * {
    auto & instance = LSan::getLocalInstance();
    auto   ptr      = LSan::malloc(size);
    
    if (ptr != nullptr && !instance.getIgnoreMalloc()) {
        instance.setIgnoreMalloc(true);
        if (size == 0) {
            if (!__lsan_invalidCrash || __lsan_glibc) {
                warn("Invalid allocation of size 0", __builtin_return_address(0));
            } else {
                crash("Invalid allocation of size 0", __builtin_return_address(0));
            }
        }
        instance.addMalloc(MallocInfo(ptr, size, __builtin_return_address(0)));
        instance.setIgnoreMalloc(false);
    }
    return ptr;
}

auto calloc(std::size_t objectSize, std::size_t count) -> void * {
    auto & instance = LSan::getLocalInstance();
    auto   ptr      = LSan::calloc(objectSize, count);
    
    if (ptr != nullptr && !instance.getIgnoreMalloc()) {
        instance.setIgnoreMalloc(true);
        if (objectSize * count == 0) {
            if (!__lsan_invalidCrash || __lsan_glibc) {
                warn("Invalid allocation of size 0", __builtin_return_address(0));
            } else {
                crash("Invalid allocation of size 0", __builtin_return_address(0));
            }
        }
        instance.addMalloc(MallocInfo(ptr, objectSize * count, __builtin_return_address(0)));
        instance.setIgnoreMalloc(false);
    }
    return ptr;
}

auto realloc(void * pointer, std::size_t size) -> void * {
    auto & instance = LSan::getLocalInstance();
    auto   ignored  = instance.getIgnoreMalloc();
    if (!ignored) {
        instance.setIgnoreMalloc(true);
    }
    void * ptr = LSan::realloc(pointer, size);
    if (!ignored) {
        if (ptr != nullptr) {
            if (pointer != ptr) {
                if (pointer != nullptr) {
                    instance.removeMalloc(pointer);
                }
                instance.addMalloc(MallocInfo(ptr, size, __builtin_return_address(0)));
            } else {
                instance.changeMalloc(MallocInfo(ptr, size, __builtin_return_address(0)));
            }
        }
        instance.setIgnoreMalloc(false);
    }
    return ptr;
}

void free(void * pointer) {
    auto & instance = LSan::getLocalInstance();
    
    if (!instance.getIgnoreMalloc()) {
        instance.setIgnoreMalloc(true);
        if (pointer == nullptr && __lsan_freeNull) {
            warn("Free of NULL", __builtin_return_address(0));
        }
        bool removed = instance.removeMalloc(pointer);
        if (__lsan_invalidFree && !removed) {
            if (__lsan_invalidCrash) {
                crash("Invalid free", __builtin_return_address(0));
            } else {
                warn("Invalid free", __builtin_return_address(0));
            }
        }
        instance.setIgnoreMalloc(false);
    }
    LSan::free(pointer);
}
