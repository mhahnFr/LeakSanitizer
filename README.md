# Welcome to the LeakSanitizer!
_TODO: What's that?_

## Usage
_Usage coming soon_  
_Download or compile_  
_How to install_

_Leaks_

### Behaviour
Since v1.6? the behaviour can be adjusted by setting the following environment variables
to their indicated values:

| Name                           | Description                                                     | Values            | Default value |
|--------------------------------|-----------------------------------------------------------------|-------------------|---------------|
| `LSAN_HUMAN_PRINT`             | Whether to print human-readably formatted                       | `true`, `false`   | `true`        |
| `LSAN_PRINT_COUT`              | Whether to print to the default output stream                   | `true`, `false`   | `false`       |
| `LSAN_PRINT_FORMATTED`         | Whether to print using ANSI escape codes                        | `true`, `false`   | `true`        |
| `LSAN_PRINT_LICENSE`           | Whether to print the license notice on exit                     | `true`, `false`   | `true`        |
| `LSAN_PRINT_WEBSITE`           | Whether to print to print the link to the website               | `true`, `false`   | `true`        |
| `LSAN_INVALID_CRASH`           | Whether to terminate if an invalid action is detected           | `true`, `false`   | `true`        |
| `LSAN_INVALID_FREE`            | Whether to detect invalid de-allocations                        | `true`, `false`   | `false`       |
| `LSAN_FREE_NULL`               | Whether to issue a warning if `NULL` is `free`d                 | `true`, `false`   | `false`       |
| `LSAN_STATS_ACTIVE`            | Whether to enable the statistical bookkeeping                   | `true`, `false`   | `false`       |
| `LSAN_LEAK_COUNT`              | The amount of leaks to be printed in detail                     | `0` to `SIZE_MAX` | `100`         |
| `LSAN_CALLSTACK_SIZE`          | The amount of frames to be printed in a callstack               | `0` to `SIZE_MAX` | `20`          |
| `LSAN_FIRST_PARTY_THRESHOLD`   | **since v1.7** The amount of first party frames                 | `0` to `SIZE_MAX` | `3`           |
| `LSAN_PRINT_EXIT_POINT`        | **since v1.7** Whether to print the callstack of the exit point | `true`, `false`   | `false`       |
| ~~`LSAN_PRINT_STATS_ON_EXIT`~~ | since v1.7 **deprecated**, will be removed in v2                | `true`, `false`   | `false`       |

_Signals_

_Stats_

## Behind the scenes or: How does it work?
_Coming soon_

## Final notes
This project is licensed under the terms of the GPL 3.0.

© Copyright 2022 - 2023 [mhahnFr][1] and contributors

[1]: https://github.com/mhahnFr
