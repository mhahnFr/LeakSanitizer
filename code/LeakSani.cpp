/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2022 - 2023  mhahnFr and contributors
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

#include <dlfcn.h>

#include <algorithm>
#include <iostream>

#include "LeakSani.hpp"

#include "Formatter.hpp"
#include "bytePrinter.hpp"
#include "signalHandlers.hpp"

#include "../include/lsan_internals.h"
#include "../include/lsan_stats.h"

#ifdef __GLIBC__
extern "C" void * __libc_malloc (size_t);
extern "C" void * __libc_calloc (size_t, size_t);
extern "C" void * __libc_realloc(void *, size_t);
extern "C" void   __libc_free   (void *);

void * (*LSan::malloc) (size_t)         = __libc_malloc;
void * (*LSan::calloc) (size_t, size_t) = __libc_calloc;
void * (*LSan::realloc)(void *, size_t) = __libc_realloc;
void   (*LSan::free)   (void *)         = __libc_free;

#else
void * (*LSan::malloc) (size_t)         = reinterpret_cast<void * (*)(size_t)>        (dlsym(RTLD_NEXT, "malloc"));
void * (*LSan::calloc) (size_t, size_t) = reinterpret_cast<void * (*)(size_t, size_t)>(dlsym(RTLD_NEXT, "calloc"));
void * (*LSan::realloc)(void *, size_t) = reinterpret_cast<void * (*)(void *, size_t)>(dlsym(RTLD_NEXT, "realloc"));
void   (*LSan::free)   (void *)         = reinterpret_cast<void   (*)(void *)>        (dlsym(RTLD_NEXT, "free"));

#endif /* __GLIBC__ */

void (*LSan::exit)(int) = _Exit;

LSan & LSan::getInstance() {
    static LSan * instance = new LSan();
    return *instance;
}

LSan::LSan() {
    atexit(reinterpret_cast<void (*)()>(__exit_hook));
    struct sigaction s{};
    s.sa_sigaction = crashHandler;
    sigaction(SIGSEGV, &s, nullptr);
    sigaction(SIGBUS, &s, nullptr);
    signal(SIGUSR1, statsSignal);
    signal(SIGUSR2, callstackSignal);
}

auto LSan::removeMalloc(const void * pointer) -> MallocInfoRemoved {
    std::lock_guard lock(infoMutex);
    
    auto it = infos.find(pointer);
    if (it == infos.end()) {
        return MallocInfoRemoved(false, std::nullopt);
    } else if (it->second.isDeleted()) {
        return MallocInfoRemoved(false, it->second);
    }
    if (__lsan_statsActive) {
        stats -= it->second;
        it->second.setDeleted(true);
    } else {
        infos.erase(it);
    }
    return MallocInfoRemoved(true, std::nullopt);
}

auto LSan::changeMalloc(const MallocInfo & info) -> bool {
    std::lock_guard lock(infoMutex);
    
    auto it = infos.find(info.getPointer());
    if (it == infos.end()) {
        return false;
    }
    if (__lsan_statsActive) {
        if (it->second.getPointer() != info.getPointer()) {
            stats -= it->second;
            stats += info;
        } else {
            stats.replaceMalloc(it->second.getSize(), info.getSize());
        }
        it->second.setDeleted(true);
    }
    infos.insert_or_assign(info.getPointer(), info);
    return true;
}

void LSan::addMalloc(MallocInfo && info) {
    std::lock_guard lock(infoMutex);
    
    if (__lsan_statsActive) {
        stats += info;
    }
    
    infos.insert_or_assign(info.getPointer(), info);
}

auto LSan::getLocalIgnoreMalloc() -> bool & {
    static thread_local bool ignoreMalloc = false;
    return ignoreMalloc;
}

auto LSan::getTotalAllocatedBytes() -> std::size_t {
    std::lock_guard lock(infoMutex);
    
    std::size_t ret = 0;
    std::for_each(infos.cbegin(), infos.cend(), [&ret] (auto & elem) {
        ret += elem.second.getSize();
    });
    return ret;
}

auto LSan::getLeakCount() -> std::size_t {
    std::lock_guard lock(infoMutex);
    
    if (__lsan_statsActive) {
        return static_cast<std::size_t>(std::count_if(infos.cbegin(), infos.cend(), [] (auto & elem) -> bool {
            return !elem.second.isDeleted();
        }));
    } else {
        return infos.size();
    }
}

auto LSan::getTotalLeakedBytes() -> std::size_t {
    std::lock_guard lock(infoMutex);
    
    std::size_t ret = 0;
    for (const auto & elem : infos) {
        if (!elem.second.isDeleted()) {
            ret += elem.second.getSize();
        }
    }
    return ret;
}

void LSan::__exit_hook() {
    using Formatter::Style;
    
    setIgnoreMalloc(true);
    std::ostream & out = __lsan_printCout ? std::cout : std::cerr;
    out << std::endl
        << Formatter::get(Style::GREEN) << "Exiting" << Formatter::clear(Style::GREEN)
        << std::endl << std::endl
        << getInstance() << std::endl;
    if (__lsan_printStatsOnExit) {
        __lsan_printStats();
    }
    printInformations();
    internalCleanUp();
}

void internalCleanUp() {
    delete &LSan::getInstance();
}

void LSan::printInformations() {
    using Formatter::Style;
    
    std::ostream & out = __lsan_printCout ? std::cout : std::cerr;
    out << "Report by " << Formatter::get(Style::BOLD) << "LeakSanitizer " << Formatter::clear(Style::BOLD)
        << Formatter::get(Style::ITALIC) << VERSION << Formatter::clear(Style::ITALIC)
        << std::endl << std::endl;
    if (__lsan_printLicense) { printLicense(); }
    if (__lsan_printWebsite) { printWebsite(); }
}

void LSan::printLicense() {
    std::ostream & out = __lsan_printCout ? std::cout : std::cerr;
    out << "Copyright (C) 2022 - 2023 mhahnFr and contributors" << std::endl
        << "Licensed under the terms of the GPL 3.0."           << std::endl
        << std::endl;
}

void LSan::printWebsite() {
    using Formatter::Style;
    
    std::ostream & out = __lsan_printCout ? std::cout : std::cerr;
    out << Formatter::get(Style::ITALIC)
        << "For more information, visit "
        << Formatter::get(Style::UNDERLINED)
        << "github.com/mhahnFr/LeakSanitizer"
        << Formatter::clear(Style::UNDERLINED) << Formatter::clear(Style::ITALIC)
        << std::endl << std::endl;
}

std::ostream & operator<<(std::ostream & stream, LSan & self) {
    using Formatter::Style;
    
    std::lock_guard lock(self.infoMutex);
    
    if (!self.infos.empty()) {
        stream << Formatter::get(Style::ITALIC);
        const std::size_t totalLeaks = self.getLeakCount();
        stream << totalLeaks << " leaks total, " << bytesToString(self.getTotalLeakedBytes()) << " total" << std::endl << std::endl;
        std::size_t i = 0;
        for (const auto & leak : self.infos) {
            if (!leak.second.isDeleted()) {
                stream << leak.second << std::endl;
                if (++i == __lsan_leakCount) {
                    if (self.callstackSizeExceeded) {
                        stream << "Hint:" << Formatter::get(Style::GREYED)
                               << Formatter::get(Style::ITALIC) << " to see longer callstacks, increase the value of "
                               << Formatter::clear(Style::ITALIC) << Formatter::clear(Style::GREYED) << "LSAN_CALLSTACK_SIZE" << Formatter::get(Style::GREYED) << " (__lsan_callstackSize)" << Formatter::get(Style::ITALIC)
                               << " (currently " << Formatter::clear(Style::ITALIC) << Formatter::clear(Style::GREYED) << __lsan_callstackSize << Formatter::get(Style::ITALIC) << Formatter::get(Style::GREYED) << ")."
                               << Formatter::clear(Style::ITALIC) << Formatter::clear(Style::GREYED) << std::endl;
                        self.callstackSizeExceeded = false;
                    }
                    stream << std::endl << Formatter::get(Style::UNDERLINED) << Formatter::get(Style::ITALIC)
                           << "And " << totalLeaks - i << " more..." << Formatter::clear(Style::UNDERLINED) << std::endl << std::endl
                           << Formatter::clear(Style::ITALIC) << "Hint:" << Formatter::get(Style::GREYED)
                           << Formatter::get(Style::ITALIC) << " to see more, increase the value of "
                           << Formatter::clear(Style::ITALIC) << Formatter::clear(Style::GREYED) << "LSAN_LEAK_COUNT" << Formatter::get(Style::GREYED) << " (__lsan_leakCount)" << Formatter::get(Style::ITALIC)
                           << " (currently " << Formatter::clear(Style::ITALIC) << Formatter::clear(Style::GREYED) << __lsan_leakCount << Formatter::get(Style::ITALIC) << Formatter::get(Style::GREYED) << ")."
                           << Formatter::clear(Style::ITALIC) << Formatter::clear(Style::GREYED) << std::endl;
                    break;
                }
            }
        }
        stream << Formatter::clear(Style::ITALIC);
    }
    return stream;
}
