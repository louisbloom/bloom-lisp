;;; bloom-lisp-mode.el --- Major mode for editing bloom-lisp source  -*- lexical-binding: t; -*-

;; Author: Thomas Christensen
;; URL: https://codeberg.org/thomasc/bloom-lisp
;; Version: 0.1.0
;; Keywords: languages, lisp, bloom-lisp
;; Package-Requires: ((emacs "25.1"))

;; This file is free software; you can redistribute it and/or modify
;; it under the terms of the GNU General Public License as published by
;; the Free Software Foundation; either version 3, or (at your option)
;; any later version.

;;; Commentary:

;; Provides syntax highlighting and basic editing support for bloom-lisp
;; source files.  Unlike the stock `lisp-mode', this mode knows about
;; bloom-lisp's special forms (`define', `set!', `defmacro', `do', `case',
;; `condition-case', `unwind-protect', ...), reader syntax (`pkg:symbol',
;; `:keyword', `#t', `#f', `#\\char'), and its ~120 built-in functions.

;;; Code:

(require 'lisp-mode)

;; ============================================================================
;; Customization
;; ============================================================================

(defgroup bloom-lisp nil
  "Major mode for editing bloom-lisp source."
  :prefix "bloom-lisp-"
  :group 'languages)

;; ============================================================================
;; Faces
;; ============================================================================

(defface bloom-lisp-special-form-face
  '((t (:inherit font-lock-keyword-face)))
  "Face for bloom-lisp special forms."
  :group 'bloom-lisp)

(defface bloom-lisp-stdlib-face
  '((t (:inherit font-lock-builtin-face)))
  "Face for bloom-lisp stdlib macros (defun, when, unless, defvar, ...)."
  :group 'bloom-lisp)

(defface bloom-lisp-builtin-face
  '((t (:inherit font-lock-builtin-face)))
  "Face for bloom-lisp built-in functions (the ~120 C-level builtins)."
  :group 'bloom-lisp)

(defface bloom-lisp-definition-name-face
  '((t (:inherit font-lock-function-name-face)))
  "Face for names introduced by `defun', `define', `defmacro', etc."
  :group 'bloom-lisp)

(defface bloom-lisp-constant-face
  '((t (:inherit font-lock-constant-face)))
  "Face for bloom-lisp constants (`nil', `#t', `#f')."
  :group 'bloom-lisp)

(defface bloom-lisp-keyword-literal-face
  '((t (:inherit font-lock-constant-face)))
  "Face for `:keyword' self-evaluating literals."
  :group 'bloom-lisp)

(defface bloom-lisp-character-face
  '((t (:inherit font-lock-string-face)))
  "Face for character literals (`#\\a', `#\\space', `#\\x41', `#\\uHHHH')."
  :group 'bloom-lisp)

(defface bloom-lisp-package-face
  '((t (:inherit font-lock-type-face)))
  "Face for the package prefix in `pkg:symbol'."
  :group 'bloom-lisp)

;; ============================================================================
;; Symbol lists
;; ============================================================================

(defconst bloom-lisp--special-forms
  '("quote" "quasiquote" "if" "define" "set!" "lambda" "defmacro"
    "let" "let*" "progn" "do" "cond" "case" "and" "or"
    "condition-case" "unwind-protect" "package-ref")
  "Bloom-lisp special forms (from `SPECIAL_FORMS' in include/lisp.h).")

(defconst bloom-lisp--stdlib-macros
  '("defun" "when" "unless" "defvar" "defconst" "defalias")
  "Bloom-lisp stdlib macros defined via Lisp in src/lisp.c.")

(defconst bloom-lisp--builtins
  '(;; --- Arithmetic (src/builtins_arithmetic.c) ---
    "+" "-" "*" "/" "quotient" "remainder" "even?" "odd?"
    ;; --- Comparison (src/builtins_comparison.c) ---
    ">" "<" "=" ">=" "<=" "not" "eq?" "equal?" "string=?"
    ;; --- Type predicates (src/builtins_type_predicates.c) ---
    "null?" "atom?" "pair?" "integer?" "boolean?" "number?" "vector?"
    "hash-table?" "string?" "symbol?" "keyword?" "list?" "function?"
    "macro?" "builtin?" "callable?"
    ;; --- Characters (src/builtins_characters.c) ---
    "char?" "char-code" "code-char" "char->string" "string->char"
    "char=?" "char<?" "char>?" "char<=?" "char>=?"
    "char-upcase" "char-downcase"
    "char-alphabetic?" "char-numeric?" "char-whitespace?"
    ;; --- Strings (src/builtins_strings.c) ---
    "concat" "number->string" "string->number" "split" "join"
    "string<?" "string>?" "string<=?" "string>=?"
    "string-contains?" "string-index" "string-match?"
    "substring" "string-ref" "string-prefix?" "string-replace"
    "string-upcase" "string-downcase" "string-trim"
    ;; --- Symbols (src/builtins_symbols.c) ---
    "symbol->string" "string->symbol" "keyword-name"
    ;; --- Lists (src/builtins_lists.c) ---
    "car" "cdr" "caar" "cadr" "cdar" "cddr" "caddr" "cadddr"
    "first" "second" "third" "fourth" "rest"
    "set-car!" "set-cdr!" "cons" "list" "length" "list-ref"
    "reverse" "append" "assoc" "assq" "assv" "alist-get"
    "member" "memq" "map" "mapcar" "filter" "apply"
    ;; --- Vectors (src/builtins_vectors.c) ---
    "make-vector" "vector-ref" "vector-set!" "vector-push!" "vector-pop!"
    ;; --- Hash tables (src/builtins_hash_tables.c) ---
    "make-hash-table" "hash-ref" "hash-set!" "hash-remove!" "hash-clear!"
    "hash-count" "hash-keys" "hash-values" "hash-entries"
    ;; --- Regex (src/builtins_regex.c) ---
    "regex-match?" "regex-find" "regex-find-all" "regex-extract"
    "regex-replace" "regex-replace-all" "regex-split"
    "regex-escape" "regex-valid?"
    ;; --- Functions / meta (src/builtins_functions.c) ---
    "function-params" "function-body" "function-name"
    "documentation" "bound?" "set-documentation!"
    "eval" "exit" "quit"
    ;; --- Printing (src/builtins_printing.c) ---
    "princ" "prin1" "print" "format" "terpri"
    ;; --- File I/O (src/builtins_file_io.c) ---
    "open" "close" "read-line" "write-line" "write-string"
    "stream-eol" "read-sexp" "read-json" "read-file-raw"
    "delete-file" "load"
    ;; --- String ports (src/builtins_string_ports.c) ---
    "open-input-string" "port-peek-char" "port-read-char"
    "port-position" "port-source" "port-eof?" "string-port?"
    ;; --- Filesystem (src/builtins_filesystem.c) ---
    "home-directory" "expand-path" "getenv" "data-directory"
    "file-exists?" "mkdir"
    ;; --- Time / profiling (src/builtins_time_profiling.c) ---
    "current-time-ms" "profile-start" "profile-stop"
    "profile-report" "profile-reset"
    ;; --- Errors (src/builtins_errors.c) ---
    "error?" "error-type" "error-message" "error-stack" "error-data"
    "signal" "error"
    ;; --- Packages (src/builtins_packages.c) ---
    "in-package" "current-package" "package-symbols"
    "list-packages" "package-save"
    ;; --- Aliases created via defalias in src/lisp.c ---
    "doc" "doc-set!" "string-append" "string-split" "string-join"
    "string-length" "package-set" "package-current" "package-list")
  "Bloom-lisp built-in functions registered via REGISTER in src/builtins_*.c,
plus Lisp-level aliases defined via `defalias' in src/lisp.c.")

;; ============================================================================
;; Font-lock
;; ============================================================================

(defun bloom-lisp--head-regexp (names)
  "Build a regexp matching one of NAMES immediately after an open paren.
Group 1 is the name."
  (concat "(\\s-*\\(" (regexp-opt names) "\\)\\_>"))

(defconst bloom-lisp--definers
  '("defun" "define" "defmacro" "defvar" "defconst" "defalias")
  "Heads that introduce a named definition; the next symbol is the name.")

(defconst bloom-lisp--symbol-char
  "[[:alnum:]_!?*+/<=>-]"
  "Character class (as a string fragment) matching a bloom-lisp symbol char.
Does not include `:' so that package prefixes can be separated.")

(defconst bloom-lisp-font-lock-keywords
  `(;; Character literals: `#\\a', `#\\space', `#\\x41', `#\\u03B1'.
    ("#\\\\\\(?:space\\|newline\\|tab\\|return\\|escape\\|null\\|backspace\\|delete\\|alarm\\|x[0-9a-fA-F]+\\|u[0-9a-fA-F]+\\|.\\)"
     (0 'bloom-lisp-character-face))
    ;; Booleans `#t' and `#f'.  `#' has `expression-prefix' syntax, so
    ;; `\\_<' does not fire on it; anchor on preceding whitespace, paren,
    ;; or buffer start.
    ("\\(?:^\\|[ \t\n(')`,@]\\)\\(#[tf]\\)\\_>"
     (1 'bloom-lisp-constant-face))
    ;; `nil'.
    ("\\_<nil\\_>"
     (0 'bloom-lisp-constant-face))
    ;; `:keyword' self-evaluating literal (word-starting colon).
    (,(concat "\\_<:" bloom-lisp--symbol-char "+\\_>")
     (0 'bloom-lisp-keyword-literal-face))
    ;; `pkg:symbol' package-qualified reference: paint the `pkg' prefix.
    (,(concat "\\_<\\(" bloom-lisp--symbol-char "+\\):"
              bloom-lisp--symbol-char "+\\_>")
     (1 'bloom-lisp-package-face))
    ;; Special forms.
    (,(bloom-lisp--head-regexp bloom-lisp--special-forms)
     (1 'bloom-lisp-special-form-face))
    ;; Stdlib macros.
    (,(bloom-lisp--head-regexp bloom-lisp--stdlib-macros)
     (1 'bloom-lisp-stdlib-face))
    ;; Built-in functions (in head position: `(FOO ...)').
    (,(bloom-lisp--head-regexp bloom-lisp--builtins)
     (1 'bloom-lisp-builtin-face))
    ;; Name in `(DEFINER NAME ...)' or `(define (NAME ...) ...)'.
    (,(concat "(\\s-*\\(?:"
              (regexp-opt bloom-lisp--definers)
              "\\)\\_>[ \t\n]*(?\\s-*\\('?\\_<[^][ \t\n()]+\\_>\\)")
     (1 'bloom-lisp-definition-name-face nil t)))
  "Font-lock keywords for `bloom-lisp-mode'.")

;; ============================================================================
;; Indentation hints
;; ============================================================================
;; Attach `lisp-indent-function' properties for bloom-lisp forms that the
;; parent `lisp-mode' either doesn't know or would indent as a plain call.
;; A numeric value N means "N distinguished args before the body"; the
;; symbol `defun' means defun-style (first line carries the signature,
;; subsequent lines are body).

(dolist (spec '((if              . 2)
                (let             . 1)
                (let*            . 1)
                (lambda          . 1)
                (defun           . defun)
                (defmacro        . defun)
                (do              . 2)
                (case            . 1)
                (cond            . 0)
                (condition-case  . 2)
                (unwind-protect  . 1)
                (when            . 1)
                (unless          . 1)
                (progn           . 0)
                (and             . 0)
                (or              . 0)))
  (put (car spec) 'lisp-indent-function (cdr spec)))

;; ============================================================================
;; Mode definition
;; ============================================================================

;;;###autoload
(define-derived-mode bloom-lisp-mode lisp-mode "Bloom-Lisp"
  "Major mode for editing bloom-lisp source files."
  ;; Replace lisp-mode's font-lock-defaults wholesale so its
  ;; Common-Lisp-oriented keyword list (which can mis-highlight
  ;; bloom-lisp forms) does not leak through.
  (setq-local font-lock-defaults
              '(bloom-lisp-font-lock-keywords nil nil nil nil))
  ;; Parent `lisp-mode' sets `lisp-indent-function' (the variable) to
  ;; `common-lisp-indent-function', which reads a different property
  ;; (`common-lisp-indent-function') than the numeric-style
  ;; `lisp-indent-function' property we `put' above.  Switch to the
  ;; Emacs-Lisp-style indenter so our puts take effect.
  (setq-local lisp-indent-function 'lisp-indent-function))

(provide 'bloom-lisp-mode)

;;; bloom-lisp-mode.el ends here
