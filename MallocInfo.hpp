//
//  MallocInfo.hpp
//  LeakSanitizer
//
//  Created by Manuel Hahn on 16.07.22.
//  Copyright © 2022 mhahnFr. All rights reserved.
//

#ifndef MallocInfo_hpp
#define MallocInfo_hpp

#include <vector>
#include <ostream>
#include <string>
#include <utility>

using namespace std::rel_ops;

class MallocInfo {
    constexpr static int CALLSTACK_SIZE = 128;
    
    MallocInfo(const void * const, size_t, const std::string &, int, int, bool);
    
    const void * const          pointer;
    const size_t                size;
    
    const std::string           createdInFile;
    const int                   createdOnLine;
    const bool                  createdSet;
    std::vector<std::string>    createdCallstack;

    std::string                 deletedInFile;
    int                         deletedOnLine;
    std::vector<std::string>    deletedCallstack;
    
    static void printCallstack(const std::vector<std::string> &, std::ostream &);
    
public:
    MallocInfo(const void * const, size_t, int = 5);
    MallocInfo(const void * const, size_t, const std::string &, int, int = 4);
    ~MallocInfo() = default;
    
    const void * const  getPointer()        const;
    const std::string & getCreatedInFile()  const;
    int                 getCreatedOnLine()  const;
    size_t              getSize()           const;
    
    const std::string & getDeletedInFile()  const;
    void                setDeletedInFile(const std::string &);
    
    int                 getDeletedOnLine()  const;
    void                setDeletedOnLine(int);
    void                generateDeletedCallstack();
    
    void                printCreatedCallstack(std::ostream &) const;
    void                printDeletedCallstack(std::ostream &) const;
    
    const std::vector<std::string> & getDeletedCallstack() const;
    const std::vector<std::string> & getCreatedCallstack() const;

    static const std::vector<std::string> createCallstack(int = 1);
    
    friend bool operator==(const MallocInfo &, const MallocInfo &);
    friend bool operator<(const MallocInfo &, const MallocInfo &);
};

#endif /* MallocInfo_hpp */
