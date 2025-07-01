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

#include <pthread.h>
#include <thread>

namespace lsan {
/**
 * Structures information about a particular thread.
 */
class ThreadInfo {
    /** The internal thread counter.             */
    static unsigned long threadId;

    /** The number of the thread.                */
    unsigned long number;
    /** The stack size.                          */
    std::size_t stackSize;
    /** The @c std::thread::id of the thread.    */
    std::thread::id id;
    /** The @c pthread_t id of the thread.       */
    pthread_t thread;
    /** The pointer to the stack begin or top.   */
    void* stackTop;
#ifdef __linux__
    /** Whether to consider this thread.         */
    bool dead = false;
    /** The current stack pointer of the thread. */
    void* sp = nullptr;
#endif

public:
    /**
     * Constructs an information structure for a thread.
     *
     * @param stackSize the stack size of the thread
     * @param stackTop the beginning pointer of the thread's stack
     * @param number the number (id) of the thread
     * @param id the C++ thread id
     * @param thread the POSIX thread id
     */
    constexpr inline ThreadInfo(const std::size_t stackSize,
                                void* stackTop = __builtin_frame_address(0),
                                const unsigned long number = createThreadId(),
                                const std::thread::id& id = std::this_thread::get_id(),
                                const pthread_t& thread = pthread_self()):
        number(number), stackSize(stackSize), id(id), thread(thread), stackTop(stackTop) {}

    /**
     * Returns the number of the thread.
     *
     * @return the number of the thread
     */
    constexpr inline auto getNumber() const -> unsigned long {
        return number;
    }

    /**
     * Returns the POSIX thread id.
     *
     * @return the POSIX thread id
     */
    constexpr inline auto getThread() const -> pthread_t {
        return thread;
    }

    /**
     * Returns the @c std::thread::id of the represented thread.
     *
     * @return the C++ thread id
     */
    constexpr inline auto getId() const -> const std::thread::id& {
        return id;
    }

    /**
     * Returns the stack size of the thread.
     *
     * @return the stack size
     */
    constexpr inline auto getStackSize() const -> std::size_t {
        return stackSize;
    }

    /**
     * Returns the stack top.
     *
     * @return the top of the stack
     */
    constexpr inline auto getStackTop() const -> void* {
#ifdef __APPLE__
        return stackTop;
#else
        return reinterpret_cast<void*>(uintptr_t(stackTop) + stackSize);
#endif
    }

#ifdef __linux__
    /**
     * Returns whether the represented thread is still existent.
     *
     * @return whether to consider this thread information
     */
    constexpr inline auto isDead() const -> bool {
        return dead;
    }

    /**
     * Marks this thread information as outdated.
     */
    constexpr inline void kill() {
        dead = true;
    }

    /**
     * Sets the stack pointer for the represented thread.
     *
     * @param sp the stack pointer
     */
    void setSP(void* sp);

    /**
     * Returns the current stack pointer of the represented thread as set with
     * the method @code setSP(void*)@endcode.
     *
     * @return the current stack pointer
     */
    auto getSP() const -> void*;
#endif

    inline auto operator==(const ThreadInfo& other) const -> bool {
        return id == other.id;
    }

    /**
     * Generates and returns a new thread number.
     *
     * @return the newly generated thread number
     */
    static inline auto createThreadId() -> unsigned long {
        return ++threadId;
    }
};
}

#endif /* ThreadInfo_hpp */
