# String Ports

O(1) sequential character access to strings.

## `open-input-string`

Create a string port for reading characters from a string.

### Parameters

- `string` - The source string to read from

### Returns

A string port that provides O(1) sequential access to characters.

### Examples

```lisp
(define p (open-input-string "hello"))
(port-peek-char p)  ; => #\h
(port-read-char p)  ; => #\h (advances position)
```

### See Also

- `port-peek-char` - Peek at current character
- `port-read-char` - Read and advance

## `port-peek-char`

Return current character without advancing position.

### Parameters

- `port` - A string port

### Returns

Current character, or `nil` at end of string.

### Examples

```lisp
(define p (open-input-string "hi"))
(port-peek-char p)  ; => #\h
(port-peek-char p)  ; => #\h (unchanged)
```

### Notes

O(1) operation — does not walk the string.

## `port-read-char`

Read current character and advance position.

### Parameters

- `port` - A string port

### Returns

Current character, or `nil` at end of string.

### Examples

```lisp
(define p (open-input-string "hi"))
(port-read-char p)  ; => #\h
(port-read-char p)  ; => #\i
(port-read-char p)  ; => nil
```

### Notes

O(1) operation — does not walk the string.

## `port-position`

Return current character position in the port.

### Parameters

- `port` - A string port

### Returns

Integer position (0-based).

### Examples

```lisp
(define p (open-input-string "hello"))
(port-position p)   ; => 0
(port-read-char p)
(port-position p)   ; => 1
```

## `port-source`

Return the source string of the port.

### Parameters

- `port` - A string port

### Returns

The original string the port was created from.

### Examples

```lisp
(define p (open-input-string "hello"))
(port-source p)     ; => "hello"
```

### Notes

Useful for substring extraction combined with `port-position`.

## `port-eof?`

Test if port is at end of string.

### Parameters

- `port` - A string port

### Returns

`#t` if all characters have been read, `#f` otherwise.

### Examples

```lisp
(define p (open-input-string "a"))
(port-eof? p)       ; => #f
(port-read-char p)
(port-eof? p)       ; => #t
```

## `string-port?`

Test if a value is a string port.

### Parameters

- `value` - Any value to test

### Returns

`#t` if the value is a string port, `#f` otherwise.

### Examples

```lisp
(string-port? (open-input-string "hi"))  ; => #t
(string-port? "hello")                   ; => #f
```
