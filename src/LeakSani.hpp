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

#include <atomic>
#include <map>
#include <mutex>
#include <optional>
#include <ostream>
#include <regex>
#include <set>
#include <utility>

#include <pthread.h>
#include <lsan_internals.h>

#include "ATracker.hpp"
#include "MallocInfo.hpp"

#ifdef BENCHMARK
 #include "timing.hpp"
#endif

#include "allocations/realAlloc.hpp"
#include "statistics/Stats.hpp"

namespace lsan {
/**
 * This class manages everything this sanitizer is capable to do.
 */
class LSan: public ATracker {
    /** An object holding all statistics.                                               */
    Stats                                    stats;
    /** Indicates whether the set callstack size has been exceeded during the printing. */
    bool                                     callstackSizeExceeded = false;
    /** The optional user regular expression.                                           */
    std::optional<std::optional<std::regex>> userRegex;
    /** The user regex error message.                                                   */
    std::optional<std::string> userRegexError;
    std::set<ATracker*> tlsTrackers;
    std::mutex tlsTrackerMutex;

    /** The runtime name of this sanitizer.                                             */
    std::string libName;

#ifdef BENCHMARK
    std::map<timing::AllocType, timing::Timings> timingMap;
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
    
    /**
     * Loads the user first party regular expression.
     */
    inline void loadUserRegex() {
        userRegex = generateRegex(__lsan_firstPartyRegex);
    }
    
    auto maybeRemoveMalloc1(void* pointer) -> std::pair<const bool, std::optional<MallocInfo::CRef>>;

    static auto loadName() -> std::string;

protected:
    virtual inline void addToStats(const MallocInfo& info) override {
        stats += info;
    }

public:
    inline static void* operator new(std::size_t count) {
        return real::malloc(count);
    }

    auto removeMalloc(ATracker* tracker, void* pointer) -> std::pair<const bool, std::optional<MallocInfo::CRef>>;
    void changeMalloc(ATracker* tracker, MallocInfo&& info);
    void registerTracker(ATracker* tracker);
    void deregisterTracker(ATracker* tracker);
    void absorbLeaks(std::map<const void* const, MallocInfo>&& leaks);

    static std::atomic_bool finished;
    void finish() override;

    LSan();
   ~LSan() = default;
    
    LSan(const LSan &)              = delete;
    LSan(const LSan &&)             = delete;
    LSan & operator=(const LSan &)  = delete;
    LSan & operator=(const LSan &&) = delete;
    
    const pthread_key_t saniKey;

#ifdef BENCHMARK
    constexpr inline auto getTimingMap() -> std::map<timing::AllocType, timing::Timings>& {
        return timingMap;
    }

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
     * Returns the runtime library name of this sanitizer.
     *
     * @return the runtime library name
     */
    inline auto getLibName() -> const std::string & {
        if (libName.empty()) {
            libName = loadName();
        }
        return libName;
    }
    
    /**
     * Returns the mutex for the memory allocation infos.
     *
     * @return the mutex
     */
    constexpr inline auto getInfoMutex() -> std::mutex & {
        return infoMutex;
    }

    void changeMalloc(MallocInfo&& info) override;

    /**
     * Removes the allocation record acossiated with the given pointer.
     *
     * @param pointer the allocation pointer
     * @return a pair with a boolean indicating the success and optionally the already deleted allocation record
     */
    auto removeMalloc(void* pointer) -> std::pair<const bool, std::optional<MallocInfo::CRef>> override;
    
    /**
     * Calculates and returns the total count of allocated bytes that are stored inside the
     * principal list containing the allocation records.
     *
     * @return the total count of bytes found in the principal list
     */
    auto getTotalAllocatedBytes() -> std::size_t;
    
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
    constexpr inline auto getFragmentationInfos() const -> const std::map<const void * const, MallocInfo> & {
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

    friend std::ostream & operator<<(std::ostream &, LSan &);
};
}

#endif /* LeakSani_hpp */
