# Bloom Lisp

An embeddable Lisp interpreter library written in C. This implementation follows traditional Lisp naming conventions and provides a REPL for testing and demonstration.

**[Language Reference](LANGUAGE_REFERENCE.md)** - Full language documentation with data types, functions, and examples

## Features

### Core Language

- **Data Types**: Numbers, integers, booleans, strings (UTF-8), characters, lists, vectors, hash tables, lambdas, errors
- **Special Forms**: `quote`, `quasiquote`, `if`, `define`, `set!`, `lambda`, `defmacro`, `let`/`let*`, `progn`, `do`, `cond`, `case`, `condition-case`, `unwind-protect`
- **Macros**: Code transformation with `defmacro`, quasiquote (`` ` ``), unquote (`,`), unquote-splicing (`,@`), and built-in `defun` macro
- **Functions**: Arithmetic, strings, lists, vectors, hash tables, regex (PCRE2), file I/O
- **Type Predicates**: `null?`, `atom?`, `integer?`, `boolean?`, `number?`, `string?`, `char?`, `vector?`, `hash-table?`, `error?`

See **[LANGUAGE_REFERENCE.md](LANGUAGE_REFERENCE.md)** for complete function listings and examples.

### Advanced Features

- **Condition System**: Emacs Lisp-style error handling with `signal`, `condition-case`, `unwind-protect`, and error introspection
- **Tail Call Optimization**: Trampoline-based tail recursion enables efficient recursive algorithms without stack overflow
- **Lexical Scoping**: First-class functions and closures with captured environments
- **Regex Support**: Full PCRE2 integration for advanced pattern matching with UTF-8 support
- **UTF-8 Support**: Full Unicode string support with codepoint-based operations
  - `length`, `substring`, and `string-ref` use codepoint indexing
  - Handles multi-byte characters correctly (CJK, emoji, etc.)
- **Memory Management**: Automatic garbage collection with Boehm GC

## Building

### Prerequisites

Install the required dependencies:

**Linux (Debian/Ubuntu):**

```bash
sudo apt-get install libgc-dev libpcre2-dev autoconf automake
```

**Linux (Fedora):**

```bash
sudo dnf install gc-devel pcre2-devel autoconf automake
```

**macOS:**

```bash
brew install bdw-gc pcre2 autoconf automake
```

### Build from Source

```bash
./autogen.sh
./configure
make
make check  # Run tests
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

The library is self-contained and can be integrated into any project. For detailed packaging instructions, see [PACKAGING.md](PACKAGING.md).

**Using pkg-config:**

```bash
cc myapp.c $(pkg-config --cflags --libs bloom-lisp) -o myapp
```

**Using the Library:**

```c
#include <bloom-lisp/lisp.h>

int main() {
    lisp_init();
    Environment* env = env_create_global();

    LispObject* result = lisp_eval_string("(+ 1 2 3)", env);
    char* output = lisp_print(result);
    printf("%s\n", output);  // Prints: 6

    lisp_cleanup();
    return 0;
}
```

**Session Management (two-layer environment):**

For applications that want to separate system bindings from user bindings (enabling `session-save` / `environment-bindings`), use the two-layer pattern:

```c
lisp_init();
Environment* global = env_create_global();
// Register app-specific builtins into global...
Environment* user = env_create_session(global);
// Use `user` for eval — user bindings stay in this frame
// session-save / environment-bindings operate on this frame
```

Note: Memory is managed by Boehm GC. Call `lisp_cleanup()` once at program exit.

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

| Function                        | Description                 |
| ------------------------------- | --------------------------- |
| `lisp_init()`                   | Initialize the interpreter  |
| `lisp_eval_string(code, env)`   | Parse and evaluate a string |
| `lisp_load_file(filename, env)` | Load and evaluate a file    |
| `lisp_cleanup()`                | Free global resources       |

### Parsing and Evaluation

| Function               | Description                           |
| ---------------------- | ------------------------------------- |
| `lisp_read(input)`     | Parse input into AST                  |
| `lisp_eval(expr, env)` | Evaluate an expression                |
| `lisp_print(obj)`      | Convert object to string (GC-managed) |

### Object Creation

| Function                   | Description                  |
| -------------------------- | ---------------------------- |
| `lisp_make_number(value)`  | Create floating-point number |
| `lisp_make_integer(value)` | Create integer               |
| `lisp_make_string(value)`  | Create string                |
| `lisp_make_symbol(name)`   | Create symbol                |
| `lisp_make_boolean(value)` | Create boolean               |
| `lisp_make_cons(car, cdr)` | Create cons cell             |
| `lisp_make_error(message)` | Create error object          |

### Environment Management

| Function                       | Description                                |
| ------------------------------ | ------------------------------------------ |
| `env_create(parent)`           | Create new environment                     |
| `env_create_global()`          | Create global environment with built-ins   |
| `env_create_session(global)`   | Create user session frame on top of global |
| `env_define(env, name, value)` | Define variable                            |
| `env_lookup(env, name)`        | Look up variable                           |

## License

MIT License - see LICENSE file for details.
