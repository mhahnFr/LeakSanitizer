/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2023 - 2024  mhahnFr
 *
 * This file is part of the LeakSanitizer. This library is free software:
 * you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this library, see the file LICENSE.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <filesystem>
#include <iostream>

#if __has_include(<unistd.h>)
 #include <unistd.h>

 #define LSAN_HAS_UNISTD
#endif

#include "lsanMisc.hpp"

#include "formatter.hpp"
#include "callstacks/callstackHelper.hpp"

#include "../include/lsan_internals.h"
#include "../CallstackLibrary/include/callstack.h"

namespace lsan {
auto _getIgnoreMalloc() -> bool & {
    static bool ignore = false;
    return ignore;
}

auto getInstance() -> LSan & {
    static auto instance = new LSan();
    return *instance;
}

/**
 * Prints the license information of this sanitizer.
 *
 * @param out the output stream to print to
 * @return the given output stream
 */
static inline auto printLicense(std::ostream & out) -> std::ostream & {
    out << "Copyright (C) 2022 - 2024  mhahnFr and contributors" << std::endl
        << "Licensed under the terms of the GPL 3.0."            << std::endl
        << std::endl;
    
    return out;
}

/**
 * Prints the link to the website of this sanitizer.
 *
 * @param out the output stream to print to
 * @return the given output stream
 */
static inline auto printWebsite(std::ostream & out) -> std::ostream & {
    using formatter::Style;
    
    out << formatter::get<Style::ITALIC>
        << "For more information, visit "
        << formatter::format<Style::UNDERLINED>("github.com/mhahnFr/LeakSanitizer")
        << formatter::clear<Style::ITALIC>
        << std::endl << std::endl;
    
    return out;
}

auto printInformation(std::ostream & out) -> std::ostream & {
    using formatter::Style;
    
    out << "Report by " << formatter::format<Style::BOLD>("LeakSanitizer ")
        << formatter::format<Style::ITALIC>(VERSION)
        << std::endl << std::endl
        << printLicense
        << printWebsite;
    
    return out;
}

void exitHook() {
    using formatter::Style;
    
    setIgnoreMalloc(true);
    auto & out = getOutputStream();
    out << std::endl << formatter::format<Style::GREEN>("Exiting");
    
    if (__lsan_printExitPoint) {
        out << formatter::format<Style::ITALIC>(", stacktrace:") << std::endl;
        callstackHelper::format(lcs::callstack(__builtin_return_address(0)), out);
    }
    out << std::endl     << std::endl
        << getInstance() << std::endl
        << printInformation;
    internalCleanUp();
}

auto maybeHintRelativePaths(std::ostream & out) -> std::ostream & {
    if (__lsan_relativePaths) {
        out << printWorkingDirectory << std::endl;
    }
    return out;
}

auto printWorkingDirectory(std::ostream & out) -> std::ostream & {
    out << "Note: " << formatter::format<formatter::Style::GREYED>("Paths are relative to the") << " working directory: "
        << std::filesystem::current_path() << std::endl;
    
    return out;
}

auto isATTY() -> bool {
#ifdef LSAN_HAS_UNISTD
    return isatty(__lsan_printCout ? STDOUT_FILENO : STDERR_FILENO);
#else
    return __lsan_printFormatted;
#endif
}

auto has(const std::string & var) -> bool {
    return getenv(var.c_str()) != nullptr;
}
}
