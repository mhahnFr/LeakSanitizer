/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2023 - 2026  mhahnFr
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

#ifndef callstackHelper_format_hpp
#define callstackHelper_format_hpp

#include <callstack.h>
#include <string>

#include "../formatter/formatter.hpp"

/** This namespace includes the helper functions for the callstacks. */
namespace lsan::callstackHelper {
/**
 * Formats the given callstack onto the given output stream.
 *
 * @param callstack the callstack
 * @param stream the stream to print to
 * @param indent the leading indentation to be used
 */
[[nodiscard]] auto format(lcs::callstack& callstack, std::ostream& stream, const std::string& indent = "") -> bool;

/**
 * Formats the given callstack onto the given output stream.
 *
 * @param callstack the callstack
 * @param out the stream to print to
 * @param indent the leading indentation to be used
 */
[[nodiscard]] static inline auto format(lcs::callstack&& callstack, std::ostream& out, const std::string& indent = "") -> bool {
    return format(callstack, out, indent);
}

/**
 * @brief Returns the name of the binary file of the given callstack frame.
 *
 * The file name is allowed to be a relative if relative paths are activated.
 *
 * @param frame the callstack frame
 * @return the name of the binary file of the given callstack frame
 */
static inline auto getCallstackFrameName(const callstack_frame & frame) -> std::string {
    [[unlikely]] if (frame.binaryFile == nullptr) {
        return "<< Unknown >>";
    }

    return behaviour::getBehaviour().relativePaths() ? callstack_frame_getShortestName(&frame) : frame.binaryFile;
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
    return behaviour::getBehaviour().relativePaths() ? callstack_frame_getShortestSourceFile(&frame) : frame.sourceFile;
}

/**
 * Formats the given callstack frame onto the given output stream using the
 * given style.
 *
 * @param frame the callstack frame to be formatted
 * @param out the output stream
 * @param singleLine whether the frame info should be printed onto a single line
 * @tparam S the style to be used
 */
template<formatter::Style S = formatter::Style::NONE>
static inline void formatFrame(const callstack_frame& frame, std::ostream& out, const bool singleLine = false) {
    using namespace formatter;

    if (const auto willPrintMore = frame.sourceFile != nullptr || (singleLine ? frame.function != nullptr : true);
        behaviour::getBehaviour().printBinaries() || !willPrintMore) {
        bool reset = false;
        if constexpr (S == Style::GREYED || S == Style::BOLD) {
            reset = true;
        }
        out << formatter::format<Style::ITALIC>("(" + formatString<Style::BLUE>(getCallstackFrameName(frame)) + ")")
            << (reset ? get<S>() : "") << (willPrintMore ? ":" : "") << " ";
    }
    bool needsBrackets = false;
    if (frame.sourceFile == nullptr || behaviour::getBehaviour().printFunctions()) {
        if (!(frame.function == nullptr && singleLine)) {
            out << (frame.function == nullptr ? "<< Unknown >>" : frame.function);
            needsBrackets = true;
        }
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
    out << clear<S>;
    if (!singleLine) {
        out << std::endl;
    }
}
}

#endif /* callstackHelper_format_hpp */
