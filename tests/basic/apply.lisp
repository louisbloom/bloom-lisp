;; Tests for (apply fn args-list)
(load "tests/test-helpers.lisp")

;; Apply with builtins
(assert-equal (apply + (list 1 2 3)) 6 "apply builtin +")
(assert-equal (apply * (list 2 3 4)) 24 "apply builtin *")
(assert-equal (apply car (list (list 1 2))) 1 "apply builtin car")
(assert-equal (apply list (list 1 2 3)) '(1 2 3) "apply builtin list")
;; Apply with lambda
(assert-equal (apply (lambda (a b) (+ a b)) (list 3 4)) 7 "apply lambda")
(assert-equal (apply (lambda (x) (* x x)) (list 5)) 25 "apply lambda unary")
;; Apply with no args
(assert-equal (apply (lambda () 42) nil) 42 "apply lambda no args")
;; Apply with optional parameters
(assert-equal (apply (lambda (a &optional b) (list a b)) (list 1)) '(1 nil)
 "apply lambda optional not provided")
(assert-equal (apply (lambda (a &optional b) (list a b)) (list 1 2)) '(1 2)
 "apply lambda optional provided")
;; Apply with rest parameters
(assert-equal (apply (lambda (a &rest more) (list a more)) (list 1 2 3))
 '(1 (2 3)) "apply lambda rest")
(assert-equal (apply (lambda (&rest all) all) (list 1 2 3)) '(1 2 3)
 "apply lambda rest only")

;; Apply with named function
(define double (lambda (x) (* x 2)))

(assert-equal (apply double (list 7)) 14 "apply named function")

;; Error cases
(assert-error (apply 42 (list 1)) "apply non-function")
(assert-error (apply + 5) "apply non-list args")

