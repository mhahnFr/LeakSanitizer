//
//  leaksan.h
//  LeakSanitizer
//
//  Created by Manuel Hahn on 16.07.22.
//  Copyright © 2022 mhahnFr. All rights reserved.
//

#ifndef leaksan_h
#define leaksan_h

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

void * __wrap_malloc(size_t, const char *, int);
void   __wrap_free(void *, const char *, int);
void   __wrap_exit(int, const char *, int);

#ifdef __cplusplus
} // extern "C"
#endif

#define malloc(size)  __wrap_malloc(size, __FILE__, __LINE__)
#define free(pointer) __wrap_free(pointer, __FILE__, __LINE__)
#define exit(code)    __wrap_exit(code, __FILE__, __LINE__)

#endif /* leaksan_h */
