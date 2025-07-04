# Suppressions
Since version **1.11**, the LeakSanitizer provides a sophisticated system for suppressing irrelevant memory leaks.

This system is based on JSON files containing suppression objects as defined below. A JSON schema is also available.

## Suppression object
A suppression object is designed to match memory leaks. This can be achieved by defining one or multiple of the
available metadata to be matched.

### `name`
The name used to identify a particular suppression object.

**Type**: String  
**Necessity**: Optional

### `size`
The size of the memory block.

**Type**: Non-negative integral number  
**Necessity**: Optional

### `imageName`
A regular expression to match the runtime image in which the leak is to be found.

**Type**: Regular expression *(String)*  
**Necessity**: Either this or `functions`

### `type`
The type of the memory leak.

| Value | Location in which the leak is found in |
|-------|----------------------------------------|
| `0`   | Objective-C runtime (direct)           |
| `1`   | Objective-C runtime (indirect)         |
| `2`   | Stack (direct)                         |
| `3`   | Stack (indirect)                       |
| `4`   | Global storage (direct)                |
| `5`   | Global storage (indirect)              |
| `6`   | Thread-local storage (direct)          |
| `7`   | Thread-local storage (indirect)        |
| `8`   | No location                            |
| `9`   | Indirect via a leak found nowhere      |

**Type**: Non-negative integral number  
**Necessity**: Optional

### `functions`
**Type**: Array  
**Necessity**: Either this or `imageName`

This array describes an abstracted stacktrace of memory leaks to match.

You can use the name of the function (as used by the linker) directly or use the stacktrace entry object described below.
At least one entry needs to be defined.

#### Stacktrace entry object
##### `name`
The name of the function as used by the linker.

**Type**: String  
**Necessity**: Either this or `libraryRegex`

##### `libraryRegex`
At least one callstack frame with a runtime image whose name matches these regular expressions.

> [!TIP]
> The special value `LSAN_SYSTEM_LIBRARIES` can be used in order to match any runtime image known to be provided by
> the system.

**Type**: Regular expression *(String)* or array of regular expressions  
**Necessity**: Either this or `name`

##### `offset`
The byte offset into the function.

Only used when the property `name` is defined.

**Type**: Non-negative integral number  
**Necessity**: Optional

##### `library`
Hint in which runtime image to search for the function.

Only used when the property `name` is defined.

**Type**: String  
**Necessity**: Optional

#### Suppression stacktrace matching
Stacktraces are matched from the top. When using a library regular expression, multiple frames may be matched by the
entry, however, at least one frame must be matched in order to match the entire stacktrace.

##### Example with `LSAN_SYSTEM_LIBRARIES`
Consider the following example suppression stacktrace:
```JSON
{
  "...": "...",
  "functions": [
    { "libraryRegex": "LSAN_SYSTEM_LIBRARIES" },
    "-[NSApplication run]"
  ]
}
```

| Actual stacktrace                                                                                 | Matching performed by the example above |
|---------------------------------------------------------------------------------------------------|-----------------------------------------|
| `# 1: (/usr/lib/system/libsystem_malloc.dylib) _malloc_type_calloc_outlined + 66`                 | Matched by `LSAN_SYSTEM_LIBRARIES`      |
| `# 2: (/usr/lib/libobjc.A.dylib) _objc_rootAllocWithZone + 52`                                    | Matched by `LSAN_SYSTEM_LIBRARIES`      |                   
| `# 3: (/usr/lib/libobjc.A.dylib) objc_alloc_init + 33`                                            | Matched by `LSAN_SYSTEM_LIBRARIES`      |
| `# 4: (/System/Library/Frameworks/AppKit.framework/Versions/C/AppKit) -[NSApplication run] + 610` | Matched by `-[NSApplication run]`       |
| ` ->  (fdf) mlx_loop (minilibx_macos/mlx_init_loop.m:104:3)`                                      | *No further matching performed*         |
| `# 6: (fdf) onApplicationFinishedLaunching (delegate/app_delegate.c:36:2)`                        | *No further matching performed*         |
| `# 7: (fdf) main (main.c:26:10)`                                                                  | *No further matching performed*         |
| `# 8: (/usr/lib/dyld) start + 3056`                                                               | *No further matching performed*         |
