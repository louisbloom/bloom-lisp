# Ditty Lisp

An embeddable Lisp interpreter library and syntax highlighter written in C. This implementation follows traditional Lisp naming conventions and provides a REPL for testing and demonstration.

**[Language Reference](LANGUAGE_REFERENCE.md)** - Full language documentation with data types, functions, and examples

## Features

### Core Language

- **Data Types**: Numbers, integers, booleans, strings (UTF-8), characters, symbols, keywords, lists, vectors, hash tables, lambdas, errors, regex, string ports, file streams
- **Special Forms**: `quote`, `quasiquote`, `if`, `define`, `set!`, `lambda`, `defmacro`, `let`/`let*`, `progn`, `do`, `cond`, `case`, `and`, `or`, `condition-case`, `unwind-protect`
- **Reader Syntax**: `pkg:symbol` qualified access (desugars to `package-ref`)
- **Function Parameters**: Required, optional (`&optional`), and rest (`&rest`) parameters with `lambda`
- **Macros**: Code transformation with `defmacro`, quasiquote (`` ` ``), unquote (`,`), unquote-splicing (`,@`), and built-in `defun`, `defvar`, `defconst`, `defalias` macros
- **Functions**: Arithmetic, strings, lists, vectors, hash tables, regex (PCRE2), file I/O, string ports, filesystem, packages, profiling
- **Type Predicates**: `null?`, `atom?`, `pair?`, `list?`, `integer?`, `boolean?`, `number?`, `string?`, `char?`, `symbol?`, `keyword?`, `vector?`, `hash-table?`, `function?`, `callable?`, `error?`, `regex?`

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
- **Semantic Classification**: The `lisp_sf_kind()` API classifies special forms by role (quote, define, control, macro-def, binding, body, exception) — used by Flare for accurate highlighting without hardcoded keyword lists

### Flare — Syntax Highlighting

Flare is a built-in syntax highlighting module (compiled into `libditty.a`) that produces ANSI terminal output at 8-color, 16-color, 256-color, and truecolor depths.

- **Runtime-driven classification**: Uses the ditty `Environment` as the single source of truth for symbol classification — no parallel hardcoded keyword lists
- **Two lexers**: Ditty Lisp and CommonMark/Markdown (fenced code blocks with `lisp`/`ditty` info strings are sub-lexed through the Lisp lexer)
- **Four built-in styles**: Dracula, Monokai, GitHub Dark, GitHub Light
- **Modular pipeline**: Lex → Style lookup → Format, each stage pluggable
- **`flare` CLI**: A `cat`-like tool that highlights source files to the terminal, with auto-detection of file language

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
  ../configure --prefix=$HOME/.local
make -j$(nproc)
make check          # Run the test suite
make install        # Install library, headers, REPL, pkg-config file
```

Useful targets (from `build/`):

- `make` — build `libditty.a` (includes Flare) and `flare` and `cli/ditty`
- `make check` — run the test suite
- `make install` — install library, headers, REPL, `flare`, pkg-config files
- `make format` — clang-format on C, shfmt on shell, prettier on Markdown, lisp-fmt on Lisp
- `make bear` — produce `compile_commands.json` for clangd

The default build enables AddressSanitizer and UndefinedBehaviorSanitizer. Use `--enable-release` for an optimized production build or `--enable-debug` for an unsanitized debug build (-O0 -g3).

### Emacs Major Mode

An Emacs major mode for ditty source (`ditty-mode.el`) lives in `emacs/`. It is installed by `make install` to `$(datadir)/emacs/site-lisp/ditty/`. Byte-compilation and ERT tests are manual targets (Emacs is not a build dependency):

```bash
make -C build/emacs byte-compile   # .el → .elc
make -C build/emacs test           # ERT test suite
make -C build/emacs elisp-format   # auto-indent .el files
```

See `emacs/ditty-mode.el` header for installation and usage instructions.

Docstrings are generated from `doc/*.md` into `src/docstrings.gen.h` automatically by `make` (declared as `BUILT_SOURCES` in `src/Makefile.am`); to regenerate manually run `scripts/gen-docstrings.sh doc > src/docstrings.gen.h`.

### Windows

Ditty builds natively on Windows using MSYS2/UCRT64. The `#ifdef _WIN32`
guards in the source handle Windows-specific APIs (file paths, timing,
environment variables). The interactive REPL requires
[boba](https://codeberg.org/thomasc/boba), which also builds natively
on Windows via MSYS2.

```bash
# In an MSYS2 UCRT64 shell:
./autogen.sh
mkdir build && cd build
../configure --prefix=$HOME/.local
make -j$(nproc)
make check
make install
```

### Installation

```bash
sudo make install
```

This installs:

- `ditty` executable
- `flare` executable (syntax highlighting CLI)
- `libditty.a` static library (interpreter + syntax highlighting)
- Header files to `$(includedir)/ditty/` (including `highlight.h` for Flare)
- `ditty.pc` pkg-config file
- `lisp-fmt.lisp` Lisp source formatter

## Usage

### Command Line Options

```bash
ditty                      # Start interactive REPL
ditty -e "CODE"            # Execute CODE and exit
ditty FILE [FILE...]       # Execute FILE(s) and exit
ditty FILE -- [ARG...]     # Run FILE with script arguments
ditty -h, --help           # Show help message
```

### `flare` — Syntax Highlighting CLI

```bash
flare FILE                        # Highlight FILE to stdout (truecolor + dracula)
flare -f 256 -s monokai FILE      # 256-color monokai
flare -l commonmark README.md    # Highlight Markdown
flare -                           # Read stdin
flare --help                      # Show options
```

Options: `-f`/`--format` (`truecolor`, `256`, `16`, `8`), `-s`/`--style` (`dracula`, `monokai`, `github-dark`, `github-light`), `-l`/`--language` (`auto`, `ditty`, `commonmark`/`markdown`). Auto-detection selects by file extension.

### REPL Mode

```bash
./cli/ditty
```

The REPL uses [boba](https://codeberg.org/thomasc/boba) for inline terminal
rendering — no alternate screen, output flows into the terminal's own
scrollback. Syntax highlighting is applied to the input as you type via
the Flare highlighting engine.

Features:

- **Live syntax highlighting** - Input is highlighted as you type using Flare
- **Inline rendering** - No alt-screen takeover; output appears in normal scrollback
- **Multi-line editing** - Incomplete forms get continuation lines with auto-indent
- **Eval on Enter** - Enter evaluates when the form is complete; inserts a newline otherwise
- **Input history** - Arrow keys navigate previous inputs
- **Tab completion** - Completes symbol names from the environment
- **Ctrl+C** - Aborts the current edit and starts a fresh prompt
- **Ctrl+D** - Exits the REPL on empty input; deletes char under cursor otherwise

REPL Commands:

- `/quit` - Exit the REPL
- `/load <filename>` - Load and execute a Lisp file

### Command-Line Execution

```bash
./cli/ditty -e "(+ 1 2 3)"                               # => 6
./cli/ditty -e "(map (lambda (x) (* x 2)) '(1 2 3 4 5))" # => (2 4 6 8 10)
./cli/ditty -e '(concat "hello" " " "world")'            # => "hello world"
```

Exit code is 0 on success, 1 on error.

### Running Files

```bash
./cli/ditty script.lisp
```

## Embedding in Your Application

The library is self-contained and can be integrated into any C project.

**Example:**

```c
#include <ditty/lisp.h>

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

> **Embedder contract.** A `LispObject *` is **not** always a real pointer — it may be a tagged immediate value (small integer, char, `NIL`, `LISP_TRUE`). Never read `obj->type` or `obj->value.X` directly; always go through the `LISP_*` accessor macros (`LISP_TYPE`, `LISP_INT_VAL`, `LISP_CAR`, `LISP_CDR`, `LISP_LAMBDA_NAME`, ...). The macros are defined in `<ditty/lisp_value.h>` and decode the tag bits and follow boxed-out pointers transparently. See [Object representation & GC](#object-representation--gc) below.

### Integration Options

**pkg-config (after `make install`):**

```bash
# ditty (interpreter + syntax highlighting)
cc myapp.c $(pkg-config --cflags --libs ditty) -o myapp
```

**Copy into your project** and link directly:

```bash
cc -I./libs/ditty/include \
   myapp.c \
   ./libs/ditty/src/libditty.a \
   $(pkg-config --libs bdw-gc libpcre2-8) -lm \
   -o myapp
```

**Autotools subproject:**

```bash
# In your configure.ac
AC_CONFIG_SUBDIRS([libs/ditty])
```

```makefile
# In your Makefile.am
SUBDIRS = libs/ditty
myapp_LDADD = libs/ditty/src/libditty.a
```

## Repository layout

```
include/            public headers — lisp.h, lisp_value.h, utf8.h, file_utils.h
  ditty/         installed header subdirectory — highlight.h (Flare API)
src/                interpreter core
  lisp.c              constructors, lisp_init, NIL/LISP_TRUE, stdlib macros
  eval.c              eval loop, 17 special forms, package-ref dispatch, macro expansion
  reader.c            S-expression parser
  print.c             object → string
  env.c               environments, call stack, handler contexts
  hash_table.c        FNV-1a hash table (used by interner and user hash tables)
  builtins.c          registers all per-category builtin tables
  builtins_*.c        one file per category (arithmetic, strings, lists, ...)
  regex.c             PCRE2 helpers
  utf8.c              UTF-8 codepoint helpers
  file_utils.c        file path resolution and opening
  flare_*.c         syntax highlighting (lexer, styles, formatter, color)
    lexer.c             lexer factory, registry
    lexer_ditty.c  Ditty Lisp scanner (runtime-driven classification)
    lexer_commonmark.c  CommonMark/Markdown scanner (v0.31.2, two-phase parsing)
    token_type.c        token type hierarchy helpers
    style.c             style storage and hierarchy resolution
    style_*.c           built-in styles (dracula, monokai, github-dark, github-light)
    formatter.c         formatter struct (wraps color depth)
    formatter_terminal.c terminal SGR/ANSI emission
    color.c             RGB ↔ 256 ↔ 16 ↔ 8 color conversion
    highlight.c         one-shot flare_highlight() convenience function
flare/                flare — syntax highlighting CLI (links libditty.a)
lisp/               Lisp source formatter (lisp-fmt.lisp, loaded by ditty)
emacs/              Emacs major mode (ditty-mode.el, ditty-mode-tests.el)
cli/               interactive TUI REPL (links boba)
tests/              test suite
  basic/               foundational feature tests
  advanced/            macro, TCO, condition system, regex, etc.
  regression/         bug-specific regression tests
  snapshots/           reference ANSI output for flare rendering tests
  test_sf_kind.c       C unit test for special form classification
  test_*.c             C unit tests for flare components
  test-helpers.lisp    assertion macros for Lisp tests
  flare_testkit.h      assertion macros for C flare tests
  run-test.sh          Lisp test wrapper script
doc/                builtin docstrings (Markdown → src/docstrings.gen.h at build)
scripts/            build helpers (gen-docstrings.sh)
```

### Adding a builtin

1. Implement `LispObject *my_func(LispObject *args, Environment *env)` in the appropriate `src/builtins_*.c` (or create a new category file and call its `register_*_builtins` from `src/builtins.c`).
2. Add `REGISTER("my-func", my_func)` inside that file's `register_*_builtins` function.
3. Document it in the matching `doc/*.md` so the docstring lands in `(documentation 'my-func)` automatically.

See `src/builtins_internal.h` for the `REGISTER` macro, `CHECK_ARGS_n` arity guards, and the `LIST_APPEND` helper.

### Tests

Lisp files under `tests/` load `tests/test-helpers.lisp` and call `assert-equal` / `assert-true` / `assert-false` / `assert-error` / `assert-nil`. Tests abort on first failure via `(error ...)`; exit code 0 = pass. `make check` runs the full suite in parallel; `./tests/run-test.sh tests/basic/strings.lisp` runs a single file (the `DITTY_BIN` env var overrides which binary to use).

C unit tests (`test_sf_kind`, `test_token_type`, `test_color`, `test_style`, `test_lexer`, `test_lexer_commonmark`, `test_formatter`, `test_api`, `test_roundtrip`) are compiled and run by `make check` alongside the Lisp tests. They link against `libditty.a` (which includes all flare code).

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

; Named functions (defun macro)
(defun cube (n) (* n n n))
(cube 3)                             ; => 27

; Optional and rest parameters
(defun flex (a &optional b &rest rest)
  (list a b rest))
(flex 1 2 3 4 5)                     ; => (1 2 (3 4 5))

; Lists
(map (lambda (x) (* x 2)) '(1 2 3))  ; => (2 4 6)
(filter even? '(1 2 3 4 5 6))        ; => (2 4 6)

; Strings with Unicode
(length "Hello, 世界! 🌍")            ; => 12

; Conditionals
(if (> 10 5) "yes" "no")             ; => "yes"

; Error handling
(condition-case err
  (/ 1 0)
  (error (format nil "caught: ~A" (error-message err))))
```

**For full documentation, see [LANGUAGE_REFERENCE.md](LANGUAGE_REFERENCE.md)**

## C API Reference

### Core Functions

| Function                        | Description                                   |
| ------------------------------- | --------------------------------------------- |
| `lisp_init()`                   | Initialize interpreter, return environment    |
| `lisp_eval_string(code, env)`   | Parse and evaluate first expression in string |
| `lisp_load_file(filename, env)` | Load and evaluate a file (all expressions)    |
| `lisp_cleanup()`                | Free global resources                         |

### Parsing and Evaluation

| Function               | Description                                              |
| ---------------------- | -------------------------------------------------------- |
| `lisp_read(input)`     | Parse input into AST (`const char **`, advances pointer) |
| `lisp_eval(expr, env)` | Evaluate an expression                                   |
| `lisp_print(obj)`      | Convert object to string (GC-managed)                    |
| `lisp_princ(obj)`      | Print without quotes (human-readable)                    |
| `lisp_prin1(obj)`      | Print with quotes (machine-readable)                     |
| `lisp_print_cl(obj)`   | Print Emacs Lisp–style                                   |

### Function Invocation

| Function                             | Description                             |
| ------------------------------------ | --------------------------------------- |
| `lisp_apply(func, args, env)`        | Call a function with evaluated arg list |
| `lisp_call_0(func, env)`             | Call a function with 0 arguments        |
| `lisp_call_1(func, arg1, env)`       | Call a function with 1 argument         |
| `lisp_call_2(func, arg1, arg2, env)` | Call a function with 2 arguments        |
| `lisp_call_3(func, a1, a2, a3, env)` | Call a function with 3 arguments        |

### Object Creation

| Function                                        | Description                                  |
| ----------------------------------------------- | -------------------------------------------- |
| `lisp_make_number(value)`                       | Create floating-point number                 |
| `lisp_make_integer(value)`                      | Create integer (tagged fixnum or heap)       |
| `lisp_make_char(codepoint)`                     | Create character (tagged immediate)          |
| `lisp_make_string(value)`                       | Create string                                |
| `lisp_intern(name)`                             | Intern a symbol (reuse existing if interned) |
| `lisp_make_symbol(name)`                        | Alias for `lisp_intern`                      |
| `lisp_make_keyword(name)`                       | Create keyword (separate interning table)    |
| `lisp_make_boolean(value)`                      | Create boolean                               |
| `lisp_make_cons(car, cdr)`                      | Create cons cell                             |
| `lisp_make_lambda(params, body, closure, name)` | Create lambda (simple)                       |
| `lisp_make_lambda_ext(...)`                     | Create lambda with optional/rest params      |
| `lisp_make_macro(params, body, closure, name)`  | Create macro                                 |
| `lisp_make_vector(capacity)`                    | Create vector                                |
| `lisp_make_hash_table()`                        | Create hash table                            |
| `lisp_make_error(message)`                      | Create error object (no stack trace)         |
| `lisp_make_error_with_stack(msg, env)`          | Create error with stack trace                |
| `lisp_make_typed_error(type, msg, data, env)`   | Typed error with stack trace                 |
| `lisp_make_typed_error_simple(type, msg, env)`  | Typed error from string                      |
| `lisp_make_builtin(func, name)`                 | Create builtin function                      |
| `lisp_make_string_port(str)`                    | Create input string port                     |
| `lisp_make_output_string_port()`                | Create output string port                    |
| `lisp_make_regex(code)`                         | Wrap compiled PCRE2 pattern (GC finalizer)   |
| `lisp_make_file_stream(file)`                   | Wrap `FILE*` as file stream                  |

### Error Handling

| Function                                            | Description                          |
| --------------------------------------------------- | ------------------------------------ |
| `lisp_make_error(msg)`                              | Create `'error` without stack trace  |
| `lisp_make_error_with_stack(msg, env)`              | Create `'error` with stack trace     |
| `lisp_make_typed_error(type, msg, data, env)`       | Typed error with stack trace         |
| `lisp_make_typed_error_simple(type_name, msg, env)` | Typed error from C string            |
| `lisp_attach_stack_trace(err, env)`                 | Attach stack trace to existing error |

### Semantic Classification

| Function            | Description                                   |
| ------------------- | --------------------------------------------- |
| `lisp_sf_kind(sym)` | Return `SfKind` for special form, or `-1`     |
| `lisp_sf_count()`   | Return number of special forms (currently 17) |

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

> Mutating `set-car!` and `set-cdr!` are available as Lisp builtins, not as C API functions. Use `LISP_CAR(v)` and `LISP_CDR(v)` as lvalues to mutate cons cells from C.

### Value accessors (read these instead of `obj->value.X`)

Defined in `<ditty/lisp_value.h>`. Required for correctness — see [Object representation & GC](#object-representation--gc).

| Macro                              | What it returns                                        |
| ---------------------------------- | ------------------------------------------------------ |
| `LISP_TYPE(v)`                     | The `LispType` of `v` (decodes tag bits transparently) |
| `LISP_INT_VAL(v)`                  | `long long` value of a fixnum (tagged or heap)         |
| `LISP_NUM_VAL(v)`                  | `double` value of a heap-allocated number              |
| `LISP_CHAR_VAL(v)`                 | `uint32_t` Unicode codepoint                           |
| `LISP_BOOL_VAL(v)`                 | `int` — `1` if `v == LISP_TRUE`, else `0`              |
| `LISP_STR_VAL(v)`                  | `char *` for `LISP_STRING`                             |
| `LISP_SYM_VAL(v)`                  | `Symbol *` for `LISP_SYMBOL`/`LISP_KEYWORD`            |
| `LISP_CAR(v)`, `LISP_CDR(v)`       | Cons fields (lvalues — assignable)                     |
| `LISP_LAMBDA_NAME(v)`, etc.        | Lambda fields (follows the boxed-out `LambdaInfo *`)   |
| `LISP_ERROR_MESSAGE(v)`, etc.      | Error fields (follows the boxed-out `ErrorInfo *`)     |
| `LISP_VECTOR_ITEMS(v)`, etc.       | Vector fields                                          |
| `LISP_HT_BUCKETS(v)`, etc.         | Hash-table fields                                      |
| `LISP_REGEX_CODE(v)`               | `pcre2_code *` for `LISP_REGEX`                        |
| `LISP_STRING_PORT_BUFFER(v)`, etc. | String port fields                                     |

There's one of these per (type, field) pair for boxed-out variants and one per scalar/struct member otherwise. Field names match the union member names.

### Hash Table Operations

| Function                                | Description             |
| --------------------------------------- | ----------------------- |
| `hash_table_get_entry(table, key)`      | Look up entry by key    |
| `hash_table_set_entry(table, key, val)` | Insert or update entry  |
| `hash_table_remove_entry(table, key)`   | Remove entry by key     |
| `hash_table_clear(table)`               | Remove all entries      |
| `hash_keys_equal(a, b)`                 | Key equality comparison |

### Environment Management

| Function                                   | Description                          |
| ------------------------------------------ | ------------------------------------ |
| `env_create(parent)`                       | Create child environment frame       |
| `env_define(env, sym, value, package)`     | Define variable in package           |
| `env_lookup(env, sym)`                     | Look up variable                     |
| `env_lookup_in_package(env, sym, package)` | Look up variable in specific package |
| `env_set(env, sym, value)`                 | Update existing variable             |
| `env_current_package(env)`                 | Get current package symbol           |

## Authors

- Thomas Christensen

## License

[MIT License](COPYING)
