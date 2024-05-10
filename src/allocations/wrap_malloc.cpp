/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2022 - 2024  mhahnFr and contributors
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

#include <malloc/malloc.h>

#include <iostream>

#include "wrap_malloc.hpp"

#include "interpose.hpp"
#include "realAlloc.hpp"
#include "../LeakSani.hpp"
#include "../lsanMisc.hpp"
#include "../timing.hpp"
#include "../crashWarner/crash.hpp"
#include "../crashWarner/warn.hpp"
#include "../initialization/init.hpp"

#include "../../include/lsan_internals.h"

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
    if (!lsan::inited) {
        return lsan::real::malloc(size);
    }
    if (size == 0) {
        return lsan::real::malloc(size);
    }

    void* toReturn;
    BENCH(std::lock_guard lock(lsan::getInstance().getMutex());, std::chrono::nanoseconds, lockingTime);
    if (!lsan::getIgnoreMalloc()) {
        lsan::setIgnoreMalloc(true);
        lsan::overAlloc::amount = size;
        lsan::overAlloc::calloc = false;
        BENCH_ONLY(std::chrono::nanoseconds allocTime;)
        BENCH({
            auto node = lsan::getInstance().addMalloc(lsan::MallocInfo(nullptr, size));
            node.key() = lsan::overAlloc::allocation.second;
            node.mapped().pointer = lsan::overAlloc::allocation.second;
            node.mapped().size    = lsan::overAlloc::allocation.first;
            toReturn = lsan::overAlloc::allocation.second;
            BENCH_ONLY(allocTime = lsan::overAlloc::time;)
            lsan::getInstance().readdMalloc(std::move(node));
        }, std::chrono::nanoseconds, totalTrackingTime);
        BENCH_ONLY({
            lsan::timing::addTotalTime(lockingTime + totalTrackingTime, lsan::timing::AllocType::malloc);
            lsan::timing::addTrackingTime(totalTrackingTime - allocTime, lsan::timing::AllocType::malloc);
            lsan::timing::addSystemTime(allocTime, lsan::timing::AllocType::malloc);
            lsan::timing::addLockingTime(lockingTime, lsan::timing::AllocType::malloc);
        })
        lsan::setIgnoreMalloc(false);
    } else {
        toReturn = lsan::real::malloc(size);
    }
    return toReturn;
}

auto calloc(std::size_t objectSize, std::size_t count) -> void * { // TODO: What if calloc malloc's?
    if (!lsan::inited) {
        return real::calloc(objectSize, count);
    }
    // TODO: Overflow check
    const size_t size = objectSize * count;
    if (size == 0) {
        return nullptr;
    }

    void* toReturn;
    BENCH(std::lock_guard lock(lsan::getInstance().getMutex());, std::chrono::nanoseconds, lockingTime);
    if (!lsan::getIgnoreMalloc()) {
        lsan::setIgnoreMalloc(true);
        lsan::overAlloc::amount = size;
        lsan::overAlloc::calloc = true;
        BENCH_ONLY(std::chrono::nanoseconds allocTime;)
        BENCH({
            auto node = lsan::getInstance().addMalloc(lsan::MallocInfo(nullptr, size));
            node.key() = lsan::overAlloc::allocation.second;
            node.mapped().pointer = lsan::overAlloc::allocation.second;
            node.mapped().size = lsan::overAlloc::allocation.first;
            toReturn = lsan::overAlloc::allocation.second;
            BENCH_ONLY(allocTime = lsan::overAlloc::time;)
            lsan::getInstance().readdMalloc(std::move(node));
        }, std::chrono::nanoseconds, totalTrackingTime);
        BENCH_ONLY({
            lsan::timing::addTotalTime(lockingTime + totalTrackingTime, lsan::timing::AllocType::calloc);
            lsan::timing::addTrackingTime(totalTrackingTime - allocTime, lsan::timing::AllocType::calloc);
            lsan::timing::addSystemTime(allocTime, lsan::timing::AllocType::calloc);
            lsan::timing::addLockingTime(lockingTime, lsan::timing::AllocType::calloc);
        })
        lsan::setIgnoreMalloc(false);
    } else {
        toReturn = lsan::real::calloc(objectSize, count);
    }
    return toReturn;
}

auto realloc(void* pointer, std::size_t size) -> void* {
    if (!lsan::inited) {
        return lsan::real::realloc(pointer, size);
    }

    void* toReturn;
    BENCH(std::lock_guard lock(lsan::getInstance().getMutex());, std::chrono::nanoseconds, lockingTime);
    if (!lsan::getIgnoreMalloc()) {
        lsan::setIgnoreMalloc(true);

        BENCH_ONLY(std::chrono::nanoseconds allocTime;)
        BENCH({
            std::size_t oldSize = 0;
            if (pointer != nullptr) {
                const auto& it = lsan::getInstance().getRecord(pointer);
                if (!it) {
                    auto ptr = lsan::real::realloc(pointer, size);
                    lsan::setIgnoreMalloc(false);
                    return ptr;
                }
                oldSize = (*it)->second.size;
            }

            lsan::overAlloc::amount = size;
            lsan::overAlloc::calloc = false;
            auto node = lsan::getInstance().addMalloc(lsan::MallocInfo(nullptr, size)); // TODO: Copy callstack over?
            node.key() = lsan::overAlloc::allocation.second;
            node.mapped().pointer = lsan::overAlloc::allocation.second;
            node.mapped().size = lsan::overAlloc::allocation.first;
            toReturn = lsan::overAlloc::allocation.second;
            BENCH_ONLY(allocTime = lsan::overAlloc::time;)
            lsan::getInstance().readdMalloc(std::move(node));

            if (pointer != nullptr) {
                std::memcpy(toReturn, pointer, size < oldSize ? size : oldSize);
                lsan::getInstance().removeMalloc(pointer);
                BENCH_ONLY(allocTime += overAlloc::time;)
            }
        }, std::chrono::nanoseconds, totalTrackingTime);
        BENCH_ONLY({
            lsan::timing::addTotalTime(lockingTime + totalTrackingTime, lsan::timing::AllocType::realloc);
            lsan::timing::addTrackingTime(totalTrackingTime - allocTime, lsan::timing::AllocType::realloc);
            lsan::timing::addSystemTime(allocTime, lsan::timing::AllocType::realloc);
            lsan::timing::addLockingTime(lockingTime, lsan::timing::AllocType::realloc);
        })
        lsan::setIgnoreMalloc(false);
    } else {
        toReturn = lsan::real::realloc(pointer, size);
    }
    return toReturn;
}

void free(void* pointer) {
    if (!lsan::inited) {
        lsan::real::free(pointer);
        return;
    }
    BENCH(std::lock_guard lock(lsan::getInstance().getMutex());, std::chrono::nanoseconds, lockingTime);
    if (!lsan::getIgnoreMalloc()) {
        lsan::setIgnoreMalloc(true);
        BENCH_ONLY(std::chrono::nanoseconds allocTime {0};)
        BENCH({
            if (pointer == nullptr) {
                if (__lsan_freeNull) {
                    lsan::warn("Free of NULL");
                }
            } else {
                const auto& it = lsan::getInstance().removeMalloc(pointer);
                BENCH_ONLY(allocTime = overAlloc::time;)
                if (!it.first) {
                    if (__lsan_invalidFree && it.second) {
                        if (__lsan_invalidCrash) {
                            crash("Invalid free", it.second);
                        } else {
                            warn("Invalid free", it.second);
                        }
                    } else {
                        lsan::real::free(pointer);
                        lsan::setIgnoreMalloc(false);
                        return;
                    }
                }
            }
        }, std::chrono::nanoseconds, totalTrackingTime);
        BENCH_ONLY({
            lsan::timing::addTotalTime(lockingTime + totalTrackingTime, lsan::timing::AllocType::free);
            lsan::timing::addTrackingTime(totalTrackingTime - allocTime, lsan::timing::AllocType::free);
            lsan::timing::addSystemTime(allocTime, lsan::timing::AllocType::free);
            lsan::timing::addLockingTime(lockingTime, lsan::timing::AllocType::free);
        })
        lsan::setIgnoreMalloc(false);
    }
}

auto malloc_size(const void* pointer) -> std::size_t {
    if (!lsan::inited) return ::malloc_size(pointer);

    std::size_t toReturn;
    std::lock_guard lock(lsan::getInstance().getMutex());
    if (!lsan::getIgnoreMalloc()) {
        lsan::setIgnoreMalloc(true);
        const auto& it = lsan::getInstance().getRecord((void*) pointer);
        if (!it) {
            toReturn = ::malloc_size(pointer);
        } else {
            toReturn = (*it)->second.size;
        }
        lsan::setIgnoreMalloc(false);
    } else {
        toReturn = ::malloc_size(pointer);
    }
    return toReturn;
}

malloc_zone_t* malloc_zone_from_ptr(const void* ptr) {
    if (!lsan::inited) {
        return ::malloc_zone_from_ptr(ptr);
    }

    malloc_zone_t* toReturn;
    std::lock_guard lock(lsan::getInstance().getMutex());
    if (!lsan::getIgnoreMalloc()) {
        lsan::setIgnoreMalloc(true);
        const auto& it = lsan::getInstance().getRecord((void*) ptr);
        if (!it) {
            toReturn = ::malloc_zone_from_ptr(ptr);
        } else {
            abort();
        }
        lsan::setIgnoreMalloc(false);
    } else {
        toReturn = ::malloc_zone_from_ptr(ptr);
    }

    return toReturn;
}

void* malloc_zone_realloc(malloc_zone_t* zone, void* ptr, size_t size) {
    if (!lsan::inited) {
        return ::malloc_zone_realloc(zone, ptr, size);
    }

    void* toReturn;
    std::lock_guard lock(lsan::getInstance().getMutex());
    if (!lsan::getIgnoreMalloc()) {
        lsan::setIgnoreMalloc(true);
        const auto& it = lsan::getInstance().getRecord(ptr);
        if (!it) {
            toReturn = ::malloc_zone_realloc(zone, ptr, size);
        } else {
            crashForce("Mist");
        }
        lsan::setIgnoreMalloc(false);
    } else {
        toReturn = ::malloc_zone_realloc(zone, ptr, size);
    }
    return toReturn;
}

void malloc_zone_batch_free(malloc_zone_t*, void**, unsigned) {
    crashForce("mmm");
}

void malloc_zone_free(malloc_zone_t* zone, void* ptr) {
    if (!lsan::inited) {
        return ::malloc_zone_free(zone, ptr);
    }

    std::lock_guard lock(lsan::getInstance().getMutex());
    if (!lsan::getIgnoreMalloc()) {
        lsan::setIgnoreMalloc(true);
        const auto& it = lsan::getInstance().getRecord(ptr);
        if (!it) {
            ::malloc_zone_free(zone, ptr);
        } else  {
            crashForce("Msit 2");
        }
        lsan::setIgnoreMalloc(false);
    } else {
        ::malloc_zone_free(zone, ptr);
    }
}

void malloc_destroy_zone(malloc_zone_t*) {
    crashForce("bla");
}

#ifndef __linux__
} /* namespace lsan */

INTERPOSE(lsan::malloc,  malloc);
INTERPOSE(lsan::calloc,  calloc);
INTERPOSE(lsan::realloc, realloc);
INTERPOSE(lsan::free,    free);
INTERPOSE(lsan::malloc_size, malloc_size);
INTERPOSE(lsan::malloc_zone_from_ptr, malloc_zone_from_ptr);
INTERPOSE(lsan::malloc_zone_realloc, malloc_zone_realloc);
INTERPOSE(lsan::malloc_zone_free, malloc_zone_free);
INTERPOSE(lsan::malloc_zone_batch_free, malloc_zone_batch_free);
INTERPOSE(lsan::malloc_destroy_zone, malloc_destroy_zone);

#endif /* !__linux__ */
