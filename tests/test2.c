#include <stdlib.h>
#include <lsan_stats.h>
#include <lsan_internals.h>

int main() {
    __lsan_trackMemory = true;
    void* ar[1000];
    for (size_t i = 0; i < 1000; ++i) {
        ar[i] = malloc(1);
    }
    __lsan_printFragmentationStats();
    __lsan_printStats();
    /*for (size_t i = 0; i < 25; ++i) {
        free(ar[i]);
    }
    for (size_t i = 75; i < 100; ++i) {
        free(ar[i]);
    }*/
    for (size_t i = 0; i < 1000; ++i) {
        if (i % 3 == 0) {
            free(ar[i]);
        }
    }
    __lsan_printFragmentationStats();
    __lsan_printStats();
    atexit(NULL);
}
