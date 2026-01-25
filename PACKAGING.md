# Packaging Bloom Lisp for Use in Other Projects

This library can be used independently in other projects. Here's how to package and integrate it.

## Option 1: System Installation with pkg-config

Build and install the library system-wide:

```bash
./autogen.sh
./configure --prefix=/usr/local
make
sudo make install
```

Then in your project, use pkg-config:

```bash
cc myapp.c $(pkg-config --cflags --libs bloom-lisp) -o myapp
```

Or in a Makefile:

```makefile
CFLAGS += $(shell pkg-config --cflags bloom-lisp)
LDFLAGS += $(shell pkg-config --libs bloom-lisp)

myapp: myapp.c
	$(CC) $(CFLAGS) $< $(LDFLAGS) -o $@
```

## Option 2: Copy into Your Project

Copy the `bloom-lisp/` directory into your project and build it as a subproject:

```bash
cp -r /path/to/bloom-lisp ./libs/
```

Then build the library and link against it:

```bash
cd libs/bloom-lisp
./autogen.sh
./configure
make
```

Link in your build:

```bash
cc -I./libs/bloom-lisp/include \
   myapp.c \
   ./libs/bloom-lisp/src/libbloomlisp.a \
   $(pkg-config --libs bdw-gc libpcre2-8) -lm \
   -o myapp
```

## Option 3: Build as a Subproject with Autotools

If your project also uses autotools, you can integrate bloom-lisp as a subdirectory:

```bash
# In your configure.ac
AC_CONFIG_SUBDIRS([libs/bloom-lisp])
```

```makefile
# In your Makefile.am
SUBDIRS = libs/bloom-lisp

myapp_LDADD = libs/bloom-lisp/src/libbloomlisp.a
```

## Dependencies

The library requires:

- **libgc** (Boehm GC) - Memory management
- **libpcre2-8** - Regex support

### Installing Dependencies

**Linux (Debian/Ubuntu):**

```bash
sudo apt-get install libgc-dev libpcre2-dev
```

**Linux (Fedora):**

```bash
sudo dnf install gc-devel pcre2-devel
```

**macOS:**

```bash
brew install bdw-gc pcre2
```

**Windows (MSYS2 UCRT64):**

```bash
pacman -S mingw-w64-ucrt-x86_64-gc mingw-w64-ucrt-x86_64-pcre2
```

## Using the Library in C Code

```c
#include <bloom-lisp/lisp.h>  // If installed system-wide
// #include "lisp.h"          // If embedded in project

int main() {
    // Initialize the interpreter (includes GC)
    lisp_init();

    // Create environment
    Environment *env = env_create_global();

    // Evaluate Lisp code
    LispObject *result = lisp_eval_string("(+ 1 2 3)", env);
    char *output = lisp_print(result);
    printf("Result: %s\n", output);

    // Cleanup (GC handles most memory)
    env_free(env);
    lisp_cleanup();

    return 0;
}
```

## Library API

See `README.md` for complete API documentation.

Key functions:

- `lisp_init()` - Initialize interpreter
- `lisp_eval_string(const char* code, Environment* env)` - Evaluate Lisp code
- `lisp_load_file(const char* filename, Environment* env)` - Load and evaluate file
- `env_create_global()` - Create environment with built-ins
- `lisp_cleanup()` - Clean up resources

## Building as Standalone

To build the library and REPL independently:

```bash
./autogen.sh
./configure
make
./repl/bloom-repl  # Test the REPL
```

## Running Tests

```bash
make check
```

## License

See the LICENSE file for license information.
