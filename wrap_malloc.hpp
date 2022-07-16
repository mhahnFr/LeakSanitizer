//
//  wrap_malloc.hpp
//  LeakSanitizer
//
//  Created by Manuel Hahn on 16.07.22.
//  Copyright © 2022 mhahnFr. All rights reserved.
//

#ifndef wrap_malloc_h
#define wrap_malloc_h

#include "crash.hpp"
#include <cstddef>

void * __wrap_malloc(size_t, const char *, int);
void   __wrap_free(void *, const char *, int);

[[ noreturn ]] void __wrap_exit(int, const char *, int);

#endif /* wrap_malloc_h */
