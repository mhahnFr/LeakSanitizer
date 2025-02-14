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

#ifndef crashOrWarn_hpp
#define crashOrWarn_hpp

#include "crash.hpp"
#include "warn.hpp"

#include "../lsanMisc.hpp"

namespace lsan {
/**
 * Calls either @c crash or @c warn with the given arguments.
 *
 * @param args the arguments to forward
 * @tparam Args the type of the arguments to forward
 */
template<typename ...Args>
constexpr static inline void crashOrWarn(Args&& ...args) {
    if (getBehaviour().invalidCrash()) {
        crash(std::forward<Args>(args)...);
    } else {
        warn(std::forward<Args>(args)...);
    }
}
}

#endif /* crashOrWarn_hpp */
