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

#ifndef LeakKindStats_hpp
#define LeakKindStats_hpp

#include <deque>

#include "../MallocInfo.hpp"

namespace lsan {
/**
 * This structure represents the statistics for the different kind of leaks.
 */
struct LeakKindStats {
    /** The amount of leaks on the stack.                                */
    std::size_t stack          = 0,
    /** The amount of leaks found via leaks on the stack.                */
                stackIndirect  = 0,
    /** The amount of leaks in global space.                             */
                global         = 0,
    /** The amount of leaks found via global leaks.                      */
                globalIndirect = 0,
    /** The amount of thread-local leaks.                                */
                tlv            = 0,
    /** The amount of leaks found via thread-local leaks.                */
                tlvIndirect    = 0,
    /** The amount of root leaks.                                        */
                lost           = 0,
    /** The amount of leaks found via lost leaks.                        */
                lostIndirect   = 0;

    /** The count of bytes found on the stack.                           */
    std::size_t bytesStack          = 0,
    /** The count of bytes found via the stack.                          */
                bytesStackIndirect  = 0,
    /** The count of bytes found in global space.                        */
                bytesGlobal         = 0,
    /** The count of bytes found via the global space.                   */
                bytesGlobalIndirect = 0,
    /** The count of bytes found in thread-local values.                 */
                bytesTlv            = 0,
    /** The count of bytes found via thread-local values.                */
                bytesTlvIndirect    = 0,
    /** The count of bytes lost.                                         */
                bytesLost           = 0,
    /** The count of bytes found via lost leaks.                         */
                bytesLostIndirect   = 0;

    /** The allocation records found on the stack.                       */
    std::deque<MallocInfo::Ref> recordsStack;
    /** The allocation records found in the Objective-C runtime library. */
    std::deque<MallocInfo::Ref> recordsObjC;
    /** The allocation records found in global space.                    */
    std::deque<MallocInfo::Ref> recordsGlobal;
    /** The allocation records found in thread-local storage.            */
    std::deque<MallocInfo::Ref> recordsTlv;
    /** The allocation records directly lost.                            */
    std::deque<MallocInfo::Ref> recordsLost;

    /**
     * Returns the total amount of lost memory leaks.
     *
     * @return the total amount of lost leaks
     */
    [[nodiscard]] constexpr inline auto getTotalLost() const -> std::size_t {
        return lost + lostIndirect;
    }

    /**
     * Returns the total amount of reachable memory leaks.
     *
     * @return the total amount of reachable leaks
     */
    [[nodiscard]] constexpr inline auto getTotalReachable() const -> std::size_t {
        return stack + stackIndirect + global + globalIndirect + tlv + tlvIndirect;
    }

    /**
     * Returns the total amount of memory leaks.
     *
     * @return the total amount of leaks
     */
    [[nodiscard]] constexpr inline auto getTotal() const -> std::size_t {
        return getTotalLost() + getTotalReachable();
    }

    /**
     * Returns the total amount of lost bytes.
     *
     * @return the total amount of lost bytes
     */
    [[nodiscard]] constexpr inline auto getLostBytes() const -> std::size_t {
        return bytesLost + bytesLostIndirect;
    }

    /**
     * Returns the total amount of reachable bytes.
     *
     * @return the total amount of reachable bytes
     */
    [[nodiscard]] constexpr inline auto getReachableBytes() const -> std::size_t {
        return bytesStack  + bytesStackIndirect
             + bytesGlobal + bytesGlobalIndirect
             + bytesTlv    + bytesTlvIndirect;
    }

    /**
     * Returns the total amount of leaked bytes.
     *
     * @return the total amount of bytes
     */
    [[nodiscard]] constexpr inline auto getTotalBytes() const -> std::size_t {
        return getLostBytes() + getReachableBytes();
    }
};
}

#endif /* LeakKindStats_hpp */
