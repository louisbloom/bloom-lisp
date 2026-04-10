# File I/O

File reading, writing, and loading.

## `open`

Open a file for reading or writing.

### Parameters

- `filename` - Path to file (string)
- `mode` - Optional mode: `"r"` (read, default), `"w"` (write), `"a"` (append)
- `eol` - Optional explicit end-of-line style: `"\n"` or `"\r\n"`.
  If omitted, read-mode streams auto-detect the EOL by peeking up to
  4 KB of the file; write-mode streams default to `"\n"`.

### Returns

File stream object for use with `read-line`, `write-line`, `write-string`,
`close`, etc. Returns error if file cannot be opened.

### Examples

```lisp
(define f (open "test.txt" "r"))          ; Open for reading, auto-detect EOL
(define f (open "out.txt" "w"))           ; Open for writing, defaults to LF
(define f (open "out.txt" "w" "\r\n"))    ; Open for writing with CRLF
(define f (open "log.txt" "a"))           ; Open for appending
```

### Notes

- Always close files with `(close stream)` when done. Files are not auto-closed.
- The EOL style is stored on the stream and drives `write-line` and
  `write-string`: both translate any `\n` in their input to the stored
  EOL, Emacs-Lisp-style.

### See Also

- `close` - Close file stream
- `read-line` - Read line from file
- `write-line` - Write line to file
- `write-string` - Write string verbatim (with EOL translation)
- `stream-eol` - Query the stream's current EOL style

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

Write string to file stream with a trailing newline.

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

- Automatically appends the stream's EOL after the text. This is
  `"\n"` by default for write-mode streams, `"\r\n"` for CRLF streams
  (auto-detected on read, or passed explicitly to `open`).
- Any `\n` inside `text` is also translated to the stream's EOL.
- Flushes output immediately.

### See Also

- `read-line` - Read line from file
- `write-string` - Write without appending a terminator

## `write-string`

Write a string to a file stream verbatim (no trailing terminator).
Any `\n` in the string is translated to the stream's stored EOL.

### Parameters

- `stream` - File stream object from `open`
- `text` - String to write

### Returns

The text that was written.

### Examples

```lisp
;; Round-trip a CRLF file without double newlines at EOF:
(let* ((in (open "src.txt" "r"))           ; auto-detects CRLF
       (eol (stream-eol in)))
  (close in)
  (let ((out (open "dest.txt" "w" eol)))   ; inherit the style
    (write-string out "line1\nline2\n")     ; "\n" becomes "\r\n"
    (close out)))                           ; file ends with exactly one \r\n
```

### Notes

- Unlike `write-line`, does **not** append any terminator. Pair it with
  content that already ends in `\n` if you want a trailing EOL.
- Any `\n` bytes inside the text are translated to the stream's EOL,
  so an LF-internal string round-trips cleanly through a CRLF stream.
- Flushes output immediately.

### See Also

- `write-line` - Write a line with a trailing terminator
- `stream-eol` - Query the stream's EOL style
- `open` - Set the EOL style explicitly

## `stream-eol`

Return the EOL style stored on a file stream.

### Parameters

- `stream` - File stream object from `open`

### Returns

`"\n"` for LF streams, `"\r\n"` for CRLF streams.

### Examples

```lisp
(define f (open "windows.txt" "r"))
(stream-eol f)  ; => "\r\n"  (auto-detected)
(close f)

(define g (open "unix.txt" "r"))
(stream-eol g)  ; => "\n"
(close g)
```

### Notes

- On read-mode streams the EOL is auto-detected at `open` time by
  peeking the first 4 KB of the file for a `\r\n` pair.
- On write-mode streams the EOL is either `"\n"` (default) or the
  explicit string passed as the third argument to `open`.
- The returned value is safe to pass as the third argument of `open`
  when creating a matching write stream — this is the normal pattern
  for tools that want to round-trip a file's line-ending style.

### See Also

- `open` - Set the EOL explicitly via the third argument
- `write-line` / `write-string` - Both respect the stream's EOL

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

## `read-file-raw`

Read an entire file into a string verbatim, preserving every byte
including carriage returns.

### Parameters

- `filename` - Path to file (string)

### Returns

String containing the exact file contents (no newline normalisation).
Returns error if file cannot be opened.

### Examples

```lisp
(read-file-raw "unix.txt")     ; => "line1\nline2\n"
(read-file-raw "windows.txt")  ; => "line1\r\nline2\r\n"
```

### Notes

- Unlike `read-line`, this preserves CRLF sequences — useful for tools
  that need to detect or round-trip the original line-ending style.
- Opens the file in binary mode; suitable for text files only (the
  return type is a Lisp string).
- Reads the entire file into memory; avoid on very large files.

### See Also

- `read-line` - Read line by line with newline normalisation
- `read-sexp` - Read Lisp S-expressions

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
