;; Package system tests
(load "tests/test-helpers.lisp")

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
;; list-packages includes core, user, math
;; ===========================================
(let ((pkgs (list-packages)))
  (assert-true (memq 'core pkgs) "list-packages includes core")
  (assert-true (memq 'user pkgs) "list-packages includes user")
  (assert-true (memq 'math pkgs) "list-packages includes math"))

;; ===========================================
;; load restores *package* after loading
;; ===========================================
;; We're in "user" right now
(assert-equal (current-package) 'user "still in user package before load test")
;; ===========================================
;; Unqualified names resolve across packages
;; ===========================================
(assert-equal (math-add 10 20) 30 "unqualified names resolve across packages")

;; ===========================================
;; in-package accepts symbol argument
;; ===========================================
(in-package 'user)

(assert-equal (current-package) 'user "in-package accepts symbol argument")

