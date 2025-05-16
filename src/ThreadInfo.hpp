/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2025  mhahnFr
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

#ifndef ThreadInfo_hpp
#define ThreadInfo_hpp

#include <optional>
#include <thread>

#include <pthread.h>

namespace lsan {
class ThreadInfo {
    static unsigned long threadId;

    unsigned long number;
    std::size_t stackSize;
    std::thread::id id;
    pthread_t thread;
    void* stackTop;
#ifdef __linux__
    bool dead = false;
#endif

public:
    inline ThreadInfo(std::size_t stackSize,
                      void* stackTop = __builtin_frame_address(0),
                      unsigned long number = createThreadId(),
                      const std::thread::id& id = std::this_thread::get_id(),
                      const pthread_t& thread = pthread_self()):
        number(number), stackSize(stackSize), id(id), thread(thread), stackTop(stackTop) {}

    constexpr auto getNumber() const -> unsigned long {
        return number;
    }

    constexpr auto getThread() const -> pthread_t {
        return thread;
    }

    constexpr auto getId() const -> const std::thread::id& {
        return id;
    }

    constexpr auto getStackSize() const -> std::size_t {
        return stackSize;
    }

    constexpr auto getStackTop() const -> void* {
#ifdef __APPLE__
        return stackTop;
#else
        return reinterpret_cast<void*>(uintptr_t(stackTop) + stackSize);
#endif
    }

#ifdef __linux__
    constexpr auto isDead() const -> bool {
        return dead;
    }

    constexpr void kill() {
        dead = true;
    }
#endif

    inline auto operator==(const ThreadInfo& other) const -> bool {
        return id == other.id;
    }

    static inline auto createThreadId() -> unsigned long {
        return ++threadId;
    }
};
}

#endif /* ThreadInfo_hpp */
