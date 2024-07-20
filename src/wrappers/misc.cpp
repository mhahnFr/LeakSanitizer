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
#include "../crashWarner/crash.hpp"
#include "../crashWarner/warn.hpp"

REPLACE(void, exit)(int code) noexcept(noexcept(::exit(code))) {
    bool ignoreMalloc = false;
    if (inited) {
        ignoreMalloc = getIgnoreMalloc();
        setIgnoreMalloc(true);
    }

    // The following builtin call is necessary to guarantee a frame pointer for the stack analysis.
    // Even though there is no use in the returned address, the exiting stack should not be optimized
    // away - but the other parts of the LeakSanitizer should still be optimized.
    //                                                                              - mhahnFr
    __builtin_frame_address(0);
    getInstance().classifyStackLeaksShallow();
    if (__lsan_printExitPoint) {
        getOutputStream() << maybePrintExitPoint;
    }

    if (inited && !ignoreMalloc) {
        setIgnoreMalloc(false);
    }

    real::exit(code);
}

REPLACE(auto, pthread_key_create)(pthread_key_t* key, void (*func)(void*)) noexcept(noexcept(::pthread_key_create(key, func))) -> int {
    if (key == nullptr) {
        crash("Call to pthread_key_create(pthread_key_t*, void (*)(void*)) with NULL as key");
    }
    auto toReturn = real::pthread_key_create(key, func);
    if (inited) {
        getInstance().addTLSKey(*key);
    }
    return toReturn;
}

REPLACE(auto, pthread_key_delete)(pthread_key_t key) noexcept(noexcept(::pthread_key_delete(key))) -> int {
    if (inited && !getInstance().removeTLSKey(key)) {
        warn("Call to pthread_key_delete(pthread_key_t) with invalid key");
    }
    return real::pthread_key_delete(key);
}
