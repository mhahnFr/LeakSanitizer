/*
 * LeakSanitizer - A small library showing informations about lost memory.
 *
 * Copyright (C) 2022  mhahnFr and contributors
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
#include "signalHandlers.hpp"
#include "bytePrinter.hpp"
#include "../include/lsan_stats.h"
#include "../include/lsan_internals.h"

bool __lsan_printStatsOnExit = false;

#ifdef __linux__
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

#endif /* __linux__ */

void (*LSan::exit)(int) = _Exit;

Stats * LSan::stats = nullptr;

Stats & LSan::getStats()     { return *LSan::stats;            }
bool    LSan::hasStats()     { return LSan::stats != nullptr;  }
bool    LSan::ignoreMalloc() { return LSan::getIgnoreMalloc(); }

LSan & LSan::getInstance() {
    static LSan * instance = new LSan();
    return *instance;
}

bool & LSan::getIgnoreMalloc() {
    static bool ignore = false;
    return ignore;
}

void LSan::setIgnoreMalloc(bool ignore) {
    LSan::getIgnoreMalloc() = ignore;
}

LSan::LSan() {
    atexit(reinterpret_cast<void (*)()>(__exit_hook));
    struct sigaction s{};
    s.sa_sigaction = crashHandler;
    sigaction(SIGSEGV, &s, nullptr);
    sigaction(SIGBUS, &s, nullptr);
    signal(SIGUSR1, statsSignal);
    signal(SIGUSR2, callstackSignal);
    stats = &realStats;
}

void LSan::addMalloc(const MallocInfo && mInfo) {
    std::lock_guard<std::recursive_mutex> lock(infoMutex);
    realStats += mInfo;
    infos.emplace(mInfo);
}

void LSan::changeMalloc(const MallocInfo & mInfo) {
    std::lock_guard<std::recursive_mutex> lock(infoMutex);
    auto it = infos.find(mInfo);
    if (it == infos.end()) {
        realStats += mInfo;
    } else {
        if (it->getPointer() != mInfo.getPointer()) {
            realStats -= *it;
            realStats += mInfo;
        } else {
            realStats.replaceMalloc(it->getSize(), mInfo.getSize());
        }
        // TODO: Don't replace it everytime!
        infos.erase(it);
        infos.emplace(mInfo);
    }
}

bool LSan::removeMalloc(const MallocInfo & mInfo) {
    std::lock_guard<std::recursive_mutex> lock(infoMutex);
    auto it = infos.find(mInfo);
    if (it == infos.end()) {
        return false;
    }
    realStats -= *it;
    infos.erase(it);
    return true;
}

size_t LSan::getTotalAllocatedBytes() {
    std::lock_guard<std::recursive_mutex> lock(infoMutex);
    size_t ret = 0;
    std::for_each(infos.cbegin(), infos.cend(), [&ret] (auto & elem) {
        ret += elem.getSize();
    });
    return ret;
}

void LSan::__exit_hook() {
    setIgnoreMalloc(true);
	if (__lsan_printCout) {
        std::cout << std::endl
                  << "\033[32mExiting\033[39m" << std::endl << std::endl
                  << getInstance() << std::endl;
	} else {
		std::cerr << std::endl
				  << "\033[32mExiting\033[39m" << std::endl << std::endl
				  << getInstance() << std::endl;
	}
    if (__lsan_printStatsOnExit) {
        __lsan_printStats();
    }
    internalCleanUp();
}

void internalCleanUp() {
    delete &LSan::getInstance();
}

std::ostream & operator<<(std::ostream & stream, LSan & self) {
    std::lock_guard<std::recursive_mutex> lock(self.infoMutex);
    if (!self.infos.empty()) {
        stream << "\033[3m";
        stream << self.infos.size() << " leaks total, " << bytesToString(self.getTotalAllocatedBytes()) << " total" << std::endl << std::endl;
        for (const auto & leak : self.infos) {
            stream << leak << std::endl;
        }
        stream << "\033[23m";
    }
    return stream;
}
