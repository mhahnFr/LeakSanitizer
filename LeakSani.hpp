//
//  LeakSani.hpp
//  LeakSanitizer
//
//  Created by Manuel Hahn on 16.07.22.
//  Copyright © 2022 mhahnFr. All rights reserved.
//

#ifndef LeakSani_hpp
#define LeakSani_hpp

#include <list>
#include <ostream>
#include "MallocInfo.hpp"

class LSan {
    static LSan * instance;
    
    std::list<MallocInfo> infos;
    
public:
    LSan();
    ~LSan() = default;
    
    LSan(const LSan &)              = delete;
    LSan(const LSan &&)             = delete;
    LSan & operator=(const LSan &)  = delete;
    LSan & operator=(const LSan &&) = delete;
    
    void addMalloc(const MallocInfo &&);
    bool removeMalloc(const MallocInfo &);
    
    size_t getTotalAllocatedBytes() const;
    
    void * (*malloc)(size_t);
    void   (*free)  (void *);
    void   (*exit)  (int);
    
    static LSan & getInstance();
    static void   __exit_hook();

    friend void           internalCleanUp();
    friend std::ostream & operator<<(std::ostream &, const LSan &);
};

void internalCleanUp();

#endif /* LeakSani_hpp */
