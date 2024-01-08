/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2022 - 2024  mhahnFr and contributors
 *
 * This file is part of the LeakSanitizer. This library is free software:
 * you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this library, see the file LICENSE.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <iostream>

#include "wrap_malloc.hpp"

#include "interpose.hpp"
#include "realAlloc.hpp"
#include "../LeakSani.hpp"
#include "../formatter.hpp"
#include "../lsanMisc.hpp"
#include "../callstacks/callstackHelper.hpp"
#include "../crashWarner/crash.hpp"
#include "../crashWarner/warn.hpp"
#include "../initialization/init.hpp"

#include "../../include/lsan_stats.h"
#include "../../include/lsan_internals.h"

#ifdef __linux__
auto operator new(std::size_t size) -> void * {
    if (size == 0) {
        size = 1;
    }
    
    return malloc(size);
}
#endif /* __linux__ */

namespace lsan {
auto __wrap_malloc(std::size_t size, const char * file, int line) -> void * {
    auto ret = real::malloc(size);
    
    if (ret != nullptr && inited) {
        std::lock_guard lock(getInstance().getMutex());
        if (!getIgnoreMalloc()) {
            setIgnoreMalloc(true);
            if (__lsan_zeroAllocation && size == 0) {
                warn("Implementation-defined allocation of size 0", file, line, __builtin_return_address(0));
            }
            getInstance().addMalloc(MallocInfo(ret, size, file, line, __builtin_return_address(0)));
            setIgnoreMalloc(false);
        }
    }
    return ret;
}

auto __wrap_calloc(std::size_t objectSize, std::size_t count, const char * file, int line) -> void * {
    auto ret = real::calloc(objectSize, count);
    
    if (ret != nullptr && inited) {
        std::lock_guard lock(getInstance().getMutex());
        if (!getIgnoreMalloc()) {
            setIgnoreMalloc(true);
            if (__lsan_zeroAllocation && objectSize * count == 0) {
                warn("Implementation-defined allocation of size 0", file, line, __builtin_return_address(0));
            }
            getInstance().addMalloc(MallocInfo(ret, objectSize * count, file, line, __builtin_return_address(0)));
            setIgnoreMalloc(false);
        }
    }
    return ret;
}

auto __wrap_realloc(void * pointer, std::size_t size, const char * file, int line) -> void * {
    if (!inited) return real::realloc(pointer, size);
    
    std::lock_guard lock(getInstance().getMutex());
    
    auto ignored = getIgnoreMalloc();
    if (!ignored) {
        setIgnoreMalloc(true);
    }
    void * ptr = real::realloc(pointer, size);
    if (!ignored) {
        if (ptr != nullptr) {
            if (pointer != ptr) {
                if (pointer != nullptr) {
                    getInstance().removeMalloc(pointer, __builtin_return_address(0));
                }
                getInstance().addMalloc(MallocInfo(ptr, size, file, line, __builtin_return_address(0)));
            } else {
                getInstance().changeMalloc(MallocInfo(ptr, size, file, line, __builtin_return_address(0)));
            }
        }
        setIgnoreMalloc(false);
    }
    return ptr;
}

void __wrap_free(void * pointer, const char * file, int line) {
    if (inited) {
        std::lock_guard lock(getInstance().getMutex());
        
        if (!getIgnoreMalloc()) {
            setIgnoreMalloc(true);
            if (pointer == nullptr && __lsan_freeNull) {
                warn("Free of NULL", file, line, __builtin_return_address(0));
            }
            auto [removed, record] = getInstance().removeMalloc(pointer, __builtin_return_address(0));
            if (__lsan_invalidFree && !removed) {
                if (__lsan_invalidCrash) {
                    crash("Invalid free", record, __builtin_return_address(0));
                } else {
                    warn("Invalid free", record, __builtin_return_address(0));
                }
            }
            setIgnoreMalloc(false);
        }
    }
    real::free(pointer);
}

[[ noreturn ]] void __wrap_exit(int code, const char * file, int line) {
    using formatter::Style;
    
    setIgnoreMalloc(true);
    auto & out = getOutputStream();
    out << std::endl
        << formatter::format<Style::GREEN>("Exiting") << " at "
        << formatter::get<Style::UNDERLINED> << file << ":" << line << formatter::clear<Style::UNDERLINED>
        << std::endl;
    
    if (__lsan_printExitPoint) {
        callstackHelper::format(lcs::callstack(__builtin_return_address(0)), out);
        out << std::endl;
    }
    
    out << std::endl << getInstance()
        << std::endl << printInformation;
    internalCleanUp();
    _Exit(code);
}
}

#ifndef __linux__
namespace lsan {
#endif /* !__linux__ */

auto malloc(std::size_t size) -> void * {
    auto ptr = lsan::real::malloc(size);
    
    if (ptr != nullptr && lsan::inited) {
        std::lock_guard lock(lsan::getInstance().getMutex());
        if (!lsan::getIgnoreMalloc()) {
            lsan::setIgnoreMalloc(true);
            if (__lsan_zeroAllocation && size == 0) {
                lsan::warn("Implementation-defined allocation of size 0", __builtin_return_address(0));
            }
            lsan::getInstance().addMalloc(lsan::MallocInfo(ptr, size, __builtin_return_address(0)));
            lsan::setIgnoreMalloc(false);
        }
    }
    return ptr;
}

auto calloc(std::size_t objectSize, std::size_t count) -> void * { // TODO: What if calloc malloc's?
    auto ptr = lsan::real::calloc(objectSize, count);
    
    if (ptr != nullptr && lsan::inited) {
        std::lock_guard lock(lsan::getInstance().getMutex());
        
        if (!lsan::getIgnoreMalloc()) {
            lsan::setIgnoreMalloc(true);
            if (__lsan_zeroAllocation && objectSize * count == 0) {
                lsan::warn("Implementation-defined allocation of size 0", __builtin_return_address(0));
            }
            lsan::getInstance().addMalloc(lsan::MallocInfo(ptr, objectSize * count, __builtin_return_address(0)));
            lsan::setIgnoreMalloc(false);
        }
    }
    return ptr;
}

auto realloc(void * pointer, std::size_t size) -> void * {
    if (!lsan::inited) return lsan::real::realloc(pointer, size);
    
    std::lock_guard lock(lsan::getInstance().getMutex());
    
    auto ignored = lsan::getIgnoreMalloc();
    if (!ignored) {
        lsan::setIgnoreMalloc(true);
    }
    void * ptr = lsan::real::realloc(pointer, size);
    if (!ignored) {
        if (ptr != nullptr) {
            if (pointer != ptr) {
                if (pointer != nullptr) {
                    lsan::getInstance().removeMalloc(pointer, __builtin_return_address(0));
                }
                lsan::getInstance().addMalloc(lsan::MallocInfo(ptr, size, __builtin_return_address(0)));
            } else {
                lsan::getInstance().changeMalloc(lsan::MallocInfo(ptr, size, __builtin_return_address(0)));
            }
        }
        lsan::setIgnoreMalloc(false);
    }
    return ptr;
}

void free(void * pointer) {
    if (lsan::inited) {
        std::lock_guard lock(lsan::getInstance().getMutex());
        
        if (!lsan::getIgnoreMalloc()) {
            lsan::setIgnoreMalloc(true);
            if (pointer == nullptr && __lsan_freeNull) {
                lsan::warn("Free of NULL", __builtin_return_address(0));
            }
            auto [removed, record] = lsan::getInstance().removeMalloc(pointer, __builtin_return_address(0));
            if (__lsan_invalidFree && !removed) {
                if (__lsan_invalidCrash) {
                    lsan::crash("Invalid free", record, __builtin_return_address(0));
                } else {
                    lsan::warn("Invalid free", record, __builtin_return_address(0));
                }
            }
            lsan::setIgnoreMalloc(false);
        }
    }
    lsan::real::free(pointer);
}

#ifndef __linux__
} /* namespace lsan */

INTERPOSE(lsan::malloc,  malloc);
INTERPOSE(lsan::calloc,  calloc);
INTERPOSE(lsan::realloc, realloc);
INTERPOSE(lsan::free,    free);

#endif /* !__linux__ */
