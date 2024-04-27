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

#include "../lsanMisc.hpp"

REPLACE(void, exit)(int code) noexcept {
    real::exit(code);
}

REPLACE(auto, pthread_key_create)(pthread_key_t* key, void (*func)(void*)) noexcept -> int {
    auto toReturn = real::pthread_key_create(key, func); // TODO: Nonnull check
    auto& keys = lsan::getInstance().keys;
    const auto& it = std::find(keys.cbegin(), keys.cend(), *key);
    if (it == keys.cend()) {
        keys.push_back(*key);
    }
    return toReturn;
}

REPLACE(auto, pthread_key_delete)(pthread_key_t key) noexcept -> int {
    auto& keys = lsan::getInstance().keys;
    const auto& it = std::find(keys.cbegin(), keys.cend(), key);
    if (it == keys.cend()) {
        // TODO: Deleting inexistent key
    } else {
        keys.erase(it);
    }
    return real::pthread_key_delete(key);
}
