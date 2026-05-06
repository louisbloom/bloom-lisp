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

Pure GNU Autotools — no wrapper script:

```bash
./autogen.sh
mkdir build && cd build
PKG_CONFIG_PATH=$HOME/.local/lib/pkgconfig:$HOME/.local/lib64/pkgconfig \
  ../configure --prefix=$HOME/.local --enable-debug
make -j$(nproc)
make check          # Run the test suite
make install        # Install library, headers, REPL, pkg-config file
```

Useful targets (from `build/`):

- `make` — build `libbloomlisp.a` and `repl/bloom-repl`
- `make check` — run the test suite
- `make install` — install library, headers, REPL, pkg-config file
- `make format` — clang-format on C, shfmt on shell, prettier on Markdown, lisp-fmt on Lisp
- `make bear` — produce `compile_commands.json` for clangd

Drop `--enable-debug` for an optimized release build.

Docstrings are generated from `doc/*.md` into `src/docstrings.gen.h` automatically by `make` (declared as `BUILT_SOURCES` in `src/Makefile.am`); to regenerate manually run `scripts/gen-docstrings.sh doc > src/docstrings.gen.h`.

### Cross-Compile for Windows

Cross-compiles using the Fedora mingw64 toolchain. Boehm GC is downloaded and cached under `deps/`.

```bash
sudo dnf install mingw64-gcc mingw64-pcre2   # One-time setup
scripts/build-mingw64.sh                     # Build bloom-repl.exe
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

> **Embedder contract.** A `LispObject *` is **not** always a real pointer — it may be a tagged immediate value (small integer, char, `NIL`, `LISP_TRUE`). Never read `obj->type` or `obj->value.X` directly; always go through the `LISP_*` accessor macros (`LISP_TYPE`, `LISP_INT_VAL`, `LISP_CAR`, `LISP_CDR`, `LISP_LAMBDA_NAME`, ...). The macros are defined in `<bloom-lisp/lisp_value.h>` and decode the tag bits and follow boxed-out pointers transparently. See [Object representation & GC](#object-representation--gc) below.

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

## Repository layout

```
include/         public headers — lisp.h, lisp_value.h, utf8.h, file_utils.h
src/             interpreter core
  lisp.c           constructors, lisp_init, NIL/LISP_TRUE
  eval.c           eval loop, all 16 special forms, macro expansion
  reader.c         S-expression parser
  print.c          object → string
  env.c            environments, call stack, handler contexts
  hash_table.c     FNV-1a hash table (used by interner and user hash tables)
  builtins.c       registers all per-category builtin tables
  builtins_*.c     one file per category (arithmetic, strings, lists, ...)
  regex.c          PCRE2 helpers
  utf8.c           UTF-8 codepoint helpers
repl/            interactive TUI REPL (links bloom-boba)
tests/           Lisp test suite (basic/, advanced/, regression/)
doc/             builtin docstrings (Markdown → src/docstrings.gen.h at build)
scripts/         build helpers (gen-docstrings.sh, build-mingw64.sh)
```

### Adding a builtin

1. Implement `LispObject *my_func(LispObject *args, Environment *env)` in the appropriate `src/builtins_*.c` (or create a new category file and call its `register_*_builtins` from `src/builtins.c`).
2. Add `REGISTER("my-func", my_func)` inside that file's `register_*_builtins` function.
3. Document it in the matching `doc/*.md` so the docstring lands in `(documentation 'my-func)` automatically.

See `src/builtins_internal.h` for the `REGISTER` macro, `CHECK_ARGS_n` arity guards, and the `LIST_APPEND` helper.

### Tests

Lisp files under `tests/` load `tests/test-helpers.lisp` and call `assert-equal` / `assert-true` / `assert-false` / `assert-error` / `assert-nil`. Tests abort on first failure via `(error ...)`; exit code 0 = pass. `make check` runs the full suite in parallel; `./tests/run-test.sh tests/basic/strings.lisp` runs a single file (the `BLOOM_REPL` env var overrides which binary to use).

## Object representation & GC

`sizeof(LispObject) == 24 bytes`. Every value passed around the C API is either a real pointer to a 24-byte heap object or a tagged immediate encoded directly in the `LispObject *` value itself. Both kinds coexist with Boehm GC's conservative scanner without violating its invariants.

### Tag scheme

The low 3 bits of the `LispObject *` value act as a tag:

| Low 3 bits | Meaning      | Encoding                                        |
| ---------- | ------------ | ----------------------------------------------- |
| `000`      | heap pointer | 8-byte-aligned `LispObject *` (real heap obj)   |
| `001`      | fixnum       | `(int64 << 3) \| 1` — 61-bit signed integer     |
| `010`      | char         | `(uint32 << 3) \| 2` — 21-bit Unicode codepoint |
| `011`      | singleton    | `NIL = 0x3`, `LISP_TRUE = 0xB`                  |
| `100..111` | reserved     | (future tags)                                   |

Fixnums up to ±2<sup>60</sup>, all chars, `NIL`, and `LISP_TRUE` need no allocation at all — they're encoded in the pointer value. Bigger integers, doubles, strings, symbols, conses, lambdas, etc. live on the heap with the low 3 bits zero.

### How this stays Boehm-GC-safe

Boehm GC scans memory looking for bit patterns that match real allocated heap pointers. Two failure modes to consider:

1. **A real heap pointer being missed by GC.** This would cause use-after-free. We avoid it by keeping every heap pointer as a real, untouched 8-byte-aligned address — Boehm sees those as pointers exactly as it always has.

2. **A tagged immediate being mistaken for a pointer.** A fixnum or char with low bits `001`/`010` could occasionally land on a value that falls inside a Boehm-managed heap region. When that happens GC conservatively keeps the pointed-at block alive longer than necessary. This is **false retention**, not corruption — the wrong object isn't freed, it's the _right_ object that lingers an extra cycle. Stress tests show the overhead is well under 5%.

### Boxed-out info structs

Lambda, macro, error, string-port, vector, and hash-table objects don't fit comfortably in 24 bytes. They live as separately-allocated `*Info` structs (`LambdaInfo`, `MacroInfo`, `ErrorInfo`, `StringPortInfo`, `VectorInfo`, `HashTableInfo`) with the wrapping `LispObject` holding only an 8-byte pointer to them. The `LISP_LAMBDA_NAME(v)`, `LISP_ERROR_MESSAGE(v)`, etc. macros follow that indirection for you.

### What this means for embedders

- **`obj->type` is illegal.** Use `LISP_TYPE(obj)`. It checks tag bits before dereferencing, so it's correct on tagged immediates too.
- **`obj->value.integer` is illegal for tagged fixnums.** Use `LISP_INT_VAL(obj)` — for tagged values it shifts the tag off; for big-int heap objects it reads the field.
- **`obj == NIL` and `obj == LISP_TRUE` are fine.** Both are compile-time constants; pointer comparison just compares against `0x3` / `0xB`.
- **Constructing heap objects from C** (e.g. building a custom builtin that returns a `LispObject`): use `lisp_make_*` constructors, never hand-roll a `LispObject` on the stack — Boehm needs to see the allocation.
- **Holding a `LispObject *` across a possible GC trigger.** Per the [Boehm GC allocation rule](#boehm-gc-allocation-rule-internal-data-structures), any C struct that holds a `LispObject *` must itself be allocated with `GC_malloc`, otherwise GC can't see the pointer. Locals on the stack are scanned automatically — but declare short-lived locals `volatile` if the optimizer might elide them.

### Boehm GC allocation rule (internal data structures)

When you write internal C structs that hold `LispObject *` or `Environment *` fields:

- Allocate the struct with `GC_malloc`, not `malloc`/`calloc`. Boehm only scans GC-allocated memory; a `malloc`-ed struct holding a `LispObject *` is invisible to it, and GC may collect what looks like an unreferenced object.
- Use `GC_strdup` (not `strdup`) for any string field that lives inside a GC-visible struct.
- Plain `malloc` is still fine for buffers that don't contain `LispObject *` (e.g. a temporary character buffer).

Symptoms of violation: intermittent "Undefined symbol" errors for builtins, or content-dependent random failures.

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

| Function                    | Description                          |
| --------------------------- | ------------------------------------ |
| `lisp_car(obj)`             | First element of cons                |
| `lisp_cdr(obj)`             | Rest of cons                         |
| `lisp_cadr(obj)`            | Second element                       |
| `lisp_caddr(obj)`           | Third element                        |
| `lisp_cadddr(obj)`          | Fourth element                       |
| `lisp_set_car(cell, value)` | Replace the car of `cell` (mutating) |
| `lisp_set_cdr(cell, value)` | Replace the cdr of `cell` (mutating) |

### Value accessors (read these instead of `obj->value.X`)

Defined in `<bloom-lisp/lisp_value.h>`. Required for correctness — see [Object representation & GC](#object-representation--gc).

| Macro                         | What it returns                                        |
| ----------------------------- | ------------------------------------------------------ |
| `LISP_TYPE(v)`                | The `LispType` of `v` (decodes tag bits transparently) |
| `LISP_INT_VAL(v)`             | `long long` value of a fixnum (tagged or heap)         |
| `LISP_NUM_VAL(v)`             | `double` value of a heap-allocated number              |
| `LISP_CHAR_VAL(v)`            | `uint32_t` Unicode codepoint                           |
| `LISP_BOOL_VAL(v)`            | `int` — `1` if `v == LISP_TRUE`, else `0`              |
| `LISP_STR_VAL(v)`             | `char *` for `LISP_STRING`                             |
| `LISP_SYM_VAL(v)`             | `Symbol *` for `LISP_SYMBOL`/`LISP_KEYWORD`            |
| `LISP_CAR(v)`, `LISP_CDR(v)`  | Cons fields (lvalues — assignable)                     |
| `LISP_LAMBDA_NAME(v)`, etc.   | Lambda fields (follows the boxed-out `LambdaInfo *`)   |
| `LISP_ERROR_MESSAGE(v)`, etc. | Error fields (follows the boxed-out `ErrorInfo *`)     |
| `LISP_VECTOR_ITEMS(v)`, etc.  | Vector fields                                          |
| `LISP_HT_BUCKETS(v)`, etc.    | Hash-table fields                                      |

There's one of these per (type, field) pair for boxed-out variants and one per scalar/struct member otherwise. Field names match the union member names.

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
