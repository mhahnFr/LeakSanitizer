//
//  signalHandlers.hpp
//  LeakSanitizer
//
//  Created by Manuel Hahn on 8/2/22.
//  Copyright © 2022 mhahnFr. All rights reserved.
//

#ifndef signalHandlers_hpp
#define signalHandlers_hpp

#include <csignal>

[[ noreturn ]] void crashHandler(int, siginfo_t *, void *);

#endif /* signalHandlers_hpp */
