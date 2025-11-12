/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2025  mhahnFr
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

#include "lsanFormat.hpp"

#include <filesystem>

#if __has_include(<unistd.h>)
# include <unistd.h>

# define LSAN_HAS_UNISTD
#endif

namespace lsan {
auto has(const char* var) -> bool {
    return getenv(var) != nullptr;
}

auto isATTY() -> bool {
#ifdef LSAN_HAS_UNISTD
    return isatty(behaviour::getBehaviour().printCout() ? STDOUT_FILENO : STDERR_FILENO);
#else
    return behaviour::getBehaviour().printFormatted();
#endif
}

auto maybeHintRelativePaths(std::ostream& out) -> std::ostream& {
    if (behaviour::getBehaviour().relativePaths()) {
        out << printWorkingDirectory << std::endl;
    }
    return out;
}

auto printWorkingDirectory(std::ostream& out) -> std::ostream& {
    return out << "Working directory: " << std::filesystem::current_path().string() << std::endl;
}
}
