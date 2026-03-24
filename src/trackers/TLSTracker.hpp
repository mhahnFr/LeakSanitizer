/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2024 - 2026  mhahnFr
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

#include "ATracker.hpp"

namespace lsan::trackers {
/**
 * This class represents a thread-local allocation tracker.
 */
class TLSTracker final: public ATracker {
    /** Indicates whether the tracking has finished. */
    bool finished = false;

public:
    TLSTracker();
   ~TLSTracker() override;

    auto removeMalloc(void* pointer) -> std::pair<bool, std::optional<MallocInfo::CRef>> override;
    void changeMalloc(MallocInfo&& info) override;

    auto maybeChangeMalloc(const MallocInfo& info) -> bool override;

    auto maybeRemoveMalloc(void* pointer) -> std::pair<bool, std::optional<MallocInfo::CRef>> override;

    void finish() override;
};
}

#endif /* TLSTracker_hpp */
