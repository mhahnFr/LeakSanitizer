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

#ifndef formatter_hpp
#define formatter_hpp

#include <string>

#include <lsan_internals.h>

#include "lsanMisc.hpp"

/**
 * A namespace containing helper functions and classes for formatting
 * output on terminals.
 */
namespace lsan::formatter {
/**
 * An enumeration containing possible formats.
 */
enum class Style {
    /** Represents the text color green.                                              */
    GREEN,
    /** Represents the text color red.                                                */
    RED,
    /** Represents the text color magenta.                                            */
    MAGENTA,
    AMBER,
    BLUE,
    CYAN,
    /** Represents italic text.                                                       */
    ITALIC,
    /** Represents underlined text.                                                   */
    UNDERLINED,
    /** Represents greyed color, that is, the current text color becomes less bright. */
    GREYED,
    /** Represents bold text.                                                         */
    BOLD,
    /** Represents a filled element inside a bar.                                     */
    BAR_FILLED,
    /** Represents an empty element inside a bar.                                     */
    BAR_EMPTY,
    /** Represents no style.                                                          */
    NONE
};
    
/**
 * @brief Returns an ANSI escape code for the requested style.
 *
 * The returned string might be empty if `__lsan_printFormatted` is
 * set to `false`.
 *
 * @return the corresponding escape code
 * @tparam S the requested style
 */
template<Style S>
constexpr inline auto get() -> const char * {
    if (!printFormatted()) {
        switch (S) {
            case Style::BAR_EMPTY:  return ".";
            case Style::BAR_FILLED: return "=";
            default:
                return "";
        }
    }
    switch (S) {
        case Style::BAR_EMPTY:  return " ";
        case Style::BAR_FILLED: return "*";
        case Style::BOLD:       return "\033[1m";
        case Style::GREEN:      return "\033[32m";
        case Style::GREYED:     return "\033[2m";
        case Style::ITALIC:     return "\033[3m";
        case Style::MAGENTA:    return "\033[95m";
        case Style::RED:        return "\033[31m";
        case Style::CYAN:       return "\033[36m";
        case Style::BLUE:       return "\033[94m";
        case Style::AMBER:      return "\033[33m";
        case Style::UNDERLINED: return "\033[4m";
        default:
            return "";
    }
}

/**
 * @brief Returns an ANSI escape code to clear the given style.
 *
 * The returned string might be empty if `__lsan_printFormatted` is
 * set to `false`.
 *
 * @return the corresponding escape code
 * @tparam S the style to clear
 */
template<Style S>
constexpr inline auto clear() -> const char * {
    if (!printFormatted()) {
        return "";
    }
    switch (S) {
        case Style::RED:
        case Style::GREEN:
        case Style::BLUE:
        case Style::CYAN:
        case Style::AMBER:
        case Style::MAGENTA:    return "\033[39m";
            
        case Style::BOLD:
        case Style::GREYED:     return "\033[22m";
            
        case Style::ITALIC:     return "\033[23m";
            
        case Style::UNDERLINED: return "\033[24m";
            
        default:
            return "";
    }
}

/**
 * Returns an ANSI escape code to clear all possible styles.
 *
 * @return the corresponding escape code
 */
constexpr inline auto clearAll() -> const char * {
    return "\033[0m";
}

/**
 * Returns a string with the formatting codes for the given styles.
 *
 * @return the string with the format codes
 * @tparam S the requested styles
 */
template<Style... S>
inline auto getString() -> std::string {
    std::string toReturn {};
    ((toReturn += get<S>()), ...);
    return toReturn;
}

/**
 * Sends the format codes of the given styles to the given output stream.
 *
 * @param out the output stream
 * @return the output stream
 * @tparam S the requested styles
 */
template<Style... S>
inline auto get(std::ostream & out) -> std::ostream & {
    ((out << get<S>()), ...);
    return out;
}

/**
 * Returns a string with the clearing codes for the given styles.
 *
 * @return the string with the clear codes
 * @tparam S the requested styles
 */
template<Style... S>
inline auto clearString() -> std::string {
    std::string toReturn {};
    ((toReturn += clear<S>()), ...);
    return toReturn;
}

/**
 * Sends the clear codes of the given styles to the given output stream.
 *
 * @param out the output stream
 * @return the output stream
 * @tparam S the requested styles
 */
template<Style... S>
inline auto clear(std::ostream & out) -> std::ostream & {
    ((out << clear<S>()), ...);
    return out;
}

/**
 * Formats the given string with the given styles onto the given output stream.
 *
 * @param out the output stream to print to
 * @param str the string to be formatted
 * @return the output stream
 * @tparam S the requested styles
 */
template<Style... S>
inline auto get(std::ostream & out, const std::string & str) -> std::ostream & {
    out << get<S...>
        << str
        << clear<S...>;
    return out;
}

/**
 * Formats the given string using the given styles.
 *
 * @tparam S the requested styles
 */
template<Style... S>
struct format {
    const std::string & str;
    
    format(const std::string & str): str(str) {}
};

template<Style... S>
auto operator<<(std::ostream & out, const format<S...> & f) -> std::ostream & {
    out << get<S...> << f.str << clear<S...>;
    return out;
}

/**
 * Returns a string containing the format and clear codes for the given styles
 * and the given string.
 *
 * @param str the string to be formatted
 * @return a new string with the format, the given string and the clear codes
 * @tparam S the requested styles
 */
template<Style... S>
inline auto formatString(const std::string & str) -> std::string {
    return getString<S...>() + str + clearString<S...>();
}
}

#endif /* formatter_hpp */
