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

#include <map>
#include <mutex>
#include <optional>
#include <ostream>
#include <regex>
#include <set>
#include <utility>

#include "MallocInfo.hpp"
#include "helperStructs.hpp"

#ifdef BENCHMARK
 #include "timing.hpp"
#endif

#include "initialization/init.hpp"
#include "statistics/Stats.hpp"

#include "../include/lsan_internals.h"

namespace lsan {
/**
 * This class manages everything this sanitizer is capable to do.
 */
class LSan {
    /** A pair consisting of a boolean and an optional allocation record.               */
    using MallocInfoRemoved = std::pair<const bool, std::optional<std::reference_wrapper<const MallocInfo>>>;
    
    /** A map containing all allocation records, sorted by their allocated pointers.    */
    std::map<const void * const, MallocInfo> infos;
    std::set<pthread_key_t>                  keys;
    /** An object holding all statistics.                                               */
    Stats                                    stats;
    /** Indicates whether the set callstack size has been exceeded during the printing. */
    bool                                     callstackSizeExceeded = false;
    /** The mutex used to synchronize the allocations and tracking.                     */
    std::recursive_mutex                     mutex;
    /** This mutex is used to strictly synchronize the access to the infos.             */
    std::mutex                               infoMutex;
    std::mutex                               tlsKeyMutex;
    /** The optional user regular expression.                                           */
    std::optional<std::optional<std::regex>> userRegex;
    /** The user regex error message.                                                   */
    std::optional<std::string> userRegexError;
    
    /** The runtime name of this sanitizer.                                             */
    const std::string libName;
    
#ifdef BENCHMARK
    std::map<timing::AllocType, timing::Timings> timingMap;
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
    auto classifyRecord(MallocInfo& info, const LeakType& currentType) -> std::size_t;
    
    /**
     * Loads the user first party regular expression.
     */
    inline void loadUserRegex() {
        userRegex = generateRegex(__lsan_firstPartyRegex);
    }
    
    inline auto classifyLeaks(uintptr_t begin, uintptr_t end, LeakType direct, LeakType indirect, bool skipClassifieds = false) -> std::pair<std::size_t, std::size_t> {
        std::size_t directCount   { 0 },
                    indirectCount { 0 };
        for (uintptr_t it = begin; it < end; it += sizeof(uintptr_t)) {
            const auto& record = infos.find(*reinterpret_cast<void**>(it));
            if (record == infos.end() || record->second.isDeleted() || (skipClassifieds && record->second.getLeakType() != LeakType::unclassified)) {
                continue;
            }
            if (record->second.getLeakType() > direct) {
                record->second.setLeakType(direct);
                ++directCount;
            }
            indirectCount += classifyRecord(record->second, indirect);
        }
        return std::make_pair(directCount, indirectCount);
    }
    
public:
    bool hasPrintedExit = false;

    LSan();
   ~LSan() {
        inited = false;
    }
    
    LSan(const LSan &)              = delete;
    LSan(const LSan &&)             = delete;
    LSan & operator=(const LSan &)  = delete;
    LSan & operator=(const LSan &&) = delete;
    
#ifdef BENCHMARK
    constexpr inline auto getTimingMap() -> std::map<timing::AllocType, timing::Timings>& {
        return timingMap;
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
    constexpr inline auto getLibName() const -> const std::string & {
        return libName;
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
    
    /**
     * @brief Attempts to exchange the allocation record associated with the given
     * allocation record by the given allocation record.
     *
     * @param info the allocation record to be exchanged
     * @return whether an allocation record was exchanged
     */
    auto changeMalloc(const MallocInfo & info) -> bool;
    
    /**
     * Removes the allocation record acossiated with the given pointer.
     *
     * @param pointer the allocation pointer
     * @return a pair with a boolean indicating the success and optionally the already deleted allocation record
     */
    auto removeMalloc(void* pointer) -> MallocInfoRemoved;
    
    /**
     * Adds the given allocation record.
     *
     * @param info the allocation record to be added
     */
    void addMalloc(MallocInfo && info);
    
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

    void classifyStackLeaksShallow();
    void addTLSKey(const pthread_key_t& key);
    auto removeTLSKey(const pthread_key_t& key) -> bool;

    friend std::ostream & operator<<(std::ostream &, LSan &);
};
}

#endif /* LeakSani_hpp */
