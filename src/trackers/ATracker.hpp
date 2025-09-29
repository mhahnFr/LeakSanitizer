/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2024 - 2025  mhahnFr
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

#include "../MallocInfo.hpp"
#include "../allocators/PoolAllocator.hpp"

namespace lsan::trackers {
/**
 * This class represents the base of an allocation tracker.
 */
class ATracker {
protected:
    /** A @c std::map using the @c PoolAllocator.                     */
    template<
        typename Key,
        typename T,
        typename Compare = std::less<Key>,
        typename Allocator = PoolAllocator<std::pair<const Key, T>>
    > using PoolMap = std::map<Key, T, Compare, Allocator>;
    /** The registered allocations.                                   */
    PoolMap<const void* const, MallocInfo> infos;
    /** The mutex to manage the access to the registered allocations. */
    std::mutex infoMutex;

    /**
     * Helper function to potentially add the given allocation record
     * to the statistics.
     *
     * @param info the allocation record
     */
    virtual inline void maybeAddToStats([[ maybe_unused ]] const MallocInfo& info) {}

public:
    virtual ~ATracker() = default;

    /** Indicates whether allocations should be ignored.                 */
    bool ignoreMalloc = false;
    /** Indicates whether this tracker instance needs to be deallocated. */
    bool needsDealloc = false;
    /** The mutex to guard allocations in this thread.                   */
    std::recursive_mutex mutex;

    /**
     * Registers the given allocation record.
     *
     * @param info the allocation record to be registered
     */
    inline void addMalloc(MallocInfo&& info) {
        std::lock_guard lock { infoMutex };
        
        maybeAddToStats(info);
        infos.insert_or_assign(info.getPointer(), std::move(info));
    }

    /**
     * Calls the given function while ignoring allocations caused by that function.
     *
     * @tparam F function signature of the function to be called
     * @tparam Args argument types passed to the given function
     * @param ignore whether to ignore allocations while calling the given function
     * @param func the function to be called
     * @param args the arguments to be forwarded
     * @return the result of the given function
     */
    template<typename F, typename ...Args>
    inline auto withIgnorationResult(const bool ignore, F&& func, Args&& ...args) -> std::invoke_result_t<F, Args...> {
        static_assert(std::is_invocable_v<F, Args...>,
                      "Given function must be callable with the given arguments");
        static_assert(!std::is_same_v<std::remove_cv_t<std::invoke_result_t<F, Args...>>, void>,
                      "Given function must not return void");

        std::lock_guard lock { mutex };
        const auto ignored = ignoreMalloc;
        ignoreMalloc = ignore;
        const auto& toReturn = std::invoke(std::forward<F&&>(func), std::forward<Args&&>(args)...);
        ignoreMalloc = ignored;
        return toReturn;
    }

    /**
     * Calls the given function while ignoring allocations caused by that function.
     *
     * @tparam F function signature of the function to be called
     * @tparam Args argument types passed to the given function
     * @param ignore whether to ignore allocations while calling the given function
     * @param func the function to be called
     * @param args the arguments to be forwarded
     */
    template<typename F, typename ...Args>
    inline void withIgnoration(const bool ignore, F&& func, Args&& ...args) {
        static_assert(std::is_invocable_v<F, Args...>,
                      "Given function must be callable with the given arguments");

        std::lock_guard lock { mutex };
        const auto ignored = ignoreMalloc;
        ignoreMalloc = ignore;
        std::invoke(std::forward<F&&>(func), std::forward<Args&&>(args)...);
        ignoreMalloc = ignored;
    }

    /**
     * Attempts to remove the allocation record for the given pointer.
     *
     * @param pointer the pointer of the actual allocation
     * @return whether the allocation record was removed and the potentially
     * found existing record
     */
    virtual auto removeMalloc(void* pointer) -> std::pair<bool, std::optional<MallocInfo::CRef>> = 0;

    /**
     * Changes the allocation record already registered to the given one.
     *
     * @param info the new allocation record
     */
    virtual void changeMalloc(MallocInfo&& info) = 0;

    /**
     * Marks this tracker instance as finished, that is, it will ignore all
     * upcoming allocations and upload its registered allocation records to the
     * main instance.
     */
    virtual void finish() = 0;

    /**
     * @brief Removes the allocation record associated with the given pointer
     * in this instance if it was found.
     *
     * Does not call out to the global instance.
     *
     * @param pointer the pointer whose allocation record to be removed
     * @return whether the record was removed and the potentially already as
     * deleted marked record
     */
    virtual auto maybeRemoveMalloc(void* pointer) -> std::pair<bool, std::optional<MallocInfo::CRef>> = 0;

    /**
     * @brief Changes the allocation record already registered to the given one.
     *
     * Does not search in other trackers if no record was found.
     *
     * @param info the new allocation record
     * @return whether an allocation record was found and replaced
     */
    virtual inline auto maybeChangeMalloc([[ maybe_unused ]] const MallocInfo& info) -> bool {
        return false;
    }
};
}

#endif /* ATracker_hpp */
