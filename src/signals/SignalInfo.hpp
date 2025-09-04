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

#ifndef SignalInfo_hpp
#define SignalInfo_hpp

#include <algorithm>

#include "callstack.h"

namespace lsan::signals {
struct SignalInfo {
    int code = 0, siCode = 0;
    void* faultAddress = nullptr;
    lcs::callstack callstack;

    inline void toBinary(char* buffer, const std::size_t bufferSize, char* substitute) const {
        *substitute = 0;
        std::memcpy(buffer, this, bufferSize);
        for (char i = std::numeric_limits<char>::min(); i <= std::numeric_limits<char>::max(); ++i) {
            if (i == '\0') continue;

            if (const auto end = buffer + bufferSize; std::find(buffer, end, i) == end) {
                *substitute = i;
                break;
            }
        }
        if (*substitute == 0) {
            throw std::runtime_error("No substitute found");
        }
        for (std::size_t i = 0; i < bufferSize; ++i) {
            if (buffer[i] == '\0') {
                buffer[i] = *substitute;
            }
        }
    }

    static inline auto fromBinary(const char* buffer, const std::size_t bufferSize, const char substitute) -> SignalInfo {
        auto toReturn = SignalInfo();
        for (std::size_t i = 0; i < bufferSize; ++i) {
            reinterpret_cast<char*>(&toReturn)[i] = buffer[i] == substitute ? '\0' : buffer[i];
        }
        return toReturn;
    }
};

auto createCrashMessage(int signalCode, int siCode, const void* siAddr) -> std::pair<std::string, std::optional<std::string>>;
}

#endif /* SignalInfo_hpp */
