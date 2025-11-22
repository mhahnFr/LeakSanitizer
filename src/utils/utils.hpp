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

#ifndef utils_hpp
#define utils_hpp

#include <sstream>

#include <functionInfo/functionInfo.h>

namespace lsan::utils {
/**
 * Converts the given pointer to a string.
 *
 * @param pointer the pointer to create a string representation
 * @return the string representation
 */
static inline auto toString(const void* pointer) -> std::string {
    auto stream = std::ostringstream();
    stream << pointer;
    return stream.str();
}

/**
 * Loads the function of the given name.
 *
 * @param name the name of the function as used by the linker
 * @return the function pointer or @c nullptr if the function is not loaded
 */
static inline auto loadFunc(const char* name) -> void* {
    const auto result = functionInfo_load(name);
    return result.found ? reinterpret_cast<void*>(result.begin) : nullptr;
}

#ifdef __APPLE__
# define FUNC_NAME(name) "_" #name
#else
# define FUNC_NAME(name) #name
#endif

/**
 * Creates a variable named as the requested function, whose value is the
 * loaded function.
 *
 * @param type the signature of the function
 * @param name the name of the function
 */
#define LOAD_FUNC(type, name) const auto name = reinterpret_cast<type>(utils::loadFunc(FUNC_NAME(name)))
}

#endif /* utils_hpp */
