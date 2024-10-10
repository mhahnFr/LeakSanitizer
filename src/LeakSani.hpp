/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2022 - 2024  mhahnFr
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

#ifndef LeakSani_hpp
#define LeakSani_hpp

#include <iostream>

#include <atomic>
#include <map>
#include <mutex>
#include <optional>
#include <ostream>
#include <regex>
#include <set>
#include <tuple>
#include <utility>

#include <pthread.h>
#include <lsan_internals.h>

#include "ATracker.hpp"
#include "MallocInfo.hpp"
#include "helperStructs.hpp"

#ifdef BENCHMARK
 #include "timing.hpp"
#endif

#include "wrappers/realAlloc.hpp"
#include "statistics/Stats.hpp"

namespace lsan {
/**
 * @brief This class manages everything this sanitizer is capable to do.
 *
 * It acts as an allocation tracker.
 */
class LSan final: public ATracker {
    std::set<pthread_key_t> keys;
    std::mutex tlsKeyMutex;
    std::map<std::pair<pthread_t, pthread_key_t>, const void*> tlsKeyValues;
    std::mutex tlsKeyValuesMutex;
    /** An object holding all statistics.                                               */
    Stats stats;
    /** Indicates whether the set callstack size has been exceeded during the printing. */
    bool callstackSizeExceeded = false;
    /** The optional user regular expression.                                           */
    std::optional<std::optional<std::regex>> userRegex;
    /** The user regex error message.                                                   */
    std::optional<std::string> userRegexError;
    /** The registered thread-local allocation trackers.                                */
    std::set<ATracker*> tlsTrackers;
    /** The mutex to manage the access to the registered thread-local trackers.         */
    std::mutex tlsTrackerMutex;

#ifdef BENCHMARK
    /** The registered timings of the allocations.                                      */
    std::map<timing::AllocType, timing::Timings> timingMap;
    /** The mutex to manage access to the allocation timings.                           */
    std::mutex timingMutex;
#endif

    /**
     * @brief Generates and returns a regular expression object for the given string.
     *
     * Sets the regex error message if the given string was not a valid regular expression.
     *
     * @param regex the string with the regular expression
     * @return an optional regex object
     */
    auto generateRegex(const char * regex) -> std::optional<std::regex>;

    auto classifyLeaks() -> LeakKindStats;
    auto classifyRecord(MallocInfo& info, const LeakType& currentType) -> std::pair<std::size_t, std::size_t>;

    /**
     * Creates a thread-safe copy of the thread-local tracker list.
     *
     * @return the copy
     */
    auto copyTrackerList() -> decltype(tlsTrackers);

    /**
     * Loads the user first party regular expression.
     */
    inline void loadUserRegex() {
        userRegex = generateRegex(__lsan_firstPartyRegex);
    }

    inline auto classifyLeaks(uintptr_t begin, uintptr_t end,
                              LeakType direct, LeakType indirect,
                              std::set<MallocInfo*>& directs, bool skipClassifieds = false) -> std::tuple<std::size_t, std::size_t, std::size_t, std::size_t> {
        std::size_t directCount   { 0 },
                    indirectCount { 0 },
                    directBytes   { 0 },
                    indirectBytes { 0 };
        for (uintptr_t it = begin; it < end; it += sizeof(uintptr_t)) {
            const auto& record = infos.find(*reinterpret_cast<void**>(it));
            if (record == infos.end() || record->second.deleted || (skipClassifieds && record->second.leakType != LeakType::unclassified)) {
                continue;
            }
            if (record->second.leakType > direct) {
                record->second.leakType = direct;
                ++directCount;
                directBytes += record->second.size;
                directs.insert(&record->second);
            }
            const auto& [count, bytes] = classifyRecord(record->second, indirect);
            indirectCount += count;
            indirectBytes += bytes;
        }
        return std::make_tuple(directCount, directBytes, indirectCount, indirectBytes);
    }

    template<bool Four = false>
    constexpr inline auto classifyPointerUnion(void* ptr, std::set<MallocInfo*>& directs,
                                               LeakType direct, LeakType indirect) -> std::pair<std::size_t, std::size_t> {
        std::size_t count { 0 },
                    bytes { 0 };

        constexpr const auto order = Four ? 3 : 1;

        const auto& it = infos.find(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(ptr) & ~order));
        if (it != infos.end() && it->second.leakType > direct) {
            it->second.leakType = direct;
            const auto& [c, b] = classifyRecord(it->second, indirect);
            count = c;
            bytes = b;
            directs.insert(&it->second);
        }

        return std::make_pair(count, bytes);
    }

    inline auto classifyClass(void* cls, std::set<MallocInfo*>& directs) -> std::tuple<std::size_t, std::size_t, std::size_t, std::size_t> {
        std::size_t count  { 0 },
                    bytes  { 0 },
                    iCount { 0 },
                    iBytes { 0 };

        auto classWords = reinterpret_cast<void**>(cls);
        auto cachePtr = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(classWords[2]) & ((uintptr_t) 1 << 48) - 1);
        const auto& cacheIt = infos.find(cachePtr);
        if (cacheIt != infos.end() && cacheIt->second.leakType > LeakType::globalDirect) {
            cacheIt->second.leakType = LeakType::globalDirect;
            const auto& [rCount, rBytes] = classifyRecord(cacheIt->second, LeakType::globalIndirect);
            iCount += rCount;
            iBytes += rBytes;
            ++count;
            bytes += cacheIt->second.size;
            directs.insert(&cacheIt->second);
        }

        auto ptr = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(classWords[4]) & 0x0f007ffffffffff8UL);
        const auto& it = infos.find(ptr);
        if (it != infos.end()) {
            if (it->second.leakType > LeakType::globalDirect) {
                it->second.leakType = LeakType::globalDirect;
                // FIXME: What if already as indirect counted records are found elsewhere? Happens with this one:
                const auto& [rCount, rBytes] = classifyRecord(it->second, LeakType::globalIndirect);
                iCount += rCount;
                iBytes += rBytes;
                ++count;
                bytes += it->second.size;
                directs.insert(&it->second);
            }

            auto rwStuff = reinterpret_cast<void**>(it->second.pointer);
            auto ptr = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(rwStuff[1]) & ~1);
            const auto& it = infos.find(ptr);
            if (it != infos.end() && it->second.leakType > LeakType::globalDirect) {
                it->second.leakType = LeakType::globalDirect;
                const auto& [rCount, rBytes] = classifyRecord(it->second, LeakType::globalIndirect);
                iCount += rCount;
                iBytes += rBytes;
                ++count;
                bytes += it->second.size;
                directs.insert(&it->second);
            }
        }
        return std::make_tuple(count, bytes, iCount, iBytes);
    }

protected:
    virtual inline void addToStats(const MallocInfo& info) final override {
        stats += info;
    }

public:
    /** Indicates whether the allocation tracking has finished.                     */
    static std::atomic_bool finished;
    /** Indicates whether to ignore deallocations in the TLS deallocator.           */
    static std::atomic_bool preventDealloc;
    /** The thread-local storage key used for the thread-local allocation trackers. */
    const pthread_key_t saniKey;
    bool hasPrintedExit = false;

    LSan();
   ~LSan();

    LSan(const LSan&) = delete;
    LSan(LSan&&)      = delete;

    auto operator=(const LSan&) -> LSan& = delete;
    auto operator=(LSan&&)      -> LSan& = delete;

    inline static auto operator new(std::size_t count) -> void* {
        return real::malloc(count);
    }

    inline static void operator delete(void* ptr) {
        real::free(ptr);
    }

    /**
     * @brief Attempts to remove the allocation record associated with the given pointer.
     *
     * If no record is found in this instance, all registered trackers except
     * the given one are searched for the record.
     *
     * @param tracker the tracker to not be searched
     * @param pointer the pointer to the allocation
     * @return whether a record was removed and the potentially existing record
     */
    auto removeMalloc(ATracker* tracker, void* pointer) -> std::pair<bool, std::optional<MallocInfo::CRef>>;

    /**
     * @brief Replaces the allocation record with the given one.
     *
     * If no record is found in this instance, all registered trackers except
     * the given one are searched for the record.
     *
     * @param tracker the allocation tracker to not be searched
     * @param info the new allocation record
     */
    void changeMalloc(ATracker* tracker, MallocInfo&& info);

    /**
     * Registers the given allocation tracker.
     *
     * @param tracker the allocation tracker to be registered
     */
    void registerTracker(ATracker* tracker);

    /**
     * Deregisters the given allocation tracker.
     *
     * @param tracker the allocation tracker to be deregistered
     */
    void deregisterTracker(ATracker* tracker);

    /**
     * Absorbs the given allocation records.
     */
    void absorbLeaks(PoolMap<const void* const, MallocInfo>&& leaks);

    virtual void finish() final override;

#ifdef BENCHMARK
    /**
     * Returns the allocation timings.
     *
     * @return the allocation timings
     */
    constexpr inline auto getTimingMap() -> std::map<timing::AllocType, timing::Timings>& {
        return timingMap;
    }

    /**
     * Returns the timing mutex.
     *
     * @return the timing mutex
     */
    constexpr inline auto getTimingMutex() -> std::mutex& {
        return timingMutex;
    }
#endif
    
    /**
     * Returns the user first party regular expression.
     *
     * @return the user regular expression
     */
    inline auto getUserRegex() -> const std::optional<std::regex> & {
        if (!userRegex.has_value()) {
            loadUserRegex();
        }
        return userRegex.value();
    }
    
    /**
     * Returns the mutex for the allocations and tracking.
     *
     * @return the mutex
     */
    constexpr inline auto getMutex() -> std::recursive_mutex & {
        return mutex;
    }
    
    /**
     * Returns the mutex for the memory allocation infos.
     *
     * @return the mutex
     */
    constexpr inline auto getInfoMutex() -> std::mutex & {
        return infoMutex;
    }

    virtual void changeMalloc(MallocInfo&& info) final override;

    /**
     * Removes the allocation record associated with the given pointer.
     *
     * @param pointer the allocation pointer
     * @return a pair with a boolean indicating the success and optionally the already deleted allocation record
     */
    virtual auto removeMalloc(void* pointer) -> std::pair<bool, std::optional<MallocInfo::CRef>> final override;

    /**
     * @brief Attempts to remove the allocation record associated with the given pointer.
     *
     * Does not search in the thread-local trackers.
     *
     * @param pointer the allocation pointer
     * @return whether a record was removed and the potentially existing record
     */
    virtual auto maybeRemoveMalloc(void* pointer) -> std::pair<bool, std::optional<MallocInfo::CRef>> final override;

    /**
     * Prints a hint about the exceeded callstack size if it was exceeded.
     *
     * @param out the output stream to print to
     * @return the given output stream
     */
    auto maybeHintCallstackSize(std::ostream & out) const -> std::ostream &;
    
    /**
     * Returns the globally tracked allocations.
     *
     * @return the globally tracked allocations
     */
    constexpr inline auto getFragmentationInfos() const -> const decltype(infos)& {
        return infos;
    }
    
    /**
     * Sets whether the maximum callstack size has been exceeded during the printing.
     *
     * @param exceeded whether the maximum callstack size has been exceeded
     */
    constexpr inline void setCallstackSizeExceeded(bool exceeded) {
        callstackSizeExceeded = exceeded;
    }
    
    /**
     * Returns the current instance of the statistics object.
     *
     * @return the current statistics instance
     */
    constexpr inline auto getStats() -> const Stats & {
        return stats;
    }

    void classifyStackLeaksShallow();
    void addTLSKey(const pthread_key_t& key);
    auto removeTLSKey(const pthread_key_t& key) -> bool;

    auto hasTLSKey(const pthread_key_t& key) -> bool;

    virtual auto addTLSValue(const pthread_key_t& key, const void* value) -> bool final override;

    friend auto operator<<(std::ostream&, LSan&) -> std::ostream&;
};
}

#endif /* LeakSani_hpp */
