//
//  LSan.cpp
//  LeakSanitizer
//
//  Created by Manuel Hahn on 16.07.22.
//  Copyright © 2022 mhahnFr. All rights reserved.
//

#include "LSan.hpp"

LSan LSan::instance;

LSan & LSan::getInstance() {
    return instance;
}

void LSan::addMalloc(const MallocInfo && mInfo) {
    infos.push_back(mInfo);
}

bool LSan::removeMalloc(const MallocInfo & mInfo) {
    if (std::find(infos.cbegin(), infos.cend(), mInfo) == infos.cend()) {
        return false;
    }
    infos.remove(mInfo);
    return true;
}

std::ostream & operator<<(std::ostream & stream, const LSan & self) {
    if (!self.infos.empty()) {
        stream << "\033[3m";
        for (const auto & leak : self.infos) {
            stream << "\033[1;31mLeak\033[22;39m of size " << leak.getSize() << ", allocated at \033[4m" << leak.getCreatedInFile() << ":" << leak.getCreatedOnLine() << "\033[24m" << std::endl;
        }
        stream << "\033[23m";
    }
    return stream;
}
