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

#define LCS_USE_UNSAFE_OPTIMIZATION 1

#include "callstackHelper.hpp"

#include <string>

#include <callstack.h>
#include <callstack_internals.h>

#include "../formatter.hpp"
#include "../lsanMisc.hpp"
#include "../suppression/firstPartyLibrary.hpp"

namespace lsan::callstackHelper {
/**
 * @brief Returns the name of the binary file of the given callstack frame.
 *
 * The file name is allowed to be a relative if relative paths are activated.
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
 * The file name is allowed to be relative if relative paths are activated.
 *
 * @param frame the callstack frame
 * @return the name of the source file name of the given callstack frame
 */
static inline auto getCallstackFrameSourceFile(const callstack_frame & frame) -> std::string {
    return getBehaviour().relativePaths() ? callstack_frame_getShortestSourceFile(&frame) : frame.sourceFile;
}

/**
 * Formats the given callstack frame onto the given output stream using the
 * given style.
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

/**
 * Creates and indent string if the given indentation is bigger than zero.
 *
 * @param indent the amount of indentation
 * @param indentChar the character to indent with
 * @return a suitable indentation string
 */
static inline auto getIndent(const std::string::size_type indent, const char indentChar = ' ') -> std::string {
    return indent > 0 ? std::string(indent, indentChar) : std::string();
}

void format(lcs::callstack& callstack, std::ostream& stream, const std::string& indent) {
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
    std::size_t i, printed, maxCount = 1;

    if (getBehaviour().callstackSize() > 9) {
        std::size_t toSkip = 0;
        if (size > 9) {
            for (; toSkip < size && (frames[toSkip].binaryFile == nullptr || frames[toSkip].binaryFileIsSelf); ++toSkip);
        }
        if (size - toSkip > 9) {
            if (size - toSkip < 100) {
                maxCount = 2;
            } else {
                maxCount = std::to_string(size - toSkip).size();
            }
        }
    }

    for (i = printed = 0; i < size && printed < getBehaviour().callstackSize(); ++i) {
        if (const auto& binaryFile = frames[i].binaryFile; binaryFile == nullptr || (firstPrint && frames[i].binaryFileIsSelf)) {
            continue;
        } else if (firstHit && (suppression::isFirstParty(binaryFile, !callstack_autoClearCaches) || frames[i].binaryFileIsSelf)) {
            const auto& number = std::to_string(printed + 1);
            stream << indent << formatter::get<Style::GREYED>
                   << formatter::format<Style::ITALIC>("# " + getIndent(maxCount - number.size()) + number + ": ");
            formatShared<Style::GREYED>(frames[i], stream);
        } else if (firstHit) {
            firstHit = false;
            stream << indent << formatter::get<Style::BOLD>
                   << formatter::format<Style::ITALIC>(getIndent(maxCount - 1) + " ->  ");
            formatShared<Style::BOLD>(frames[i], stream);
        } else {
            const auto& number = std::to_string(printed + 1);
            stream << indent << formatter::format<Style::ITALIC>("# " + getIndent(maxCount - number.size()) + number + ": ");
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

/**
 * Returns whether the given range or regex matches the given callstack frame.
 *
 * @param supp the regex or range
 * @param frame the callstack frame
 * @param address the address of the represented function call
 * @return whether the callstack frame was matched
 */
static inline auto match(const suppression::Suppression::RangeOrRegexType& supp, const callstack_frame* frame, const uintptr_t address) -> bool {
    if (supp.first == suppression::Suppression::Type::range) {
        const auto& [begin, end] = std::get<suppression::Suppression::RangeType>(supp.second);
        return address >= begin && address <= begin + end;
    }
    const auto& suppressions = std::get<suppression::Suppression::RegexType>(supp.second);
    return std::any_of(suppressions.begin(), suppressions.end(), [&](const std::regex& regex) {
        return frame->binaryFileIsSelf
            || (std::regex_match("LSAN_SYSTEM_LIBRARIES", regex) && suppression::isFirstParty(frame->binaryFile, !callstack_autoClearCaches))
            || std::regex_match(frame->binaryFile, regex);
    });
}

auto isSuppressed(const suppression::Suppression& suppression, lcs::callstack& callstack) -> bool {
    const callstack_frame* binaries = nullptr;
    if (suppression.hasRegexes) {
        binaries = callstack_autoClearCaches ? callstack_getBinaries(callstack) : callstack_getBinariesCached(callstack);
        if (binaries == nullptr) {
            return false;
        }
    }
    for (std::size_t i = 0; i + suppression.topCallstack.size() <= callstack->backtraceSize; ++i) {
        auto matched { false };

        for (std::size_t j = 0, k = i; j < suppression.topCallstack.size() && k < callstack->backtraceSize; ) {
            const auto& address = uintptr_t(callstack->backtrace[k]);
            const auto& hereMatch = match(suppression.topCallstack[j], binaries + k, address);
            if (suppression.topCallstack[j].first == suppression::Suppression::Type::regex) {
                auto nextMatch = false;
                if (j + 1 < suppression.topCallstack.size()) {
                    nextMatch = match(suppression.topCallstack[j + 1], binaries + k, address);
                }

                matched = hereMatch;
                if (nextMatch) {
                    ++j;
                    continue;
                }
                ++k;
                if (hereMatch) {
                    continue;
                }
                return false;
            } else {
                if (hereMatch) {
                    matched = true;
                } else {
                    matched = false;
                    break;
                }
                ++j;
                ++k;
            }
        }
        if (matched) {
            return true;
        }
    }
    return false;
}
}
