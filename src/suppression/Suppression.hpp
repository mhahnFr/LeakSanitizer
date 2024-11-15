/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2024  mhahnFr
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

#ifndef Suppression_hpp
#define Suppression_hpp

#include <optional>
#include <utility>
#include <string>
#include <vector>

#include "json/Object.hpp"

namespace lsan::suppression {
/*
 TODO: Once the reachbility is available, add the following information to this class:
 - leak type
 - runtime image name where the leak is found (optional)
 */
struct Suppression {
    std::string name;
    std::optional<std::size_t> size;

    std::vector<std::pair<uintptr_t, std::size_t>> topCallstack;

    Suppression(const json::Object& object);
};
}

#endif /* Suppression_hpp */
