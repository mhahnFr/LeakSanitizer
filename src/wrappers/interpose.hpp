/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2023 - 2025  mhahnFr
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

#ifndef interpose_hpp
#define interpose_hpp

#ifdef __linux__
#include <dlfcn.h>

#include "../lsanMisc.hpp"

namespace lsan {
/**
 * @brief Loads the function of the given name.
 *
 * Allocations made by this process are ignored.
 *
 * @param name the name of the function to load
 * @return the loaded function
 * @tparam T the type of the function
 */
template<typename T>
static inline auto loadFunction(const char* name) -> T* {
    auto& tracker = getTracker();
    std::lock_guard lock { tracker.mutex };
    bool ignoreMalloc = tracker.ignoreMalloc;
    tracker.ignoreMalloc = true;

    auto toReturn = dlsym(RTLD_NEXT, name);

    tracker.ignoreMalloc = ignoreMalloc;
    return reinterpret_cast<T*>(toReturn);
}
}

#define INTERPOSE(NEW, OLD) \
extern "C" decltype(OLD) OLD __attribute__((weak, alias(#NEW)))

#define REPLACE(RET, NAME)                                              \
namespace lsan::real {                                                  \
template<typename... Args>                                              \
static inline auto NAME(Args... args) -> decltype(::NAME(args...)) {    \
    static auto realFunc = loadFunction<decltype(::NAME)>(#NAME);       \
                                                                        \
    return realFunc(std::forward<Args>(args)...);                       \
}                                                                       \
}                                                                       \
namespace lsan {                                                        \
extern "C" decltype(::NAME) __lsan_##NAME;                              \
}                                                                       \
INTERPOSE(__lsan_##NAME, NAME);                                         \
RET lsan::__lsan_##NAME

#else
/**
 * This structure contains the data for the `__interpose` Mach-O section.
 */
struct interpose {
    /** The new function. */
    const void* newFunc;
    /** The old function. */
    const void* oldFunc;
};

#define INTERPOSE(NEW, OLD)                                                        \
static const struct interpose interpose_##OLD                                      \
    __attribute__((used, section("__DATA, __interpose"))) = {                      \
        reinterpret_cast<const void *>(reinterpret_cast<uintptr_t>(&(lsan::NEW))), \
        reinterpret_cast<const void *>(reinterpret_cast<uintptr_t>(&(OLD)))        \
    }

#define REPLACE(RET, NAME) \
namespace lsan::real {     \
using ::NAME;              \
}                          \
namespace lsan {           \
decltype(::NAME) NAME;     \
}                          \
INTERPOSE(NAME, NAME);     \
RET lsan::NAME

#endif

#endif /* interpose_hpp */
