# Comparison

Comparison and equality predicates.

## `>`

Test if numbers are in strictly decreasing order.

### Parameters

- `n1 n2 ...` - Two or more numbers to compare

### Returns

`#t` if each number is strictly greater than the next, `#f` otherwise.

### Examples

```lisp
(> 5 3)              ; => #t
(> 10 5 2)           ; => #t
(> 5 5)              ; => #f (not strictly greater)
(> 3 5)              ; => #f
```

## `<`

Test if numbers are in strictly increasing order.

### Parameters

- `n1 n2 ...` - Two or more numbers to compare

### Returns

`#t` if each number is strictly less than the next, `#f` otherwise.

### Examples

```lisp
(< 3 5)              ; => #t
(< 1 2 3 4)          ; => #t
(< 5 5)              ; => #f (not strictly less)
(< 5 3)              ; => #f
```

## `=`

Test if numbers are all equal.

### Parameters

- `n1 n2 ...` - Two or more numbers to compare

### Returns

`#t` if all numbers are equal, `#f` otherwise. Works with integers and floats.

### Examples

```lisp
(= 5 5)              ; => #t
(= 5 5 5)            ; => #t
(= 5.0 5)            ; => #t (cross-type comparison)
(= 5 3)              ; => #f
```

## `>=`

Test if numbers are in non-increasing order (greater than or equal).

### Parameters

- `n1 n2 ...` - Two or more numbers to compare

### Returns

`#t` if each number is greater than or equal to the next, `#f` otherwise.

### Examples

```lisp
(>= 5 3)             ; => #t
(>= 5 5)             ; => #t (equal is OK)
(>= 10 5 5 2)        ; => #t
(>= 3 5)             ; => #f
```

## `<=`

Test if numbers are in non-decreasing order (less than or equal).

### Parameters

- `n1 n2 ...` - Two or more numbers to compare

### Returns

`#t` if each number is less than or equal to the next, `#f` otherwise.

### Examples

```lisp
(<= 3 5)             ; => #t
(<= 5 5)             ; => #t (equal is OK)
(<= 1 2 2 3)         ; => #t
(<= 5 3)             ; => #f
```

## `not`

Logical negation.

### Parameters

- `value` - Value to negate

### Returns

`#t` if value is falsy (nil), `#f` if value is truthy.

### Examples

```lisp
(not nil)            ; => #t
(not #f)             ; => #t
(not #t)             ; => #f
(not 0)              ; => #f (0 is truthy!)
(not "")             ; => #f (empty string is truthy!)
```

### Notes

Only `nil` (and `#f`) are falsy in bloom-lisp

## `eq?`

Test pointer equality (same object in memory).

### Parameters

- `obj1` - First object
- `obj2` - Second object

### Returns

`#t` if both objects are the same object in memory, `#f` otherwise.

### Examples

```lisp
(eq? 'foo 'foo)          ; => #t (symbols are interned)
(eq? '(1 2) '(1 2))      ; => #f (different list objects)
(define x '(1 2))
(define y x)
(eq? x y)                ; => #t (same object)
```

### Notes

- Fast pointer comparison
- Reliable for symbols (always interned)
- NOT reliable for numbers or strings
- Use `equal?` for structural equality

## `equal?`

Test deep structural equality.

### Parameters

- `obj1` - First object
- `obj2` - Second object

### Returns

`#t` if objects have the same structure and values, `#f` otherwise.

### Examples

```lisp
(equal? '(1 2 3) '(1 2 3))      ; => #t (same values)
(equal? "abc" "abc")            ; => #t (same string)
(equal? '(1 (2 3)) '(1 (2 3)))  ; => #t (nested lists)
(equal? 1 1.0)                  ; => #f (different types)
```

### Notes

- Recursive comparison of lists, vectors, hash tables
- Default choice for "are these values the same?" checks

## `string=?`

Test string equality.

### Parameters

- `str1` - First string
- `str2` - Second string

### Returns

`#t` if strings have identical character sequences, `#f` otherwise.

### Examples

```lisp
(string=? "foo" "foo")     ; => #t
(string=? "foo" "bar")     ; => #f
(string=? "" "")           ; => #t
```
