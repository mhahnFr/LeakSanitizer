/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2022 - 2024  mhahnFr
 *
 * This file is part of the LeakSanitizer.
 *
 * The LeakSanitizer is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * The LeakSanitizer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with the
 * LeakSanitizer, see the file LICENSE.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <iostream>

#include <lsan_internals.h>

#include "wrap_malloc.hpp"

#include "interpose.hpp"
#include "realAlloc.hpp"

#include "../LeakSani.hpp"
#include "../lsanMisc.hpp"
#include "../timing.hpp"
#include "../crashWarner/crash.hpp"
#include "../crashWarner/warn.hpp"
#include "../initialization/init.hpp"

#ifdef __linux__
auto operator new(std::size_t size) -> void * {
    if (size == 0) {
        size = 1;
    }
    
    return malloc(size);
}
#endif /* __linux__ */

/*
 * These wrapper functions still reside here to not break compatibility with
 * previously compiled programs.
 * They will be removed in one of the next version, at latest in version 2.
 *
 *                                                                  - mhahnFr
 */
namespace lsan {
auto __wrap_malloc(std::size_t size, const char*, int) -> void* {
    return malloc(size);
}

auto __wrap_calloc(std::size_t objectSize, std::size_t count, const char*, int) -> void* {
    return calloc(objectSize, count);
}

auto __wrap_realloc(void* pointer, std::size_t size, const char*, int) -> void* {
    return realloc(pointer, size);
}

void __wrap_free(void* pointer, const char*, int) {
    return free(pointer);
}

[[ noreturn ]] void __wrap_exit(int code, const char*, int) {
    exit(code);
}
}

#ifndef __linux__
namespace lsan {
#endif /* !__linux__ */

auto malloc(std::size_t size) -> void * {
    BENCH(auto ptr = lsan::real::malloc(size);, std::chrono::nanoseconds, systemTime);
    
    if (ptr != nullptr && lsan::inited) {
        BENCH(std::lock_guard lock(lsan::getInstance().getMutex());, std::chrono::nanoseconds, lockingTime);
        
        if (!lsan::getIgnoreMalloc()) {
            lsan::setIgnoreMalloc(true);
            BENCH({
                if (__lsan_zeroAllocation && size == 0) {
                    lsan::warn("Implementation-defined allocation of size 0");
                }
                lsan::getInstance().addMalloc(lsan::MallocInfo(ptr, size));
            }, std::chrono::nanoseconds, trackingTime);
            
            BENCH_ONLY({
                lsan::timing::addTotalTime(systemTime + lockingTime + trackingTime, lsan::timing::AllocType::malloc);
                lsan::timing::addTrackingTime(trackingTime, lsan::timing::AllocType::malloc);
                lsan::timing::addSystemTime(systemTime, lsan::timing::AllocType::malloc);
                lsan::timing::addLockingTime(lockingTime, lsan::timing::AllocType::malloc);
            })
            
            lsan::setIgnoreMalloc(false);
        }
    }
    return ptr;
}

auto calloc(std::size_t objectSize, std::size_t count) -> void * { // TODO: What if calloc malloc's?
    BENCH(auto ptr = lsan::real::calloc(objectSize, count);, std::chrono::nanoseconds, sysTime);
    
    if (ptr != nullptr && lsan::inited) {
        BENCH(std::lock_guard lock(lsan::getInstance().getMutex());, std::chrono::nanoseconds, lockingTime);
        
        if (!lsan::getIgnoreMalloc()) {
            lsan::setIgnoreMalloc(true);
            BENCH({
                if (__lsan_zeroAllocation && objectSize * count == 0) {
                    lsan::warn("Implementation-defined allocation of size 0");
                }
                lsan::getInstance().addMalloc(lsan::MallocInfo(ptr, objectSize * count));
            }, std::chrono::nanoseconds, trackingTime);
            
            BENCH_ONLY({
                lsan::timing::addTotalTime(sysTime + lockingTime + trackingTime, lsan::timing::AllocType::calloc);
                lsan::timing::addTrackingTime(trackingTime, lsan::timing::AllocType::calloc);
                lsan::timing::addSystemTime(sysTime, lsan::timing::AllocType::calloc);
                lsan::timing::addLockingTime(lockingTime, lsan::timing::AllocType::calloc);
            })
            lsan::setIgnoreMalloc(false);
        }
    }
    return ptr;
}

auto realloc(void * pointer, std::size_t size) -> void * {
    if (!lsan::inited) return lsan::real::realloc(pointer, size);
    
    BENCH(std::lock_guard lock(lsan::getInstance().getMutex());, std::chrono::nanoseconds, lockingTime);
    
    auto ignored = lsan::getIgnoreMalloc();
    if (!ignored) {
        lsan::setIgnoreMalloc(true);
    }
    BENCH(void * ptr = lsan::real::realloc(pointer, size);, std::chrono::nanoseconds, sysTime);
    if (!ignored) {
        BENCH({
            if (ptr != nullptr) {
                if (pointer != ptr) {
                    if (pointer != nullptr) {
                        lsan::getInstance().removeMalloc(pointer);
                    }
                    lsan::getInstance().addMalloc(lsan::MallocInfo(ptr, size));
                } else {
                    lsan::getInstance().changeMalloc(lsan::MallocInfo(ptr, size));
                }
            }
        }, std::chrono::nanoseconds, trackingTime);
        
        BENCH_ONLY({
            lsan::timing::addTrackingTime(trackingTime, lsan::timing::AllocType::realloc);
            lsan::timing::addLockingTime(lockingTime, lsan::timing::AllocType::realloc);
            lsan::timing::addSystemTime(sysTime, lsan::timing::AllocType::realloc);
            lsan::timing::addTotalTime(sysTime + trackingTime + lockingTime, lsan::timing::AllocType::realloc);
        })
        lsan::setIgnoreMalloc(false);
    }
    return ptr;
}

void free(void * pointer) {
    BENCH_ONLY(std::chrono::nanoseconds totalTime {0});
    
    if (lsan::inited) {
        BENCH(std::lock_guard lock(lsan::getInstance().getMutex());, std::chrono::nanoseconds, lockingTime);
        
        if (!lsan::getIgnoreMalloc()) {
            lsan::setIgnoreMalloc(true);
            BENCH({
                if (pointer == nullptr && __lsan_freeNull) {
                    lsan::warn("Free of NULL");
                }
                auto it = lsan::getInstance().removeMalloc(pointer);
                if (__lsan_invalidFree && !it.first) {
                    if (__lsan_invalidCrash) {
                        lsan::crash("Invalid free", it.second);
                    } else {
                        lsan::warn("Invalid free", it.second);
                    }
                }
            }, std::chrono::nanoseconds, trackingTime);
            BENCH_ONLY({
                lsan::timing::addTrackingTime(trackingTime, lsan::timing::AllocType::free);
                lsan::timing::addLockingTime(lockingTime, lsan::timing::AllocType::free);
                
                totalTime = lockingTime + trackingTime;
            })
            lsan::setIgnoreMalloc(false);
        }
    }
    
    BENCH(lsan::real::free(pointer);, std::chrono::nanoseconds, sysTime);
    BENCH_ONLY({
        if (lsan::inited) {
            std::lock_guard lock { lsan::getInstance().getMutex() };
            if (!lsan::getIgnoreMalloc()) {
                lsan::setIgnoreMalloc(true);
                
                lsan::timing::addSystemTime(sysTime, lsan::timing::AllocType::free);
                lsan::timing::addTotalTime(totalTime + sysTime, lsan::timing::AllocType::free);
                
                lsan::setIgnoreMalloc(false);
            }
        }
    })
}

#ifndef __linux__
} /* namespace lsan */

INTERPOSE(lsan::malloc,  malloc);
INTERPOSE(lsan::calloc,  calloc);
INTERPOSE(lsan::realloc, realloc);
INTERPOSE(lsan::free,    free);

#endif /* !__linux__ */
