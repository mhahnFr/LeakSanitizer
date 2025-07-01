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

#ifndef PseudoTracker_hpp
#define PseudoTracker_hpp

#include "ATracker.hpp"

#include "../lsanMisc.hpp"

namespace lsan::trackers {
class PseudoTracker final: public ATracker {
    LSan& main = getInstance();

protected:
    inline void maybeAddToStats(const MallocInfo& info) override {
        infos.erase(info.getPointer());
        main.addMalloc(MallocInfo(info));
    }

public:
    inline PseudoTracker() {
        main.registerTracker(this);
    }

    inline ~PseudoTracker() override {
        main.deregisterTracker(this);
    }

    inline auto removeMalloc(void* pointer) -> std::pair<bool, std::optional<MallocInfo::CRef>> override {
        return main.removeMalloc(pointer);
    }

    inline void changeMalloc(MallocInfo&& info) override {
        main.changeMalloc(std::move(info));
    }

    inline void finish() override {}

    inline auto maybeRemoveMalloc(void* pointer) -> std::pair<bool, std::optional<MallocInfo::CRef>> override {
        return main.maybeRemoveMalloc(pointer);
    }

    inline auto maybeChangeMalloc(const MallocInfo& info) -> bool override {
        return main.maybeChangeMalloc(info);
    }
};
}

#endif /* PseudoTracker_hpp */
