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

#ifdef __linux__
# include <mutex>
#endif

#include "ThreadInfo.hpp"

namespace lsan {
unsigned long ThreadInfo::threadId = 0;

#ifdef __linux__
/** The mutex used to synchronize the access to the stack pointers. */
static std::mutex mutex;

void ThreadInfo::setSP(void* sp) {
    std::lock_guard lock { mutex };
    ThreadInfo::sp = sp;
}

auto ThreadInfo::getSP() const -> void* {
    std::lock_guard lock { mutex };
    return sp;
}
#endif
}
