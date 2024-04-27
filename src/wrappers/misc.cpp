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

#include "interpose.hpp"
#include "misc.hpp"
#include "miscReal.hpp"

#include "../lsanMisc.hpp"

namespace lsan {
void __lsan_exit(int code) {
    real::exit(code);
}

auto __lsan_pthread_key_create(pthread_key_t* key, void (*func)(void*)) -> int {
    auto toReturn = real::pthread_key_create(key, func); // TODO: Nonnull check
    auto& keys = lsan::getInstance().keys;
    const auto& it = std::find(keys.cbegin(), keys.cend(), *key);
    if (it == keys.cend()) {
        keys.push_back(*key);
    }
    return toReturn;
}

auto __lsan_pthread_key_delete(pthread_key_t key) -> int {
    auto& keys = lsan::getInstance().keys;
    const auto& it = std::find(keys.cbegin(), keys.cend(), key);
    if (it == keys.cend()) {
        // TODO: Deleting inexistent key
    } else {
        keys.erase(it);
    }
    return real::pthread_key_delete(key);
}

#ifdef __linux__
extern "C" {
void exit(int) __attribute__((weak, alias("__lsan_exit")));
int pthread_key_create(pthread_key_t*, void (*)(void*)) __attribute__((weak, alias("__lsan_pthread_key_create")));
int pthread_key_delete(pthread_key_t) __attribute__((weak, alias("__lsan_pthread_key_delete")));
}
#else
INTERPOSE(lsan::__lsan_exit, exit);
INTERPOSE(lsan::__lsan_pthread_key_create, pthread_key_create);
INTERPOSE(lsan::__lsan_pthread_key_delete, pthread_key_delete);
#endif
}
