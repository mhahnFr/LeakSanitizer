/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2022 - 2023  mhahnFr and contributors
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

#ifndef Formatter_hpp
#define Formatter_hpp

#include <string>

#include "../include/lsan_internals.h"

/**
 * A namespace containing helper functions and classes for formatting
 * output on terminals.
 */
namespace Formatter {
/// An enumeration containing possible formats.
enum class Style {
    /// Represents the text color green.
    GREEN,
    /// Represents the text color red.
    RED,
    /// Represents the text color magenta.
    MAGENTA,
    /// Represents *italic* text.
    ITALIC,
    /// Represents underlined text.
    UNDERLINED,
    /// Represents greyed color, that is, the current text color becomes less bright.
    GREYED,
    /// Represents **bold** text.
    BOLD,
    /// Represents a filled element inside a bar.
    BAR_FILLED,
    /// Represents an empty element inside a bar.
    BAR_EMPTY
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
    if (!__lsan_printFormatted) {
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
        case Style::UNDERLINED: return "\033[4m";
        default:
            return "";
    }
}

/**
 * @brief Returns an ANSI esape code to clear the given style.
 *
 * The returned string might be empty if `__lsan_printFormatted` is
 * set to `false`.
 *
 * @return the corresponding escape code
 * @tparam S the style to clear
 */
template<Style S>
constexpr inline auto clear() -> const char * {
    if (!__lsan_printFormatted) {
        return "";
    }
    switch (S) {
        case Style::RED:
        case Style::GREEN:
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

template<Style... S>
inline auto getString() -> std::string {
    std::string toReturn {};
    ((toReturn += get<S>()), ...);
    return toReturn;
}

template<Style... S>
inline auto get(std::ostream & out) -> std::ostream & {
    ((out << get<S>()), ...);
    return out;
}

template<Style... S>
inline auto clearString() -> std::string {
    std::string toReturn {};
    ((toReturn += clear<S>()), ...);
    return toReturn;
}

template<Style... S>
inline auto clear(std::ostream & out) -> std::ostream & {
    ((out << clear<S>()), ...);
    return out;
}

template<Style... S>
inline auto get(std::ostream & out, const std::string & str) -> std::ostream & {
    out << get<S...>
        << str
        << clear<S...>;
    return out;
}

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

template<Style... S>
inline auto formatString(const std::string & str) -> std::string {
    return getString<S...>() + str + clearString<S...>();
}
}

#endif /* Formatter_hpp */
