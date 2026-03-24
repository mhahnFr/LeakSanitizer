/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2025 - 2026  mhahnFr
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
#include <cstring>
#include <limits>
#include <optional>

#include "callstack.h"

namespace lsan::signals {
/**
 * Represents the gathered information about a caught signal.
 */
struct SignalInfo {
    /** The signal code.                             */
    int code = 0,
    /** The SI code of the signal.                   */
        siCode = 0;
    /** The faulting address.                        */
    void* faultAddress = nullptr;
    /** The callstack where the signal was received. */
    lcs::callstack callstack;

    /**
     * Serializes this instance, replacing zero bytes by a substitute.
     *
     * @param buffer     the buffer into which to serialize
     * @param bufferSize the size of the given buffer
     * @param substitute the substitute used for zero bytes
     */
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
        if (*substitute == 0) [[unlikely]] {
            throw std::runtime_error("No substitute found");
        }
        for (std::size_t i = 0; i < bufferSize; ++i) {
            if (buffer[i] == '\0') {
                buffer[i] = *substitute;
            }
        }
    }

    /**
     * Deserializes an instance from the given buffer, treating bytes matching
     * the given substitute as zero bytes.
     *
     * @param buffer     the buffer from which to deserialize
     * @param bufferSize the size of the given buffer
     * @param substitute the substitute for zero bytes
     * @return the deserialized instance
     */
    static inline auto fromBinary(const char* buffer, const std::size_t bufferSize, const char substitute) -> SignalInfo {
        auto toReturn = SignalInfo();
        for (std::size_t i = 0; i < bufferSize; ++i) {
            reinterpret_cast<char*>(&toReturn)[i] = buffer[i] == substitute ? '\0' : buffer[i];
        }
        return toReturn;
    }
};

/**
 * Creates an appropriate crash message for the given signal information.
 *
 * @param signalCode the code of the signal
 * @param siCode     the SI code of the signal
 * @param siAddr     the SI address of the signal
 * @return an appropriate crash message with an optional second line
 */
auto createCrashMessage(int signalCode, int siCode, const void* siAddr) -> std::pair<std::string, std::optional<std::string>>;
}

#endif /* SignalInfo_hpp */
