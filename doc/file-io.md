# File I/O

File reading, writing, and loading.

## `open`

Open a file for reading or writing.

### Parameters

- `filename` - Path to file (string)
- `mode` - Optional mode: `"r"` (read, default), `"w"` (write), `"a"` (append)

### Returns

File stream object for use with `read-line`, `write-line`, `close`, etc.
Returns error if file cannot be opened.

### Examples

```lisp
(define f (open "test.txt" "r"))  ; Open for reading
(define f (open "out.txt" "w"))   ; Open for writing
(define f (open "log.txt" "a"))   ; Open for appending
```

### Notes

Always close files with `(close stream)` when done. Files are not auto-closed.

### See Also

- `close` - Close file stream
- `read-line` - Read line from file
- `write-line` - Write line to file

## `close`

Close a file stream.

### Parameters

- `stream` - File stream object from `open`

### Returns

`nil`

### Examples

```lisp
(define f (open "test.txt" "r"))
(read-line f)
(close f)
```

### Notes

Always close files when done. Closing an already-closed stream is safe (no-op).

### See Also

- `open` - Open file stream

## `read-line`

Read one line from file stream.

### Parameters

- `stream` - File stream object from `open`

### Returns

String containing the line (without newline), or `nil` at end-of-file.

### Examples

```lisp
(define f (open "test.txt" "r"))
(define line (read-line f))  ; => "first line"
(read-line f)                ; => "second line"
(read-line f)                ; => nil (EOF)
(close f)
```

### Notes

- Handles Unix (LF), Windows (CRLF), and old Mac (CR) line endings
- Returns `nil` at end-of-file
- Does not include the newline character in returned string

### See Also

- `write-line` - Write line to file
- `read-sexp` - Read S-expressions from file

## `write-line`

Write string to file stream with newline.

### Parameters

- `stream` - File stream object from `open`
- `text` - String to write

### Returns

The text that was written.

### Examples

```lisp
(define f (open "out.txt" "w"))
(write-line f "first line")
(write-line f "second line")
(close f)
```

### Notes

- Automatically appends newline after text
- Flushes output immediately

### See Also

- `read-line` - Read line from file

## `read-sexp`

Read S-expressions from file.

### Parameters

- `stream-or-filename` - File stream or filename string

### Returns

Single S-expression if file contains one, or list of all S-expressions.
Returns error if parse fails.

### Examples

```lisp
; File contains: (+ 1 2) (* 3 4)
(read-sexp "math.lisp")  ; => ((+ 1 2) (* 3 4))

; File contains: (define x 10)
(read-sexp "single.lisp")  ; => (define x 10)
```

### Notes

- Accepts filename string or open file stream
- Auto-closes file if filename provided
- Returns single expr if file has one, list if multiple

### See Also

- `load` - Load and evaluate file
- `read-json` - Read JSON from file

## `read-json`

Read JSON from file (basic parser).

### Parameters

- `stream-or-filename` - File stream or filename string

### Returns

Lisp object representing JSON:

- JSON object → hash table
- JSON array → list (NOT YET SUPPORTED)
- JSON string → string
- JSON number → integer or float
- JSON true → `#t`
- JSON false/null → `nil`

### Examples

```lisp
; File contains: {"name": "Alice", "age": 30}
(define obj (read-json "data.json"))
(hash-ref obj "name")  ; => "Alice"
(hash-ref obj "age")   ; => 30
```

### Limitations

- Basic implementation: nested objects and arrays not yet supported
- Only top-level objects work reliably

### See Also

- `read-sexp` - Read Lisp S-expressions from file

## `delete-file`

Delete a file from filesystem.

### Parameters

- `filename` - Path to file (string)

### Returns

`nil` on success, error if deletion fails.

### Examples

```lisp
(delete-file "temp.txt")  ; => nil
(delete-file "/no/such/file")  ; => Error: ...
```

### Notes

Permanent operation - deleted files cannot be recovered.

### Errors

Returns error with system message if deletion fails (permission denied, file not found, etc.)

## `load`

Load and evaluate Lisp file.

### Parameters

- `filename` - Path to Lisp file (string)

### Returns

Result of last expression in file, or error if load fails.

### Examples

```lisp
(load "utils.lisp")       ; Load utility functions
(load "~/.lisprc")         ; Load config file
```

### Notes

- Evaluates all expressions in file sequentially
- Returns value of last expression
- File must be valid Lisp syntax
- Definitions are added to current environment
- Use `expand-path` to handle `~/` in paths

### See Also

- `read-sexp` - Read without evaluating
- `expand-path` - Expand `~/ paths
