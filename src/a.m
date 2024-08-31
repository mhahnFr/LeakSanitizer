//
//  a.cpp
//  LeakSanitizer
//
//  Created by mhahnFr on 21.08.24.
//  Copyright © 2024 mhahnFr. All rights reserved.
//

#include <Foundation/NSObject.h>

#include <objc/objc.h>
#include <objc/runtime.h>
#include <dlfcn.h>

static IMP oldAlloc, oldDealloc, oldAllocZone;

static id _alloc(id self, SEL s) {
    __builtin_printf("A\n");
    return ((id(*)(id, SEL)) oldAlloc)(self, s);
}

static void _dealloc(id self, SEL s) {
    __builtin_printf("B\n");
    return ((void(*)(id, SEL)) oldDealloc)(self, s);
}

static id _allocZone(id self, SEL s, id zone) {
    __builtin_printf("C\n");
    return ((id(*)(id, SEL, id)) oldAllocZone)(self, s, zone);
}

void init(void) __attribute__((constructor));

@class SwiftObject;

void DumpObjcMethods(Class clz) {

    unsigned int methodCount = 0;
    Method *methods = class_copyMethodList(clz, &methodCount);

    __builtin_printf("Found %d methods on '%s'\n", methodCount, class_getName(clz));

    for (unsigned int i = 0; i < methodCount; i++) {
        Method method = methods[i];

        __builtin_printf("\t'%s' has method named '%s' of encoding '%s'\n",
               class_getName(clz),
               sel_getName(method_getName(method)),
               method_getTypeEncoding(method));
    }

    free(methods);
}

//extern void* (*_swift_allocObject)(void*, size_t, size_t);

/*
 SWIFT_RUNTIME_EXPORT
 HeapObject *(*SWIFT_RT_DECLARE_ENTRY _swift_allocObject)(
     HeapMetadata const *metadata, size_t requiredSize,
     size_t requiredAlignmentMask) = _swift_allocObject_;
 */

void swizzle(Class c) {
    Method dealloc = class_getInstanceMethod(c, @selector(dealloc));
    Method alloc = class_getClassMethod(c, @selector(alloc));
    Method allocZone = class_getClassMethod(c, @selector(allocWithZone:));

    oldAlloc = method_setImplementation(alloc, (IMP) _alloc);
    oldDealloc = method_setImplementation(dealloc, (IMP) _dealloc);
    oldAllocZone = method_setImplementation(allocZone, (IMP) _allocZone);
    NSLog(@"d: %p a: %p az: %p", _dealloc, _alloc, _allocZone);

    NSLog(@"%p", method_getImplementation(alloc));

    DumpObjcMethods(c);
}

void* (*orig)(void*, size_t, size_t) = NULL;
void* myImpl(void*a, size_t b, size_t c) {
    __builtin_printf("Swift alloc: %zu\n", b);
    return orig(a, b, c);
}
void init(void) {
//    NSLog(@"%p", _swift_allocObject);
    id c = objc_getClass("NSObject");
    id cc = objc_getClass("_TtCs12_SwiftObject");
    void*(**sao)(void*, size_t, size_t) = dlsym(RTLD_DEFAULT, "_swift_allocObject");
    orig = *sao;
    *sao = myImpl;
    NSLog(@"Arsh: %p", (*sao)(NULL, 0, 0));
    swizzle(cc);
}
