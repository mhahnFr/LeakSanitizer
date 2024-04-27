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

#ifndef miscReal_hpp
#define miscReal_hpp

#include <cstdlib>
#include <pthread.h>

namespace lsan::real {
// TODO: Linux compatibility
static inline auto pthread_key_create(pthread_key_t* key, void (*func)(void*)) -> int {
    return ::pthread_key_create(key, func);
}

static inline auto pthread_key_delete(pthread_key_t key) -> int {
    return ::pthread_key_delete(key);
}

[[ noreturn ]] static inline void exit(int code) {
    ::exit(code);
}
}

#endif /* miscReal_hpp */
