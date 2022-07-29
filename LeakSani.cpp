//
//  LSan.cpp
//  LeakSanitizer
//
//  Created by Manuel Hahn on 16.07.22.
//  Copyright © 2022 mhahnFr. All rights reserved.
//

#include <dlfcn.h>
#include <algorithm>
#include <iostream>
#include "LeakSani.hpp"

LSan * LSan::instance = new LSan();

//void * (*LSan::malloc)(size_t) = reinterpret_cast<void * (*)(size_t)>(dlsym(RTLD_NEXT, "malloc"));
//void   (*LSan::free)(void *)   = reinterpret_cast<void (*)(void *)>  (dlsym(RTLD_NEXT, "free"));
//void   (*LSan::exit)(int)      = reinterpret_cast<void (*)(int)>     (dlsym(RTLD_NEXT, "exit"));

LSan & LSan::getInstance() {
    return *instance;
}

LSan::LSan() {
    malloc = ::malloc;
    free   = ::free;
    exit   = _Exit;
    atexit(reinterpret_cast<void(*)()>(__exit_hook));
}

void LSan::addMalloc(const MallocInfo && mInfo) {
    infos.push_back(mInfo);
}

bool LSan::removeMalloc(const MallocInfo & mInfo) {
    if (std::find(infos.cbegin(), infos.cend(), mInfo) == infos.cend()) {
        return false;
    }
    infos.remove(mInfo);
    return true;
}

size_t LSan::getTotalAllocatedBytes() const {
    size_t ret = 0;
    std::for_each(infos.cbegin(), infos.cend(), [&ret] (auto & elem) {
        ret += elem.getSize();
    });
    return ret;
}

void LSan::__exit_hook() {
    std::cout << "\033[32mExiting\033[39m" << std::endl
              << getInstance() << std::endl;
    internalCleanUp();
}

void internalCleanUp() {
    delete LSan::instance;
}

std::ostream & operator<<(std::ostream & stream, const LSan & self) {
    if (!self.infos.empty()) {
        stream << "\033[3m";
        stream << self.infos.size() << " leaks total, " << self.getTotalAllocatedBytes() << " bytes total" << std::endl;
        for (const auto & leak : self.infos) {
            stream << "\033[1;31mLeak\033[22;39m of size " << leak.getSize() << ", allocated at \033[4m" << leak.getCreatedInFile() << ":" << leak.getCreatedOnLine() << "\033[24m" << std::endl;
        }
        stream << "\033[23m";
    }
    return stream;
}
