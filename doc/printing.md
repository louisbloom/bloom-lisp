# Printing

Output functions (Common Lisp style).

## `princ`

Print object in human-readable form (without quotes on strings).

### Parameters

- `object` - Any object to print

### Returns

The object that was printed.

### Examples

```lisp
(princ "Hello")         ; Prints: Hello (no quotes)
(princ 42)              ; Prints: 42
(princ '(1 2 3))        ; Prints: (1 2 3)
```

### Notes

Output goes to stdout. Strings print without surrounding quotes.

## `prin1`

Print object in readable representation (with quotes on strings).

### Parameters

- `object` - Any object to print

### Returns

The object that was printed.

### Examples

```lisp
(prin1 "Hello")         ; Prints: "Hello" (with quotes)
(prin1 42)              ; Prints: 42
(prin1 '(1 2 3))        ; Prints: (1 2 3)
```

### Notes

Output goes to stdout. Strings print with surrounding quotes.

## `print`

Print object like prin1 but with newlines before and after.

### Parameters

- `object` - Any object to print

### Returns

The object that was printed.

### Examples

```lisp
(print "Hello")         ; Prints: \
"Hello"\

(print 42)              ; Prints: \
42\

```

### Notes

Output goes to stdout. Adds newline before and after output.

## `format`

Formatted output with directives (Common Lisp style).

### Parameters

- `destination` - `nil` (return string) or `#t` (print to stdout)
- `format-string` - Format string with directives
- `args...` - Arguments to format

### Returns

Formatted string if `destination` is `nil`, otherwise `nil`.

### Format Directives

- `~A` or `~a` - Aesthetic (princ-style, no quotes)
- `~S` or `~s` - S-expression (prin1-style, with quotes)
- `~%` - Newline
- `~~` - Literal tilde (~)

### Examples

```lisp
(format nil "Hello, ~A!" "World")     ; => "Hello, World!"
(format nil "~A + ~A = ~A" 2 3 5)     ; => "2 + 3 = 5"
(format nil "String: ~S" "test")     ; => "String: \"test\""
(format nil "Line 1~%Line 2")         ; => "Line 1\
Line 2"
(format #t "Hello!~%" )               ; Prints: Hello!\

                                       ; => nil
```

## `terpri`

Print newline (terminate print).

### Returns

`nil`

### Examples

```lisp
(princ "Hello")
(terpri)                ; Prints newline
(princ "World")
```

### Notes

Useful for adding newlines between output.
