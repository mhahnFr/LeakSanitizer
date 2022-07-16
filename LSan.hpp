//
//  LSan.hpp
//  LeakSanitizer
//
//  Created by Manuel Hahn on 16.07.22.
//  Copyright © 2022 mhahnFr. All rights reserved.
//

#ifndef LSan_hpp
#define LSan_hpp

#include <list>
#include "MallocInfo.hpp"

class LSan {
    static LSan instance;
    
    std::list<MallocInfo> infos;
    
public:
    void addMalloc(const MallocInfo &&);
    void removeMalloc(const MallocInfo &);
    
    static LSan & getInstance();
};

#endif /* LSan_hpp */
