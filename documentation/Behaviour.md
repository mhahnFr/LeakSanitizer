# Behaviour
The LeakSanitizer allows you to adjust its behaviour using a set of variables.

Since version
- **1.6** they can be set as environment variables
- **1.11** the *C* API for the behavioral variables is deprecated *(find its documentation [here][1])*

## Environment variables
### `LSAN_HUMAN_PRINT`
Indicates whether to print in a human-readable format.

**Type**: [Boolean][2]  
**Default value**: `true`

### `LSAN_PRINT_COUT`
Whether to print to the standard output stream.

**Type**: [Boolean][2]  
**Default value**: `false`

### `LSAN_PRINT_FORMATTED`
Whether to print the output using ANSI escape codes.

**Type**: [Boolean][2]  
**Default value**: `true`

### `LSAN_INVALID_CRASH`
Whether to terminate the program once an invalid action is detected.

**Type**: [Boolean][2]  
**Default value**: `true`

### `LSAN_INVALID_FREE`
Whether to check for invalid memory deallocations.

**Type**: [Boolean][2]  
**Default value**: `true`

### `LSAN_FREE_NULL`
Whether to issue a warning when deallocating a null pointer.

**Type**: [Boolean][2]  
**Default value**: `false`

### `LSAN_STATS_ACTIVE`
Whether to activate the statistical bookkeeping.

[The *C* API for the statistics][4] is only functional if either this variable is set to `true`
or [`LSAN_AUTO_STATS`][3] is set to a valid time interval.

**Type**: [Boolean][2]  
**Default value**: `false`

### `LSAN_CALLSTACK_SIZE`
Defines the number of lines for a callstack to print.

**Type**: Non-negative integral number  
**Default value**: `20`

### `LSAN_PRINT_EXIT_POINT`
Whether to print the callstack of the exit point.

Might lead to strange callstacks since calling `exit` may optimize the stack for never returning from that function.

**Type**: [Boolean][2]  
**Default value**: `false`  
**Since**: v1.7

### `LSAN_PRINT_BINARIES`
Whether to print the binary file names associated with a particular call frame within stacktraces.

**Type**: [Boolean][2]  
**Default value**: `true`  
**Since**: v1.8

### `LSAN_PRINT_FUNCTIONS`
Whether to always print function names within stacktraces, even if debug symbols are available.

**Type**: [Boolean][2]  
**Default value**: `true`  
**Since**: v1.8

### `LSAN_RELATIVE_PATHS`
Whether to use relative paths where appropriate.

**Type**: [Boolean][2]  
**Default value**: `true`  
**Since**: v1.8

### `LSAN_ZERO_ALLOCTION`
Whether to issue a warning when allocating zero bytes of memory.

**Type**: [Boolean][2]  
**Default value**: `false`  
**Since**: v1.8

### `LSAN_AUTO_STATS`
The time interval for printing the statistics automatically.

Implicitly sets [`LSAN_STATS_ACTIVE`][5] to `true`.

**Types**: [Time interval][6], non-negative integral number *(interpreted as seconds)*  
**Default value**: *None*  
**Since**: v1.11

### `LSAN_SUPPRESSION_DEVELOPER`
Whether to print errors and warnings related to the suppression system.

This might be helpful in order to write your own suppression files.

**Type**: [Boolean][2]  
**Default value**: `false`  
**Since**: v1.11

### `LSAN_INDIRECT_LEAKS`
Whether to print the stacktraces of indirectly leaked memory allocations.

**Type**: [Boolean][2]  
**Default value**: `false`  
**Since**: v1.11

### `LSAN_REACHABLE_LEAKS`
Whether to print the stacktraces of memory leaks to which a pointer was found.

**Type**: [Boolean][2]  
**Default value**: `true`  
**Since**: v1.11

### `LSAN_SUPPRESSION_FILES`
List of additional suppression files to be considered.

**Type**: [File list][7]  
**Default value**: *None*  
**Since**: v1.11

### `LSAN_SYSTEM_LIBRARY_FILES`
List of additional system library files to be considered.

**Type**: [File list][7]  
**Default value**: *None*  
**Since**: v1.11

### Variable types
#### Boolean
Boolean variables can be assigned a number, they are interpreted as in the programming language **C**.  
They can also be assigned (case-insensitive): `true` and `false`.

#### File list
File lists consist of file paths separated by `:`.

#### Time interval
Time intervals are defined as non-negative integral number, which may immediately be followed by the time unit, of which are supported:
- `ns`: nanoseconds
- `us`: microseconds
- `ms`: milliseconds
- `s`: seconds
- `m`: minutes
- `h`: hours

[1]: https://github.com/mhahnFr/LeakSanitizer/wiki/lsan_internals.h
[2]: #boolean
[3]: #lsan_auto_stats
[4]: https://github.com/mhahnFr/LeakSanitizer/wiki/lsan_stats.h
[5]: #lsan_stats_active
[6]: #time-interval
[7]: #file-list