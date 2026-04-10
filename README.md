# Bloom Lisp

An embeddable Lisp interpreter library written in C. This implementation follows traditional Lisp naming conventions and provides a REPL for testing and demonstration.

**[Language Reference](LANGUAGE_REFERENCE.md)** - Full language documentation with data types, functions, and examples

## Features

### Core Language

- **Data Types**: Numbers, integers, booleans, strings (UTF-8), characters, symbols, keywords, lists, vectors, hash tables, lambdas, errors
- **Special Forms**: `quote`, `quasiquote`, `if`, `define`, `set!`, `lambda`, `defmacro`, `let`/`let*`, `progn`, `do`, `cond`, `case`, `and`, `or`, `condition-case`, `unwind-protect`, `package-ref`
- **Macros**: Code transformation with `defmacro`, quasiquote (`` ` ``), unquote (`,`), unquote-splicing (`,@`), and built-in `defun` macro
- **Functions**: Arithmetic, strings, lists, vectors, hash tables, regex (PCRE2), file I/O, packages, profiling
- **Type Predicates**: `null?`, `atom?`, `pair?`, `list?`, `integer?`, `boolean?`, `number?`, `string?`, `char?`, `symbol?`, `keyword?`, `vector?`, `hash-table?`, `function?`, `callable?`, `error?`

See **[LANGUAGE_REFERENCE.md](LANGUAGE_REFERENCE.md)** for complete function listings and examples.

### Advanced Features

- **Package System**: Symbol-based namespace management with `in-package`, `current-package`, and `pkg:symbol` qualified access syntax
- **Condition System**: Emacs Lisp-style error handling with `signal`, `condition-case`, `unwind-protect`, and error introspection
- **Tail Call Optimization**: Trampoline-based tail recursion enables efficient recursive algorithms without stack overflow
- **Lexical Scoping**: First-class functions and closures with captured environments
- **Regex Support**: Full PCRE2 integration for advanced pattern matching with UTF-8 support
- **UTF-8 Support**: Full Unicode string support with codepoint-based operations
  - `length`, `substring`, and `string-ref` use codepoint indexing
  - Handles multi-byte characters correctly (CJK, emoji, etc.)
- **EOL-Aware File I/O**: File streams auto-detect their line-ending style (LF or CRLF) on open-for-read and transparently preserve it on write, Emacs Lisp style. `write-string` and `write-line` translate any embedded `\n` to the stream's stored EOL, so tools like source formatters can round-trip Windows files without any manual `\r` handling.
- **Memory Management**: Automatic garbage collection with Boehm GC

## Building

### Prerequisites

Install the required dependencies:

**Linux (Debian/Ubuntu):**

```bash
sudo apt-get install libgc-dev libpcre2-dev autoconf automake
```

**Linux (Fedora 41+):**

```bash
sudo dnf install gc-devel pcre2-devel autoconf automake
```

**macOS:**

```bash
brew install bdw-gc pcre2 autoconf automake
```

### Build from Source

```bash
./build.sh              # Debug build, install to ~/.local
./build.sh --no-debug   # Release build with optimizations
./build.sh --bear       # Build + generate compile_commands.json
make -C build check     # Run tests
```

### Cross-Compile for Windows

Cross-compiles using the Fedora mingw64 toolchain. Boehm GC is downloaded and built from source automatically.

```bash
sudo dnf install mingw64-gcc mingw64-pcre2   # One-time setup
./build.sh --mingw64                          # Build bloom-repl.exe
```

Output: `build-mingw64/repl/bloom-repl.exe` with DLLs (`libpcre2-8-0.dll`, `libgcc_s_seh-1.dll`, `libwinpthread-1.dll`).

The Windows build includes file execution and `-e` eval modes but not the interactive TUI REPL, which requires [bloom-boba](https://codeberg.org/thomasc/bloom-boba) (a Windows port of bloom-boba is planned).

Test with wine64:

```bash
wine64 build-mingw64/repl/bloom-repl.exe -e "(+ 1 2 3)"   # => 6
make -C build-mingw64 check                                 # Full test suite
```

### Installation

```bash
sudo make install
```

This installs:

- `bloom-repl` executable
- `libbloomlisp.a` static library
- Header files to `$(includedir)/bloom-lisp/`
- `bloom-lisp.pc` pkg-config file

## Usage

### Command Line Options

```bash
bloom-repl                      # Start interactive REPL
bloom-repl -e "CODE"            # Execute CODE and exit
bloom-repl FILE [FILE...]       # Execute FILE(s) and exit
bloom-repl FILE -- [ARG...]     # Run FILE with script arguments
bloom-repl -h, --help           # Show help message
```

### REPL Mode

```bash
./repl/bloom-repl
```

Features:

- **Input history** - Arrow keys navigate previous inputs (persisted across sessions)
- **Multiline editing** - Automatically continues incomplete expressions
- **Tab completion** - Completes symbol names from the environment

REPL Commands:

- `:quit` - Exit the REPL
- `:load <filename>` - Load and execute a Lisp file

### Command-Line Execution

```bash
./repl/bloom-repl -e "(+ 1 2 3)"                               # => 6
./repl/bloom-repl -e "(map (lambda (x) (* x 2)) '(1 2 3 4 5))" # => (2 4 6 8 10)
./repl/bloom-repl -e '(concat "hello" " " "world")'            # => "hello world"
```

Exit code is 0 on success, 1 on error.

### Running Files

```bash
./repl/bloom-repl script.lisp
```

## Embedding in Your Application

The library is self-contained and can be integrated into any C project.

**Example:**

```c
#include <bloom-lisp/lisp.h>

int main() {
    Environment* env = lisp_init();

    LispObject* result = lisp_eval_string("(+ 1 2 3)", env);
    char* output = lisp_print(result);
    printf("%s\n", output);  // Prints: 6

    lisp_cleanup();
    return 0;
}
```

Memory is managed by Boehm GC. Call `lisp_cleanup()` once at program exit.

### Integration Options

**pkg-config (after `make install`):**

```bash
cc myapp.c $(pkg-config --cflags --libs bloom-lisp) -o myapp
```

**Copy into your project** and link directly:

```bash
cc -I./libs/bloom-lisp/include \
   myapp.c \
   ./libs/bloom-lisp/src/libbloomlisp.a \
   $(pkg-config --libs bdw-gc libpcre2-8) -lm \
   -o myapp
```

**Autotools subproject:**

```bash
# In your configure.ac
AC_CONFIG_SUBDIRS([libs/bloom-lisp])
```

```makefile
# In your Makefile.am
SUBDIRS = libs/bloom-lisp
myapp_LDADD = libs/bloom-lisp/src/libbloomlisp.a
```

## Quick Start

```lisp
; Arithmetic and variables
(define x 10)
(+ x 20)                             ; => 30

; Functions
(define square (lambda (n) (* n n)))
(square 5)                           ; => 25

; Lists
(map (lambda (x) (* x 2)) '(1 2 3))  ; => (2 4 6)
(filter even? '(1 2 3 4 5 6))        ; => (2 4 6)

; Strings with Unicode
(length "Hello, 世界! 🌍")            ; => 12

; Conditionals
(if (> 10 5) "yes" "no")             ; => "yes"
```

**For full documentation, see [LANGUAGE_REFERENCE.md](LANGUAGE_REFERENCE.md)**

## C API Reference

### Core Functions

| Function                        | Description                                |
| ------------------------------- | ------------------------------------------ |
| `lisp_init()`                   | Initialize interpreter, return environment |
| `lisp_eval_string(code, env)`   | Parse and evaluate a string                |
| `lisp_load_file(filename, env)` | Load and evaluate a file                   |
| `lisp_cleanup()`                | Free global resources                      |

### Parsing and Evaluation

| Function               | Description                           |
| ---------------------- | ------------------------------------- |
| `lisp_read(input)`     | Parse input into AST                  |
| `lisp_eval(expr, env)` | Evaluate an expression                |
| `lisp_print(obj)`      | Convert object to string (GC-managed) |
| `lisp_princ(obj)`      | Print without quotes (human-readable) |
| `lisp_prin1(obj)`      | Print with quotes (machine-readable)  |

### Function Invocation

| Function                             | Description                             |
| ------------------------------------ | --------------------------------------- |
| `lisp_apply(func, args, env)`        | Call a function with evaluated arg list |
| `lisp_call_0(func, env)`             | Call a function with 0 arguments        |
| `lisp_call_1(func, arg1, env)`       | Call a function with 1 argument         |
| `lisp_call_2(func, arg1, arg2, env)` | Call a function with 2 arguments        |
| `lisp_call_3(func, a1, a2, a3, env)` | Call a function with 3 arguments        |

### Object Creation

| Function                        | Description                                  |
| ------------------------------- | -------------------------------------------- |
| `lisp_make_number(value)`       | Create floating-point number                 |
| `lisp_make_integer(value)`      | Create integer                               |
| `lisp_make_char(codepoint)`     | Create character                             |
| `lisp_make_string(value)`       | Create string                                |
| `lisp_make_symbol(name)`        | Create symbol                                |
| `lisp_make_boolean(value)`      | Create boolean                               |
| `lisp_make_cons(car, cdr)`      | Create cons cell                             |
| `lisp_make_vector(capacity)`    | Create vector                                |
| `lisp_make_hash_table()`        | Create hash table                            |
| `lisp_make_keyword(name)`       | Create keyword                               |
| `lisp_make_error(message)`      | Create error object                          |
| `lisp_make_builtin(func, name)` | Create builtin function                      |
| `lisp_intern(name)`             | Intern a symbol (reuse existing if interned) |

### Object Inspection

| Function                 | Description                     |
| ------------------------ | ------------------------------- |
| `lisp_is_truthy(obj)`    | Test if object is truthy        |
| `lisp_is_list(obj)`      | Test if object is a proper list |
| `lisp_is_callable(obj)`  | Test if object is callable      |
| `lisp_list_length(list)` | Return length of a list         |

### List Access

| Function           | Description           |
| ------------------ | --------------------- |
| `lisp_car(obj)`    | First element of cons |
| `lisp_cdr(obj)`    | Rest of cons          |
| `lisp_cadr(obj)`   | Second element        |
| `lisp_caddr(obj)`  | Third element         |
| `lisp_cadddr(obj)` | Fourth element        |

### Hash Table Operations

| Function                                | Description            |
| --------------------------------------- | ---------------------- |
| `hash_table_get_entry(table, key)`      | Look up entry by key   |
| `hash_table_set_entry(table, key, val)` | Insert or update entry |
| `hash_table_remove_entry(table, key)`   | Remove entry by key    |

### Environment Management

| Function                               | Description                    |
| -------------------------------------- | ------------------------------ |
| `env_create(parent)`                   | Create child environment frame |
| `env_define(env, sym, value, package)` | Define variable in package     |
| `env_lookup(env, sym)`                 | Look up variable               |
| `env_set(env, sym, value)`             | Update existing variable       |
| `env_current_package(env)`             | Get current package symbol     |

## Authors

- Thomas Christensen

## License

[MIT License](COPYING)
