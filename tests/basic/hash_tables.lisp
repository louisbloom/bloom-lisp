;; Hash table operations examples
;; Load test helper macros
(load "tests/test-helpers.lisp")

;; Create a hash table
(define ht (make-hash-table))

;; Check if hash-table? predicate works
(assert-true (hash-table? ht) "make-hash-table creates a hash table")

(assert-equal (hash-count ht) 0 "new hash table is empty")

;; Store values
(hash-set! ht "name" "Alice")
(hash-set! ht "age" 30)
(hash-set! ht "city" "New York")

(assert-equal (hash-count ht) 3 "hash table contains 3 entries")
;; Retrieve values
(assert-equal (hash-ref ht "name") "Alice" "hash-ref retrieves string value")
(assert-equal (hash-ref ht "age") 30 "hash-ref retrieves integer value")
(assert-equal (hash-ref ht "city") "New York" "hash-ref retrieves city value")

(assert-nil (hash-ref ht "missing") "hash-ref returns nil for missing key")

;; Update existing values
(hash-set! ht "age" 31)

(assert-equal (hash-ref ht "age") 31 "hash-set! updates existing value")

;; Remove entries
(hash-remove! ht "city")

(assert-nil (hash-ref ht "city") "hash-remove! removes entry")

(assert-equal (hash-count ht) 2 "hash-count decreases after removal")

;; Clear all entries
(hash-clear! ht)

(assert-equal (hash-count ht) 0 "hash-clear! empties hash table")

;; Hash tables are always truthy (even when empty - only nil is falsy in this Lisp)
(assert-true ht "empty hash table is truthy")
(assert-true (if ht #t nil) "empty hash table evaluates to true in conditional")

(hash-set! ht "key" "value")

(assert-true ht "non-empty hash table is truthy")

(assert-equal (hash-count ht) 1 "hash table has one entry after adding")

;; Use with symbols as keys
(hash-set! ht 'name "Bob")

(assert-equal (hash-ref ht 'name) "Bob"
 "hash-set! and hash-ref work with symbol keys")
;; String/symbol key interchangeability
(assert-equal (hash-ref ht "name") "Bob"
 "symbol key retrievable with string key")

(hash-set! ht "color" "red")

(assert-equal (hash-ref ht 'color) "red"
 "string key retrievable with symbol key")

;; ============================================================================
;; Non-string key types
;; ============================================================================
(define ht2 (make-hash-table))

;; Integer keys
(hash-set! ht2 42 "the answer")

(assert-equal (hash-ref ht2 42) "the answer" "integer key works")

(assert-nil (hash-ref ht2 43) "different integer returns nil")

;; Float keys
(hash-set! ht2 3.14 "pi")

(assert-equal (hash-ref ht2 3.14) "pi" "float key works")

;; Character keys
(hash-set! ht2 #\A "letter A")

(assert-equal (hash-ref ht2 #\A) "letter A" "char key works")

(assert-nil (hash-ref ht2 #\B) "different char returns nil")

;; Boolean keys (#f is nil in bloom-lisp, so they share a key)
(hash-set! ht2 #t "true val")

(assert-equal (hash-ref ht2 #t) "true val" "boolean true key works")

;; Nil key (same as #f)
(hash-set! ht2 nil "nil val")

(assert-equal (hash-ref ht2 nil) "nil val" "nil key works")
;; Verify count
(assert-equal (hash-count ht2) 5 "5 entries with mixed key types")

;; Integer keys don't collide with string keys
(hash-set! ht2 "42" "string forty-two")

(assert-equal (hash-ref ht2 42) "the answer"
 "integer 42 distinct from string 42")
(assert-equal (hash-ref ht2 "42") "string forty-two"
 "string 42 distinct from integer 42")

;; hash-keys returns original key types
(define ht3 (make-hash-table))

(hash-set! ht3 42 "int")
(hash-set! ht3 "hello" "str")

(let ((keys (hash-keys ht3)))
  (assert-equal (length keys) 2 "hash-keys returns 2 keys")
  ;; One key should be an integer, one a string
  (assert-true
   (or (and (number? (car keys)) (string? (car (cdr keys))))
       (and (string? (car keys)) (number? (car (cdr keys)))))
   "hash-keys preserves key types"))

;; Remove with non-string keys
(hash-remove! ht2 42)

(assert-nil (hash-ref ht2 42) "hash-remove! works with integer key")

(assert-equal (hash-ref ht2 "42") "string forty-two"
 "string 42 unaffected by integer 42 removal")

;; Update with non-string keys
(hash-set! ht2 3.14 "tau/2")

(assert-equal (hash-ref ht2 3.14) "tau/2" "float key value updated")
