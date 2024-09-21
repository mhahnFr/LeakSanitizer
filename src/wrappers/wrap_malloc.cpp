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

/*
 * These wrapper functions still reside here to not break compatibility with
 * previously compiled programs.
 * They will be removed in one of the next version, at latest in version 2.
 *
 *                                                                  - mhahnFr
 */
#ifdef __linux__
auto operator new(std::size_t size) -> void * {
    if (size == 0) {
        size = 1;
    }

    return malloc(size);
}
#endif /* __linux__ */

namespace lsan {
extern "C" {
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

/**
 * Creates an appropriate invalid free message for the given pointer.
 *
 * @param address the invalidly freed pointer
 * @param doubleFree whether the pointer has previously been freed
 * @return a descriptive invalid free message
 */
static inline auto createInvalidFreeMessage(const void* address, bool doubleFree) -> std::string {
    using namespace formatter;
    
    return formatString<Style::BOLD, Style::RED>(doubleFree ? "Double free" : "Invalid free") 
        + " for address " + formatString<Style::BOLD>(utils::toString(address));
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
                    if (__lsan_invalidFree && !it.first) {
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
                if (__lsan_invalidFree && !it.first) {
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

#ifdef __linux__
extern "C" {
#endif /* __linux__ */

auto malloc(std::size_t size) -> void* {
    BENCH(auto ptr = real::malloc(size);, std::chrono::nanoseconds, systemTime);

    if (ptr != nullptr && !LSan::finished) {
        auto& tracker = getTracker();
        BENCH(const std::lock_guard lock(tracker.mutex);, std::chrono::nanoseconds, lockingTime);

        if (!tracker.ignoreMalloc) {
            tracker.ignoreMalloc = true;
            BENCH({
                if (__lsan_zeroAllocation && size == 0) {
                    warn("Implementation-defined allocation of size 0");
                }
                tracker.addMalloc(MallocInfo(ptr, size));
            }, std::chrono::nanoseconds, trackingTime);

            BENCH_ONLY({
                timing::addTotalTime(systemTime + lockingTime + trackingTime, timing::AllocType::malloc);
                timing::addTrackingTime(trackingTime, timing::AllocType::malloc);
                timing::addSystemTime(systemTime, timing::AllocType::malloc);
                timing::addLockingTime(lockingTime, timing::AllocType::malloc);
            })
            tracker.ignoreMalloc = false;
        }
    }
    return ptr;
}

auto calloc(std::size_t objectSize, std::size_t count) -> void* {
    BENCH(auto ptr = real::calloc(objectSize, count);, std::chrono::nanoseconds, sysTime);

    if (ptr != nullptr && !LSan::finished) {
        auto& tracker = getTracker();
        BENCH(std::lock_guard lock(tracker.mutex);, std::chrono::nanoseconds, lockingTime);

        if (!tracker.ignoreMalloc) {
            tracker.ignoreMalloc = true;
            BENCH({
                if (__lsan_zeroAllocation && objectSize * count == 0) {
                    warn("Implementation-defined allocation of size 0");
                }
                tracker.addMalloc(MallocInfo(ptr, objectSize * count));
            }, std::chrono::nanoseconds, trackingTime);

            BENCH_ONLY({
                timing::addTotalTime(sysTime + lockingTime + trackingTime, timing::AllocType::calloc);
                timing::addTrackingTime(trackingTime, timing::AllocType::calloc);
                timing::addSystemTime(sysTime, timing::AllocType::calloc);
                timing::addLockingTime(lockingTime, timing::AllocType::calloc);
            })
            tracker.ignoreMalloc = false;
        }
    }
    return ptr;
}

auto valloc(std::size_t size) -> void* {
    auto ptr = real::valloc(size);

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

auto aligned_alloc(std::size_t alignment, std::size_t size) -> void* {
    auto ptr = real::aligned_alloc(alignment, size);

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

auto realloc(void* pointer, std::size_t size) -> void* {
    if (LSan::finished) return real::realloc(pointer, size);

    auto& tracker = getTracker();
    BENCH(std::lock_guard lock(tracker.mutex);, std::chrono::nanoseconds, lockingTime);

    auto ignored = tracker.ignoreMalloc;
    if (!ignored) {
        tracker.ignoreMalloc = true;
    }
    BENCH(void* ptr = real::realloc(pointer, size);, std::chrono::nanoseconds, sysTime);
    if (!ignored) {
        BENCH({
            if (ptr != nullptr) {
                if (pointer != ptr) {
                    if (pointer != nullptr) {
                        tracker.removeMalloc(pointer);
                    }
                    tracker.addMalloc(MallocInfo(ptr, size));
                } else {
                    tracker.changeMalloc(MallocInfo(ptr, size));
                }
            }
        }, std::chrono::nanoseconds, trackingTime);

        BENCH_ONLY({
            timing::addTrackingTime(trackingTime, timing::AllocType::realloc);
            timing::addLockingTime(lockingTime, timing::AllocType::realloc);
            timing::addSystemTime(sysTime, timing::AllocType::realloc);
            timing::addTotalTime(sysTime + trackingTime + lockingTime, timing::AllocType::realloc);
        })
        tracker.ignoreMalloc = false;
    }
    return ptr;
}

void free(void* pointer) {
    if (LSan::finished) {
        real::free(pointer);
        return;
    }

    auto& tracker = getTracker();
    BENCH(const std::lock_guard lock { tracker.mutex };, std::chrono::nanoseconds, lockingTime);
    BENCH_ONLY(std::chrono::nanoseconds trackingTime;)
    auto ignored = tracker.ignoreMalloc;
    if (!ignored) {
        tracker.ignoreMalloc = true;
        BENCH({
            if (pointer == nullptr && __lsan_freeNull) {
                warn("Free of NULL");
            } else if (pointer != nullptr) {
                const auto& it = tracker.removeMalloc(pointer);
                if (__lsan_invalidFree && !it.first) {
                    if (__lsan_invalidCrash) {
                        crash(createInvalidFreeMessage(pointer, static_cast<bool>(it.second)), it.second);
                    } else {
                        warn(createInvalidFreeMessage(pointer, static_cast<bool>(it.second)), it.second);
                    }
                }
            }
        }, std::chrono::nanoseconds, trackingTimeLocal);
        BENCH_ONLY(trackingTime = trackingTimeLocal;)
    }
    BENCH(real::free(pointer);, std::chrono::nanoseconds, systemTime);
    BENCH_ONLY(if (!ignored) {
        timing::addTotalTime(systemTime + lockingTime + trackingTime, timing::AllocType::free);
        timing::addTrackingTime(trackingTime, timing::AllocType::free);
        timing::addSystemTime(systemTime, timing::AllocType::free);
        timing::addLockingTime(lockingTime, timing::AllocType::free);
    })
    if (!ignored) {
        tracker.ignoreMalloc = false;
    }
}

REPLACE(auto, posix_memalign)(void** memPtr, std::size_t alignment, std::size_t size) noexcept(noexcept(::posix_memalign(memPtr, alignment, size))) -> int {
    void** checkPtr = memPtr;
    if (checkPtr == nullptr) {
        crashForce("posix_memalign of a NULL pointer");
    }

    auto wasPtr = *memPtr;
    auto toReturn = real::posix_memalign(memPtr, alignment, size);
    if (!LSan::finished) {
        auto& tracker = getTracker();
        const std::lock_guard lock { tracker.mutex };

        if (!tracker.ignoreMalloc) {
            tracker.ignoreMalloc = true;

            if (alignment == 0 || alignment % 2 != 0 || alignment % sizeof(void*) != 0) {
                warn("posix_memalign with invalid alignment of " + std::to_string(alignment));
            }
            if (__lsan_zeroAllocation && size == 0) {
                warn("Implementation-defined allocation of size 0");
            }
            if (*memPtr != wasPtr) {
                tracker.addMalloc(MallocInfo(*memPtr, size));
            }
            tracker.ignoreMalloc = false;
        }
    }
    return toReturn;
}

#ifdef __linux__
} /* extern "C" */
#endif /* __linux__ */

} /* namespace lsan */

INTERPOSE(malloc,  malloc);
INTERPOSE(calloc,  calloc);
INTERPOSE(valloc,  valloc);
INTERPOSE(realloc, realloc);
INTERPOSE(free,    free);

INTERPOSE(aligned_alloc, aligned_alloc);

#ifdef __APPLE__
INTERPOSE(malloc_zone_malloc,   malloc_zone_malloc);
INTERPOSE(malloc_zone_calloc,   malloc_zone_calloc);
INTERPOSE(malloc_zone_valloc,   malloc_zone_valloc);
INTERPOSE(malloc_zone_realloc,  malloc_zone_realloc);
INTERPOSE(malloc_zone_memalign, malloc_zone_memalign);
INTERPOSE(malloc_zone_free,     malloc_zone_free);
INTERPOSE(malloc_destroy_zone,  malloc_destroy_zone);
INTERPOSE(malloc_zone_batch_malloc, malloc_zone_batch_malloc);
INTERPOSE(malloc_zone_batch_free,   malloc_zone_batch_free);
#endif
