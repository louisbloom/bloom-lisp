# Lists

List construction, access, and higher-order operations.

## `car`

Get the first element of a list.

### Parameters

- `list` - A non-empty list

### Returns

The first element of the list.

### Examples

```lisp
(car '(1 2 3))           ; => 1
(car '("a" "b"))        ; => "a"
(car (list 10 20 30))    ; => 10
```

### See Also

- `cdr` - Get the rest of a list
- `cons` - Construct a new list cell

## `cdr`

Get the rest of a list (all elements except the first).

### Parameters

- `list` - A non-empty list

### Returns

A list containing all elements except the first. Returns `nil` if the list has only one element.

### Examples

```lisp
(cdr '(1 2 3))           ; => (2 3)
(cdr '(1))               ; => nil
(cdr '("a" "b" "c"))    ; => ("b" "c")
```

### See Also

- `car` - Get the first element of a list
- `cons` - Construct a new list cell

## `caar`

Return the car of the car of a list. Equivalent to `(car (car x))`.

### Parameters

- `list` - A nested list

### Returns

The first element of the first element.

### Examples

```lisp
(caar '((1 2) 3))    ; => 1
(caar '((a b) c d))  ; => a
```

### See Also

- `car` - First element
- `cadr` - Second element

## `cadr`

Return the car of the cdr of a list. Equivalent to `(car (cdr x))`. Same as `second`.

### Parameters

- `list` - A list with at least two elements

### Returns

The second element of the list.

### Examples

```lisp
(cadr '(1 2 3))      ; => 2
(cadr '(a b c))      ; => b
```

### See Also

- `car` - First element
- `caddr` - Third element

## `cdar`

Return the cdr of the car of a list. Equivalent to `(cdr (car x))`.

### Parameters

- `list` - A list whose first element is a list

### Returns

The rest of the first element.

### Examples

```lisp
(cdar '((1 2 3) 4))  ; => (2 3)
(cdar '((a b) c))    ; => (b)
```

### See Also

- `cdr` - Rest of list
- `caar` - First of first

## `cddr`

Return the cdr of the cdr of a list. Equivalent to `(cdr (cdr x))`.

### Parameters

- `list` - A list with at least two elements

### Returns

The list without its first two elements.

### Examples

```lisp
(cddr '(1 2 3 4))    ; => (3 4)
(cddr '(a b))        ; => nil
```

### See Also

- `cdr` - Rest of list
- `caddr` - Third element

## `caddr`

Return the third element of a list. Equivalent to `(car (cdr (cdr x)))`. Same as `third`.

### Parameters

- `list` - A list with at least three elements

### Returns

The third element of the list.

### Examples

```lisp
(caddr '(1 2 3 4))   ; => 3
(caddr '(a b c))     ; => c
```

### See Also

- `cadr` - Second element
- `cadddr` - Fourth element

## `cadddr`

Return the fourth element of a list. Equivalent to `(car (cdr (cdr (cdr x))))`. Same as `fourth`.

### Parameters

- `list` - A list with at least four elements

### Returns

The fourth element of the list.

### Examples

```lisp
(cadddr '(1 2 3 4 5)) ; => 4
(cadddr '(a b c d))   ; => d
```

### See Also

- `caddr` - Third element
- `fourth` - Alias for this function

## `first`

Return the first element of a list. Alias for `car`.

### Parameters

- `list` - A non-empty list

### Returns

The first element of the list.

### Examples

```lisp
(first '(1 2 3))     ; => 1
(first '(a b c))     ; => a
```

### See Also

- `second` - Second element
- `rest` - All but first element

## `second`

Return the second element of a list. Alias for `cadr`.

### Parameters

- `list` - A list with at least two elements

### Returns

The second element of the list.

### Examples

```lisp
(second '(1 2 3))    ; => 2
(second '(a b c))    ; => b
```

### See Also

- `first` - First element
- `third` - Third element

## `third`

Return the third element of a list. Alias for `caddr`.

### Parameters

- `list` - A list with at least three elements

### Returns

The third element of the list.

### Examples

```lisp
(third '(1 2 3 4))   ; => 3
(third '(a b c))     ; => c
```

### See Also

- `second` - Second element
- `fourth` - Fourth element

## `fourth`

Return the fourth element of a list. Alias for `cadddr`.

### Parameters

- `list` - A list with at least four elements

### Returns

The fourth element of the list.

### Examples

```lisp
(fourth '(1 2 3 4 5)) ; => 4
(fourth '(a b c d))   ; => d
```

### See Also

- `third` - Third element
- `first` - First element

## `rest`

Return all elements except the first. Alias for `cdr`.

### Parameters

- `list` - A non-empty list

### Returns

A list of all elements except the first. Returns `nil` for a single-element list.

### Examples

```lisp
(rest '(1 2 3))      ; => (2 3)
(rest '(a))          ; => nil
```

### See Also

- `first` - First element
- `cdr` - Same function

## `set-car!`

Set the first element of a cons cell (mutating).

### Parameters

- `pair` - A cons cell to modify
- `value` - The new value for the car

### Returns

The new value.

### Examples

```lisp
(define x '(1 2 3))
(set-car! x 99)          ; => 99
x                        ; => (99 2 3)

(define y (cons 'a 'b))
(set-car! y 'z)          ; => z
y                        ; => (z . b)
```

### See Also

- `set-cdr!` - Set the rest of a cons cell
- `car` - Get the first element

## `set-cdr!`

Set the rest of a cons cell (mutating).

### Parameters

- `pair` - A cons cell to modify
- `value` - The new value for the cdr

### Returns

The new value.

### Examples

```lisp
(define x '(1 2 3))
(set-cdr! x '(99))       ; => (99)
x                        ; => (1 99)

(define y (cons 'a 'b))
(set-cdr! y 'z)          ; => z
y                        ; => (a . z)
```

### See Also

- `set-car!` - Set the first element of a cons cell
- `cdr` - Get the rest of a list

## `cons`

Construct a new list cell (cons cell).

### Parameters

- `element` - The value to place in the car (first position)
- `list` - The list to place in the cdr (rest position)

### Returns

A new list with `element` as the first item and `list` as the rest.

### Examples

```lisp
(cons 1 '(2 3))          ; => (1 2 3)
(cons 'a nil)            ; => (a)
(cons 1 (cons 2 '()))    ; => (1 2)
```

### See Also

- `car` - Get the first element
- `cdr` - Get the rest
- `list` - Create a list from multiple elements

## `list`

Create a list from the given elements.

### Parameters

- `elements...` - Zero or more elements to put in the list

### Returns

A new list containing all the provided elements. Returns `nil` for empty list.

### Examples

```lisp
(list 1 2 3)             ; => (1 2 3)
(list "a" "b" "c")      ; => ("a" "b" "c")
(list)                   ; => nil
(list (+ 1 2) (* 3 4))   ; => (3 12) (evaluates arguments)
```

### See Also

- `cons` - Construct a single list cell
- `append` - Concatenate lists

## `length`

Get the length of a sequence (list, string, or vector).

### Parameters

- `sequence` - A list, string, or vector

### Returns

Integer count of elements/characters in the sequence.

### Examples

```lisp
(length '(1 2 3))        ; => 3
(length "hello")         ; => 5
(length #(a b c))        ; => 3
(length '())              ; => 0
```

### Notes

Unified function for all sequence types. For strings, returns UTF-8 character count.

## `list-ref`

Get element at index from list (0-based).

### Parameters

- `list` - The list to access
- `index` - Zero-based index (integer)

### Returns

The element at the specified index.

### Examples

```lisp
(list-ref '(a b c) 0)        ; => a
(list-ref '(a b c) 2)        ; => c
(list-ref '(1 2 3 4) 3)      ; => 4
```

### Errors

Returns error if index is out of bounds.

## `reverse`

Reverse a list.

### Parameters

- `list` - The list to reverse

### Returns

New list with elements in reverse order.

### Examples

```lisp
(reverse '(1 2 3))           ; => (3 2 1)
(reverse '(a))               ; => (a)
(reverse '())                ; => nil
```

### Notes

Returns a new list (does not modify input).

## `append`

Concatenate multiple lists into a single list.

### Parameters

- `lists...` - Zero or more lists to concatenate

### Returns

A new list containing all elements from all input lists in order. Returns `nil` if no arguments provided.

### Examples

```lisp
(append '(1 2) '(3 4))               ; => (1 2 3 4)
(append '(1 2) '(3 4) '(5 6))        ; => (1 2 3 4 5 6)
(append '() '(1 2))                  ; => (1 2)
(append '(1 2) '())                  ; => (1 2)
(append)                             ; => nil
```

### Notes

- Returns a **new list** (does not modify input lists)
- Works with any number of lists

## `assoc`

Find key-value pair in association list using structural equality.

### Parameters

- `key` - Key to search for
- `alist` - Association list (list of `(key . value)` pairs)

### Returns

The first `(key . value)` pair where key matches (using `equal?`), or `nil` if not found.

### Examples

```lisp
(assoc 'name '((name . "Alice") (age . 30)))     ; => (name . "Alice")
(assoc 'city '((name . "Bob") (age . 25)))       ; => nil
(assoc '(1 2) '(((1 2) . "pair") (3 . "x")))   ; => ((1 2) . "pair")
```

### Notes

Uses deep structural comparison (`equal?`). For pointer equality, use `assq`.

### See Also

- `assq` - Pointer equality search
- `assv` - Value equality search
- `alist-get` - Get value directly

## `assq`

Find key-value pair in association list using pointer equality.

### Parameters

- `key` - Key to search for
- `alist` - Association list (list of `(key . value)` pairs)

### Returns

The first `(key . value)` pair where key matches (using `eq?`), or `nil` if not found.

### Examples

```lisp
(assq 'name '((name . "Alice") (age . 30)))      ; => (name . "Alice")
(assq 'city '((name . "Bob") (age . 25)))        ; => nil
```

### Notes

- Uses pointer equality (`eq?`) - fastest but only reliable for symbols
- Symbols are always interned, so `assq` is safe for symbol keys
- NOT reliable for numbers or strings (use `assoc` instead)

### See Also

- `assoc` - Structural equality search
- `eq?` - Pointer equality predicate

## `assv`

Find key-value pair in association list using value equality.

### Parameters

- `key` - Key to search for
- `alist` - Association list (list of `(key . value)` pairs)

### Returns

The first `(key . value)` pair where key matches (using `eqv?`), or `nil` if not found.

### Examples

```lisp
(assv 42 '((42 . "answer") (10 . "ten")))       ; => (42 . "answer")
(assv 'key '((key . "value") (x . "y")))        ; => (key . "value")
```

### Notes

Uses `eqv?` equality (same as `equal?` in this implementation).

### See Also

- `assoc` - Structural equality (equivalent in this implementation)
- `assq` - Pointer equality

## `alist-get`

Get value for key from association list with optional default.

### Parameters

- `key` - Key to search for
- `alist` - Association list (list of `(key . value)` pairs)
- `default` - Optional default value (defaults to `nil`)

### Returns

The value (cdr) of the first matching pair, or `default` if key not found.

### Examples

```lisp
(alist-get 'name '((name . "Alice") (age . 30)))        ; => "Alice"
(alist-get 'city '((name . "Bob")) "Unknown")          ; => "Unknown"
(alist-get 'missing '((a . 1) (b . 2)))                  ; => nil
```

### Notes

Returns the **value** directly, not the `(key . value)` pair.

### See Also

- `assoc` - Get full key-value pair
- `hash-ref` - Hash table lookup

## `member`

Find element in list using structural equality.

### Parameters

- `item` - Item to search for
- `list` - List to search in

### Returns

The tail of the list starting from the first match, or `nil` if not found.

### Examples

```lisp
(member 'b '(a b c d))          ; => (b c d)
(member 'x '(a b c))            ; => nil
(member "foo" '("bar" "foo" "baz"))  ; => ("foo" "baz")
```

### Notes

Uses deep structural comparison (`equal?`). For pointer equality, use `memq`.

### See Also

- `memq` - Pointer equality search
- `assoc` - Association list lookup

## `memq`

Find element in list using pointer equality.

### Parameters

- `item` - Item to search for
- `list` - List to search in

### Returns

The tail of the list starting from the first match, or `nil` if not found.

### Examples

```lisp
(memq 'b '(a b c d))            ; => (b c d)
(memq 'x '(a b c))              ; => nil
```

### Notes

- Uses pointer equality (`eq?`) - fastest but only reliable for symbols
- Symbols are always interned, so `memq` is safe for symbol searches
- NOT reliable for numbers or strings (use `member` instead)

### See Also

- `member` - Structural equality search
- `assq` - Association list lookup with eq?

## `map`

Apply function to each element of list, return new list of results.

### Parameters

- `function` - Function to apply to each element
- `list` - List to process

### Returns

New list containing results of applying `function` to each element.

### Examples

```lisp
(map (lambda (x) (* x 2)) '(1 2 3 4))    ; => (2 4 6 8)
(map car '((1 . 2) (3 . 4) (5 . 6)))     ; => (1 3 5)
(map string-upcase '("a" "b" "c"))      ; => ("A" "B" "C")
```

## `mapcar`

Apply function to each element of a list. Alias for `map`.

### Parameters

- `function` - Function to apply to each element
- `list` - List of elements

### Returns

New list of results from applying `function` to each element.

### Examples

```lisp
(mapcar (lambda (x) (* x 2)) '(1 2 3))  ; => (2 4 6)
```

### See Also

- `map` - Same function
- `filter` - Select elements matching predicate

## `filter`

Filter list elements that satisfy a predicate function.

### Parameters

- `predicate` - Function that returns true/false for each element
- `list` - List to filter

### Returns

New list containing only elements for which `predicate` returns non-nil.

### Examples

```lisp
(filter (lambda (x) (> x 0)) '(1 -2 3 -4 5))  ; => (1 3 5)
(filter even? '(1 2 3 4 5 6))                 ; => (2 4 6)
(filter string? '(1 "a" 2 "b"))              ; => ("a" "b")
```

## `apply`

Apply a function to a list of arguments.

### Parameters

- `function` - Function to call
- `args` - List of arguments to pass to the function

### Returns

Result of calling `function` with the elements of `args` as arguments.

### Examples

```lisp
(apply + '(1 2 3 4))        ; => 10
(apply list '(a b c))       ; => (a b c)
(apply max '(3 1 4 1 5))    ; => 5

;; Useful for calling functions with dynamic argument lists
(define args '(1 2 3))
(apply + args)              ; => 6
```
