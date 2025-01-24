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

#include <csignal>
#include <iostream>

#include "crash.hpp"
#include "warn.hpp"

#include "../lsanMisc.hpp"
#include "../formatter.hpp"
#include "../callstacks/callstackHelper.hpp"

namespace lsan {
/**
 * Prints the given message and the given callstack.
 *
 * @param message the message to be printed
 * @param callstack the callstack to be printed
 * @param reason the optional reason for the message
 * @tparam Warning whether to use warning formatting
 * @tparam SizeHint whether to print the size hint if Warning is false
 */
template<bool Warning, bool SizeHint = true>
constexpr static inline void printer(const std::string& message, lcs::callstack& callstack,
                                     const std::optional<std::string>& reason = std::nullopt) {
    using formatter::Style;
    
    constexpr const auto colour = Warning ? Style::MAGENTA : Style::RED;

    std::cerr << formatter::clearAll() << std::endl
              << formatter::format<Style::BOLD, colour>((Warning ? "Warning: " : "") + message + "!") << std::endl;
    if (reason.has_value()) {
        std::cerr << *reason << "." << std::endl;
    }
    callstackHelper::format(callstack, std::cerr);
    std::cerr << std::endl;
    
    if constexpr (!Warning && SizeHint) {
        getInstance().maybeHintCallstackSize(std::cerr);
        std::cerr << maybeHintRelativePaths;
    }
}

/**
 * Prints the given message and the given callstack.
 *
 * @param message the message to be printed
 * @param callstack the callstack to be printed
 * @tparam Warning whether to use warning formatting
 */
template<bool Warning>
constexpr static inline void printer(const std::string & message, lcs::callstack && callstack) {
    printer<Warning>(message, callstack);
}

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

    printer<Warning, false>(message, callstack);

    if (info.has_value()) {
        constexpr const auto colour = Warning ? Style::MAGENTA : Style::RED;
        const auto& record = info.value().get();
        const auto& showThread = true; // TODO: Properly implement

        std::cerr << format<Style::ITALIC, colour>("Previously allocated"s
                                                   + (showThread ? " by " + formatThreadId(record.threadId) : "")
                                                   + " here:") << std::endl;
        record.printCreatedCallstack(std::cerr);
        std::cerr << std::endl;
        if (record.deletedCallstack.has_value()) {
            std::cerr << format<Style::ITALIC, colour>("Previously freed"s
                                                       + (showThread ? " by " + formatThreadId(record.deletedId) : "")
                                                       + " here:") << std::endl;
            record.printDeletedCallstack(std::cerr);
            std::cerr << std::endl;
        }
    }
    if constexpr (!Warning) {
        getInstance().maybeHintCallstackSize(std::cerr);
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
    const auto& suppressions = lsan::getSuppressions();
    if (!callstackHelper::isSuppressed(suppressions.cbegin(), suppressions.cend(), callstack)) {
        function(callstack);
    }
}

void warn(const std::string & message) {
    withCallstack([&] (auto & callstack) {
        printer<true>(message, callstack);
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
        printer<false>(message, callstack);
        abort();
    });
}

void crashForce(const std::string & message) {
    printer<false>(message, lcs::callstack());
    abort();
}

void crashForce(const std::string& message, const std::optional<std::string>& reason, lcs::callstack&& callstack) {
    printer<false>(message, callstack, reason);
    abort();
}

void crash(const std::string& message,
           const std::optional<MallocInfo::CRef>& info) {
    withCallstack([&] (auto& callstack) {
        printer<false>(message, info, callstack);
        abort();
    });
}

[[ noreturn ]] void abort() {
    signal(SIGABRT, SIG_DFL);
    std::abort();
}
}
