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

    unsigned long number = ++threadId;
    std::thread::id id;
    pthread_t thread;

public:
    std::optional<void*> beginFrameAddress;

    inline ThreadInfo(const std::thread::id& id = std::this_thread::get_id(),
                      const pthread_t& thread = pthread_self(),
                      const std::optional<void*>& beginFrameAddress = __builtin_frame_address(0)):
        id(id), thread(thread), beginFrameAddress(beginFrameAddress) {}

    constexpr inline auto getNumber() const -> unsigned long {
        return number;
    }

    constexpr inline auto getThread() const -> pthread_t {
        return thread;
    }

    constexpr inline auto getId() const -> const std::thread::id& {
        return id;
    }
};
}

#endif /* ThreadInfo_hpp */
