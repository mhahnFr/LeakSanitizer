#include <thread>
#include <mutex>
#include <vector>
#include <iostream>

#include "include/lsan_stats.h"
#include "include/lsan_internals.h"

volatile bool run = true;

void s(int) {
    run = false;
}

void a() {
    while (run) {
        char * a = (char*) malloc(1);
        *a = 0;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        free(a);
    }
}

void b() {
    while (run) {
        char * a = (char*) malloc(10);
        *a = 0;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        free(a);
    }
}

#define NUM 210

int main() {
    __lsan_trackMemory = true;
    void * c = malloc(1);
    free(c);
    atexit(NULL);
    signal(SIGINT, s);
    std::vector<std::thread *> ts;
    for (int i = 0; i < NUM; ++i) {
        ts.push_back(new std::thread((i % 2 ? a : b)));
    }
    while (run) {
        std::cout << "-----------------" << std::endl;
        __lsan_printStats();
        __lsan_printFStats();
        std::cout << "-----------------" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    for (int i = 0; i < NUM; ++i) {
        ts[i]->join();
        delete ts[i];
    }
    __lsan_printStats();
    __lsan_printFStats();
}
