#include <stdlib.h>
#include <stdio.h>
#include <lsan_stats.h>
#include <lsan_internals.h>

int main() {
    __lsan_trackMemory = true;
	void * a = malloc(10);
	printf("%p\n", a);
	free(a);
	a = malloc(100);
	printf("%p\n", a);
	a = malloc(20);
	printf("%p\n", a);
	free(a);
	__lsan_printFragmentationStats();
}
