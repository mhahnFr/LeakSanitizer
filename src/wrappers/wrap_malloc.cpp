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

#include "wrap_malloc.hpp"

#include <sstream>

#include "interpose.hpp"
#include "realAlloc.hpp"

#ifdef __APPLE__
# include <mach/mach_init.h>
#endif

#include "../LeakSani.hpp"
#include "../lsanMisc.hpp"
#include "../timing.hpp"
#include "../utils.hpp"
#include "../crashWarner/crashForce.hpp"
#include "../crashWarner/crashOrWarn.hpp"
#include "../formatter/formatter.hpp"

namespace lsan {
/**
 * Creates an appropriate invalid free message for the given pointer.
 *
 * @param address the invalidly freed pointer
 * @param doubleFree whether the pointer has previously been freed
 * @return a descriptive invalid free message
 */
constexpr static inline auto createInvalidFreeMessage(const void* address, const bool doubleFree) -> std::string {
    using namespace formatter;
    
    return formatString<Style::BOLD, Style::RED>(doubleFree ? "Double free" : "Invalid free") 
        + " for address " + formatString<Style::BOLD>(utils::toString(address));
}

/**
 * @brief If the allocations are not ignored the given function is called with
 * the active tracker and the given arguments.
 *
 * If @c BENCHMARK is defined, the locking time is passed to the given
 * function, too.
 *
 * @tparam F the signature of the function to be called
 * @tparam Args the argument types
 * @param func the function to be called
 * @param args the arguments to be forwarded to the given function
 */
template<typename F, typename ...Args>
constexpr static inline void ifNotIgnored(F&& func, Args&& ...args) {
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
        std::invoke(std::forward<F>(func), tracker,
#ifdef BENCHMARK
                    std::move(lockingTime),
#endif
                    std::forward<Args&&>(args)...);
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

#define alloc(func, sizeExpr, type, ...)                                              \
    const auto allocSize = (sizeExpr);                                                \
    BENCH(const auto ptr = func(__VA_ARGS__);, std::chrono::nanoseconds, sysTime);    \
    if (ptr != nullptr && !LSan::finished) {                                          \
        ifNotIgnored([&] (auto& tracker LOCKING_TIME) {                               \
            BENCH({                                                                   \
                if (behaviour::getBehaviour().zeroAllocation() && (allocSize) == 0) { \
                    warn("Implementation-defined allocation of size 0");              \
                }                                                                     \
                tracker.addMalloc(MallocInfo(ptr, (allocSize)));                      \
            }, std::chrono::nanoseconds, trackingTime);                               \
            ADD_TIME(sysTime, lockingTime, trackingTime, timing::AllocType::type);    \
        });                                                                           \
    }                                                                                 \
    return ptr

constexpr static inline void removeAllocation(void* ptr, trackers::ATracker& tracker) {
    if (ptr == nullptr && behaviour::getBehaviour().freeNull()) {
        warn("Free of NULL");
    } else if (ptr != nullptr) {
        if (const auto& [removed, previousAlloc] = tracker.removeMalloc(ptr);
            behaviour::getBehaviour().invalidFree() && !removed) {
            crashOrWarn(createInvalidFreeMessage(ptr, bool(previousAlloc)), previousAlloc);
        }
    }
}

template<typename F, typename... Args>
constexpr static inline auto doRealloc(void* pointer, const std::size_t size, F&& func, Args&&... args) {
    if (LSan::finished) {
        return func(std::forward<Args&&>(args)...);
    }

    auto& tracker = getTracker();
    BENCH(std::lock_guard lock(tracker.mutex);, std::chrono::nanoseconds, lockingTime);

    const auto ignored = tracker.ignoreMalloc;
    if (!ignored) {
        tracker.ignoreMalloc = true;
    }
    BENCH(void* ptr = func(std::forward<Args&&>(args)...);, std::chrono::nanoseconds, sysTime);
    if (!ignored) {
        BENCH(if (ptr != nullptr) {
            if (pointer != ptr) {
                if (pointer != nullptr) {
                    tracker.removeMalloc(pointer);
                }
                tracker.addMalloc(MallocInfo(ptr, size));
            } else {
                tracker.changeMalloc(MallocInfo(ptr, size));
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

#define deallocExpr(func, trackExpr, ...)                                                \
    BENCH_ONLY(bool ignored = true;                                                      \
               std::chrono::nanoseconds trackingTimeOut;                                 \
               std::chrono::nanoseconds lockingTimeOut;)                                 \
    if (!LSan::finished) {                                                               \
        ifNotIgnored([&] (auto& tracker LOCKING_TIME) {                                  \
            BENCH(trackExpr, std::chrono::nanoseconds, trackingTime);                    \
            BENCH_ONLY({                                                                 \
                ignored = false;                                                         \
                trackingTimeOut = trackingTime;                                          \
                lockingTimeOut = lockingTime;                                            \
            })                                                                           \
        });                                                                              \
    }                                                                                    \
    BENCH(func(__VA_ARGS__);, std::chrono::nanoseconds, sysTime);                        \
    BENCH_ONLY(if (!ignored) {                                                           \
        getTracker().withIgnoration(true, [&] {                                          \
            ADD_TIME(sysTime, lockingTimeOut, trackingTimeOut, timing::AllocType::free); \
        });                                                                              \
    })

#define dealloc(func, ptr, ...) deallocExpr(func, removeAllocation(ptr, tracker) __VA_OPT__(,) __VA_ARGS__)

#ifdef __APPLE__
constexpr inline static void assertZone(const malloc_zone_t* zone, const char* message = "Called with NULL as zone") {
    if (zone == nullptr) {
        crashWarner::crashForce(message);
    }
}

auto malloc_zone_malloc(malloc_zone_t* zone, const std::size_t size) -> void* {
    assertZone(zone);
    alloc(::malloc_zone_malloc, size, malloc, zone, size);
}

auto malloc_zone_calloc(malloc_zone_t* zone, const std::size_t count, const std::size_t size) -> void* {
    assertZone(zone);
    alloc(::malloc_zone_calloc, count * size, calloc, zone, count, size);
}

auto malloc_zone_valloc(malloc_zone_t* zone, const std::size_t size) -> void* {
    assertZone(zone);
    alloc(::malloc_zone_valloc, size, malloc, zone, size);
}

auto malloc_zone_memalign(malloc_zone_t* zone, const std::size_t alignment, const std::size_t size) -> void* {
    assertZone(zone);
    alloc(::malloc_zone_memalign, size, malloc, zone, alignment, size);
}

void malloc_destroy_zone(malloc_zone_t* zone) {
    assertZone(zone, "Destroying NULL zone");
    deallocExpr(::malloc_destroy_zone,
                zone->introspect->enumerator(mach_task_self_, &tracker, MALLOC_PTR_IN_USE_RANGE_TYPE, vm_address_t(zone),
                    nullptr, [](auto, auto context, auto, auto array, auto count) {
            auto& theTracker = *reinterpret_cast<trackers::ATracker*>(context);
            for (unsigned i = 0; i < count; ++i) {
                theTracker.removeMalloc(reinterpret_cast<void*>(array[i].address));
            }
        });, zone);
}

auto malloc_zone_batch_malloc(malloc_zone_t* zone, const std::size_t size, void** results, const unsigned num_requested) -> unsigned {
    assertZone(zone, "Batch allocating with NULL zone");
    BENCH(const auto batched = ::malloc_zone_batch_malloc(zone, size, results, num_requested);, std::chrono::nanoseconds, sysTime);
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

void malloc_zone_batch_free(malloc_zone_t* zone, void** to_be_freed, const unsigned num) {
    assertZone(zone, "Batch free with NULL zone");
    deallocExpr(::malloc_zone_batch_free, for (unsigned i = 0; i < num; ++i) {
        removeAllocation(to_be_freed[i], tracker);
    }, zone, to_be_freed, num);
}

void malloc_zone_free(malloc_zone_t* zone, void* ptr) {
    assertZone(zone);
    dealloc(::malloc_zone_free, ptr, zone, ptr);
}

auto malloc_zone_realloc(malloc_zone_t* zone, void* ptr, const std::size_t size) -> void* {
    assertZone(zone);
    return doRealloc(ptr, size, ::malloc_zone_realloc, zone, ptr, size);
}
#endif

auto __lsan_malloc(const std::size_t size) -> void* {
    alloc(real::malloc, size, malloc, size);
}

auto __lsan_calloc(const std::size_t count, const std::size_t objectSize) -> void* {
    alloc(real::calloc, objectSize * count, calloc, count, objectSize);
}

auto __lsan_valloc(const std::size_t size) -> void* {
    alloc(real::valloc, size, malloc, size);
}

auto __lsan_aligned_alloc(const std::size_t alignment, const std::size_t size) -> void* {
    alloc(real::aligned_alloc, size, malloc, alignment, size);
}

auto __lsan_realloc(void* pointer, const std::size_t size) -> void* {
    return doRealloc(pointer, size, real::realloc, pointer, size);
}

void __lsan_free(void* pointer) {
    dealloc(real::free, pointer, pointer);
}

REPLACE(auto, posix_memalign)(void** memPtr, const std::size_t alignment, const std::size_t size) noexcept(noexcept(::posix_memalign(memPtr, alignment, size))) -> int {
    if (void** checkPtr = memPtr; checkPtr == nullptr) {
        crashWarner::crashForce("posix_memalign of a NULL pointer");
    }

    const auto wasPtr = *memPtr;
    BENCH(const auto toReturn = real::posix_memalign(memPtr, alignment, size);, std::chrono::nanoseconds, sysTime);
    if (!LSan::finished) {
        ifNotIgnored([&] (auto& tracker LOCKING_TIME) {
            BENCH({
                if (alignment == 0 || alignment % 2 != 0 || alignment % sizeof(void*) != 0) {
                    warn("posix_memalign with invalid alignment of " + std::to_string(alignment));
                }
                if (behaviour::getBehaviour().zeroAllocation() && size == 0) {
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
