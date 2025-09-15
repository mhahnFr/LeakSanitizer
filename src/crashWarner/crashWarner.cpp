/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2023 - 2025  mhahnFr
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

#include <iostream>

#include "core.hpp"
#include "crash.hpp"
#include "warn.hpp"
#include "../lsanMisc.hpp"
#include "../callstackHelper/callstackHelper.hpp"
#include "../formatter/formatter.hpp"

namespace lsan {
/**
 * Prints the given message, the allocation information found in the
 * optionally provided allocation record and the given callstack.
 *
 * @param message the message to be printed
 * @param info the optional allocation record
 * @param callstack the callstack to be printed
 * @tparam Warning whether to use warning formatting
 */
template<bool Warning>
constexpr static inline void printer(const std::string&                     message,
                                     const std::optional<MallocInfo::CRef>& info,
                                     lcs::callstack&                        callstack) {
    using namespace formatter;
    using namespace std::string_literals;

    crashWarner::printer<Warning, false>(message, callstack);

    auto& instance = getInstance();
    if (info.has_value()) {
        constexpr auto colour = Warning ? Style::MAGENTA : Style::RED;
        const auto& record = info.value().get();
        const auto& showThread = instance.getIsThreaded();

        std::cerr << format<Style::ITALIC, colour>("Previously allocated"s
                                                   + (showThread ? " by " + instance.getThreadDescription(record.getAllocationThread()) : "")
                                                   + " here:") << std::endl;
        record.printCreatedCallstack(std::cerr);
        std::cerr << std::endl;
        if (record.getDeallocationCallstack().has_value()) {
            std::cerr << format<Style::ITALIC, colour>("Previously freed"s
                                                       + (showThread ? " by " + instance.getThreadDescription(record.getDeallocationThread()) : "")
                                                       + " here:") << std::endl;
            record.printDeletedCallstack(std::cerr);
            std::cerr << std::endl;
        }
    }
    if constexpr (!Warning) {
        instance.maybeHintCallstackSize(std::cerr);
        std::cerr << maybeHintRelativePaths;
    }
}

/**
 * Executes the given function with a callstack up to the given omit address
 * if the generated callstack is user relevant.
 *
 * @param function the function to be executed
 * @tparam F the function's type - it will get a lcs::callstack as the only argument
 */
template<typename F>
static inline void withCallstack(const F & function) {
    auto callstack = lcs::callstack();
    if (const auto& suppressions = getSuppressions();
        !callstackHelper::isSuppressed(suppressions.cbegin(), suppressions.cend(), callstack)) {
        function(callstack);
    }
}

void warn(const std::string & message) {
    withCallstack([&] (auto & callstack) {
        crashWarner::printer<true>(message, callstack);
    });
}

void warn(const std::string& message,
          const std::optional<MallocInfo::CRef>& info) {
    withCallstack([&] (auto& callstack) {
        printer<true>(message, info, callstack);
    });
}

void crash(const std::string & message) {
    withCallstack([&] (auto & callstack) {
        crashWarner::printer<false>(message, callstack);
        abort();
    });
}

void crash(const std::string& message,
           const std::optional<MallocInfo::CRef>& info) {
    withCallstack([&] (auto& callstack) {
        printer<false>(message, info, callstack);
        abort();
    });
}
}
