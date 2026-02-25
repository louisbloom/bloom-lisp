# Type Predicates

Functions for testing the type of a value.

## `null?`

Check if a value is nil (null/empty).

### Parameters

- `value` - Any value to test

### Returns

`#t` if the value is `nil`, `#f` otherwise.

### Examples

```lisp
(null? nil)          ; => #t
(null? #f)           ; => #t (nil and #f are the same)
(null? '())          ; => #t (empty list is nil)
(null? 0)            ; => #f (0 is truthy)
(null? "")           ; => #f (empty string is truthy)
```

## `atom?`

Check if a value is an atom (not a list).

### Parameters

- `value` - Any value to test

### Returns

`#t` if the value is an atom (not a cons cell), `#f` otherwise.

### Examples

```lisp
(atom? 42)           ; => #t
(atom? "hello")      ; => #t
(atom? nil)          ; => #t
(atom? '(1 2 3))     ; => #f (lists are not atoms)
```

## `pair?`

Test if object is a cons cell (pair).

### Parameters

- `obj` - Object to test

### Returns

`#t` if object is a cons cell (pair), `#f` otherwise.

### Examples

```lisp
(pair? '(1 . 2))        ; => #t
(pair? '(a b c))        ; => #t
(pair? nil)             ; => #f (nil is not a pair)
(pair? 42)              ; => #f
(pair? "string")       ; => #f
```

### Notes

- A pair is a cons cell (`LISP_CONS` type)
- `nil` is NOT a pair (unlike `list?` which returns true for both `nil` and cons cells)
- Useful for checking if an alist element is a key-value pair

### See Also

- `list?` - Test if object is a list (nil or cons cell)
- `cons` - Create a cons cell

## `integer?`

Check if a value is an integer.

### Parameters

- `value` - Any value to test

### Returns

`#t` if the value is a 64-bit integer, `#f` otherwise.

### Examples

```lisp
(integer? 42)        ; => #t
(integer? -100)      ; => #t
(integer? 3.14)      ; => #f (float)
(integer? "42")      ; => #f (string)
```

## `boolean?`

Check if a value is a boolean (#t or #f).

### Parameters

- `value` - Any value to test

### Returns

`#t` if the value is `#t` or `#f`, `#f` otherwise.

### Examples

```lisp
(boolean? #t)        ; => #t
(boolean? #f)        ; => #t
(boolean? nil)       ; => #t (nil is #f)
(boolean? 1)         ; => #f
```

## `number?`

Check if a value is a number (integer or float).

### Parameters

- `value` - Any value to test

### Returns

`#t` if the value is an integer or float, `#f` otherwise.

### Examples

```lisp
(number? 42)         ; => #t (integer)
(number? 3.14)       ; => #t (float)
(number? "42")       ; => #f (string)
```

## `vector?`

Check if a value is a vector.

### Parameters

- `value` - Any value to test

### Returns

`#t` if the value is a vector, `#f` otherwise.

### Examples

```lisp
(vector? (make-vector 5))    ; => #t
(vector? #())                ; => #t (empty vector literal)
(vector? '(1 2 3))           ; => #f (list)
```

## `hash-table?`

Test if value is a hash table.

### Parameters

- `value` - Value to test

### Returns

`#t` if value is a hash table, `nil` otherwise.

### Examples

```lisp
(define ht (make-hash-table))
(hash-table? ht)        ; => #t
(hash-table? '(1 2 3))  ; => nil
(hash-table? 42)        ; => nil
```

### See Also

- `make-hash-table` - Create hash table
- `vector?` - Test if value is vector
- `list?` - Test if value is list

## `string?`

Check if a value is a string.

### Parameters

- `value` - Any value to test

### Returns

`#t` if the value is a string, `#f` otherwise.

### Examples

```lisp
(string? "hello")    ; => #t
(string? "")         ; => #t (empty string)
(string? 'hello)     ; => #f (symbol)
(string? 42)         ; => #f
```

## `symbol?`

Check if a value is a symbol.

### Parameters

- `value` - Any value to test

### Returns

`#t` if the value is a symbol, `#f` otherwise.

### Examples

```lisp
(symbol? 'foo)       ; => #t
(symbol? '+)         ; => #t
(symbol? "foo")      ; => #f (string)
(symbol? 42)         ; => #f
```

## `keyword?`

Check if a value is a keyword.

Keywords are self-evaluating symbols that start with a colon (`:`).

### Parameters

- `value` - Any value to test

### Returns

`#t` if the value is a keyword, `nil` otherwise.

### Examples

```lisp
(keyword? :foo)      ; => #t
(keyword? :bar-baz)  ; => #t
(keyword? 'foo)      ; => nil (symbol, not keyword)
(keyword? "foo")     ; => nil (string)
```

### See Also

- `keyword-name` - Get keyword name without colon
- `symbol?` - Check for symbols

## `list?`

Check if a value is a list (nil or cons cell).

### Parameters

- `value` - Any value to test

### Returns

`#t` if the value is `nil` or a cons cell, `#f` otherwise.

### Examples

```lisp
(list? '(1 2 3))     ; => #t
(list? nil)          ; => #t
(list? '())          ; => #t
(list? 42)           ; => #f
```

## `function?`

Check if a value is a lambda (user-defined function).

### Parameters

- `value` - Any value to test

### Returns

`#t` if the value is a lambda, `#f` otherwise.

### Examples

```lisp
(function? (lambda (x) x))  ; => #t
(function? +)               ; => #f (builtin)
(function? 42)              ; => #f
```

## `macro?`

Check if a value is a macro.

### Parameters

- `value` - Any value to test

### Returns

`#t` if the value is a macro, `#f` otherwise.

### Examples

```lisp
(macro? when)    ; => #t
(macro? +)       ; => #f
```

## `builtin?`

Check if a value is a builtin function.

### Parameters

- `value` - Any value to test

### Returns

`#t` if the value is a builtin, `#f` otherwise.

### Examples

```lisp
(builtin? +)                  ; => #t
(builtin? (lambda (x) x))    ; => #f
```

## `callable?`

Check if a value is callable (lambda, macro, or builtin).

### Parameters

- `value` - Any value to test

### Returns

`#t` if the value is callable, `#f` otherwise.

### Examples

```lisp
(callable? +)                  ; => #t
(callable? (lambda (x) x))    ; => #t
(callable? when)              ; => #t
(callable? 42)                ; => #f
```
