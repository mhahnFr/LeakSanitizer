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

bool __lsan_printStatsOnExit = false;

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

Stats * LSan::stats = nullptr;

Stats & LSan::getStats()     { return *LSan::stats;            }
bool    LSan::hasStats()     { return LSan::stats != nullptr;  }
bool    LSan::ignoreMalloc() { return LSan::getIgnoreMalloc(); }

LSan & LSan::getInstance() {
    static LSan * instance = new LSan();
    return *instance;
}

auto LSan::getLocalInstance() -> ThreadAllocInfo & {
    static thread_local ThreadAllocInfo localInfo;
    return localInfo;
}

bool & LSan::getIgnoreMalloc() {
    static thread_local bool ignore = false;
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

void LSan::registerThreadAllocInfo(ThreadAllocInfo::CRef info) {
    std::lock_guard lock(infoMutex);
    
    threadInfos.push_back(info);
}

void LSan::removeThreadAllocInfo(ThreadAllocInfo::Ref info) {
    std::lock_guard lock(infoMutex);
    
    realStats += info.get().getStats();
    infos.merge(info.get().getInfos());
    
    auto it = std::find_if(threadInfos.cbegin(), threadInfos.cend(), [&info] (const auto & elem) {
        return std::addressof(elem.get()) == std::addressof(info.get());
    });
    if (it != threadInfos.end()) {
        threadInfos.erase(it);
    }
}

void LSan::setCallstackSizeExceeded(bool exceeded) { callstackSizeExceeded = exceeded; }

void LSan::addMalloc(MallocInfo && mInfo) {
    std::lock_guard<std::recursive_mutex> lock(infoMutex);
    realStats += mInfo;
    // TODO: Only replace if the new block is bigger than the potential old one.
    infos.insert_or_assign(mInfo.getPointer(), mInfo);
}

void LSan::changeMalloc(const MallocInfo & mInfo) {
    std::lock_guard<std::recursive_mutex> lock(infoMutex);
    auto it = infos.find(mInfo.getPointer());
    if (it == infos.end()) {
        realStats += mInfo;
    } else {
        if (it->second.getPointer() != mInfo.getPointer()) {
            realStats -= it->second;
            realStats += mInfo;
        } else {
            realStats.replaceMalloc(it->second.getSize(), mInfo.getSize());
        }
        if (__lsan_trackMemory) {
            it->second.setDeleted(true);
        }
        infos.insert_or_assign(mInfo.getPointer(), mInfo);
    }
}

bool LSan::removeMalloc(const void * pointer) {
    std::lock_guard<std::recursive_mutex> lock(infoMutex);
    auto it = infos.find(pointer);
    if (it == infos.end() || it->second.isDeleted()) {
        return false;
    }
    realStats -= it->second;
    if (__lsan_trackMemory) {
        it->second.setDeleted(true);
    } else {
        infos.erase(it);
    }
    return true;
}

bool LSan::removeMalloc(const MallocInfo & mInfo) {
    return removeMalloc(mInfo.getPointer());
}

size_t LSan::getTotalAllocatedBytes() {
    std::lock_guard<std::recursive_mutex> lock(infoMutex);
    size_t ret = 0;
    std::for_each(infos.cbegin(), infos.cend(), [&ret] (auto & elem) {
        ret += elem.second.getSize();
    });
    return ret;
}

size_t LSan::getLeakCount() {
    std::lock_guard<std::recursive_mutex> lock(infoMutex);
    if (__lsan_trackMemory) {
        return static_cast<size_t>(std::count_if(infos.cbegin(), infos.cend(), [] (auto & elem) -> bool {
            return !elem.second.isDeleted();
        }));
    } else {
        return infos.size();
    }
}

size_t LSan::getTotalLeakedBytes() {
    std::lock_guard<std::recursive_mutex> lock(infoMutex);
    size_t ret = 0;
    for (const auto & elem : infos) {
        if (!elem.second.isDeleted()) {
            ret += elem.second.getSize();
        }
    }
    return ret;
}

const std::map<const void * const, MallocInfo> & LSan::getInfos() const { return infos; }
std::recursive_mutex &                           LSan::getInfoMutex()   { return infoMutex; }

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

void LSan::printInformations(){
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
        << std::endl
        << "This library is free software: you can redistribute it and/or modify it under"    << std::endl
        << "the terms of the GNU General Public License as published by the"                  << std::endl
        << "Free Software Foundation, either version 3 of the License, or (at your option)"   << std::endl
        << "any later version."                                                               << std::endl
        << std::endl
        << "You should have received a copy of the GNU General Public License along with"     << std::endl
        << "this library, see the file LICENSE. If not, see <https://www.gnu.org/licenses/>." << std::endl
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
    std::lock_guard<std::recursive_mutex> lock(self.infoMutex);
    if (!self.infos.empty()) {
        stream << Formatter::get(Style::ITALIC);
        const size_t totalLeaks = self.getLeakCount();
        stream << totalLeaks << " leaks total, " << bytesToString(self.getTotalLeakedBytes()) << " total" << std::endl << std::endl;
        size_t i = 0;
        for (const auto & leak : self.infos) {
            if (!leak.second.isDeleted()) {
                stream << leak.second << std::endl;
                if (++i == __lsan_leakCount) {
                    if (self.callstackSizeExceeded) {
                        stream << Formatter::get(Style::GREYED)
                               << "Hint:" << Formatter::get(Style::ITALIC) << " to see longer callstacks, increase the value of "
                               << Formatter::clear(Style::ITALIC) << "__lsan_callstackSize" << Formatter::get(Style::ITALIC)
                               << " (currently " << Formatter::clear(Style::ITALIC) << __lsan_callstackSize << Formatter::get(Style::ITALIC) << ")."
                               << Formatter::clear(Style::ITALIC) << Formatter::clear(Style::GREYED) << std::endl;
                        self.callstackSizeExceeded = false;
                    }
                    stream << std::endl << Formatter::get(Style::UNDERLINED) << Formatter::get(Style::ITALIC)
                           << "And " << totalLeaks - i << " more..." << Formatter::clear(Style::UNDERLINED) << std::endl
                           << Formatter::clear(Style::ITALIC) << Formatter::get(Style::GREYED)
                           << "Hint:" << Formatter::get(Style::ITALIC) << " to see more, increase the value of "
                           << Formatter::clear(Style::ITALIC) << "__lsan_leakCount" << Formatter::get(Style::ITALIC)
                           << " (currently " << Formatter::clear(Style::ITALIC) << __lsan_leakCount << Formatter::get(Style::ITALIC) << ")."
                           << Formatter::clear(Style::ITALIC) << Formatter::clear(Style::GREYED) << std::endl;
                    break;
                }
            }
        }
        stream << Formatter::clear(Style::ITALIC);
    }
    return stream;
}
