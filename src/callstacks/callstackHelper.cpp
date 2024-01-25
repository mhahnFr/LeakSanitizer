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

#include <optional>
#include <regex>

#include "callstackHelper.hpp"

#include "../formatter.hpp"
#include "../lsanMisc.hpp"
#include "../LeakSani.hpp"

#include "../../include/lsan_internals.h"

namespace lsan::callstackHelper {
/**
 * Returns whether the given binary file name is this library.
 *
 * @param name the name to be checked
 * @return whether the given name is this library
 */
static inline auto isInLSan(const std::string & name) -> bool {
    return getInstance().getLibName() == name;
}

/**
 * Returns whether the given binary file name should be ignored totally.
 *
 * @param file the binary file name to be checked
 * @return whether to totally ignore the binary
 */
static inline auto isTotallyIgnored(const std::string & file) -> bool {
    // So far totally ignored: Everything Objective-C and Swift (using ARC -> no leak).
    return file.find("libobjc.A.dylib")    != std::string::npos
        || file.rfind("/usr/lib/swift", 0) != std::string::npos;
}

/**
 * Returns whether the given file name is matched by the user defined regex.
 *
 * @param file the file name to be checked
 * @return whether the name was matched
 */
static inline auto isUserDefinedFirstParty(const std::string & file) -> bool {
    const auto & regex = getInstance().getUserRegex();
    if (!regex.has_value()) {
        return false;
    }
    
    return regex_search(file, regex.value());
}

/**
 * Returns whether the given binary file name represents a first party
 * (system) binary.
 *
 * @param file the binary file name to be checked
 * @return whether the given binary file name is first party
 */
static inline auto isFirstParty(const std::string & file) -> bool {
    return file.rfind("/usr/lib", 0) != std::string::npos
        || file.rfind("/lib", 0)     != std::string::npos
        || file.rfind("/System", 0)  != std::string::npos
        || isUserDefinedFirstParty(file);
}

auto getCallstackType(lcs::callstack & callstack) -> CallstackType {
    const auto frames = callstack_getBinaries(callstack);
    
    std::size_t firstPartyCount = 0;
    const auto frameCount = callstack_getFrameCount(callstack);
    for (std::size_t i = 0; i < frameCount; ++i) {
        const auto binaryFile = frames[i].binaryFile;
        
        if (binaryFile == nullptr || isInLSan(binaryFile)) {
            continue;
        } else if (isTotallyIgnored(binaryFile)) {
            return CallstackType::HARD_IGNORE;
        } else if (isFirstParty(binaryFile)) {
            if (++firstPartyCount > __lsan_firstPartyThreshold) {
                return CallstackType::FIRST_PARTY_ORIGIN;
            }
        } else {
            return CallstackType::USER;
        }
    }
    return CallstackType::FIRST_PARTY;
}

/**
 * @brief Returns the name of the binary file of the given callstack frame.
 *
 * The file name is allowed to be a relative if `__lsan_relativePaths` is `true`.
 *
 * @param frame the callstack frame
 * @return the name of the binary file of the given callstack frame
 */
static inline auto getCallstackFrameName(const callstack_frame & frame) -> std::string {
    if (frame.binaryFile == nullptr) {
        return "<< Unknown >>";
    }
    
    return __lsan_relativePaths ? callstack_frame_getShortestName(&frame) : frame.binaryFile;
}

/**
 * @brief Returns the name of the source file of the given callstack frame.
 *
 * The file name is allowed to be relative if `__lsan_relativePaths` is `true`.
 *
 * @param frame the callstack frame
 * @return the name of the source file name of the given callstack frame
 */
static inline auto getCallstackFrameSourceFile(const callstack_frame & frame) -> std::string {
    return __lsan_relativePaths ? callstack_frame_getShortestSourceFile(&frame) : frame.sourceFile;
}

/**
 * Formats the given callstack frame onto the given output stream using the given style.
 *
 * @param frame the callstack frame to be formatted
 * @param out the output stream
 * @tparam S the style to be used
 */
template<formatter::Style S>
static inline void formatShared(const struct callstack_frame & frame, std::ostream & out) {
    using formatter::Style;
    
    if (__lsan_printBinaries) {
        bool reset = false;
        if (S == Style::GREYED || S == Style::BOLD) {
            reset = true;
        }
        out << formatter::format<Style::ITALIC>("(" + getCallstackFrameName(frame) + ")") << (reset ? formatter::get<S>() : "") << " ";
    }
    bool needsBrackets = false;
    if (frame.sourceFile == nullptr || __lsan_printFunctions) {
        out << (frame.function == nullptr ? "<< Unknown >>" : frame.function);
        needsBrackets = true;
    }
    if (frame.sourceFile != nullptr) {
        if (needsBrackets) {
            out << " (" << formatter::get<Style::GREYED, Style::UNDERLINED>;
        }
        out << getCallstackFrameSourceFile(frame) << ":" << frame.sourceLine;
        if (frame.sourceLineColumn.has_value) {
            out << ":" << frame.sourceLineColumn.value;
        }
        if (needsBrackets) {
            out << formatter::clear<Style::GREYED, Style::UNDERLINED>;
            if (S == Style::GREYED || S == Style::BOLD) {
                out << formatter::get<S>;
            }
            out << ")";
        }
    }
    out << formatter::clear<S> << std::endl;
}

void format(lcs::callstack & callstack, std::ostream & stream) {
    using formatter::Style;
    
    const auto frames = callstack_toArray(callstack);
    const auto size   = callstack_getFrameCount(callstack);
    
    bool firstHit   = true,
         firstPrint = true;
    std::size_t i, printed;
    for (i = printed = 0; i < size && printed < __lsan_callstackSize; ++i) {
        const auto binaryFile = frames[i].binaryFile;
        
        if (binaryFile == nullptr || isInLSan(binaryFile)) {
            continue;
        } else if (firstHit && isFirstParty(binaryFile)) {
            stream << formatter::get<Style::GREYED>
                   << formatter::format<Style::ITALIC>(firstPrint ? "At: " : "at: ");
            formatShared<Style::GREYED>(frames[i], stream);
        } else if (firstHit) {
            firstHit = false;
            stream << formatter::get<Style::BOLD>
                   << formatter::format<Style::ITALIC>(firstPrint ? "In: " : "in: ");
            formatShared<Style::BOLD>(frames[i], stream);
        } else {
            stream << formatter::format<Style::ITALIC>(firstPrint ? "At: " : "at: ");
            formatShared<Style::NONE>(frames[i], stream);
        }
        firstPrint = false;
        ++printed;
    }
    if (i < size) {
        stream << std::endl << formatter::format<Style::UNDERLINED, Style::ITALIC>("And " + std::to_string(size - i) + " more lines...") << std::endl;
        getInstance().setCallstackSizeExceeded(true);
    }
}
}
