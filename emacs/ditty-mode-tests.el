;;; ditty-mode-tests.el --- Tests for ditty-mode  -*- lexical-binding: t; -*-

;; This file is free software; you can redistribute it and/or modify
;; it under the terms of the GNU General Public License as published by
;; the Free Software Foundation; either version 3, or (at your option)
;; any later version.

;;; Commentary:

;; ERT test suite for ditty-mode.

;;; Code:

(require 'ert)
(require 'ditty-mode)

;; ============================================================================
;; Helpers
;; ============================================================================

(defun ditty-test-face-at (text pos)
  "Insert TEXT into a temp buffer in `ditty-mode', return face at POS."
  (with-temp-buffer
    (ditty-mode)
    (insert text)
    (font-lock-ensure)
    (get-text-property pos 'face)))

(defun ditty-test-face-has (text pos expected-face)
  "Return non-nil if the face at POS in TEXT includes EXPECTED-FACE."
  (let ((f (ditty-test-face-at text pos)))
    (or (eq f expected-face)
        (and (listp f) (memq expected-face f)))))

(defun ditty-test-indent (text)
  "Insert TEXT, run `indent-region', return the indented result."
  (with-temp-buffer
    (ditty-mode)
    (insert text)
    (indent-region (point-min) (point-max))
    (buffer-string)))

;; ============================================================================
;; Mode activation
;; ============================================================================

(ert-deftest ditty-test-mode-name ()
  "Mode name should be Bloom-Lisp."
  (with-temp-buffer
    (ditty-mode)
    (should (equal mode-name "Bloom-Lisp"))))

(ert-deftest ditty-test-derived-from-lisp-mode ()
  "Should derive from lisp-mode."
  (with-temp-buffer
    (ditty-mode)
    (should (derived-mode-p 'lisp-mode))))

(ert-deftest ditty-test-comment-syntax ()
  "Semicolon should start a comment."
  (with-temp-buffer
    (ditty-mode)
    (insert "; a comment\n")
    (font-lock-ensure)
    (let ((ppss (syntax-ppss 5)))
      (should (nth 4 ppss)))))

(ert-deftest ditty-test-string-syntax ()
  "Double-quote should delimit a string."
  (with-temp-buffer
    (ditty-mode)
    (insert "\"hello\"")
    (font-lock-ensure)
    (let ((ppss (syntax-ppss 4)))
      (should (nth 3 ppss)))))

(ert-deftest ditty-test-paren-syntax ()
  "Parens should be paired open/close."
  (with-temp-buffer
    (ditty-mode)
    (should (eq (char-syntax ?\() ?\())
    (should (eq (char-syntax ?\)) ?\)))))

;; ============================================================================
;; Font-lock: special forms
;; ============================================================================

(defmacro ditty--deftest-special-form (name form pos)
  "Generate an ERT test asserting FORM at POS gets the special-form face.
NAME is spliced into the test name."
  (declare (indent 1))
  `(ert-deftest ,(intern (format "ditty-test-special-form-%s" name)) ()
     ,(format "`%s' should get ditty-special-form-face." name)
     (should (eq (ditty-test-face-at ,form ,pos)
                 'ditty-special-form-face))))

(ditty--deftest-special-form quote       "(quote x)"                2)
(ditty--deftest-special-form quasiquote  "(quasiquote x)"           2)
(ditty--deftest-special-form if          "(if t 1 2)"               2)
(ditty--deftest-special-form define      "(define x 1)"             2)
(ditty--deftest-special-form set!        "(set! x 1)"               2)
(ditty--deftest-special-form lambda      "(lambda (x) x)"           2)
(ditty--deftest-special-form defmacro    "(defmacro m (x) x)"       2)
(ditty--deftest-special-form let         "(let ((x 1)) x)"          2)
(ditty--deftest-special-form let*        "(let* ((x 1)) x)"         2)
(ditty--deftest-special-form progn       "(progn 1 2 3)"            2)
(ditty--deftest-special-form do          "(do () (t) 1)"            2)
(ditty--deftest-special-form cond        "(cond (t 1))"             2)
(ditty--deftest-special-form case        "(case x ((1) 'a))"        2)
(ditty--deftest-special-form and         "(and 1 2)"                2)
(ditty--deftest-special-form or          "(or 1 2)"                 2)
(ditty--deftest-special-form condition-case "(condition-case e x y)" 2)
(ditty--deftest-special-form unwind-protect "(unwind-protect x y)"   2)
(ditty--deftest-special-form package-ref "(package-ref pkg sym)"    2)

(ert-deftest ditty-test-special-form-negative ()
  "A non-special-form head should NOT get ditty-special-form-face."
  (should-not (eq (ditty-test-face-at "(notaspecialform x)" 2)
                  'ditty-special-form-face)))

(ert-deftest ditty-test-special-form-not-bare-symbol ()
  "A special-form name NOT in head position should not be highlighted."
  ;; `if' appears as an argument here, not as a call head.
  (should-not (eq (ditty-test-face-at "(foo if bar)" 6)
                  'ditty-special-form-face)))

;; ============================================================================
;; Font-lock: stdlib macros (defined in src/lisp.c)
;; ============================================================================

(defmacro ditty--deftest-stdlib (name form pos)
  "Generate an ERT test asserting FORM at POS gets the stdlib-macro face."
  (declare (indent 1))
  `(ert-deftest ,(intern (format "ditty-test-stdlib-%s" name)) ()
     ,(format "`%s' should get ditty-stdlib-face." name)
     (should (eq (ditty-test-face-at ,form ,pos)
                 'ditty-stdlib-face))))

(ditty--deftest-stdlib defun    "(defun f () 1)"            2)
(ditty--deftest-stdlib when     "(when t 1)"                2)
(ditty--deftest-stdlib unless   "(unless t 1)"              2)
(ditty--deftest-stdlib defvar   "(defvar x 1)"              2)
(ditty--deftest-stdlib defconst "(defconst x 1)"            2)
(ditty--deftest-stdlib defalias "(defalias 'a 'b)"          2)

;; ============================================================================
;; Font-lock: definition names
;; ============================================================================

(ert-deftest ditty-test-defname-defun ()
  "Name after `defun' should get definition-name-face."
  (should (eq (ditty-test-face-at "(defun my-fn () 1)" 8)
              'ditty-definition-name-face)))

(ert-deftest ditty-test-defname-define ()
  "Name after `define' should get definition-name-face."
  (should (eq (ditty-test-face-at "(define my-var 1)" 9)
              'ditty-definition-name-face)))

(ert-deftest ditty-test-defname-define-paren ()
  "Name after `(define (' (function shorthand) should get definition-name-face."
  (should (eq (ditty-test-face-at "(define (my-fn x) x)" 10)
              'ditty-definition-name-face)))

(ert-deftest ditty-test-defname-defmacro ()
  "Name after `defmacro' should get definition-name-face."
  (should (eq (ditty-test-face-at "(defmacro my-m (x) x)" 11)
              'ditty-definition-name-face)))

(ert-deftest ditty-test-defname-defvar ()
  "Name after `defvar' should get definition-name-face."
  (should (eq (ditty-test-face-at "(defvar my-var 1)" 9)
              'ditty-definition-name-face)))

(ert-deftest ditty-test-defname-defconst ()
  "Name after `defconst' should get definition-name-face."
  (should (eq (ditty-test-face-at "(defconst my-const 1)" 11)
              'ditty-definition-name-face)))

;; ============================================================================
;; Font-lock: constants (nil, #t, #f)
;; ============================================================================

(ert-deftest ditty-test-constant-nil ()
  "`nil' should get ditty-constant-face."
  (should (eq (ditty-test-face-at "(list nil)" 7)
              'ditty-constant-face)))

(ert-deftest ditty-test-constant-sharp-t ()
  "`#t' should get ditty-constant-face."
  (should (eq (ditty-test-face-at "(list #t)" 7)
              'ditty-constant-face)))

(ert-deftest ditty-test-constant-sharp-f ()
  "`#f' should get ditty-constant-face."
  (should (eq (ditty-test-face-at "(list #f)" 7)
              'ditty-constant-face)))

;; ============================================================================
;; Font-lock: keyword literals
;; ============================================================================

(ert-deftest ditty-test-keyword-literal ()
  "A `:keyword' should get ditty-keyword-literal-face."
  (should (eq (ditty-test-face-at "(list :foo)" 7)
              'ditty-keyword-literal-face)))

(ert-deftest ditty-test-keyword-with-dashes ()
  "A `:keyword-with-dashes' should get keyword-literal-face."
  (should (eq (ditty-test-face-at "(list :foo-bar)" 7)
              'ditty-keyword-literal-face)))

(ert-deftest ditty-test-keyword-in-string-not-highlighted ()
  "A colon inside a string is not a keyword literal."
  (should-not (eq (ditty-test-face-at "(x \":foo\")" 5)
                  'ditty-keyword-literal-face)))

;; ============================================================================
;; Font-lock: character literals
;; ============================================================================

(ert-deftest ditty-test-char-literal ()
  "`#\\a' should get ditty-character-face."
  (should (ditty-test-face-has "(list #\\a)" 7
                                    'ditty-character-face)))

(ert-deftest ditty-test-char-named ()
  "`#\\space' should get ditty-character-face."
  (should (ditty-test-face-has "(list #\\space)" 7
                                    'ditty-character-face)))

(ert-deftest ditty-test-char-named-newline ()
  "`#\\newline' should get ditty-character-face."
  (should (ditty-test-face-has "(list #\\newline)" 7
                                    'ditty-character-face)))

(ert-deftest ditty-test-char-hex ()
  "`#\\x41' should get ditty-character-face."
  (should (ditty-test-face-has "(list #\\x41)" 7
                                    'ditty-character-face)))

(ert-deftest ditty-test-char-unicode ()
  "`#\\u03B1' should get ditty-character-face."
  (should (ditty-test-face-has "(list #\\u03B1)" 7
                                    'ditty-character-face)))

;; ============================================================================
;; Font-lock: package-qualified symbols
;; ============================================================================

(ert-deftest ditty-test-package-prefix ()
  "The `pkg' prefix in `pkg:symbol' should get ditty-package-face."
  (should (eq (ditty-test-face-at "(mypkg:foo)" 2)
              'ditty-package-face)))

(ert-deftest ditty-test-package-name-not-package ()
  "The symbol after `:' in `pkg:symbol' should NOT get package-face."
  (should-not (eq (ditty-test-face-at "(mypkg:foo)" 9)
                  'ditty-package-face)))

;; ============================================================================
;; Font-lock: builtins (one test per category, representative sample)
;; ============================================================================

(defmacro ditty--deftest-builtin (label form pos)
  "Generate an ERT test asserting FORM at POS gets ditty-builtin-face.
LABEL identifies the builtin for the test name."
  (declare (indent 1))
  `(ert-deftest ,(intern (format "ditty-test-builtin-%s" label)) ()
     ,(format "`%s' should get ditty-builtin-face." label)
     (should (eq (ditty-test-face-at ,form ,pos)
                 'ditty-builtin-face))))

;; Arithmetic
(ditty--deftest-builtin plus       "(+ 1 2)"              2)
(ditty--deftest-builtin minus      "(- 1 2)"              2)
(ditty--deftest-builtin times      "(* 1 2)"              2)
(ditty--deftest-builtin divide     "(/ 1 2)"              2)
(ditty--deftest-builtin quotient   "(quotient 7 2)"       2)
(ditty--deftest-builtin even?      "(even? 2)"            2)
;; Comparison
(ditty--deftest-builtin equals     "(= 1 1)"              2)
(ditty--deftest-builtin eq?        "(eq? 'a 'a)"          2)
(ditty--deftest-builtin equal?     "(equal? 1 1)"         2)
(ditty--deftest-builtin string=?   "(string=? \"a\" \"a\")" 2)
(ditty--deftest-builtin not        "(not t)"              2)
;; Type predicates
(ditty--deftest-builtin null?      "(null? nil)"          2)
(ditty--deftest-builtin pair?      "(pair? '(1))"         2)
(ditty--deftest-builtin hash-table? "(hash-table? x)"     2)
(ditty--deftest-builtin keyword?   "(keyword? :a)"        2)
(ditty--deftest-builtin callable?  "(callable? f)"        2)
;; Characters
(ditty--deftest-builtin char-upcase "(char-upcase c)"     2)
(ditty--deftest-builtin char-alphabetic? "(char-alphabetic? c)" 2)
(ditty--deftest-builtin char=?     "(char=? a b)"         2)
;; Strings
(ditty--deftest-builtin concat     "(concat \"a\" \"b\")" 2)
(ditty--deftest-builtin string-append "(string-append \"a\")" 2)
(ditty--deftest-builtin split      "(split \",\" s)"      2)
(ditty--deftest-builtin join       "(join xs \",\")"      2)
(ditty--deftest-builtin substring  "(substring s 0 3)"    2)
(ditty--deftest-builtin string-trim "(string-trim s)"     2)
(ditty--deftest-builtin string-match? "(string-match? r s)" 2)
(ditty--deftest-builtin number->string "(number->string 1)" 2)
;; Symbols
(ditty--deftest-builtin symbol->string "(symbol->string 'a)" 2)
;; Lists
(ditty--deftest-builtin car        "(car '(1))"           2)
(ditty--deftest-builtin cdr        "(cdr '(1))"           2)
(ditty--deftest-builtin cons       "(cons 1 2)"           2)
(ditty--deftest-builtin length     "(length '(1))"        2)
(ditty--deftest-builtin mapcar     "(mapcar f xs)"        2)
(ditty--deftest-builtin filter     "(filter p xs)"        2)
(ditty--deftest-builtin assoc      "(assoc k a)"          2)
(ditty--deftest-builtin memq       "(memq x xs)"          2)
(ditty--deftest-builtin apply      "(apply f xs)"         2)
(ditty--deftest-builtin set-car!   "(set-car! p 1)"       2)
;; Vectors
(ditty--deftest-builtin make-vector "(make-vector 10)"    2)
(ditty--deftest-builtin vector-push! "(vector-push! v x)" 2)
;; Hash tables
(ditty--deftest-builtin make-hash-table "(make-hash-table)" 2)
(ditty--deftest-builtin hash-ref   "(hash-ref h k)"       2)
(ditty--deftest-builtin hash-set!  "(hash-set! h k v)"    2)
(ditty--deftest-builtin hash-keys  "(hash-keys h)"        2)
;; Regex
(ditty--deftest-builtin regex-match? "(regex-match? r s)" 2)
(ditty--deftest-builtin regex-replace-all "(regex-replace-all r s t)" 2)
(ditty--deftest-builtin regex-find "(regex-find r s)"     2)
;; File I/O
(ditty--deftest-builtin open-io    "(open \"f\" :r)"      2)
(ditty--deftest-builtin close-io   "(close s)"            2)
(ditty--deftest-builtin read-line-io "(read-line s)"      2)
(ditty--deftest-builtin load-io    "(load \"f\")"         2)
;; Printing
(ditty--deftest-builtin princ-p    "(princ x)"            2)
(ditty--deftest-builtin prin1-p    "(prin1 x)"            2)
(ditty--deftest-builtin format-p   "(format \"~a\" x)"    2)
(ditty--deftest-builtin terpri-p   "(terpri)"             2)
;; String ports
(ditty--deftest-builtin open-input-string "(open-input-string s)" 2)
;; Filesystem
(ditty--deftest-builtin file-exists? "(file-exists? f)"   2)
(ditty--deftest-builtin expand-path "(expand-path p)"     2)
;; Time/profiling
(ditty--deftest-builtin current-time-ms "(current-time-ms)" 2)
;; Errors
(ditty--deftest-builtin signal-e   "(signal 'foo nil)"    2)
(ditty--deftest-builtin error-e    "(error \"boom\")"     2)
(ditty--deftest-builtin error-type "(error-type e)"       2)
;; Packages
(ditty--deftest-builtin in-package "(in-package foo)"     2)
(ditty--deftest-builtin current-package "(current-package)" 2)
;; Functions / meta
(ditty--deftest-builtin eval-m     "(eval x)"             2)
(ditty--deftest-builtin documentation-m "(documentation f)" 2)
(ditty--deftest-builtin bound?-m   "(bound? 'x)"          2)

(ert-deftest ditty-test-builtin-not-for-unknown ()
  "An unknown head symbol should NOT get ditty-builtin-face."
  (should-not (eq (ditty-test-face-at "(somerandomuserfunc x)" 2)
                  'ditty-builtin-face)))

;; ============================================================================
;; Regression: weapons-show-all trailing-token font-lock leak
;; ============================================================================
;; Reported symptom under stock lisp-mode: the trailing sequence
;;   ":" msg)))))
;; at the end of the defun all rendered in a single color, as if one token's
;; face leaked into the following tokens.  This test asserts each token in
;; that tail gets an independent face under ditty-mode.

(defconst ditty--weapons-show-all-sample
  "\
(defun weapons-show-all ()
  \"Show summary of all weapon sets, marking the active one.\"
  (let ((count (weapons-set-count)))
    (if (= count 0)
      (weapons-echo \"No weapon sets defined\")
      (let ((msg \"\"))
        (do ((keys (hash-keys *weapons-sets*) (cdr keys))) ((null? keys))
          (let* ((key (car keys))
                 (mainhand-list (weapons-get-mainhand key))
                 (offhand-list (weapons-get-offhand key))
                 (marker (if (weapons-active? key) \"->\" \".\")))
            (set! msg
             (concat msg \"\\n  \" marker \" \" key \" (\"
              (number->string (length mainhand-list)) \" mainhand, \"
              (number->string (length offhand-list)) \" offhand)\"))))
        (weapons-echo
         (concat (number->string count) \" set\" (if (= count 1) \"\" \"s\")
          (if *weapons-active*
            (concat \" (active: \" *weapons-active* \")\")
            \" (no active)\") \":\" msg))))))
"
  "User's reported sample — see the trailing `\":\" msg)))))' tokens.")

(defun ditty-test--face-of-token (text token occurrence)
  "Return face at first char of the OCCURRENCE-th match of TOKEN in TEXT.
Fontifies TEXT in a ditty-mode buffer.  OCCURRENCE is 1-based."
  (with-temp-buffer
    (ditty-mode)
    (insert text)
    (font-lock-ensure)
    (goto-char (point-min))
    (let ((n occurrence)
          (case-fold-search nil))
      (while (> n 0)
        (unless (search-forward token nil t)
          (error "Token %S not found (occurrence %d)" token occurrence))
        (setq n (1- n))))
    (get-text-property (match-beginning 0) 'face)))

(ert-deftest ditty-test-weapons-sample-set!-is-special-form ()
  "In the weapons sample, `set!' should get special-form-face."
  (should (eq (ditty-test--face-of-token
               ditty--weapons-show-all-sample "set!" 1)
              'ditty-special-form-face)))

(ert-deftest ditty-test-weapons-sample-do-is-special-form ()
  "In the weapons sample, `do' should get special-form-face."
  (should (eq (ditty-test--face-of-token
               ditty--weapons-show-all-sample "do" 1)
              'ditty-special-form-face)))

(ert-deftest ditty-test-weapons-sample-let*-is-special-form ()
  "In the weapons sample, `let*' should get special-form-face."
  (should (eq (ditty-test--face-of-token
               ditty--weapons-show-all-sample "let*" 1)
              'ditty-special-form-face)))

(ert-deftest ditty-test-weapons-sample-concat-is-builtin ()
  "In the weapons sample, `concat' should get builtin-face (not anything else)."
  (should (eq (ditty-test--face-of-token
               ditty--weapons-show-all-sample "concat" 1)
              'ditty-builtin-face)))

(ert-deftest ditty-test-weapons-sample-hash-keys-is-builtin ()
  (should (eq (ditty-test--face-of-token
               ditty--weapons-show-all-sample "hash-keys" 1)
              'ditty-builtin-face)))

(ert-deftest ditty-test-weapons-sample-null?-is-builtin ()
  (should (eq (ditty-test--face-of-token
               ditty--weapons-show-all-sample "null?" 1)
              'ditty-builtin-face)))

(ert-deftest ditty-test-weapons-sample-number->string-is-builtin ()
  (should (eq (ditty-test--face-of-token
               ditty--weapons-show-all-sample "number->string" 1)
              'ditty-builtin-face)))

;; --- The trailing `":" msg)))))' regression ---

(ert-deftest ditty-test-weapons-sample-trailing-string-is-string-face ()
  "The trailing `\":\"' should be a string (font-lock-string-face)."
  (let ((face (ditty-test--face-of-token
               ditty--weapons-show-all-sample "\":\"" 1)))
    ;; The `\"' char itself is the first char of the match; its face should
    ;; be the standard string face from lisp-mode's syntactic fontification.
    (should (or (eq face 'font-lock-string-face)
                (and (listp face) (memq 'font-lock-string-face face))))))

(ert-deftest ditty-test-weapons-sample-trailing-msg-has-no-leak ()
  "The trailing `msg' symbol must NOT inherit string/constant/builtin face.
This is the specific leak the user reported under stock lisp-mode."
  (let* ((sample ditty--weapons-show-all-sample))
    (with-temp-buffer
      (ditty-mode)
      (insert sample)
      (font-lock-ensure)
      (goto-char (point-min))
      ;; Jump to the final `\":\" msg' — it's the last `msg' in the buffer.
      (goto-char (point-max))
      (should (search-backward " msg" nil t))
      (forward-char 1)                    ; point is now on `m' of `msg'
      (let ((face (get-text-property (point) 'face)))
        (should-not (eq face 'font-lock-string-face))
        (should-not (eq face 'ditty-constant-face))
        (should-not (eq face 'ditty-builtin-face))
        (should-not (eq face 'ditty-keyword-literal-face))
        (when (listp face)
          (should-not (memq 'font-lock-string-face face))
          (should-not (memq 'ditty-constant-face face))
          (should-not (memq 'ditty-builtin-face face))
          (should-not (memq 'ditty-keyword-literal-face face)))))))

(ert-deftest ditty-test-weapons-sample-trailing-parens-unfaced ()
  "The five trailing close-parens should not inherit anything from prior tokens."
  (with-temp-buffer
    (ditty-mode)
    (insert ditty--weapons-show-all-sample)
    (font-lock-ensure)
    (goto-char (point-max))
    ;; Skip trailing newline(s), then check the final `)' cluster.
    (skip-chars-backward " \t\n")
    (dotimes (_ 5)
      (backward-char 1)
      (let ((face (get-text-property (point) 'face)))
        (should-not (eq face 'font-lock-string-face))
        (should-not (eq face 'ditty-constant-face))
        (should-not (eq face 'ditty-builtin-face))
        (should-not (eq face 'ditty-keyword-literal-face))))))

;; ============================================================================
;; Indentation
;; ============================================================================

(ert-deftest ditty-test-indent-when ()
  "`when' body should indent two spaces."
  (should (equal (ditty-test-indent "(when cond\nbody)")
                 "(when cond\n  body)")))

(ert-deftest ditty-test-indent-unless ()
  "`unless' body should indent two spaces."
  (should (equal (ditty-test-indent "(unless cond\nbody)")
                 "(unless cond\n  body)")))

(ert-deftest ditty-test-indent-let ()
  "`let' body should indent two spaces."
  (should (equal (ditty-test-indent "(let ((x 1))\nbody)")
                 "(let ((x 1))\n  body)")))

(ert-deftest ditty-test-indent-let* ()
  "`let*' body should indent two spaces."
  (should (equal (ditty-test-indent "(let* ((x 1))\nbody)")
                 "(let* ((x 1))\n  body)")))

(ert-deftest ditty-test-indent-condition-case ()
  "`condition-case' body: var, form, handlers aligned."
  ;; Expected layout:
  ;;   (condition-case err
  ;;       (do-stuff)
  ;;     (error (handle)))
  ;; The protected form is a distinguished arg and gets extra indent;
  ;; handlers are body args at lisp-body-indent.
  (should (equal (ditty-test-indent
                  "(condition-case err\n(do-stuff)\n(error (handle)))")
                 "(condition-case err\n    (do-stuff)\n  (error (handle)))")))

(ert-deftest ditty-test-indent-unwind-protect ()
  "`unwind-protect' body: protected form distinguished, cleanups body-indented."
  (should (equal (ditty-test-indent
                  "(unwind-protect\n(body)\n(cleanup))")
                 "(unwind-protect\n    (body)\n  (cleanup))")))

(ert-deftest ditty-test-indent-case ()
  "`case' clauses should indent two spaces (body indent)."
  (should (equal (ditty-test-indent
                  "(case x\n((1) 'one)\n((2) 'two))")
                 "(case x\n  ((1) 'one)\n  ((2) 'two))")))

(ert-deftest ditty-test-indent-do ()
  "`do' bindings/test distinguished; body indented."
  (should (equal (ditty-test-indent
                  "(do ((i 0 (+ i 1)))\n((= i 10))\n(print i))")
                 "(do ((i 0 (+ i 1)))\n    ((= i 10))\n  (print i))")))

(ert-deftest ditty-test-indent-defun ()
  "`defun' body should indent two spaces."
  (should (equal (ditty-test-indent "(defun f ()\nbody)")
                 "(defun f ()\n  body)")))

(ert-deftest ditty-test-indent-defmacro ()
  "`defmacro' body should indent two spaces."
  (should (equal (ditty-test-indent "(defmacro m (x)\nbody)")
                 "(defmacro m (x)\n  body)")))

(ert-deftest ditty-test-indent-cond ()
  "`cond' clauses indent two spaces (no distinguished args)."
  (should (equal (ditty-test-indent "(cond\n((p1) 1)\n((p2) 2))")
                 "(cond\n  ((p1) 1)\n  ((p2) 2))")))

;;; ditty-mode-tests.el ends here
