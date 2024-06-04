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

#ifndef TLSTracker_hpp
#define TLSTracker_hpp

#include <atomic>

#include "ATracker.hpp"

namespace lsan {
class TLSTracker: public ATracker {
private:
    std::atomic_bool finished = false;

    auto maybeRemoveMalloc1(void* pointer) -> std::pair<const bool, std::optional<MallocInfo::CRef>>;

public:
    TLSTracker();
   ~TLSTracker();

    virtual auto removeMalloc(void* pointer) -> std::pair<const bool, std::optional<MallocInfo::CRef>> final override;
    virtual void changeMalloc(MallocInfo&& info) final override;

    virtual auto maybeRemoveMalloc(void* pointer) -> bool final override;
    virtual auto maybeChangeMalloc(const MallocInfo& info) -> bool final override;

    virtual void finish() final override;
};
}

#endif /* TLSTracker_hpp */
