# Vectors

Mutable, indexed array operations.

## `make-vector`

Create a new vector with specified size and optional initial value.

### Parameters

- `size` - Number of elements (integer)
- `initial-value` - Optional value to initialize all elements (defaults to `nil`)

### Returns

A new vector with `size` elements, all initialized to `initial-value`.

### Examples

```lisp
(make-vector 5)          ; => #(nil nil nil nil nil)
(make-vector 3 0)        ; => #(0 0 0)
(make-vector 4 "x")      ; => #("x" "x" "x" "x")
```

## `vector-ref`

Get element at index from vector.

### Parameters

- `vector` - The vector to access
- `index` - Zero-based index (integer)

### Returns

The element at the specified index.

### Examples

```lisp
(define v (make-vector 3 0))
(vector-ref v 0)         ; => 0
(vector-ref v 2)         ; => 0
```

### Errors

Returns error if index is out of bounds.

## `vector-set!`

Set element at index in vector (mutating operation).

### Parameters

- `vector` - The vector to modify
- `index` - Zero-based index (integer)
- `value` - Value to store at index

### Returns

The value that was set.

### Examples

```lisp
(define v (make-vector 3))
(vector-set! v 0 "a")    ; => "a"
(vector-set! v 1 "b")    ; => "b"
v                        ; => #("a" "b" nil)
```

### Notes

- Modifies vector in place
- Returns error if index out of bounds

## `vector-push!`

Append element to end of vector (mutating operation).

### Parameters

- `vector` - The vector to modify
- `value` - Value to append

### Returns

The value that was pushed.

### Examples

```lisp
(define v (make-vector 0))
(vector-push! v 10)      ; => 10
(vector-push! v 20)      ; => 20
v                        ; => #(10 20)
```

### Notes

Vector grows dynamically to accommodate new elements.

## `vector-pop!`

Remove and return last element from vector (mutating operation).

### Parameters

- `vector` - The vector to modify

### Returns

The element that was removed from the end.

### Examples

```lisp
(define v #(10 20 30))
(vector-pop! v)          ; => 30
(vector-pop! v)          ; => 20
v                        ; => #(10)
```

### Errors

Returns error if vector is empty.
