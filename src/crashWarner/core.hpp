/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2025  mhahnFr
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

#ifndef core_hpp
#define core_hpp

#include <callstack.h>
#include <iostream>
#include <optional>
#include <string>

#include "../hinter.hpp"
#include "../callstackHelper/format.hpp"
#include "../formatter/formatter.hpp"

namespace lsan {
namespace crashWarner {
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
static inline void printer(const std::string& message, lcs::callstack& callstack,
                           const std::optional<std::string>& reason = std::nullopt) {
    using namespace formatter;

    constexpr auto colour = Warning ? Style::MAGENTA : Style::RED;

    std::cerr << clearAll() << std::endl
              << format<Style::BOLD, colour>((Warning ? "Warning: " : "") + message + "!") << std::endl;
    if (reason.has_value()) {
        std::cerr << *reason << "." << std::endl;
    }
    const auto ex = callstackHelper::format(callstack, std::cerr);
    std::cerr << std::endl;

    if constexpr (!Warning && SizeHint) {
        std::ostringstream oss;
        hinter::maybeHintCallstackSize(oss, ex);
        if (const auto& str = oss.str(); !str.empty()) {
            std::cerr << "Hints:" << std::endl << str;
        }
        std::cerr << std::endl << maybeHintRelativePaths;
    } else {
        // TODO: if (ex) getInstance().setCallstackSizeExceeded(true);
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
}

/**
 * This function resets the signal handler for @c SIGABRT and performs the abort.
 */
[[ noreturn ]] void abort();
}

#endif /* core_hpp */
