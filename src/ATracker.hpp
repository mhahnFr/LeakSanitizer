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

#include <lsan_internals.h>

#include "MallocInfo.hpp"

namespace lsan {
/**
 * This class represents the base of an allocation tracker.
 */
class ATracker {
protected:
    /** The registered allocations.                                   */
    std::map<const void* const, MallocInfo> infos;
    /** The mutex to manage the access to the registered allocations. */
    std::mutex infoMutex;

    /**
     * Helper function to potentially add the given allocation record
     * to the statistics.
     *
     * @param info the allocation record
     */
    virtual inline void addToStats([[ maybe_unused ]] const MallocInfo& info) {}

public:
    virtual ~ATracker() = default;

    /** Indicates whether allocations should be ignored. */
    bool ignoreMalloc = false;
    /** The mutex to guard allocations in this thread.   */
    std::recursive_mutex mutex;

    /**
     * Registers the given allocation record.
     *
     * @param info the allocation record to be registered
     */
    inline void addMalloc(MallocInfo&& info) {
        std::lock_guard lock { infoMutex };
        if (__lsan_statsActive) {
            addToStats(info);
        }
        infos.insert_or_assign(info.pointer, std::move(info));
    }

    /**
     * Attempts to remove the allocation record for the given pointer.
     *
     * @param pointer the pointer of the actual allocation
     * @return whether the allocation record was removed and the potentially found existing record
     */
    virtual auto removeMalloc(void* pointer) -> std::pair<const bool, std::optional<MallocInfo::CRef>> = 0;

    /**
     * Changes the allocation record already registered to the given one.
     *
     * @param info the new allocation record
     */
    virtual void changeMalloc(MallocInfo&& info) = 0;

    /**
     * Marks this tracker instance as finished, that is, it will ignore all upcoming allocations
     * and upload it's registered allocation records to the main instance.
     */
    virtual void finish() = 0;

    /**
     * @brief Removes the allocation record associated with the given pointer.
     *
     * Does not search in other trackers if no record was found.
     *
     * @param pointer the allocation pointer
     * @return whether a record was found and removed
     */
    virtual auto maybeRemoveMalloc([[ maybe_unused ]] void* pointer) -> bool {
        return false;
    }

    /**
     * @brief Changes the allocation record already registered to the given one.
     *
     * Does not search in other trackers if no record was found.
     *
     * @param info the new allocation record
     * @return whether an allocation record was found and replaced
     */
    virtual auto maybeChangeMalloc([[ maybe_unused ]] const MallocInfo& info) -> bool {
        return false;
    }
};
}

#endif /* ATracker_hpp */
