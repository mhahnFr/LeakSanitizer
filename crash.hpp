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

[[ noreturn ]] void crash(const std::string &, const char *, int, int = 3);
[[ noreturn ]] void crash(const std::string &, int = 3);

#endif /* crash_hpp */
