/*
 * LeakSanitizer - A small library showing informations about lost memory.
 *
 * Copyright (C) 2022  mhahnFr
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see https://www.gnu.org/licenses/.
 */

#include <dlfcn.h>
#include <algorithm>
#include <iostream>
#include "LeakSani.hpp"
#include "signalHandlers.hpp"

extern "C" void * __libc_malloc(size_t);
extern "C" void   __libc_free(void *);
void * (*LSan::malloc)(size_t) = __libc_malloc;//reinterpret_cast<void * (*)(size_t)>(dlsym(RTLD_NEXT, "malloc"));
void   (*LSan::free)  (void *) = __libc_free;//reinterpret_cast<void   (*)(void *)>(dlsym(RTLD_NEXT, "free"));
void   (*LSan::exit)  (int)    = _Exit;

LSan * LSan::instance = nullptr;

bool LSan::omitMalloc = true,
     LSan::omitFree   = true;

LSan & LSan::getInstance() {
    if (instance == nullptr) {
        instance = new LSan();
    }
    return *instance;
}

bool LSan::isNull()       { return instance == nullptr; }
bool LSan::ignoreMalloc() { return omitMalloc;          }
bool LSan::ignoreFree()   { return omitFree;            }
void LSan::setIgnoreMalloc(bool ignore) {
    omitMalloc = omitFree = ignore;
}

LSan::LSan() {
    atexit(reinterpret_cast<void(*)()>(__exit_hook));
    struct sigaction s{};
    s.sa_sigaction = crashHandler;
    sigaction(SIGSEGV, &s, nullptr);
    sigaction(SIGBUS, &s, nullptr);
    omitMalloc = omitFree = false;
}

void LSan::addMalloc(const MallocInfo && mInfo) {
    std::lock_guard<std::recursive_mutex> lock(infoMutex);
    omitFree = omitMalloc = true;
    infos.emplace(mInfo);
    omitFree = omitMalloc = false;
}

bool LSan::removeMalloc(const MallocInfo & mInfo) {
    std::lock_guard<std::recursive_mutex> lock(infoMutex);
    omitFree = omitMalloc = true;
    auto it = infos.find(mInfo);
    if (it == infos.end()) {
        omitFree = omitMalloc = false;
        return false;
    }
    infos.erase(it);
    omitFree = omitMalloc = false;
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
    omitFree = omitMalloc = true;
    std::cout << std::endl
              << "\033[32mExiting\033[39m" << std::endl << std::endl
              << getInstance() << std::endl;
    internalCleanUp();
}

void internalCleanUp() {
    delete LSan::instance;
}

std::ostream & operator<<(std::ostream & stream, LSan & self) {
    std::lock_guard<std::recursive_mutex> lock(self.infoMutex);
    LSan::omitFree = LSan::omitMalloc = true;
    if (!self.infos.empty()) {
        stream << "\033[3m";
        stream << self.infos.size() << " leaks total, " << self.getTotalAllocatedBytes() << " bytes total" << std::endl << std::endl;
        for (const auto & leak : self.infos) {
            stream << leak << std::endl;
        }
        stream << "\033[23m";
    }
    LSan::omitFree = LSan::omitMalloc = false;
    return stream;
}
