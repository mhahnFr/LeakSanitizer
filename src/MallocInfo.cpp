/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2022 - 2023  mhahnFr
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

#include "MallocInfo.hpp"

#include "LeakSani.hpp"
#include "formatter.hpp"
#include "bytePrinter.hpp"

#include "../include/lsan_internals.h"

namespace lsan {
auto operator<<(std::ostream & stream, const MallocInfo & self) -> std::ostream & {
    using formatter::Style;
    
    stream << formatter::get<Style::ITALIC>
           << formatter::format<Style::BOLD, Style::RED>("Leak") << " of size "
           << formatter::clear<Style::ITALIC>
           << bytesToString(self.size) << formatter::get<Style::ITALIC> << ", ";
    if (self.createdInFile.has_value() && self.createdOnLine.has_value()) {
        stream << "allocated at " << formatter::format<Style::UNDERLINED>(self.createdInFile.value() + ":" + std::to_string(self.createdOnLine.value()));
    } else {
        stream << "allocation stacktrace:";
    }
    stream << std::endl;
    self.printCreatedCallstack(stream);
    return stream;
}
}
