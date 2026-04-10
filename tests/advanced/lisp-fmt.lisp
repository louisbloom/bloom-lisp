;;; Lisp Source Code Formatter Tests
;;; Tests for telnet-lisp/lisp/lisp-fmt.lisp
(load "tests/test-helpers.lisp")
(load "lisp/lisp-fmt.lisp")

;; ============================================================
;; Basic Atom Formatting
;; ============================================================
(assert-equal (sexp-to-string 42) "42" "integer formatting")
(assert-equal (sexp-to-string 3.14) "3.14" "float formatting")
(assert-equal (sexp-to-string "hello") "\"hello\"" "string formatting")
(assert-equal (sexp-to-string 'foo) "foo" "symbol formatting")
(assert-equal (sexp-to-string #t) "#t" "true formatting")
;; Note: In this Lisp, #f and nil are the same value
(assert-equal (sexp-to-string nil) "nil" "nil/false formatting")
;; ============================================================
;; Simple List Formatting
;; ============================================================
;; Note: '() equals nil in this Lisp, which formats as "nil"
(assert-equal (sexp-to-string 'nil) "nil" "empty list is nil")
(assert-equal (sexp-to-string '(a)) "(a)" "single element list")
(assert-equal (sexp-to-string '(a b c)) "(a b c)" "simple list")
(assert-equal (sexp-to-string '(+ 1 2)) "(+ 1 2)" "function call list")
(assert-equal (sexp-to-string '((a b) (c d))) "((a b) (c d))" "nested lists")
;; ============================================================
;; Vector Formatting
;; ============================================================
(assert-equal (sexp-to-string #()) "#()" "empty vector")
(assert-equal (sexp-to-string #(1 2 3)) "#(1 2 3)" "simple vector")
;; ============================================================
;; Quote/Quasiquote Shorthand
;; ============================================================
;; Quote shorthand
(assert-equal (sexp-to-string '(quote x)) "'x" "quote outputs as apostrophe")
(assert-equal (sexp-to-string '(quote (a b c))) "'(a b c)" "quoted list")
(assert-equal (format-sexp '(quote (a b c)) 0) "'(a b c)"
 "format-sexp uses quote shorthand")
;; Quasiquote shorthand
(assert-equal (sexp-to-string '(quasiquote x)) "`x"
 "quasiquote outputs as backtick")
(assert-equal (sexp-to-string '(quasiquote (a b c))) "`(a b c)"
 "quasiquoted list")
;; Unquote shorthand
(assert-equal (sexp-to-string '(unquote x)) ",x" "unquote outputs as comma")
(assert-equal (sexp-to-string '(unquote-splicing x)) ",@x"
 "unquote-splicing outputs as comma-at")
;; Nested quasiquote with unquote
(assert-equal (sexp-to-string '(quasiquote (a (unquote b) c))) "`(a ,b c)"
 "quasiquote with nested unquote")
;; Complex quasiquote
(assert-equal
 (sexp-to-string '(quasiquote (list (unquote a) (unquote-splicing b))))
 "`(list ,a ,@b)" "quasiquote with unquote and unquote-splicing")
;; ============================================================
;; Format-sexp Tests (with indentation)
;; ============================================================
;; Simple expressions stay on one line
(assert-equal (format-sexp '(+ 1 2) 0) "(+ 1 2)" "simple expr on one line")
;; Nested short expressions
(assert-equal (format-sexp '(+ (* 2 3) 4) 0) "(+ (* 2 3) 4)"
 "nested expr on one line")
;; ============================================================
;; Special Form Indentation
;; ============================================================
;; Short defun stays on one line
(assert-equal (format-sexp '(defun foo (x) (+ x 1)) 0)
 "(defun foo (x) (+ x 1))" "short defun on one line")
;; simple define
(assert-equal (format-sexp '(define x 42) 0) "(define x 42)"
 "simple define on one line")
;; Longer defun uses multi-line (via format-special-form)
;; Build a defun that's too long for one line
(assert-equal
 (format-sexp
  '(defun calculate-something-complex (first-param second-param)
     (+ first-param second-param)) 0)
 "(defun calculate-something-complex (first-param second-param)\n  (+ first-param second-param))"
 "long defun gets body indent")
;; ============================================================
;; Let Form Formatting
;; ============================================================
;; Short let stays on one line
(assert-equal (format-sexp '(let ((x 1)) x) 0) "(let ((x 1)) x)"
 "short let on one line")
;; Longer let* forces multi-line
(assert-equal
 (format-sexp
  '(let* ((first-variable 100)
          (second-variable 200))
     (+ first-variable second-variable)) 0)
 "(let* ((first-variable 100)\n       (second-variable 200))\n  (+ first-variable second-variable))"
 "longer let* with multiple bindings")
;; ============================================================
;; Cond Form Formatting
;; ============================================================
;; Short cond stays on one line
(assert-equal (format-sexp '(cond ((= x 1) "one") (#t "other")) 0)
 "(cond ((= x 1) \"one\") (#t \"other\"))" "short cond on one line")
;; ============================================================
;; If Form Formatting
;; ============================================================
;; Short if stays on one line
(assert-equal (format-sexp '(if test then else) 0) "(if test then else)"
 "short if on one line")
;; ============================================================
;; Dotted Pair Formatting
;; ============================================================
(assert-equal (sexp-to-string (cons 'a 'b)) "(a . b)" "dotted pair")
(assert-equal (sexp-to-string '(a b . c)) "(a b . c)" "improper list")

;; ============================================================
;; Round-trip Formatting (idempotence)
;; ============================================================
;; Format twice should produce same result
(let ((expr '(defun test (a b) (+ a b))))
  (let ((once (format-sexp expr 0))
        (twice (format-sexp expr 0)))
    (assert-equal once twice "format is idempotent")))

;; ============================================================
;; Comment-Preservation Regression Tests
;; ============================================================
;; These use format-source-string which runs source text through the
;; full reader → parser → formatter pipeline so comment handling is
;; actually exercised.

;; Bug 1: comment immediately after ( in a quoted list must not be eaten.
(assert-equal
 (format-source-string "(define colors '(;; primary\n  a\n  b\n  c))\n")
 "(define colors\n  '(;; primary\n    a\n    b\n    c))\n"
 "comment immediately after ( in quoted list is preserved")

;; Bug 2: trailing comment before ) must not be eaten and ) must be on
;; its own line (otherwise ; swallows the )).
(assert-equal
 (format-source-string "(define z '(a b c\n  ;; trailing\n  ))\n")
 "(define z\n  '(a b c\n    ;; trailing\n   ))\n"
 "trailing comment before ) is preserved and ) is on its own line")

;; Bug 3: a quoted list with a mid-list comment aligns continuation
;; lines correctly under the first element (not two spaces extra).
;; b and c share a source line so they pack together; the alignment
;; check is that ;; mid and b are both at col 4 under a at col 4.
(assert-equal
 (format-source-string "(define y '(a\n  ;; mid\n  b c))\n")
 "(define y\n  '(a\n    ;; mid\n    b c))\n"
 "quoted list mid-comment aligns continuation lines correctly")

;; Bug 4: a quoted list with comments is NOT squeezed inline.
(assert-equal
 (format-source-string "(define x '(;; first\n  a))\n")
 "(define x\n  '(;; first\n    a))\n"
 "quoted list with comment is rendered multi-line, not flattened")

;; Quasiquoted list with first-child comment
(assert-equal
 (format-source-string "(define w `(;; first\n  a b))\n")
 "(define w\n  `(;; first\n    a b))\n"
 "quasiquoted list with first-child comment is preserved")

;; Vertical intent: elements on different source lines stay on
;; different output lines, even when they would fit inline.
(assert-equal
 (format-source-string "'(a\n  b\n  c)\n")
 "'(a\n  b\n  c)\n"
 "vertical intent preserved: one element per line stays vertical")

;; Inline comment on let binding stays inline
(assert-equal
 (format-source-string "(let ((a 1) ;; inline\n      (b 2))\n  a)\n")
 "(let ((a 1) ;; inline\n      (b 2))\n  a)\n"
 "inline comment on let binding stays inline with the binding")

;; Leading comment on later let binding
(assert-equal
 (format-source-string
  "(let ((a 1)\n      ;; leading\n      (b 2))\n  a)\n")
 "(let ((a 1)\n      ;; leading\n      (b 2))\n  a)\n"
 "leading comment on later let binding is preserved above it")

;; Top-level comment between defines: blank line before, no blank after
(assert-equal
 (format-source-string
  "(define x 1)\n\n;; section header\n(define y 2)\n")
 "(define x 1)\n\n;; section header\n(define y 2)\n"
 "top-level section header has blank before, single newline after")

;; Top-level inline trailing comment does not consume blank line after
(assert-equal
 (format-source-string "(define x 1) ;; note\n\n(define y 2)\n")
 "(define x 1) ;; note\n\n(define y 2)\n"
 "top-level inline trailing comment preserves blank line to next form")

;; Two consecutive comment lines stay contiguous
(assert-equal
 (format-source-string "(define x 1)\n\n;; line 1\n;; line 2\n(define y 2)\n")
 "(define x 1)\n\n;; line 1\n;; line 2\n(define y 2)\n"
 "consecutive comment lines stay tight-clustered")

;; Format-source-string is idempotent on every comment case
(let ((cases
       (list "(define colors '(;; primary\n  a\n  b\n  c))\n"
        "(define z '(a b c\n  ;; trailing\n  ))\n"
        "(define y '(a\n  ;; mid\n  b c))\n"
        "(let ((a 1) ;; inline\n      (b 2))\n  a)\n"
        "(define x 1)\n\n;; section\n(define y 2)\n")))
  (do ((remaining cases (cdr remaining))) ((null? remaining))
    (let* ((src (car remaining))
           (once (format-source-string src))
           (twice (format-source-string once)))
      (assert-equal once twice "format-source-string is idempotent"))))

;; Self-format smoke: formatting lisp-fmt.lisp must be byte-identical to
;; the on-disk source. Catches any regression in one line.
(let* ((source (read-file-to-string "lisp/lisp-fmt.lisp"))
       (formatted (format-source-string source)))
  (assert-equal (string-trim source) (string-trim formatted)
   "lisp-fmt.lisp self-format is byte-identical"))

;; ============================================================
;; Success message
;; ============================================================
(princ "All formatter tests passed!\n")

