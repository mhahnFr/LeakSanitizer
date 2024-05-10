/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2024  mhahnFr
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

#ifndef OverAllocator_hpp
#define OverAllocator_hpp

#include <new>
#include <utility>

#include "timing.hpp"

#include "allocations/realAlloc.hpp"

#define LSAN_OVER_ALLOC 3

namespace lsan {
namespace overAlloc {
extern std::size_t amount;
extern std::pair<std::size_t, void*> allocation;
extern bool calloc;
BENCH_ONLY(extern std::chrono::nanoseconds time;)
}

template<typename T>
struct OverAllocator {
    using value_type = T;

    OverAllocator() = default;
    template<typename U>
    OverAllocator(const OverAllocator<U>&) {}

    auto allocate(std::size_t count) -> T* {
#if LSAN_OVER_ALLOC == 1
        bool reset = false;
        std::size_t size = count * sizeof(T) + sizeof(void*);
        if (count == 1 && overAlloc::amount > 0) {
            size += overAlloc::amount;
            reset = true;
        }
        void* ptr;
        BENCH({
            if (reset && overAlloc::calloc) {
                ptr = lsan::real::calloc(size, 1);
            } else {
                ptr = lsan::real::malloc(size);
            }
        }, std::chrono::nanoseconds, sysTime);
        BENCH_ONLY(overAlloc::time = sysTime;)
        if (ptr == nullptr) {
            throw std::bad_alloc();
        }
        void** addr = reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(ptr) + overAlloc::amount);
        *addr = ptr;
        if (reset) {
            overAlloc::allocation = std::make_pair(overAlloc::amount, ptr);
            overAlloc::amount = 0;
            overAlloc::calloc = false;
        }
        return reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(ptr) + overAlloc::allocation.first + sizeof(void*));
#else

        bool reset = false;
        std::size_t size = count * sizeof(T);
        if (count == 1 && overAlloc::amount > 0) {
            size += overAlloc::amount;
            reset = true;
        }
        void* ptr;
        BENCH({
            if (reset && overAlloc::calloc) {
                ptr = lsan::real::calloc(size, 1);
            } else {
                ptr = lsan::real::malloc(size);
            }
        }, std::chrono::nanoseconds, sysTime);
        BENCH_ONLY(overAlloc::time = sysTime;)
        if (ptr == nullptr) {
            throw std::bad_alloc();
        }
        if (reset) {
#if LSAN_OVER_ALLOC == 3
            overAlloc::allocation = std::make_pair(overAlloc::amount, lsan::real::calloc(overAlloc::amount, 1));
#else
            overAlloc::allocation = std::make_pair(overAlloc::amount, reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(ptr) + sizeof(T)));
#endif
            overAlloc::amount = 0;
            overAlloc::calloc = false;
        }
        return reinterpret_cast<T*>(ptr);
#endif
    }

    void deallocate(T* ptr, std::size_t) {
#if LSAN_OVER_ALLOC == 1
        BENCH({
            real::free(*reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(ptr) - sizeof(void*)));
        }, std::chrono::nanoseconds, sysTime);
#else
        BENCH({
            real::free(ptr);
        }, std::chrono::nanoseconds, sysTime);
#endif
        BENCH_ONLY(overAlloc::time = sysTime;)
    }
};

template<typename T, typename U>
inline auto operator==(const OverAllocator<T>&, const OverAllocator<U>&) -> bool {
    return true;
}

template<typename T, typename U>
inline auto operator!=(const OverAllocator<T>& lhs, const OverAllocator<U>& rhs) -> bool {
    return !(lhs == rhs);
}
}

#endif /* OverAllocator_hpp */
