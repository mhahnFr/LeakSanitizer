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

#include <sstream>

#include <lsan_internals.h>

#include "wrap_malloc.hpp"

#include "interpose.hpp"
#include "realAlloc.hpp"

#ifdef __APPLE__
 #include <mach/mach_init.h>
 #include <malloc/malloc.h>
#endif

#include "../LeakSani.hpp"
#include "../formatter.hpp"
#include "../lsanMisc.hpp"
#include "../utils.hpp"
#include "../timing.hpp"
#include "../crashWarner/crash.hpp"
#include "../crashWarner/warn.hpp"

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

/**
 * Creates an appropriate invalid free message for the given pointer.
 *
 * @param address the invalidly freed pointer
 * @param doubleFree whether the pointer has previously been freed
 * @return a descriptive invalid free message
 */
static inline auto createInvalidFreeMessage(const void* address, bool doubleFree) -> std::string {
    using namespace lsan::formatter;
    
    return formatString<Style::BOLD, Style::RED>(doubleFree ? "Double free" : "Invalid free") 
        + " for address " + formatString<Style::BOLD>(lsan::utils::toString(address));
}

#ifdef __APPLE__
auto malloc_zone_malloc(malloc_zone_t* zone, std::size_t size) -> void* {
    if (zone == nullptr) {
        crashForce("Called with NULL as zone");
    }

    auto ptr = ::malloc_zone_malloc(zone, size);
    if (ptr != nullptr && !LSan::finished) {
        auto& tracker = getTracker();
        const std::lock_guard lock { tracker.mutex };
        if (!tracker.ignoreMalloc) {
            tracker.ignoreMalloc = true;

            if (__lsan_zeroAllocation && size == 0) {
                warn("Implementation-defined allocation of size 0");
            }
            tracker.addMalloc(MallocInfo(ptr, size));
            tracker.ignoreMalloc = false;
        }
    }
    return ptr;
}

auto malloc_zone_calloc(malloc_zone_t* zone, std::size_t count, std::size_t size) -> void* {
    if (zone == nullptr) {
        crashForce("Called with NULL as zone");
    }

    auto ptr = ::malloc_zone_calloc(zone, count, size);
    if (ptr != nullptr && !LSan::finished) {
        auto& tracker = getTracker();
        const std::lock_guard lock { tracker.mutex };
        if (!tracker.ignoreMalloc) {
            tracker.ignoreMalloc = true;

            if (__lsan_zeroAllocation && size == 0) {
                warn("Implementation-defined allocation of size 0");
            }
            tracker.addMalloc(MallocInfo(ptr, count * size));
            tracker.ignoreMalloc = false;
        }
    }
    return ptr;
}

auto malloc_zone_valloc(malloc_zone_t* zone, std::size_t size) -> void* {
    if (zone == nullptr) {
        crashForce("Called with NULL as zone");
    }

    auto ptr = ::malloc_zone_valloc(zone, size);
    if (ptr != nullptr && !LSan::finished) {
        auto& tracker = getTracker();
        const std::lock_guard lock { tracker.mutex };
        if (!tracker.ignoreMalloc) {
            tracker.ignoreMalloc = true;

            if (__lsan_zeroAllocation && size == 0) {
                warn("Implementation-defined allocation of size 0");
            }
            tracker.addMalloc(MallocInfo(ptr, size));
            tracker.ignoreMalloc = false;
        }
    }
    return ptr;
}

auto malloc_zone_memalign(malloc_zone_t* zone, std::size_t alignment, std::size_t size) -> void* {
    if (zone == nullptr) {
        crashForce("Called with NULL as zone");
    }

    auto ptr = ::malloc_zone_memalign(zone, alignment, size);
    if (ptr != nullptr && !LSan::finished) {
        auto& tracker = getTracker();
        const std::lock_guard lock { tracker.mutex };
        if (!tracker.ignoreMalloc) {
            tracker.ignoreMalloc = true;

            if (__lsan_zeroAllocation && size == 0) {
                warn("Implementation-defined allocation of size 0");
            }
            tracker.addMalloc(MallocInfo(ptr, size));
            tracker.ignoreMalloc = false;
        }
    }
    return ptr;
}

void malloc_destroy_zone(malloc_zone_t* zone) {
    if (zone == nullptr) {
        crashForce("Destroying NULL zone");
    }

    if (!LSan::finished) {
        auto& tracker = getTracker();
        const std::lock_guard lock { tracker.mutex };
        if (!tracker.ignoreMalloc) {
            tracker.ignoreMalloc = true;
            zone->introspect->enumerator(mach_task_self_,
                                         &tracker,
                                         MALLOC_PTR_IN_USE_RANGE_TYPE,
                                         reinterpret_cast<vm_address_t>(zone),
                                         nullptr, [](task_t, auto context, unsigned, auto array, unsigned count) {
                auto& tracker = *reinterpret_cast<ATracker*>(context);
                for (unsigned i = 0; i < count; ++i) {
                    tracker.removeMalloc(reinterpret_cast<void*>(array[i].address));
                }
            });
            tracker.ignoreMalloc = false;
        }
    }

    ::malloc_destroy_zone(zone);
}

auto malloc_zone_batch_malloc(malloc_zone_t* zone, std::size_t size, void** results, unsigned num_requested) -> unsigned {
    if (zone == nullptr) {
        crashForce("Batch allocating with NULL zone");
    }
    auto batched = ::malloc_zone_batch_malloc(zone, size, results, num_requested);
    if (!LSan::finished && batched > 0) {
        auto& tracker = getTracker();
        const std::lock_guard lock { tracker.mutex };
        if (!tracker.ignoreMalloc) {
            tracker.ignoreMalloc = true;
            for (std::size_t i = 0; i < batched; ++i) {
                tracker.addMalloc(MallocInfo(results[i], size));
            }
            tracker.ignoreMalloc = false;
        }
    }
    return batched;
}

void malloc_zone_batch_free(malloc_zone_t* zone, void** to_be_freed, unsigned num) {
    if (zone == nullptr) {
        crashForce("Batch free with NULL zone");
    }
    if (!LSan::finished && num > 0) {
        auto& tracker = getTracker();
        const std::lock_guard lock { tracker.mutex };
        if (!tracker.ignoreMalloc) {
            tracker.ignoreMalloc = true;
            for (unsigned i = 0; i < num; ++i) {
                if (to_be_freed[i] == nullptr && __lsan_freeNull) {
                    warn("Free of NULL");
                } else if (to_be_freed[i] != nullptr) {
                    const auto& it = tracker.removeMalloc(to_be_freed[i]);
                    if (__lsan_invalidFreeLevel > 0 && !it.first) {
                        if (__lsan_invalidCrash) {
                            crash(createInvalidFreeMessage(to_be_freed[i], static_cast<bool>(it.second)), it.second);
                        } else {
                            warn(createInvalidFreeMessage(to_be_freed[i], static_cast<bool>(it.second)), it.second);
                        }
                    }
                }
            }
            tracker.ignoreMalloc = false;
        }
    }
    ::malloc_zone_batch_free(zone, to_be_freed, num);
}

void malloc_zone_free(malloc_zone_t* zone, void* ptr) {
    if (zone == nullptr) {
        crashForce("Called with NULL as zone");
    }

    if (!LSan::finished) {
        auto& tracker = getTracker();
        const std::lock_guard lock { tracker.mutex };
        if (!tracker.ignoreMalloc) {
            tracker.ignoreMalloc = true;
            if (ptr == nullptr && __lsan_freeNull) {
                warn("Free of NULL");
            } else if (ptr != nullptr) {
                const auto& it = tracker.removeMalloc(ptr);
                if (__lsan_invalidFreeLevel > 0 && !it.first) {
                    if (__lsan_invalidCrash) {
                        crash(createInvalidFreeMessage(ptr, static_cast<bool>(it.second)), it.second);
                    } else {
                        warn(createInvalidFreeMessage(ptr, static_cast<bool>(it.second)), it.second);
                    }
                }
            }
            tracker.ignoreMalloc = false;
        }
    }
    ::malloc_zone_free(zone, ptr);
}

auto malloc_zone_realloc(malloc_zone_t* zone, void* ptr, std::size_t size) -> void* {
    if (zone == nullptr) {
        crashForce("Called with NULL as zone");
    }

    if (LSan::finished) {
        return ::malloc_zone_realloc(zone, ptr, size);
    }
    auto& tracker = getTracker();
    std::lock_guard lock { tracker.mutex };
    auto ignored = tracker.ignoreMalloc;
    if (!ignored) {
        tracker.ignoreMalloc = true;
    }
    auto toReturn = ::malloc_zone_realloc(zone, ptr, size);
    if (!ignored) {
        if (toReturn != nullptr) {
            if (toReturn != ptr) {
                if (ptr != nullptr) {
                    tracker.removeMalloc(ptr);
                }
                tracker.addMalloc(MallocInfo(toReturn, size));
            } else {
                tracker.changeMalloc(MallocInfo(toReturn, size));
            }
        }
        tracker.ignoreMalloc = false;
    }
    return toReturn;
}
#endif

auto malloc(std::size_t size) -> void * {
    BENCH(auto ptr = lsan::real::malloc(size);, std::chrono::nanoseconds, systemTime);
    
    if (ptr != nullptr && !lsan::LSan::finished) {
        auto& tracker = lsan::getTracker();
        BENCH(const std::lock_guard lock(tracker.mutex);, std::chrono::nanoseconds, lockingTime);

        if (!tracker.ignoreMalloc) {
            tracker.ignoreMalloc = true;
            BENCH({
                if (__lsan_zeroAllocation && size == 0) {
                    lsan::warn("Implementation-defined allocation of size 0");
                }
                tracker.addMalloc(lsan::MallocInfo(ptr, size));
            }, std::chrono::nanoseconds, trackingTime);
            
            BENCH_ONLY({
                lsan::timing::addTotalTime(systemTime + lockingTime + trackingTime, lsan::timing::AllocType::malloc);
                lsan::timing::addTrackingTime(trackingTime, lsan::timing::AllocType::malloc);
                lsan::timing::addSystemTime(systemTime, lsan::timing::AllocType::malloc);
                lsan::timing::addLockingTime(lockingTime, lsan::timing::AllocType::malloc);
            })
            
            tracker.ignoreMalloc = false;
        }
    }
    return ptr;
}

auto calloc(std::size_t objectSize, std::size_t count) -> void* {
    BENCH(auto ptr = lsan::real::calloc(objectSize, count);, std::chrono::nanoseconds, sysTime);
    
    if (ptr != nullptr && !lsan::LSan::finished) {
        auto& tracker = lsan::getTracker();
        BENCH(std::lock_guard lock(tracker.mutex);, std::chrono::nanoseconds, lockingTime);

        if (!tracker.ignoreMalloc) {
            tracker.ignoreMalloc = true;
            BENCH({
                if (__lsan_zeroAllocation && objectSize * count == 0) {
                    lsan::warn("Implementation-defined allocation of size 0");
                }
                tracker.addMalloc(lsan::MallocInfo(ptr, objectSize * count));
            }, std::chrono::nanoseconds, trackingTime);
            
            BENCH_ONLY({
                lsan::timing::addTotalTime(sysTime + lockingTime + trackingTime, lsan::timing::AllocType::calloc);
                lsan::timing::addTrackingTime(trackingTime, lsan::timing::AllocType::calloc);
                lsan::timing::addSystemTime(sysTime, lsan::timing::AllocType::calloc);
                lsan::timing::addLockingTime(lockingTime, lsan::timing::AllocType::calloc);
            })
            tracker.ignoreMalloc = false;
        }
    }
    return ptr;
}

auto valloc(std::size_t size) -> void* {
    auto ptr = lsan::real::valloc(size);

    if (ptr != nullptr && !lsan::LSan::finished) {
        auto& tracker = lsan::getTracker();
        const std::lock_guard lock { tracker.mutex };

        if (!tracker.ignoreMalloc) {
            tracker.ignoreMalloc = true;

            if (__lsan_zeroAllocation && size == 0) {
                lsan::warn("Implementation-defined allocation of size 0");
            }
            tracker.addMalloc(lsan::MallocInfo(ptr, size));
            tracker.ignoreMalloc = false;
        }
    }
    return ptr;
}

auto aligned_alloc(std::size_t alignment, std::size_t size) -> void* {
    auto ptr = lsan::real::aligned_alloc(alignment, size);

    if (ptr != nullptr && !lsan::LSan::finished) {
        auto& tracker = lsan::getTracker();
        const std::lock_guard lock { tracker.mutex };

        if (!tracker.ignoreMalloc) {
            tracker.ignoreMalloc = true;

            if (__lsan_zeroAllocation && size == 0) {
                lsan::warn("Implementation-defined allocation of size 0");
            }
            tracker.addMalloc(lsan::MallocInfo(ptr, size));
            tracker.ignoreMalloc = false;
        }
    }
    return ptr;
}

auto realloc(void * pointer, std::size_t size) -> void * {
    if (lsan::LSan::finished) return lsan::real::realloc(pointer, size);

    auto& tracker = lsan::getTracker();
    BENCH(std::lock_guard lock(tracker.mutex);, std::chrono::nanoseconds, lockingTime);

    auto ignored = tracker.ignoreMalloc;
    if (!ignored) {
        tracker.ignoreMalloc = true;
    }
    BENCH(void* ptr = lsan::real::realloc(pointer, size);, std::chrono::nanoseconds, sysTime);
    if (!ignored) {
        BENCH({
            if (ptr != nullptr) {
                if (pointer != ptr) {
                    if (pointer != nullptr) {
                        tracker.removeMalloc(pointer);
                    }
                    tracker.addMalloc(lsan::MallocInfo(ptr, size));
                } else {
                    tracker.changeMalloc(lsan::MallocInfo(ptr, size));
                }
            }
        }, std::chrono::nanoseconds, trackingTime);
        
        BENCH_ONLY({
            lsan::timing::addTrackingTime(trackingTime, lsan::timing::AllocType::realloc);
            lsan::timing::addLockingTime(lockingTime, lsan::timing::AllocType::realloc);
            lsan::timing::addSystemTime(sysTime, lsan::timing::AllocType::realloc);
            lsan::timing::addTotalTime(sysTime + trackingTime + lockingTime, lsan::timing::AllocType::realloc);
        })
        tracker.ignoreMalloc = false;
    }
    return ptr;
}

void free(void* pointer) {
    if (lsan::LSan::finished) {
        lsan::real::free(pointer);
        return;
    }

    auto& tracker = lsan::getTracker();
    BENCH(const std::lock_guard lock { tracker.mutex };, std::chrono::nanoseconds, lockingTime);
    BENCH_ONLY(std::chrono::nanoseconds trackingTime;)
    auto ignored = tracker.ignoreMalloc;
    if (!ignored) {
        tracker.ignoreMalloc = true;
        BENCH({
            if (pointer == nullptr && __lsan_freeNull) {
                lsan::warn("Free of NULL");
            } else if (pointer != nullptr) {
                const auto& it = tracker.removeMalloc(pointer);
                if (__lsan_invalidFreeLevel > 0 && !it.first) {
                    if (__lsan_invalidCrash) {
                        lsan::crash(createInvalidFreeMessage(pointer, static_cast<bool>(it.second)), it.second);
                    } else {
                        lsan::warn(createInvalidFreeMessage(pointer, static_cast<bool>(it.second)), it.second);
                    }
                }
            }
        }, std::chrono::nanoseconds, trackingTimeLocal);
        BENCH_ONLY(trackingTime = trackingTimeLocal;)
    }
    BENCH(lsan::real::free(pointer);, std::chrono::nanoseconds, systemTime);
    BENCH_ONLY(if (!ignored) {
        lsan::timing::addTotalTime(systemTime + lockingTime + trackingTime, lsan::timing::AllocType::free);
        lsan::timing::addTrackingTime(trackingTime, lsan::timing::AllocType::free);
        lsan::timing::addSystemTime(systemTime, lsan::timing::AllocType::free);
        lsan::timing::addLockingTime(lockingTime, lsan::timing::AllocType::free);
    })
    if (!ignored) {
        tracker.ignoreMalloc = false;
    }
}

#ifndef __linux__
auto posix_memalign(void** memPtr, std::size_t alignment, std::size_t size) -> int {
    void** checkPtr = memPtr;
    if (checkPtr == nullptr) {
        lsan::crashForce("posix_memalign of a NULL pointer");
    }

    auto wasPtr = *memPtr;
    auto toReturn = lsan::real::posix_memalign(memPtr, alignment, size);
    if (!lsan::LSan::finished) {
        auto& tracker = lsan::getTracker();
        const std::lock_guard lock { tracker.mutex };

        if (!tracker.ignoreMalloc) {
            tracker.ignoreMalloc = true;

            if (alignment == 0 || alignment % 2 != 0 || alignment % sizeof(void*) != 0) {
                lsan::warn("posix_memalign with invalid alignment of " + std::to_string(alignment));
            }
            if (__lsan_zeroAllocation && size == 0) {
                lsan::warn("Implementation-defined allocation of size 0");
            }
            if (*memPtr != wasPtr) {
                tracker.addMalloc(lsan::MallocInfo(*memPtr, size));
            }
            tracker.ignoreMalloc = false;
        }
    }
    return toReturn;
}
#endif

#ifndef __linux__
} /* namespace lsan */

INTERPOSE(lsan::malloc,  malloc);
INTERPOSE(lsan::calloc,  calloc);
INTERPOSE(lsan::valloc,  valloc);
INTERPOSE(lsan::realloc, realloc);
INTERPOSE(lsan::free,    free);

INTERPOSE(lsan::aligned_alloc,  aligned_alloc);
#ifndef __linux__
INTERPOSE(lsan::posix_memalign, posix_memalign);
#endif

#ifdef __APPLE__
INTERPOSE(lsan::malloc_zone_malloc,   malloc_zone_malloc);
INTERPOSE(lsan::malloc_zone_calloc,   malloc_zone_calloc);
INTERPOSE(lsan::malloc_zone_valloc,   malloc_zone_valloc);
INTERPOSE(lsan::malloc_zone_realloc,  malloc_zone_realloc);
INTERPOSE(lsan::malloc_zone_memalign, malloc_zone_memalign);
INTERPOSE(lsan::malloc_destroy_zone,  malloc_destroy_zone);
INTERPOSE(lsan::malloc_zone_free,     malloc_zone_free);
INTERPOSE(lsan::malloc_zone_batch_malloc, malloc_zone_batch_malloc);
INTERPOSE(lsan::malloc_zone_batch_free,   malloc_zone_batch_free);
#endif

#endif /* !__linux__ */
