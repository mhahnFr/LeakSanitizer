/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2024 - 2025  mhahnFr
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

REPLACE(void, exit)(const int code) noexcept(noexcept(::exit(code))) {
    getTracker().withIgnoration(true, [] {
        if (getBehaviour().printExitPoint()) {
            getOutputStream() << maybePrintExitPoint;
        }
    });

    real::exit(code);
}

/*
 * The following function replacement is a hack to convince the linker on Linux
 * to always link with the LeakSanitizer, even in the case no allocation function
 * is called directly.
 *                                                              - mhahnFr
 */
#if defined(__linux__) && !defined(__APPLE__)
extern "C" int __libc_start_main(
        int (*)(int, char**, char**), int, char**, void (*)(void), void (*)(void), void (*)(void), void*);

REPLACE(auto, __libc_start_main)(int (*main)(int, char**, char**),
                                 int    argc, 
                                 char** ubp_av,
                                 void (*init)(void),
                                 void (*fini)(void),
                                 void (*rtld_fini)(void),
                                 void* stack_end) -> int {
    return  real::__libc_start_main(main, argc, ubp_av, init, fini, rtld_fini, stack_end);
}
#endif
