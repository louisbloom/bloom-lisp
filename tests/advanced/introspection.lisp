;; Tests for type predicates, function introspection, and environment-bindings
(load "tests/test-helpers.lisp")

;; ============================================
;; Type predicates
;; ============================================
;; function?
(assert-true (function? (lambda (x) x)) "lambda is function?")

(assert-false (function? +) "builtin is not function?")
(assert-false (function? 42) "number is not function?")
(assert-false (function? "hello") "string is not function?")

;; macro?
(assert-true (macro? when) "when is macro?")

(assert-false (macro? +) "builtin is not macro?")
(assert-false (macro? (lambda (x) x)) "lambda is not macro?")

;; builtin?
(assert-true (builtin? +) "plus is builtin?")
(assert-true (builtin? car) "car is builtin?")

(assert-false (builtin? (lambda (x) x)) "lambda is not builtin?")
(assert-false (builtin? 42) "number is not builtin?")

;; callable?
(assert-true (callable? +) "builtin is callable?")
(assert-true (callable? (lambda (x) x)) "lambda is callable?")
(assert-true (callable? when) "macro is callable?")

(assert-false (callable? 42) "number is not callable?")
(assert-false (callable? "hello") "string is not callable?")

;; atom?
(assert-true (atom? 42) "number is atom?")
(assert-true (atom? "hello") "string is atom?")
(assert-true (atom? 'foo) "symbol is atom?")
(assert-true (atom? nil) "nil is atom?")

(assert-false (atom? '(1 2)) "list is not atom?")

;; boolean?
(assert-true (boolean? #t) "true is boolean?")

(assert-false (boolean? #f) "#f is nil, not boolean")
(assert-false (boolean? nil) "nil is not boolean?")
(assert-false (boolean? 1) "number is not boolean?")
(assert-false (boolean? "true") "string is not boolean?")

;; vector?
(assert-true (vector? #(1 2 3)) "vector literal is vector?")
(assert-true (vector? (make-vector 0)) "empty vector is vector?")

(assert-false (vector? '(1 2 3)) "list is not vector?")
(assert-false (vector? 42) "number is not vector?")

;; ============================================
;; Error introspection
;; ============================================
(let ((err (condition-case e (signal 'test-error "boom") (error e))))
  (assert-true (list? (error-stack err)) "error-stack returns a list")
  (assert-true (member "signal" (error-stack err))
   "error-stack includes signal"))

;; ============================================
;; Function introspection
;; ============================================
;; function-params
(define my-add (lambda (x y) (+ x y)))

(assert-equal (function-params my-add) '(x y) "function-params on lambda")
;; function-body
(assert-equal (function-body my-add) '((+ x y)) "function-body on lambda")
;; function-name
(assert-equal (function-name +) "+" "function-name on builtin")
(assert-equal (function-name car) "car" "function-name on car")

(assert-nil (function-name (lambda (x) x)) "anonymous lambda has no name")

;; Named function via defun
(defun greet (name) (concat "Hello, " name))

(assert-equal (function-name greet) "greet" "function-name on named function")
(assert-equal (function-params greet) '(name)
 "function-params on named function")

;; Macro introspection
(defmacro my-when (cond . body) `(if ,cond (progn ,@body) nil))

(assert-equal (function-params my-when) '(cond . body)
 "function-params on macro")

(assert-true (function? my-add) "my-add is function?")
(assert-true (macro? my-when) "my-when is macro?")

;; ============================================
;; environment-bindings
;; ============================================
;; environment-bindings returns alist for current frame
(define test-var-eb 99)

(let ((bindings (environment-bindings)))
  ;; Should find test-var-eb in bindings
  (assert-true (assoc 'test-var-eb bindings)
   "environment-bindings includes test-var-eb")
  (assert-equal (cdr (assoc 'test-var-eb bindings)) 99
   "environment-bindings value correct"))

