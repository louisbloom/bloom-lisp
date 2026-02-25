# Errors

Error creation, signaling, and introspection.

## `error?`

Test if value is an error object.

### Parameters

- `value` - Value to test

### Returns

`#t` if value is an error object, `#f` otherwise.

### Examples

```lisp
(define err nil)
(condition-case e
    (signal 'test-error "boom")
  (error (set! err e)))

(error? err)            ; => #t
(error? 42)             ; => #f
(error? "string")       ; => #f
```

### See Also

- `error-type` - Get error type symbol
- `error-message` - Get error message
- `signal` - Raise typed error

## `error-type`

Get error type symbol from error object.

### Parameters

- `error` - Error object

### Returns

Symbol representing the error type (e.g., `'error`, `'division-by-zero`, `'file-error`).

### Examples

```lisp
(define err nil)
(condition-case e
    (signal 'my-error "test")
  (error (set! err e)))

(error-type err)        ; => my-error

; Different error types
(condition-case e
    (/ 10 0)
  (error (error-type e)))  ; => division-by-zero
```

### Errors

Returns error if argument is not an error object.

### See Also

- `error?` - Test if value is error
- `error-message` - Get error message
- `signal` - Raise typed error

## `error-message`

Get error message string from error object.

### Parameters

- `error` - Error object

### Returns

String containing the human-readable error message.

### Examples

```lisp
(define err nil)
(condition-case e
    (signal 'custom-error "Something went wrong")
  (error (set! err e)))

(error-message err)     ; => "Something went wrong"

; With division-by-zero error
(condition-case e
    (/ 10 0)
  (error (error-message e)))  ; => "Division by zero"
```

### Errors

Returns error if argument is not an error object.

### See Also

- `error-type` - Get error type
- `error-data` - Get error data
- `error-stack` - Get call stack

## `error-stack`

Get call stack trace from error object.

### Parameters

- `error` - Error object

### Returns

List of function names in the call stack, or `nil` if no stack trace.

### Examples

```lisp
(define outer (lambda (x) (middle x)))
(define middle (lambda (x) (inner x)))
(define inner (lambda (x) (/ x 0)))

(define err nil)
(condition-case e
    (outer 10)
  (error (set! err e)))

(error-stack err)       ; => ("/" "inner" "middle" "outer")
```

### Notes

Stack traces show the sequence of function calls from innermost (where error occurred) to outermost.

### Errors

Returns error if argument is not an error object.

### See Also

- `error-message` - Get error message
- `error-type` - Get error type

## `error-data`

Get error data payload from error object.

### Parameters

- `error` - Error object

### Returns

The data associated with the error, or `nil` if no data.

### Examples

```lisp
; Error with simple data
(define err nil)
(condition-case e
    (signal 'test-error "message")
  (error (set! err e)))

(error-data err)        ; => "message"

; Error with complex data
(condition-case e
    (signal 'file-error '("cannot open" "file.txt" 404))
  (error (error-data e)))  ; => ("cannot open" "file.txt" 404)
```

### Notes

Error data can be any Lisp value: string, number, list, etc.

### Errors

Returns error if argument is not an error object.

### See Also

- `error-message` - Get error message
- `error-type` - Get error type
- `signal` - Raise error with data

## `signal`

Raise a typed error (Emacs Lisp-style exception).

### Parameters

- `error-type` - Symbol identifying the error type
- `data` - Optional data payload (any Lisp value)

### Returns

Does not return (raises error that must be caught with `condition-case`).

### Examples

```lisp
; Simple error
(signal 'my-error "Something went wrong")
; => ERROR: [my-error] Something went wrong

; Error with structured data
(signal 'file-error '("cannot open" "file.txt" 404))
; => ERROR: [file-error] file-error: ("cannot open" "file.txt" 404)

; Catching errors
(condition-case e
    (signal 'division-by-zero "Cannot divide by zero")
  (division-by-zero "Caught division error")
  (error "Caught other error"))
; => "Caught division error"
```

### Common Error Types

- `'error` - Generic error (catch-all)
- `'division-by-zero` - Division by zero
- `'wrong-type-argument` - Wrong type passed to function
- `'wrong-number-of-arguments` - Arity mismatch
- `'void-variable` - Undefined symbol
- `'file-error` - File I/O errors
- `'range-error` - Out of bounds access

You can define custom error types using any symbol.

### Notes

If `data` is a string, it's used as the error message. Otherwise, a message is constructed from the error type and data.

### See Also

- `error` - Raise generic error (convenience function)
- `condition-case` - Catch and handle errors
- `error-type` - Get error type from error object

## `error`

Raise a generic error with given message (convenience function).

### Parameters

- `message` - Error message string (or any value, will be converted to string)

### Returns

Does not return (raises error that must be caught with `condition-case`).

### Examples

```lisp
(error "Something went wrong")
; => ERROR: [error] Something went wrong

; With non-string message
(error 404)
; => ERROR: [error] 404

; Catching errors
(condition-case e
    (error "test error")
  (error (error-message e)))
; => "test error"
```

### Notes

This is equivalent to `(signal 'error message)`. For typed errors, use `signal` instead.

### See Also

- `signal` - Raise typed error
- `condition-case` - Catch and handle errors
- `error-message` - Get error message from error object
