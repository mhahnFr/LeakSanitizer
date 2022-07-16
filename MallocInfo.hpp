//
//  MallocInfo.hpp
//  LeakSanitizer
//
//  Created by Manuel Hahn on 16.07.22.
//  Copyright © 2022 mhahnFr. All rights reserved.
//

#ifndef MallocInfo_hpp
#define MallocInfo_hpp

#include <string>

class MallocInfo {
    const void * const pointer;
    const std::string  createdInFile;
    const int          createdOnLine;
    
    std::string        deletedInFile;
    int                deletedOnLine;
    
public:
    MallocInfo(const void * const pointer): MallocInfo(pointer, "<Unknown>", 1) {}
    MallocInfo(const void * const, const std::string &, int);
    ~MallocInfo() = default;
    
    const void * const  getPointer()        const;
    const std::string & getCreatedInFile()  const;
    int                 getCreatedOnLine()  const;
    
    const std::string & getDeletedInFile()  const;
    void                setDeletedInFile(const std::string &);
    
    int                 getDeletedOnLine()  const;
    void                setDeletedOnLine(int);
};

#endif /* MallocInfo_hpp */
