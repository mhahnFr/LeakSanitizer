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

_Signals_

_Stats_

## Behind the scenes or: How does it work?
_Coming soon_

## Final notes
This project is licensed under the terms of the GPL 3.0.

© Copyright 2022 - 2023 [mhahnFr][1] and contributors

[1]: https://github.com/mhahnFr
