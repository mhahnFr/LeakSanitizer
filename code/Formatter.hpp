/*
 * LeakSanitizer - A small library showing informations about lost memory.
 *
 * Copyright (C) 2022  mhahnFr and contributors
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
     * The returned string might be empty if the `__lsan_printFormatted` is
     * set to `false`.
     *
     * @param style the requested style
     * @return the corresponding escape code
     */
    auto get(Style style)   -> std::string;

    /**
     * @brief Returns an ANSI esape code to clear the given style.
     *
     * The returned string might be empty if the `__lsan_printFormatted` is
     * set to `false`.
     *
     * @param style the style to clear
     * @return the corresponding escape code
     */
    auto clear(Style style) -> std::string;

    /**
     * Returns an ANSI escape code to clear all possible styles.
     *
     * @return the corresponding escape code
     */
    auto clearAll()         -> std::string;
}

#endif /* Formatter_hpp */
