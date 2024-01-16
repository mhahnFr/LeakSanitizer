# Welcome to the LeakSanitizer!
The LeakSanitizer is a tool designed to debug memory leaks.

It can be used with almost any programming language that compiles to native
machine code.  
Officially supported are currently: **C**, **C++**, **Objective-C**, **Swift**.

> **Note** for Swift and Objective-C:
> 
> Objective-C and Swift objects are never considered to become memory leaks - 
> even in the case of strong reference cycles.

This sanitizer has been optimized for both **macOS** and **Linux** - all memory leaks are
detected on both platforms.

## Quickstart
Use the LeakSanitizer to check for memory leaks as follows:
1. Clone the repository: `git clone --recursive https://github.com/mhahnFr/LeakSanitizer.git`
2. Build it: `cd LeakSanitizer && make`
3. Link your code with: `-L<path/to/library> -llsan`

> **Update** the LeakSanitizer using:
> 
> ```shell
> make update
> ```

More explanation can be found in the [wiki][7]; the detailed explanation follows below.

## Usage
### Installation
Get started by either downloading a prebuilt version of this sanitizer [here][1].
Alternatively you can also build it from source:
1. Clone the repository: `git clone --recursive https://github.com/mhahnFr/LeakSanitizer.git`
2. go into the cloned repository: `cd LeakSanitizer`
3. and build the library: `make`

Or in one step:
```shell
git clone --recursive https://github.com/mhahnFr/LeakSanitizer.git && cd LeakSanitizer && make
```

Once you have a copy of this sanitizer you can install it using the following command:
```shell
make INSTALL_PATH=/usr/local install
```
If you downloaded a release you can simply move the headers and the library anywhere
you like.

### Updating
Update the LeakSanitizer by either downloading the new release or, when cloned from the repository, using:
```shell
make update
```
Install it again as described [above][8].

### How to use
To use this sanitizer simply link your application against it (recommended) or preload its library.

#### Linking (_recommended_)
- Add `-L<path/to/library>` if this sanitizer has not been installed in one of the default
directories.
- On Linux add `-rdynamic` to the linking flags.
 
Link with: `-llsan`
 
> Example **macOS**: `-L<path/to/library> -llsan`

> Example **Linux**: `-rdynamic -L<path/to/library> -llsan`

#### Preloading
Add this sanitizer's library to the dynamic loader preload environment variable:
- **macOS**: `DYLD_INSERT_LIBRARIES=<path/to/library>`
> Example:
> ```shell
> DYLD_INSERT_LIBRARIES=/usr/local/lib/liblsan.dylib ./a.out
> ```
- **Linux**: `LD_PRELOAD=<path/to/library>`
> Example:
> ```shell
> LD_PRELOAD=/usr/local/lib/liblsan.so ./a.out
> ```

### Leaks
Once this sanitizer is bundled with your application the detected memory leaks are
printed upon termination.

```C
// main.c

#include <string.h>
#include <stdlib.h>

int main(void) {
    void * a = malloc(1023);
    a = strdup("Hello World!");
    a = NULL;
    a = malloc(1000);
    free(a);
}
```
Compiled and linked on macOS with `cc main.c -L<path/to/library> -llsan`
creates the following output:
```
Exiting

2 leaks total, 1.01 KiB total          

Leak of size 1023 B, allocation stacktrace:
In: main + 18
at: start + 1903

Leak of size 13 B, allocation stacktrace:
At: strdup + 32
in: main + 34
at: start + 1903


Summary: 2 leaks, 1.01 KiB lost.
```

#### Line numbers
To (partially) add line numbers, you can add `-Wno-gnu-include-next -I<path/to/library>/include` to the compiling flags
of your code.

The previous example compiled and linked on macOS with
`cc main.c -Wno-gnu-include-next -I<path/to/library>/include -L<path/to/library> -llsan` creates the following output:
```
Exiting

2 leaks total, 1.01 KiB total       

Leak of size 1023 B, allocated at main.c:7
In: main + 30
at: start + 1903

Leak of size 13 B, allocation stacktrace:
At: strdup + 32
in: main + 46
at: start + 1903


Summary: 2 leaks, 1.01 KiB lost.
```

### Behaviour
Since version 1.6 the behaviour of this sanitizer can be adjusted by setting the 
following environment variables to their indicated values:

| Name                         | Description                                                                    | Values            | Default value |
|------------------------------|--------------------------------------------------------------------------------|-------------------|---------------|
| `LSAN_HUMAN_PRINT`           | Print human-readably formatted                                                 | `true`, `false`   | `true`        |
| `LSAN_PRINT_COUT`            | Print to the default output stream                                             | `true`, `false`   | `false`       |
| `LSAN_PRINT_FORMATTED`       | Print using ANSI escape codes                                                  | `true`, `false`   | `true`        |
| `LSAN_INVALID_CRASH`         | Terminate if an invalid action is detected                                     | `true`, `false`   | `true`        |
| `LSAN_INVALID_FREE`          | Detect invalid de-allocations                                                  | `true`, `false`   | `false`       |
| `LSAN_FREE_NULL`             | Issue a warning if `NULL` is `free`d                                           | `true`, `false`   | `false`       |
| `LSAN_STATS_ACTIVE`          | Enable the statistical bookkeeping                                             | `true`, `false`   | `false`       |
| `LSAN_LEAK_COUNT`            | The amount of leaks to be printed in detail                                    | `0` to `SIZE_MAX` | `100`         |
| `LSAN_CALLSTACK_SIZE`        | The amount of frames to be printed in a callstack                              | `0` to `SIZE_MAX` | `20`          |
| `LSAN_FIRST_PARTY_THRESHOLD` | **Since v1.7:** The amount of first party frames                               | `0` to `SIZE_MAX` | `3`           |
| `LSAN_PRINT_EXIT_POINT`      | **Since v1.7:** Print the callstack of the exit point                          | `true`, `false`   | `false`       |
| `LSAN_PRINT_BINARIES`        | **Since v1.8:** Print the binary file names                                    | `true`, `false`   | `true`        |
| `LSAN_PRINT_FUNCTIONS`       | **Since v1.8:** Always print the function names                                | `true`, `false`   | `true`        |
| `LSAN_RELATIVE_PATHS`        | **Since v1.8:** Allow relative paths to be printed                             | `true`, `false`   | `true`        |
| `LSAN_ZERO_ALLOCATION`       | **Since v1.8:** Issue a warning when `0` byte are allocated                    | `true`, `false`   | `false`       |
| `LSAN_FIRST_PARTY_REGEX`     | **Since v1.8:** Binary files matching this regex are considered "first party". | *Any regex*       | *None*        |
| `LSAN_PRINT_LICENSE`         | *Deprecated in v1.8:* Print the license notice on exit                         | `true`, `false`   | `true`        |
| `LSAN_PRINT_WEBSITE`         | *Deprecated in v1.8:* Print the link to the website                            | `true`, `false`   | `true`        |

More on the environment variables [here][2].

### Signals
This sanitizer comes with handlers for the following signals:

| Signal               | Action                                                                                |
|----------------------|---------------------------------------------------------------------------------------|
| `SIGSEGV` & `SIGBUS` | Printing the callstack of the crash.                                                  |
| `SIGUSR1`            | Printing the statistics if enabled using `LSAN_STATS_ACTIVE` or `__lsan_statsActive`. |
| `SIGUSR2`            | Printing the current callstack.                                                       |

More on the signal handlers [here][3].

### Statistics
The statistics of the tracked memory can be queried at runtime. To do so activate the statistical
bookkeeping by setting either the environment variable `LSAN_STATS_ACTIVE` or the variable `__lsan_statsActive`
to `true`.  
The statistics then can be queried using the following API:

| Function                         | Description                                                                              |
|----------------------------------|------------------------------------------------------------------------------------------|
| `__lsan_getTotalMallocs()`       | Returns the total count of allocations registered.                                       |
| `__lsan_getTotalBytes()`         | Returns the total count of allocated bytes.                                              |
| `__lsan_getTotalFrees()`         | Returns the total count of registered and `free`d objects.                               |
| `__lsan_getCurrentMallocCount()` | Returns the count of currently active allocations.                                       |
| `__lsan_getCurrentByteCount()`   | Returns the amount of currently allocated bytes.                                         |
| `__lsan_getMallocPeek()`         | Returns the highest amount of allocations at the same time.                              |
| `__lsan_getBytePeek()`           | Returns the highest amount of bytes allocated at the same time.                          |
| `__lsan_printStats()`            | Prints the statistics to the output stream specified by `LSAN_PRINT_COUT`.               |
| `__lsan_printFStats()`           | Prints the fragmentation statistics to the output stream specified by `LSAN_PRINT_COUT`. |

More on the statistics [here][4].

## Behind the scenes or: How does it work?
In order to track the memory allocations this sanitizer replaces the four allocation management functions
`malloc`, `calloc`, `realloc` and `free`. Every allocation and de-allocation is registered and a backtrace
is stored for it.  
Its own allocations are not tracked.

The signal handlers and the wrapper functions are installed once the sanitizer has been loaded by the
dynamic loader.

When the exit handler registered using `atexit` is invoked the allocated memory is examined and
the detected memory leaks are printed.  
The backtraces are translated using the [CallstackLibrary][5].

## Final notes
This project is licensed under the terms of the GPL 3.0.

© Copyright 2022 - 2024 [mhahnFr][6] and contributors

[1]: https://github.com/mhahnFr/LeakSanitizer/releases
[2]: https://github.com/mhahnFr/LeakSanitizer/wiki/Behaviour
[3]: https://github.com/mhahnFr/LeakSanitizer/wiki/Signal-handlers
[4]: https://github.com/mhahnFr/LeakSanitizer/wiki/Home#Statistics
[5]: https://github.com/mhahnFr/CallstackLibrary
[6]: https://github.com/mhahnFr
[7]: https://github.com/mhahnFr/LeakSanitizer/wiki
[8]: #installation
