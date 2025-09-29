/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2024 - 2025  mhahnFr
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

#ifndef FunctionNotFoundException_hpp
#define FunctionNotFoundException_hpp

#include <stdexcept>
#include <string>

namespace lsan::suppression {
/**
 * Represents an exception including information about the function that was
 * not found.
 */
class FunctionNotFoundException final: public std::runtime_error {
    /** The function name.    */
    const std::string functionName;
    /** The suppression name. */
    const std::string suppressionName;

public:
    /**
     * Constructs the exception using the given information.
     *
     * @param function the name of the function
     * @param suppressionName the name of the suppression
     */
    inline FunctionNotFoundException(const std::string& function, const std::string& suppressionName):
        std::runtime_error("Function '" + function + "' not found for suppression '" + suppressionName + "'"),
        functionName(function),
        suppressionName(suppressionName) {}

    /**
     * Returns the name of the function that was not found.
     *
     * @return the name of the function
     */
    constexpr inline auto getFunctionName() const -> const std::string& {
        return functionName;
    }

    /**
     * Returns the name of the suppression which caused this exception.
     *
     * @return the name of the suppression
     */
    constexpr inline auto getSuppressionName() const -> const std::string& {
        return suppressionName;
    }
};
}

#endif /* FunctionNotFoundException_hpp */
