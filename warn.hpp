//
//  warn.hpp
//  LeakSanitizer
//
//  Created by Manuel Hahn on 8/3/22.
//  Copyright © 2022 mhahnFr. All rights reserved.
//

#ifndef warn_hpp
#define warn_hpp

#include <string>

void warn(const std::string &, const char *, int, int = 3);
void warn(const std::string &, int = 3);

#endif /* warn_hpp */
