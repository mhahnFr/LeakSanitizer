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

#ifndef ATracker_hpp
#define ATracker_hpp

#include <map>
#include <mutex>
#include <optional>
#include <utility>

#include "MallocInfo.hpp"

namespace lsan {
class ATracker {
protected:
    std::map<const void* const, MallocInfo> infos;
    std::mutex infoMutex;

public:
    virtual ~ATracker() = default;

    bool ignoreMalloc = false;
    std::recursive_mutex mutex;

    inline void addMalloc(MallocInfo&& info) {
        std::lock_guard lock { infoMutex };
        infos.insert_or_assign(info.pointer, std::move(info));
    }

    virtual auto removeMalloc(void* pointer) -> std::pair<const bool, std::optional<MallocInfo::CRef>> = 0;
    virtual void changeMalloc(MallocInfo&& info) = 0;

    virtual auto maybeRemoveMalloc([[ maybe_unused ]] void* pointer) -> bool {
        return false;
    }

    virtual auto maybeChangeMalloc([[ maybe_unused ]] const MallocInfo& info) -> bool {
        return false;
    }
};
}

#endif /* ATracker_hpp */
