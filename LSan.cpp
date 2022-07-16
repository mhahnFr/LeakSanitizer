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

void LSan::removeMalloc(const MallocInfo & mInfo) {
    infos.remove(mInfo);
}
