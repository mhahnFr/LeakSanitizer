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

#include "TLSTracker.hpp"

#include "lsanMisc.hpp"

namespace lsan {
TLSTracker::TLSTracker() {
    getInstance().registerTracker(this);
}

TLSTracker::~TLSTracker() {
    if (finished) return;

    finished = true;

    std::lock_guard lock { mutex };
    ignoreMalloc = true;

    pthread_setspecific(getInstance().saniKey, std::addressof(getInstance()));
    getInstance().deregisterTracker(this);

    std::lock_guard lock1 { infoMutex };
    if (__lsan_invalidFree) {
        for (auto it = infos.cbegin(); it != infos.cend();) {
            if (it->second.deleted) {
                it = infos.erase(it);
            } else {
                ++it;
            }
        }
    }
    getInstance().absorbLeaks(std::move(infos));
}

void TLSTracker::finish() {
    if (finished) return;

    finished = true;

    std::lock_guard lock  { mutex     };
    std::lock_guard lock1 { infoMutex };

    ignoreMalloc = true;

    if (__lsan_invalidFree) {
        for (auto it = infos.cbegin(); it != infos.cend();) {
            if (it->second.deleted) {
                it = infos.erase(it);
            } else {
                ++it;
            }
        }
    }
    getInstance().absorbLeaks(std::move(infos));
    infos = decltype(infos)();
}

auto TLSTracker::maybeRemoveMalloc(void* pointer) -> std::pair<bool, std::optional<MallocInfo::CRef>> {
    std::lock_guard lock { infoMutex };

    const auto& it = infos.find(pointer);
    if (it == infos.end()) {
        return std::make_pair(false, std::nullopt);
    }
    if (it->second.deleted) {
        return std::make_pair(false, std::ref(it->second));
    }
    if (__lsan_invalidFree) {
        it->second.markDeleted();
    } else {
        infos.erase(it);
    }
    return std::make_pair(true, std::nullopt);
}

auto TLSTracker::removeMalloc(void* pointer) -> std::pair<bool, std::optional<MallocInfo::CRef>> {
    const auto& result = maybeRemoveMalloc(pointer);

    if (!result.first) {
        const auto& globalResult = getInstance().removeMalloc(this, pointer);
        if (!globalResult.first) {
            if (globalResult.second && result.second) {
                return globalResult.second->get().isMoreRecent(result.second->get()) ? globalResult : result;
            }
            return globalResult.second ? globalResult : result;
        }
        return globalResult;
    }
    return result;
}

void TLSTracker::changeMalloc(MallocInfo&& info) {
    std::lock_guard lock { infoMutex };

    const auto& it = infos.find(info.pointer);
    if (it == infos.end()) {
        getInstance().changeMalloc(this, std::move(info));
        return;
    }
    infos.insert_or_assign(info.pointer, std::move(info));
}

auto TLSTracker::maybeChangeMalloc(const MallocInfo& info) -> bool {
    std::lock_guard lock { infoMutex };

    const auto& it = infos.find(info.pointer);
    if (it == infos.end()) {
        return false;
    }
    infos.insert_or_assign(info.pointer, std::move(info));
    return true;
}
}
