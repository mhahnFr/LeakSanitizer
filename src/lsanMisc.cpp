/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2023  mhahnFr
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
 */
static inline void printLicense() {
    std::ostream & out = __lsan_printCout ? std::cout : std::cerr;
    out << "Copyright (C) 2022 - 2023  mhahnFr and contributors" << std::endl
        << "Licensed under the terms of the GPL 3.0."            << std::endl
        << std::endl;
}

/**
 * Prints the link to the website of this sanitizer.
 */
static inline void printWebsite() {
    using formatter::Style;
    
    std::ostream & out = __lsan_printCout ? std::cout : std::cerr;
    out << formatter::get<Style::ITALIC>
        << "For more information, visit "
        << formatter::format<Style::UNDERLINED>("github.com/mhahnFr/LeakSanitizer")
        << formatter::clear<Style::ITALIC>
        << std::endl << std::endl;
}

void printInformation() {
    using formatter::Style;
    
    std::ostream & out = __lsan_printCout ? std::cout : std::cerr;
    out << "Report by " << formatter::format<Style::BOLD>("LeakSanitizer ")
        << formatter::format<Style::ITALIC>(VERSION)
        << std::endl << std::endl;
    if (__lsan_printLicense) printLicense();
    if (__lsan_printWebsite) printWebsite();
}

void exitHook() {
    using formatter::Style;
    
    setIgnoreMalloc(true);
    std::ostream & out = __lsan_printCout ? std::cout : std::cerr;
    out << std::endl << formatter::format<Style::GREEN>("Exiting");
    
    if (__lsan_printExitPoint) {
        out << formatter::format<Style::ITALIC>(", stacktrace:") << std::endl;
        callstackHelper::format(lcs::callstack(__builtin_return_address(0)), out);
    }
    out << std::endl     << std::endl
        << getInstance() << std::endl;
    printInformation();
    internalCleanUp();
}

void maybeHintRelativePaths(std::ostream & out) {
    if (__lsan_relativePaths) {
        printWorkingDirectory(out);
        out << std::endl;
    }
}

void printWorkingDirectory(std::ostream & out) {
    out << "Note: " << formatter::format<formatter::Style::GREYED>("Paths are relative to the") << " working directory: "
        << std::filesystem::current_path() << std::endl;
}
}
