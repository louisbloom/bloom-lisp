;;; Test keyword operations
(load "tests/test-helpers.lisp")

;; Test keyword? predicate
(assert-true (keyword? :foo) "keyword? recognizes :foo")
(assert-true (keyword? :bar-baz) "keyword? recognizes :bar-baz")
(assert-true (keyword? :123) "keyword? recognizes :123")

;; Test keyword? with non-keywords
(assert-nil (keyword? 'foo) "keyword? rejects symbol")
(assert-nil (keyword? "foo") "keyword? rejects string")
(assert-nil (keyword? 42) "keyword? rejects integer")
(assert-nil (keyword? nil) "keyword? rejects nil")

;; Test self-evaluation
(assert-equal :foo :foo "keyword evaluates to itself")
(assert-equal :bar-baz :bar-baz "keyword with hyphen evaluates to itself")

;; Test eq comparison (pointer equality due to interning)
(assert-true (eq? :foo :foo) "same keywords are eq?")

(assert-nil (eq? :foo :bar) "different keywords are not eq?")
(assert-nil (eq? :foo 'foo) "keyword not eq? to symbol")

;; Test equal comparison
(assert-true (equal? :foo :foo) "same keywords are equal?")

(assert-nil (equal? :foo :bar) "different keywords are not equal?")
(assert-nil (equal? :foo 'foo) "keyword not equal? to symbol")

;; Test keyword-name
(assert-equal (keyword-name :foo) "foo"
 "keyword-name returns name without colon")
(assert-equal (keyword-name :bar-baz) "bar-baz" "keyword-name handles hyphens")
;; Test keywords in data structures
(assert-equal (car '(:a 1)) :a "keyword in list car")
;; Test printing
(assert-equal (format nil "~S" :foo) ":foo" "keyword prints with colon")
(assert-equal (format nil "~A" :foo) ":foo" "keyword princ with colon")
