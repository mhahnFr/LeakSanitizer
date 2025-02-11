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

#include <map>
#include <string>

#include <lsan_internals.h>

#define LCS_USE_UNSAFE_OPTIMIZATION 1
#include <callstack.h>
#include <callstack_internals.h>

#include "callstackHelper.hpp"

#include "../formatter.hpp"
#include "../lsanMisc.hpp"

namespace lsan::callstackHelper {
/**
 * An enumeration containing the currently known classifications of a binary file path.
 */
enum class Classification {
    /** Indicates the file path is first party.    */
    firstParty,
    /** Indicates the file path is user-defined.   */
    none
};

/** Caches the classifications of the file paths. */
static std::map<const char*, Classification> cache;

/**
 * Returns whether the given binary file name represents a first party
 * (system) binary.
 *
 * @param file the binary file name to be checked
 * @return whether the given binary file name is first party
 */
static inline auto isFirstPartyCore(const std::string& file) -> bool {
    return file.rfind("/usr/lib", 0) != std::string::npos
        || file.rfind("/lib", 0)     != std::string::npos
        || file.rfind("/System", 0)  != std::string::npos;
}

/**
 * Classifies the given binary file name.
 *
 * @param file the binary file name to be checked
 * @return the classification of the file name
 */
static inline auto classify(const std::string& file) -> Classification {
    if (isFirstPartyCore(file)) {
        return Classification::firstParty;
    }
    return Classification::none;
}

/**
 * Classifies and caches the given binary file name.
 *
 * @param file the binary file name to be classified
 * @return the classification of the file name
 */
static inline auto classifyAndCache(const char* file) -> Classification {
    const auto& toReturn = classify(file);
    cache.emplace(std::make_pair(file, toReturn));
    return toReturn;
}

/**
 * @brief Returns whether the given binary file name is first party.
 *
 * Uses the cache.
 *
 * @param file the binary file name to be checked
 * @return whether the given binary file name is first party
 */
static inline auto isFirstPartyCached(const char* file) -> bool {
    const auto& it = cache.find(file);
    if (it != cache.end()) {
        return it->second == Classification::firstParty;
    }
    return classifyAndCache(file) == Classification::firstParty;
}

/**
 * @brief Returns whether the given binary file name is first party.
 *
 * Uses the cache if appropriate.
 *
 * @param file the binary file name to be checked
 * @return whether the given binary file name is first party
 */
static inline auto isFirstParty(const char* file) -> bool {
    return callstack_autoClearCaches ? isFirstPartyCore(file) : isFirstPartyCached(file);
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
    
    return getBehaviour().relativePaths() ? callstack_frame_getShortestName(&frame) : frame.binaryFile;
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
    return getBehaviour().relativePaths() ? callstack_frame_getShortestSourceFile(&frame) : frame.sourceFile;
}

/**
 * Formats the given callstack frame onto the given output stream using the given style.
 *
 * @param frame the callstack frame to be formatted
 * @param out the output stream
 * @tparam S the style to be used
 */
template<formatter::Style S>
static inline void formatShared(const callstack_frame& frame, std::ostream& out) {
    using namespace formatter;

    if (getBehaviour().printBinaries()) {
        bool reset = false;
        if constexpr (S == Style::GREYED || S == Style::BOLD) {
            reset = true;
        }
        out << formatter::format<Style::ITALIC>("(" + formatString<Style::BLUE>(getCallstackFrameName(frame)) + ")") << (reset ? get<S>() : "") << " ";
    }
    bool needsBrackets = false;
    if (frame.sourceFile == nullptr || getBehaviour().printFunctions()) {
        out << (frame.function == nullptr ? "<< Unknown >>" : frame.function);
        needsBrackets = true;
    }
    if (frame.sourceFile != nullptr) {
        if (needsBrackets) {
            out << " (";
        }
        out << get<Style::CYAN> << getCallstackFrameSourceFile(frame) << ":" << frame.sourceLine;
        if (frame.sourceLineColumn > 0) {
            out << ":" << frame.sourceLineColumn;
        }
        out << clear<Style::CYAN>;
        if (needsBrackets) {
            if constexpr (S == Style::GREYED || S == Style::BOLD) {
                out << get<S>;
            }
            out << ")";
        }
    }
    out << clear<S> << std::endl;
}

void format(lcs::callstack & callstack, std::ostream & stream, const std::string& indent) {
    using formatter::Style;

    if (!callstack_autoClearCaches) {
        // Make sure to use the cached values and
        // potentially fail early.
        //
        //                          - mhahnFr
        if (callstack_getBinariesCached(callstack) == nullptr) {
            stream << indent << formatter::format<Style::RED>("LSan: Error: Failed to translate the callstack.") << std::endl;
            return;
        }
    }
    const auto& frames = callstack_toArray(callstack);
    const auto& size   = callstack_getFrameCount(callstack);

    if (frames == nullptr) {
        stream << indent << formatter::format<Style::RED>("LSan: Error: Failed to translate the callstack.") << std::endl;
        return;
    }

    bool firstHit   = true,
         firstPrint = true;
    std::size_t i, printed;
    for (i = printed = 0; i < size && printed < getBehaviour().callstackSize(); ++i) {
        const auto binaryFile = frames[i].binaryFile;
        
        if (binaryFile == nullptr || (firstPrint && frames[i].binaryFileIsSelf)) {
            continue;
        } else if (firstHit && (isFirstParty(binaryFile) || frames[i].binaryFileIsSelf)) {
            stream << indent << formatter::get<Style::GREYED>
                   << formatter::format<Style::ITALIC>(firstPrint ? "At: " : "at: ");
            formatShared<Style::GREYED>(frames[i], stream);
        } else if (firstHit) {
            firstHit = false;
            stream << indent << formatter::get<Style::BOLD>
                   << formatter::format<Style::ITALIC>(firstPrint ? "In: " : "in: ");
            formatShared<Style::BOLD>(frames[i], stream);
        } else {
            stream << indent << formatter::format<Style::ITALIC>(firstPrint ? "At: " : "at: ");
            formatShared<Style::NONE>(frames[i], stream);
        }
        firstPrint = false;
        ++printed;
    }
    if (i < size) {
        stream << std::endl << indent << formatter::format<Style::UNDERLINED, Style::ITALIC>("And " + std::to_string(size - i) + " more line" + (size - i > 1 ? "s" : "") + "...") << std::endl;
        getInstance().setCallstackSizeExceeded(true);
    }
}

auto isSuppressed(const suppression::Suppression& suppression, lcs::callstack& callstack) -> bool {
    for (std::size_t i = 0; i + suppression.topCallstack.size() <= callstack->backtraceSize; ++i) {
        auto matched { false };
        for (std::size_t j = 0; j < suppression.topCallstack.size(); ++j) {
            const auto& frameAddress = uintptr_t(callstack->backtrace[i + j]);
            const auto& range = suppression.topCallstack[j];
            if (frameAddress >= range.first && frameAddress <= range.first + range.second) {
                matched = true;
            } else {
                matched = false;
                break;
            }
        }
        if (matched) {
            return true;
        }
    }
    return false;
}
}
