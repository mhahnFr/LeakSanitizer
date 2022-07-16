//
//  crash.hpp
//  LeakSanitizer
//
//  Created by Manuel Hahn on 16.07.22.
//  Copyright © 2022 mhahnFr. All rights reserved.
//

#ifndef crash_hpp
#define crash_hpp

#include <string>

[[ noreturn ]] void crash(const std::string &, const char *, int);
[[ noreturn ]] void crash(const std::string &);

#endif /* crash_hpp */
