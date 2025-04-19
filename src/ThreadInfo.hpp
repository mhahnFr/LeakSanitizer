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

#ifdef __linux__
# include <unistd.h>
#endif

namespace lsan {
class ThreadInfo {
    static unsigned long threadId;

    unsigned long number;
    std::size_t stackSize;
    std::thread::id id;
    pthread_t thread;
    void* stackTop;
#ifdef __linux__
    pid_t tid;
    bool dead = false;
#endif

public:
    inline ThreadInfo(std::size_t stackSize,
                      void* stackTop = __builtin_frame_address(0),
                      unsigned long number = createThreadId(),
                      const std::thread::id& id = std::this_thread::get_id(),
                      const pthread_t& thread = pthread_self()
#ifdef __linux__
                      , const pid_t tid = gettid()
#endif
                      ):
        number(number), stackSize(stackSize), id(id), thread(thread), stackTop(stackTop)
#ifdef __linux__
        , tid(tid)
#endif
    {}

    constexpr inline auto getNumber() const -> unsigned long {
        return number;
    }

    constexpr inline auto getThread() const -> pthread_t {
        return thread;
    }

    constexpr inline auto getId() const -> const std::thread::id& {
        return id;
    }

    constexpr inline auto getStackSize() const -> std::size_t {
        return stackSize;
    }

    constexpr inline auto getStackTop() const -> void* {
        return stackTop;
    }

#ifdef __linux__
    constexpr inline auto getTid() const -> pid_t {
        return tid;
    }

    constexpr inline auto isDead() const -> bool {
        return dead;
    }

    constexpr inline void kill() {
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
