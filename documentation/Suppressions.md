# Suppressions
Since version **1.11**, the LeakSanitizer provides a sophisticated system for suppressing irrelevant memory leaks.

This system is based on JSON files containing suppression objects as defined below. A [JSON schema][1] is also available.

In order to use your own suppression file(s), add them to the environment variable [`LSAN_SUPPRESSION_FILES`][8]:
```shell
LSAN_SUPPRESSION_FILES='fooSuppressions.json:suppressionsBar.json' ./a.out
```

Activate more debug messages for the development of suppression files by setting the environment variable
[`LSAN_SUPPRESSION_DEVELOPER`][9] to `true`:
```shell
LSAN_SUPPRESSION_DEVELOPER=true ./a.out
```
If this mode is active, function names are printed as they are used by the linker.

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
**Necessity**: Either this or [`functions`][2]

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
**Necessity**: Either this or [`imageName`][3]

This array describes an abstracted stacktrace of memory leaks to match.

You can use the name of the function (as used by the linker) directly or use the stacktrace entry object described below.
At least one entry needs to be defined.

#### Stacktrace entry object
##### `name`
The name of the function as used by the linker.

**Type**: String  
**Necessity**: Either this or [`libraryRegex`][4]

##### `libraryRegex`
At least one callstack frame with a runtime image whose name matches these regular expressions.

> [!TIP]
> The special value `LSAN_SYSTEM_LIBRARIES` can be used in order to match any runtime image known to be provided by
> the system. [Adapt the known system libraries by using its extension system.][6]

**Type**: Regular expression *(String)* or array of regular expressions  
**Necessity**: Either this or [`name`][5]

##### `offset`
The byte offset into the function.

Only used when the property [`name`][5] is defined.

**Type**: Non-negative integral number  
**Necessity**: Optional

##### `library`
Hint in which runtime image to search for the function.

Only used when the property [`name`][5] is defined.

**Type**: String  
**Necessity**: Optional

#### Suppression stacktrace matching
Stacktraces are matched from the top. When using a library regular expression, multiple frames may be matched by the
entry, however, at least one frame must be matched in order to match the entire stacktrace.

##### Example without library regular expressions
Consider the following example suppression stacktrace:
```JSON
{
  "...": "...",
  "functions": [
    "foo",
    "main"
  ]
}
```

| Actual stacktrace              | Matching performed by the example above |
|--------------------------------|-----------------------------------------|
| `(a.out) bar + 23`             | *Does not match, so it is skipped*      |
| `(a.out) foo + 12`             | Matched by `foo`                        |
| `(a.out) main + 9`             | Matched by `main`                       |
| `(/usr/lib/dyld) start + 2345` | *No further matching performed*         |

If another function was present between `foo` and `main`, the callstack would be considered to not have matched the
suppression.

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

| Actual stacktrace                                                            | Matching performed by the example above |
|------------------------------------------------------------------------------|-----------------------------------------|
| `(/usr/lib/system/libsystem_malloc.dylib) _malloc_type_calloc_outlined + 66` | Matched by `LSAN_SYSTEM_LIBRARIES`      |
| `(/usr/lib/libobjc.A.dylib) _objc_rootAllocWithZone + 52`                    | Matched by `LSAN_SYSTEM_LIBRARIES`      |                   
| `(/usr/lib/libobjc.A.dylib) objc_alloc_init + 33`                            | Matched by `LSAN_SYSTEM_LIBRARIES`      |
| `(/System/Library/.../AppKit) -[NSApplication run] + 610`                    | Matched by `-[NSApplication run]`       |
| `(fdf) mlx_loop (minilibx_macos/mlx_init_loop.m:104:3)`                      | *No further matching performed*         |
| `(fdf) onApplicationFinishedLaunching (delegate/app_delegate.c:36:2)`        | *No further matching performed*         |
| `(fdf) main (main.c:26:10)`                                                  | *No further matching performed*         |
| `(/usr/lib/dyld) start + 3056`                                               | *No further matching performed*         |

## System library detection
The system library detection available using the special regular expression value of `LSAN_SYSTEM_LIBRARIES` can be
extended by providing a simple JSON file. This JSON file should include an array of regular expressions matching the
binary file names of the runtime images to be treated as a system library.

**Example**:
```JSON
[
  "^/System/Library/.*$"
]
```

In order to use your own JSON file(s), add them to the environment variable [`LSAN_SYSTEM_LIBRARY_FILES`][7]:
```shell
LSAN_SYSTEM_LIBRARY_FILES='libraryFoo.json:barLibrary.json' ./a.out
```

[1]: ../suppressions/schema.json
[2]: #functions
[3]: #imagename
[4]: #libraryregex
[5]: #name-1
[6]: #system-library-detection
[7]: Behaviour#lsan_system_library_files
[8]: Behaviour#lsan_suppression_files
[9]: Behaviour#lsan_suppression_developer