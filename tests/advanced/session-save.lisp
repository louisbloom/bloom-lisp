;; Tests for session introspection and session-save
(load "tests/test-helpers.lisp")

;; Helper: read all lines from a file into a list
(defun read-all-lines (filename)
  (let ((f (open filename "r"))
        (lines '()))
    (do ((line (read-line f) (read-line f)))
      ((null? line) (close f) (reverse lines))
      (set! lines (cons line lines)))))

;; Helper: read entire file as a single string
(defun read-file-string (filename) (join (read-all-lines filename) "\n"))

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

;; ============================================
;; session-save roundtrip tests
;; ============================================
(define session-file "tests/advanced/_test_session.lisp")

;; Define various types for roundtrip
(define rt-int 42)

(define rt-float 3.14)

(define rt-string "hello world")

(define rt-char #\A)

(define rt-bool #t)

(define rt-nil nil)

(define rt-keyword :my-key)

(define rt-list '(1 2 3))

(define rt-nested-list '((a 1) (b 2)))

(define rt-vector #(10 20 30))

(define rt-lambda (lambda (a b) (+ a b)))

(define rt-ht
  (let ((ht (make-hash-table))) (hash-set! ht "x" 1) (hash-set! ht "y" 2) ht))

;; Define a macro for roundtrip
(defmacro rt-macro (x) `(+ ,x 1))

;; Builtin alias
(define rt-alias +)

;; Nested list with lambda inside
(define rt-nested-with-fn (list 1 (lambda (x) (* x 2)) 3))

;; Save the session
(unwind-protect
  (progn (session-save session-file)
    ;; Verify the file exists and is loadable by reading it
    (define session-content (read-all-lines session-file))
    ;; The file should have content
    (assert-true (> (length session-content) 0) "session file has content")
    ;; Verify the file contains expected definitions
    (define content-str (join session-content "\n"))
    (assert-true (regex-match? "\\(define rt-int 42\\)" content-str)
     "session has rt-int")
    (assert-true (regex-match? "\\(define rt-string" content-str)
     "session has rt-string")
    (assert-true (regex-match? "\\(define rt-lambda" content-str)
     "session has rt-lambda")
    (assert-true (regex-match? "\\(defmacro rt-macro" content-str)
     "session has rt-macro")
    (assert-true (regex-match? "\\(define rt-alias" content-str)
     "session has rt-alias")
    (assert-true (regex-match? "\\(define rt-ht" content-str)
     "session has rt-ht"))
  ;; Cleanup
  (delete-file session-file))

;; ============================================
;; Docstrings with special characters roundtrip
;; ============================================
(define docstring-test-file "tests/advanced/_test_docstrings.lisp")

(defun fn-with-quotes (x)
  "A function that mentions \"hello\" and backslash \\ and newline."
  (+ x 1))

(defmacro mac-with-quotes (x) "A macro with \"quotes\" inside." `(+ ,x 1))

(unwind-protect
  (progn (session-save docstring-test-file)
    ;; The saved file should be loadable (no parse errors from unescaped quotes)
    (let ((content (read-file-string docstring-test-file)))
      ;; Verify escaped quotes appear in the output
      (assert-true (string-contains? content "\\\"hello\\\"")
       "docstring quotes are escaped in saved session")
      (assert-true (string-contains? content "\\\\")
       "docstring backslashes are escaped in saved session"))
    ;; Verify the file is loadable by actually loading it
    (load docstring-test-file)
    (assert-equal (fn-with-quotes 5) 6
     "function with quoted docstring works after reload"))
  (delete-file docstring-test-file))

;; ============================================
;; User redefining a builtin
;; ============================================
(define test-shadow-file "tests/advanced/_test_shadow.lisp")

(unwind-protect
  (progn (define my-plus +) (define + 42)
    (assert-equal + 42 "shadowed + is 42")
    (session-save test-shadow-file)
    ;; Verify the file contains the shadow
    (let ((content (read-file-string test-shadow-file)))
      (assert-true (regex-match? "\\(define \\+ 42\\)" content)
       "session has shadowed +")))
  (delete-file test-shadow-file))

