/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2023  mhahnFr
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

#include "ThreadAllocInfo.hpp"

#include "../LeakSani.hpp"

#include "../../include/lsan_internals.h"

ThreadAllocInfo::ThreadAllocInfo() {
    LSan::getInstance().registerThreadAllocInfo(*this);
}

ThreadAllocInfo::~ThreadAllocInfo() {
    LSan::getInstance().removeThreadAllocInfo(*this);
}

void ThreadAllocInfo::addMalloc(MallocInfo && info) {
    std::lock_guard lock(infosMutex);

    stats += info;
    infos.insert_or_assign(info.getPointer(), info);
}

void ThreadAllocInfo::changeMalloc(const MallocInfo & info) {
    std::lock_guard lock(infosMutex);
    
    auto it = infos.find(info.getPointer());
    if (it == infos.end()) {
        // TODO: Maybe allocated in another thread?
        stats += info;
    } else {
        if (it->second.getPointer() != info.getPointer()) {
            stats -= it->second;
            stats += info;
        } else {
            stats.replaceMalloc(it->second.getSize(), info.getSize());
        }
        if (__lsan_trackMemory) {
            it->second.setDeleted(true);
        }
        infos.insert_or_assign(info.getPointer(), info);
    }
}

auto ThreadAllocInfo::removeMalloc(const void * pointer) -> bool {
    std::lock_guard lock(infosMutex);
    
    auto it = infos.find(pointer);
    
    if (it == infos.end()) {
        // TODO: Maybe allocated in another thread?
        return false;
    } else if (it->second.isDeleted()) {
        return false;
    }
    stats -= it->second;
    if (__lsan_trackMemory) {
        it->second.setDeleted(true);
    } else {
        infos.erase(it);
    }
    return true;
}

void ThreadAllocInfo::lockMutex() const {
    infosMutex.lock();
}

void ThreadAllocInfo::unlockMutex() const {
    infosMutex.unlock();
}
