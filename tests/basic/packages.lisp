;; Package system tests
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

;; ===========================================
;; current-package defaults to "user"
;; ===========================================
(assert-equal (current-package) 'user "current-package returns user by default")

;; ===========================================
;; in-package changes current package
;; ===========================================
(in-package "math")

(assert-equal (current-package) 'math "in-package changes package to math")

;; Define bindings in math package
(define math-add (lambda (a b) (+ a b)))

(define math-pi 3.14159)

;; ===========================================
;; package-symbols returns bindings
;; ===========================================
(let ((syms (package-symbols "math")))
  (assert-true (list? syms) "package-symbols returns a list")
  ;; Should contain math-add and math-pi
  (assert-true (assoc 'math-add syms) "package-symbols contains math-add")
  (assert-true (assoc 'math-pi syms) "package-symbols contains math-pi"))
;; package-symbols accepts symbol argument
(let ((syms (package-symbols 'math)))
  (assert-true (assoc 'math-add syms) "package-symbols accepts symbol arg"))

;; ===========================================
;; Qualified access with pkg:symbol
;; ===========================================
(in-package "user")

(assert-equal math:math-pi 3.14159 "pkg:symbol resolves across packages")
(assert-equal (math:math-add 2 3) 5 "pkg:symbol resolves function in package")
;; ===========================================
;; core:+ resolves to addition
;; ===========================================
(assert-equal (core:+ 10 20) 30 "core:+ resolves to addition")

;; ===========================================
;; Undefined pkg:symbol is an error
;; ===========================================
(assert-error (eval 'math:nonexistent) "undefined pkg:symbol signals error")

;; ===========================================
;; list-packages includes core, user, math
;; ===========================================
(let ((pkgs (list-packages)))
  (assert-true (memq 'core pkgs) "list-packages includes core")
  (assert-true (memq 'user pkgs) "list-packages includes user")
  (assert-true (memq 'math pkgs) "list-packages includes math"))

;; ===========================================
;; Unqualified names resolve across packages
;; ===========================================
(assert-equal (current-package) 'user "still in user package")
(assert-equal (math-add 10 20) 30 "unqualified names resolve across packages")

;; ===========================================
;; in-package accepts symbol argument
;; ===========================================
(in-package 'user)

(assert-equal (current-package) 'user "in-package accepts symbol argument")

;; ===========================================
;; package- prefixed aliases
;; ===========================================
(package-set "alias-test")

(assert-equal (package-current) 'alias-test
 "package-set / package-current work")

(define alias-test-var 1)

(package-set "user")

(let ((pkgs (package-list)))
  (assert-true (memq 'core pkgs) "package-list includes core")
  (assert-true (memq 'alias-test pkgs) "package-list includes alias-test"))

;; ===========================================
;; package-save roundtrip
;; ===========================================
(define save-file "tests/basic/_test_pkg_save.lisp")

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

(defmacro rt-macro (x) `(+ ,x 1))

(define rt-alias +)

(unwind-protect
  (progn (package-save save-file)
    (define content-str (read-file-string save-file))
    (assert-true (> (length (split content-str "\n")) 0)
     "package-save file has content")
    (assert-true (regex-match? "\\(define rt-int 42\\)" content-str)
     "package-save has rt-int")
    (assert-true (regex-match? "\\(define rt-string" content-str)
     "package-save has rt-string")
    (assert-true (regex-match? "\\(define rt-lambda" content-str)
     "package-save has rt-lambda")
    (assert-true (regex-match? "\\(defmacro rt-macro" content-str)
     "package-save has rt-macro")
    (assert-true (regex-match? "\\(define rt-alias" content-str)
     "package-save has rt-alias")
    (assert-true (regex-match? "\\(define rt-ht" content-str)
     "package-save has rt-ht"))
  (delete-file save-file))

;; ===========================================
;; package-save with specific package argument
;; ===========================================
(define pkg-save-file "tests/basic/_test_pkg_save2.lisp")

(unwind-protect
  (progn (package-save pkg-save-file "math")
    (define content-str (read-file-string pkg-save-file))
    (assert-true (regex-match? "math-pi" content-str)
     "package-save with package arg saves that package")
    (assert-false (regex-match? "rt-int" content-str)
     "package-save with package arg does not save other packages"))
  (delete-file pkg-save-file))

;; ===========================================
;; Docstrings with special characters roundtrip
;; ===========================================
(define docstring-test-file "tests/basic/_test_docstrings.lisp")

(defun fn-with-quotes (x)
  "A function that mentions \"hello\" and backslash \\ and newline."
  (+ x 1))

(defmacro mac-with-quotes (x) "A macro with \"quotes\" inside." `(+ ,x 1))

(unwind-protect
  (progn (package-save docstring-test-file)
    (let ((content (read-file-string docstring-test-file)))
      (assert-true (string-contains? content "\\\"hello\\\"")
       "docstring quotes are escaped in saved file")
      (assert-true (string-contains? content "\\\\")
       "docstring backslashes are escaped in saved file"))
    (load docstring-test-file)
    (assert-equal (fn-with-quotes 5) 6
     "function with quoted docstring works after reload"))
  (delete-file docstring-test-file))

;; ===========================================
;; User redefining a builtin
;; ===========================================
(define test-shadow-file "tests/basic/_test_shadow.lisp")

(unwind-protect
  (progn (define my-plus +) (define + 42)
    (assert-equal + 42 "shadowed + is 42")
    (package-save test-shadow-file)
    (let ((content (read-file-string test-shadow-file)))
      (assert-true (regex-match? "\\(define \\+ 42\\)" content)
       "package-save has shadowed +")))
  (delete-file test-shadow-file))

