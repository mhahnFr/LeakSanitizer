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

#ifndef helperStructs_hpp
#define helperStructs_hpp

#include <cstddef>
#include <set>
#include <tuple>
#include <utility>

#include "MallocInfo.hpp"

namespace lsan {
/**
 * This structure represents the statistics for the different kind of leaks.
 */
struct LeakKindStats {
    /** The amount of leaks on the stack.                     */
    std::size_t stack          = 0,
    /** The amount of leaks found via leaks on the stack.     */
                stackIndirect  = 0,
                objC           = 0,
                objCIndirect   = 0,
    /** The amount of leaks in global space.                  */
                global         = 0,
    /** The amount of leaks found via global leaks.           */
                globalIndirect = 0,
    /** The amount of thread-local leaks.                     */
                tlv            = 0,
    /** The amount of leaks found via thread-local leaks.     */
                tlvIndirect    = 0,
    /** The amount of root leaks.                             */
                lost           = 0,
    /** The amount of leaks found via lost leaks.             */
                lostIndirect   = 0;

    /** The count of bytes found on the stack.                */
    std::size_t bytesStack          = 0,
    /** The count of bytes found via the stack.               */
                bytesStackIndirect  = 0,
                bytesObjC           = 0,
                bytesObjCIndirect   = 0,
    /** The count of bytes found in global space.             */
                bytesGlobal         = 0,
    /** The count of bytes found via the global space.        */
                bytesGlobalIndirect = 0,
    /** The count of bytes found in thread-local values.      */
                bytesTlv            = 0,
    /** The count of bytes found via thread-local values.     */
                bytesTlvIndirect    = 0,
    /** The count of bytes lost.                              */
                bytesLost           = 0,
    /** The count of bytes found via lost leaks.              */
                bytesLostIndirect   = 0;

    /** The allocation records found on the stack.            */
    std::set<MallocInfo*> recordsStack;
    std::set<MallocInfo*> recordsObjC;
    /** The allocation records found in global space.         */
    std::set<MallocInfo*> recordsGlobal;
    /** The allocation records found in thread-local storage. */
    std::set<MallocInfo*> recordsTlv;
    /** The allocation records directly lost.                 */
    std::set<MallocInfo*> recordsLost;

    /**
     * Returns the total amount of lost memory leaks.
     *
     * @return the total amount of lost leaks
     */
    constexpr inline auto getTotalLost() const -> std::size_t {
        return lost + lostIndirect;
    }

    /**
     * Returns the total amount of reachable memory leaks.
     *
     * @return the total amount of reachable leaks
     */
    constexpr inline auto getTotalReachable() const -> std::size_t {
        return stack + stackIndirect + global + globalIndirect + tlv + tlvIndirect + objC + objCIndirect;
    }

    /**
     * Returns the total amount of memory leaks.
     *
     * @return the total amount of leaks
     */
    constexpr inline auto getTotal() const -> std::size_t {
        return getTotalLost() + getTotalReachable();
    }

    /**
     * Returns the total amount of lost bytes.
     *
     * @return the total amount of lost bytes
     */
    constexpr inline auto getLostBytes() const -> std::size_t {
        return bytesLost + bytesLostIndirect;
    }

    /**
     * Returns the total amount of reachable bytes.
     *
     * @return the total amount of reachable bytes
     */
    constexpr inline auto getReachableBytes() const -> std::size_t {
        return bytesStack  + bytesStackIndirect
             + bytesGlobal + bytesGlobalIndirect
             + bytesTlv    + bytesTlvIndirect
             + bytesObjC   + bytesObjCIndirect;
    }

    /**
     * Returns the total amount of leaked bytes.
     *
     * @return the total amount of bytes
     */
    constexpr inline auto getTotalBytes() const -> std::size_t {
        return getLostBytes() + getReachableBytes();
    }
};

/**
 * This strcuture represents a memory region.
 */
struct Region {
    /** The begin of the region. */
    void* begin,
    /** The end of the region.   */
        * end;

    /**
     * Constructs a region defined by the given bounds.
     *
     * @param begin the begin of the region
     * @param end the end of the region
     */
    Region(void* begin, void* end): begin(begin), end(end) {}
};

using CountAndBytes = std::pair<std::size_t, std::size_t>;
using CountAndBytesAndIndirect = std::tuple<std::size_t, std::size_t, std::size_t, std::size_t>;
}

#endif /* helperStructs_hpp */
