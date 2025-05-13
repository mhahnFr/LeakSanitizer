/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2022 - 2025  mhahnFr
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

#include "interpose.hpp"
#include "realAlloc.hpp"

#ifdef __APPLE__
# include <mach/mach_init.h>
# include <malloc/malloc.h>
#endif

#include "../LeakSani.hpp"
#include "../formatter.hpp"
#include "../lsanMisc.hpp"
#include "../utils.hpp"
#include "../timing.hpp"
#include "../crashWarner/crashOrWarn.hpp"

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

template<typename F, typename ...Args>
inline void ifNotIgnored(F&& func, Args&& ...args) {
    static_assert(std::is_invocable_v<F, trackers::ATracker&,
#ifdef BENCHMARK
                  std::chrono::nanoseconds&&,
#endif
                  Args...>,
                  "The given function is called with the tracker and its given arguments");

    auto& tracker = getTracker();
    BENCH(std::lock_guard lock { tracker.mutex };, std::chrono::nanoseconds, lockingTime);
    if (!tracker.ignoreMalloc) {
        tracker.ignoreMalloc = true;
        std::invoke(std::move(func), tracker,
#ifdef BENCHMARK
                    std::move(lockingTime),
#endif
                    std::move(args)...);
        tracker.ignoreMalloc = false;
    }
}

#ifdef BENCHMARK
# define LOCKING_TIME , auto&& lockingTime

# define ADD_TIME(sys, lock, track, type) do {              \
    const auto& __sys   = (sys);                            \
    const auto& __lock  = (lock);                           \
    const auto& __track = (track);                          \
    const auto& __type  = (type);                           \
                                                            \
    timing::addTotalTime(__sys + __lock + __track, __type); \
    timing::addTrackingTime(__track, __type);               \
    timing::addSystemTime(__sys, __type);                   \
    timing::addLockingTime(__lock, __type);                 \
} while (0)

#else
# define LOCKING_TIME

# define ADD_TIME(sys, lock, track, type)
#endif

#ifdef __APPLE__
auto malloc_zone_malloc(malloc_zone_t* zone, std::size_t size) -> void* {
    if (zone == nullptr) {
        crashForce("Called with NULL as zone");
    }

    BENCH(auto ptr = ::malloc_zone_malloc(zone, size);, std::chrono::nanoseconds, sysTime);
    if (ptr != nullptr && !LSan::finished) {
        ifNotIgnored([&] (auto& tracker LOCKING_TIME) {
            BENCH({
                if (getBehaviour().zeroAllocation() && size == 0) {
                    warn("Implementation-defined allocation of size 0");
                }
                tracker.addMalloc(MallocInfo(ptr, size));
            }, std::chrono::nanoseconds, trackingTime);
            ADD_TIME(sysTime, lockingTime, trackingTime, timing::AllocType::malloc);
        });
    }
    return ptr;
}

auto malloc_zone_calloc(malloc_zone_t* zone, std::size_t count, std::size_t size) -> void* {
    if (zone == nullptr) {
        crashForce("Called with NULL as zone");
    }

    BENCH(auto ptr = ::malloc_zone_calloc(zone, count, size);, std::chrono::nanoseconds, sysTime);
    if (ptr != nullptr && !LSan::finished) {
        ifNotIgnored([&] (auto& tracker LOCKING_TIME) {
            BENCH({
                if (getBehaviour().zeroAllocation() && size == 0) {
                    warn("Implementation-defined allocation of size 0");
                }
                tracker.addMalloc(MallocInfo(ptr, count * size));
            }, std::chrono::nanoseconds, trackingTime);
            ADD_TIME(sysTime, lockingTime, trackingTime, timing::AllocType::calloc);
        });
    }
    return ptr;
}

auto malloc_zone_valloc(malloc_zone_t* zone, std::size_t size) -> void* {
    if (zone == nullptr) {
        crashForce("Called with NULL as zone");
    }

    BENCH(auto ptr = ::malloc_zone_valloc(zone, size);, std::chrono::nanoseconds, sysTime);
    if (ptr != nullptr && !LSan::finished) {
        ifNotIgnored([&] (auto& tracker LOCKING_TIME) {
            BENCH({
                if (getBehaviour().zeroAllocation() && size == 0) {
                    warn("Implementation-defined allocation of size 0");
                }
                tracker.addMalloc(MallocInfo(ptr, size));
            }, std::chrono::nanoseconds, trackingTime);
            ADD_TIME(sysTime, lockingTime, trackingTime, timing::AllocType::malloc);
        });
    }
    return ptr;
}

auto malloc_zone_memalign(malloc_zone_t* zone, std::size_t alignment, std::size_t size) -> void* {
    if (zone == nullptr) {
        crashForce("Called with NULL as zone");
    }

    BENCH(auto ptr = ::malloc_zone_memalign(zone, alignment, size);, std::chrono::nanoseconds, sysTime);
    if (ptr != nullptr && !LSan::finished) {
        ifNotIgnored([&] (auto& tracker LOCKING_TIME) {
            BENCH({
                if (getBehaviour().zeroAllocation() && size == 0) {
                    warn("Implementation-defined allocation of size 0");
                }
                tracker.addMalloc(MallocInfo(ptr, size));
            }, std::chrono::nanoseconds, trackingTime);
            ADD_TIME(sysTime, lockingTime, trackingTime, timing::AllocType::malloc);
        });
    }
    return ptr;
}

void malloc_destroy_zone(malloc_zone_t* zone) {
    if (zone == nullptr) {
        crashForce("Destroying NULL zone");
    }

    BENCH_ONLY(bool ignored = true;
               std::chrono::nanoseconds trackingTimeOut;
               std::chrono::nanoseconds lockingTimeOut;)
    if (!LSan::finished) {
        ifNotIgnored([&] (auto& tracker LOCKING_TIME) {
            BENCH({
                zone->introspect->enumerator(mach_task_self_,
                                             &tracker,
                                             MALLOC_PTR_IN_USE_RANGE_TYPE,
                                             vm_address_t(zone),
                                             nullptr, [](auto, auto context, auto, auto array, auto count) {
                    auto& tracker = *reinterpret_cast<trackers::ATracker*>(context);
                    for (unsigned i = 0; i < count; ++i) {
                        tracker.removeMalloc(reinterpret_cast<void*>(array[i].address));
                    }
                });
            }, std::chrono::nanoseconds, trackingTime);
            BENCH_ONLY({
                ignored = false;
                trackingTimeOut = trackingTime;
                lockingTimeOut = lockingTime;
            })
        });
    }
    BENCH(::malloc_destroy_zone(zone);, std::chrono::nanoseconds, sysTime);
    BENCH_ONLY(if (!ignored) {
        getTracker().withIgnoration(true, [&] {
            ADD_TIME(sysTime, lockingTimeOut, trackingTimeOut, timing::AllocType::free);
        });
    })
}

auto malloc_zone_batch_malloc(malloc_zone_t* zone, std::size_t size, void** results, unsigned num_requested) -> unsigned {
    if (zone == nullptr) {
        crashForce("Batch allocating with NULL zone");
    }
    BENCH(auto batched = ::malloc_zone_batch_malloc(zone, size, results, num_requested);, std::chrono::nanoseconds, sysTime);
    if (!LSan::finished && batched > 0) {
        ifNotIgnored([&] (auto& tracker LOCKING_TIME) {
            BENCH(for (std::size_t i = 0; i < batched; ++i) {
                tracker.addMalloc(MallocInfo(results[i], size));
            }, std::chrono::nanoseconds, trackingTime);
            ADD_TIME(sysTime, lockingTime, trackingTime, timing::AllocType::malloc);
        });
    }
    return batched;
}

void malloc_zone_batch_free(malloc_zone_t* zone, void** to_be_freed, unsigned num) {
    if (zone == nullptr) {
        crashForce("Batch free with NULL zone");
    }
    BENCH_ONLY(bool ignored = true;
               std::chrono::nanoseconds trackingTimeOut;
               std::chrono::nanoseconds lockingTimeOut;)
    if (!LSan::finished && num > 0) {
        ifNotIgnored([&] (auto& tracker LOCKING_TIME) {
            BENCH({
                for (unsigned i = 0; i < num; ++i) {
                    if (to_be_freed[i] == nullptr && getBehaviour().freeNull()) {
                        warn("Free of NULL");
                    } else if (to_be_freed[i] != nullptr) {
                        const auto& it = tracker.removeMalloc(to_be_freed[i]);
                        if (getBehaviour().invalidFree() && !it.first) {
                            crashOrWarn(createInvalidFreeMessage(to_be_freed[i], bool(it.second)), it.second);
                        }
                    }
                }
            }, std::chrono::nanoseconds, trackingTime);
            BENCH_ONLY({
                ignored = false;
                trackingTimeOut = trackingTime;
                lockingTimeOut = lockingTime;
            })
        });
    }
    BENCH(::malloc_zone_batch_free(zone, to_be_freed, num);, std::chrono::nanoseconds, sysTime);
    BENCH_ONLY(if (!ignored) {
        getTracker().withIgnoration(true, [&] {
            ADD_TIME(sysTime, lockingTimeOut, trackingTimeOut, timing::AllocType::free);
        });
    })
}

void malloc_zone_free(malloc_zone_t* zone, void* ptr) {
    if (zone == nullptr) {
        crashForce("Called with NULL as zone");
    }

    BENCH_ONLY(bool ignored = true;
               std::chrono::nanoseconds trackingTimeOut;
               std::chrono::nanoseconds lockingTimeOut;)
    if (!LSan::finished) {
        ifNotIgnored([&] (auto& tracker LOCKING_TIME) {
            BENCH({
                if (ptr == nullptr && getBehaviour().freeNull()) {
                    warn("Free of NULL");
                } else if (ptr != nullptr) {
                    const auto& it = tracker.removeMalloc(ptr);
                    if (getBehaviour().invalidFree() && !it.first) {
                        crashOrWarn(createInvalidFreeMessage(ptr, bool(it.second)), it.second);
                    }
                }
            }, std::chrono::nanoseconds, trackingTime);
            BENCH_ONLY({
                ignored = false;
                trackingTimeOut = trackingTime;
                lockingTimeOut = lockingTime;
            })
        });
    }
    BENCH(::malloc_zone_free(zone, ptr);, std::chrono::nanoseconds, sysTime);
    BENCH_ONLY(if (!ignored) {
        getTracker().withIgnoration(true, [&] {
            ADD_TIME(sysTime, lockingTimeOut, trackingTimeOut, timing::AllocType::free);
        });
    })
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

auto __lsan_malloc(std::size_t size) -> void* {
    BENCH(auto ptr = real::malloc(size);, std::chrono::nanoseconds, systemTime);

    if (ptr != nullptr && !LSan::finished) {
        ifNotIgnored([&] (auto& tracker LOCKING_TIME) {
            BENCH({
                if (getBehaviour().zeroAllocation() && size == 0) {
                    warn("Implementation-defined allocation of size 0");
                }
                tracker.addMalloc(MallocInfo(ptr, size));
            }, std::chrono::nanoseconds, trackingTime);
            ADD_TIME(systemTime, lockingTime, trackingTime, timing::AllocType::malloc);
        });
    }
    return ptr;
}

auto __lsan_calloc(std::size_t objectSize, std::size_t count) -> void* {
    BENCH(auto ptr = real::calloc(objectSize, count);, std::chrono::nanoseconds, sysTime);

    if (ptr != nullptr && !LSan::finished) {
        ifNotIgnored([&] (auto& tracker LOCKING_TIME) {
            BENCH({
                if (getBehaviour().zeroAllocation() && objectSize * count == 0) {
                    warn("Implementation-defined allocation of size 0");
                }
                tracker.addMalloc(MallocInfo(ptr, objectSize * count));
            }, std::chrono::nanoseconds, trackingTime);
            ADD_TIME(sysTime, lockingTime, trackingTime, timing::AllocType::calloc);
        });
    }
    return ptr;
}

auto __lsan_valloc(std::size_t size) -> void* {
    BENCH(auto ptr = real::valloc(size);, std::chrono::nanoseconds, sysTime);

    if (ptr != nullptr && !LSan::finished) {
        ifNotIgnored([&] (auto& tracker LOCKING_TIME) {
            BENCH({
                if (getBehaviour().zeroAllocation() && size == 0) {
                    warn("Implementation-defined allocation of size 0");
                }
                tracker.addMalloc(MallocInfo(ptr, size));
            }, std::chrono::nanoseconds, trackingTime);
            ADD_TIME(sysTime, lockingTime, trackingTime, timing::AllocType::malloc);
        });
    }
    return ptr;
}

auto __lsan_aligned_alloc(std::size_t alignment, std::size_t size) -> void* {
    BENCH(auto ptr = real::aligned_alloc(alignment, size);, std::chrono::nanoseconds, sysTime);

    if (ptr != nullptr && !LSan::finished) {
        ifNotIgnored([&] (auto& tracker LOCKING_TIME) {
            BENCH({
                if (getBehaviour().zeroAllocation() && size == 0) {
                    warn("Implementation-defined allocation of size 0");
                }
                tracker.addMalloc(MallocInfo(ptr, size));
            }, std::chrono::nanoseconds, trackingTime);
            ADD_TIME(sysTime, lockingTime, trackingTime, timing::AllocType::malloc);
        });
    }
    return ptr;
}

auto __lsan_realloc(void* pointer, std::size_t size) -> void* {
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

void __lsan_free(void* pointer) {
    if (LSan::finished) {
        real::free(pointer);
        return;
    }

    BENCH_ONLY(bool ignored = true;
               std::chrono::nanoseconds lockingTimeOut;
               std::chrono::nanoseconds trackingTime;)
    ifNotIgnored([&] (auto& tracker LOCKING_TIME) {
        BENCH({
            if (pointer == nullptr && getBehaviour().freeNull()) {
                warn("Free of NULL");
            } else if (pointer != nullptr) {
                const auto& it = tracker.removeMalloc(pointer);
                if (getBehaviour().invalidFree() && !it.first) {
                    crashOrWarn(createInvalidFreeMessage(pointer, static_cast<bool>(it.second)), it.second);
                }
            }
        }, std::chrono::nanoseconds, trackingTimeLocal);
        BENCH_ONLY({
            trackingTime = trackingTimeLocal;
            ignored = false;
            lockingTimeOut = lockingTime;
        })
    });
    BENCH(real::free(pointer);, std::chrono::nanoseconds, sysTime);
    BENCH_ONLY(if (!ignored) {
        getTracker().withIgnoration(true, [&] {
            ADD_TIME(sysTime, lockingTimeOut, trackingTime, timing::AllocType::free);
        });
    })
}

#ifdef __linux__
} /* extern "C" */
#endif /* __linux__ */

REPLACE(auto, posix_memalign)(void** memPtr, std::size_t alignment, std::size_t size) noexcept(noexcept(::posix_memalign(memPtr, alignment, size))) -> int {
    void** checkPtr = memPtr;
    if (checkPtr == nullptr) {
        crashForce("posix_memalign of a NULL pointer");
    }

    auto wasPtr = *memPtr;
    BENCH(auto toReturn = real::posix_memalign(memPtr, alignment, size);, std::chrono::nanoseconds, sysTime);
    if (!LSan::finished) {
        ifNotIgnored([&] (auto& tracker LOCKING_TIME) {
            BENCH({
                if (alignment == 0 || alignment % 2 != 0 || alignment % sizeof(void*) != 0) {
                    warn("posix_memalign with invalid alignment of " + std::to_string(alignment));
                }
                if (getBehaviour().zeroAllocation() && size == 0) {
                    warn("Implementation-defined allocation of size 0");
                }
                if (*memPtr != wasPtr) {
                    tracker.addMalloc(MallocInfo(*memPtr, size));
                }
            }, std::chrono::nanoseconds, trackingTime);
            ADD_TIME(sysTime, lockingTime, trackingTime, timing::AllocType::malloc);
        });
    }
    return toReturn;
}
} /* namespace lsan */

INTERPOSE(__lsan_malloc,  malloc);
INTERPOSE(__lsan_calloc,  calloc);
INTERPOSE(__lsan_valloc,  valloc);
INTERPOSE(__lsan_realloc, realloc);
INTERPOSE(__lsan_free,    free);

INTERPOSE(__lsan_aligned_alloc, aligned_alloc);

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
