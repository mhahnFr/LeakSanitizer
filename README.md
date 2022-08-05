# Welcome to the LeakSanitizer!

## Usage
In order to use this library, compile your code using the following flags: ``-I<path/to/library> -Wno-gnu-include-next``.
On ``macOS``, link using these flags: ``-L<path/to/library> -llsan -lc++``.
On ``Linux``, link with ``-rdynamic -L<path/to/library> -llsan -lstdc++``.

### Final notes
This project is licensed under the terms of the GPL 3.0.

© Copyright 2022 [mhahnFr](https://www.github.com/mhahnFr)
