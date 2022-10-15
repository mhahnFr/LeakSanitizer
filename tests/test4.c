#include "include/lsan_stats.h"
#include "include/lsan_internals.h"
#include "CallstackLibrary/include/callstack.h"

#include <stdlib.h>
int main() {
    __lsan_trackMemory = true;
    void * a = malloc(50);
    a = malloc(50);
    free(a);
    a = malloc(150);
    __lsan_printFragmentationStats();
    struct callstack * cs = callstack_new();
    callstack_delete(cs);
}
