# Welcome to the LeakSanitizer!
_TODO: What's that?_

## Usage
### Installation
Get started by either downloading a prebuilt version of this sanitizer here.
Alternatively you can also build it from source:
1. Clone the repository: `git clone --recursive https://github.com/mhahnFr/LeakSanitizer.git`
2. go into the cloned repository: `cd LeakSanitizer`
3. and build the library: `make`.

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

### How to use

_Leaks_

### Behaviour
Since version 1.6 the behaviour of this sanitizer can be adjusted by setting the 
following environment variables to their indicated values:

| Name                           | Description                                           | Values            | Default value |
|--------------------------------|-------------------------------------------------------|-------------------|---------------|
| `LSAN_HUMAN_PRINT`             | Print human-readably formatted                        | `true`, `false`   | `true`        |
| `LSAN_PRINT_COUT`              | Print to the default output stream                    | `true`, `false`   | `false`       |
| `LSAN_PRINT_FORMATTED`         | Print using ANSI escape codes                         | `true`, `false`   | `true`        |
| `LSAN_PRINT_LICENSE`           | Print the license notice on exit                      | `true`, `false`   | `true`        |
| `LSAN_PRINT_WEBSITE`           | Print the link to the website                         | `true`, `false`   | `true`        |
| `LSAN_INVALID_CRASH`           | Terminate if an invalid action is detected            | `true`, `false`   | `true`        |
| `LSAN_INVALID_FREE`            | Detect invalid de-allocations                         | `true`, `false`   | `false`       |
| `LSAN_FREE_NULL`               | Issue a warning if `NULL` is `free`d                  | `true`, `false`   | `false`       |
| `LSAN_STATS_ACTIVE`            | Enable the statistical bookkeeping                    | `true`, `false`   | `false`       |
| `LSAN_LEAK_COUNT`              | The amount of leaks to be printed in detail           | `0` to `SIZE_MAX` | `100`         |
| `LSAN_CALLSTACK_SIZE`          | The amount of frames to be printed in a callstack     | `0` to `SIZE_MAX` | `20`          |
| `LSAN_FIRST_PARTY_THRESHOLD`   | **Since v1.7:** The amount of first party frames      | `0` to `SIZE_MAX` | `3`           |
| `LSAN_PRINT_EXIT_POINT`        | **Since v1.7:** Print the callstack of the exit point | `true`, `false`   | `false`       |
| ~~`LSAN_PRINT_STATS_ON_EXIT`~~ | **Deprecated** since v1.7, will be removed in v2      | `true`, `false`   | `false`       |

More on the environment variables here.

### Signals
This sanitizer comes with handlers for the following signals:

| Signal  | Action                                                                                |
|---------|---------------------------------------------------------------------------------------|
| SIGSEGV | Printing the callstack of the crash.                                                  |
| SIGBUS  | Printing the callstack of the crash.                                                  |
| SIGUSR1 | Printing the statistics if enabled using `LSAN_STATS_ACTIVE` or `__lsan_statsActive`. |
| SIGUSR2 | Printing the current callstack.                                                       |

More on the signal handlers here.

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

## Behind the scenes or: How does it work?
In order to track the memory allocations this sanitizer replaces the four allocation management functions
`malloc`, `calloc`, `realloc` and `free`. Every allocation and de-allocation is registered and a backtrace
is stored for it.  
Its own allocations are not tracked.

The signal handlers and the wrapper functions are installed once the sanitizer has been loaded by the
dynamic loader.

When the exit handler registered using `atexit` is invoked the allocated memory is examined and
the detected memory leaks are printed.  
The backtraces are translated using the CallstackLibrary.

## Final notes
This project is licensed under the terms of the GPL 3.0.

© Copyright 2022 - 2023 [mhahnFr][1] and contributors

[1]: https://github.com/mhahnFr
