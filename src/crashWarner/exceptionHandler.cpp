/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2023  mhahnFr
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

#include <exception>
#include <sstream>
#include <typeinfo>

#include "exceptionHandler.hpp"

#include "crash.hpp"

namespace lsan {
[[ noreturn ]] static inline void handleException(std::exception & exception) noexcept {
    std::stringstream stream;
    stream << "Terminating due to uncaught exception of type ";
    stream << typeid(exception).name();
    
    crashForce(stream.str());
}

[[ noreturn ]] void exceptionHandler() noexcept {
    try {
        std::rethrow_exception(std::current_exception());
    } catch (std::exception & exception) {
        handleException(exception);
    } catch (...) {
        // TODO: Implement
    }
    crashForce("Unknown uncaught exception!");
}
}
