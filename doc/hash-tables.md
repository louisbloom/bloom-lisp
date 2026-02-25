# Hash Tables

Mutable key-value hash table operations.

## `make-hash-table`

Create a new empty hash table.

### Returns

A new hash table with no entries.

### Examples

```lisp
(define ht (make-hash-table))
(hash-set! ht "key" "value")
(hash-ref ht "key")         ; => "value"
```

## `hash-ref`

Get value for key from hash table.

### Parameters

- `hash-table` - The hash table to query
- `key` - The key to look up

### Returns

The value associated with `key`, or `nil` if key not found.

### Examples

```lisp
(define ht (make-hash-table))
(hash-set! ht "name" "Alice")
(hash-ref ht "name")        ; => "Alice"
(hash-ref ht "missing")     ; => nil
```

## `hash-set!`

Set key-value pair in hash table (mutating operation).

### Parameters

- `hash-table` - The hash table to modify
- `key` - The key to set
- `value` - The value to associate with key

### Returns

The value that was set.

### Examples

```lisp
(define ht (make-hash-table))
(hash-set! ht "name" "Alice")    ; => "Alice"
(hash-set! ht "age" 30)           ; => 30
(hash-ref ht "name")              ; => "Alice"
```

### Notes

Modifies hash table in place. If key already exists, value is updated.

## `hash-remove!`

Remove key-value pair from hash table (mutating operation).

### Parameters

- `hash-table` - The hash table to modify
- `key` - The key to remove

### Returns

`#t` if key was found and removed, `nil` if key wasn't present.

### Examples

```lisp
(define ht (make-hash-table))
(hash-set! ht "key" "value")
(hash-remove! ht "key")       ; => #t
(hash-remove! ht "missing")   ; => nil
```

## `hash-clear!`

Remove all entries from hash table (mutating).

### Parameters

- `table` - Hash table to clear

### Returns

`nil`

### Examples

```lisp
(define ht (make-hash-table))
(hash-set! ht "key1" 10)
(hash-set! ht "key2" 20)
(hash-count ht)         ; => 2
(hash-clear! ht)
(hash-count ht)         ; => 0
```

### Notes

This is a mutating operation (note the `!` suffix). The hash table is modified in place.

### See Also

- `hash-remove!` - Remove single entry
- `hash-count` - Get entry count
- `make-hash-table` - Create new hash table

## `hash-count`

Get the number of key-value pairs in hash table.

### Parameters

- `hash-table` - The hash table to measure

### Returns

Integer count of entries in the hash table.

### Examples

```lisp
(define ht (make-hash-table))
(hash-count ht)                  ; => 0
(hash-set! ht "a" 1)
(hash-set! ht "b" 2)
(hash-count ht)                  ; => 2
```

## `hash-keys`

Get list of all keys in hash table.

### Parameters

- `hash-table` - The hash table to query

### Returns

List of all keys in the hash table. Order is not guaranteed.

### Examples

```lisp
(define ht (make-hash-table))
(hash-set! ht "name" "Alice")
(hash-set! ht "age" 30)
(hash-keys ht)               ; => ("name" "age") or ("age" "name")
```

## `hash-values`

Get list of all values in hash table.

### Parameters

- `hash-table` - The hash table to query

### Returns

List of all values in the hash table. Order is not guaranteed.

### Examples

```lisp
(define ht (make-hash-table))
(hash-set! ht "name" "Alice")
(hash-set! ht "age" 30)
(hash-values ht)             ; => ("Alice" 30) or (30 "Alice")
```

## `hash-entries`

Get list of all key-value pairs as cons cells.

### Parameters

- `hash-table` - The hash table to query

### Returns

List of `(key . value)` pairs. Order is not guaranteed.

### Examples

```lisp
(define ht (make-hash-table))
(hash-set! ht "name" "Alice")
(hash-set! ht "age" 30)
(hash-entries ht)            ; => (("name" . "Alice") ("age" . 30))
```

### Notes

Useful for iterating over hash tables with association list functions.
