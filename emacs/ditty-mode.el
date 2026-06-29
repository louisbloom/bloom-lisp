;;; ditty-mode.el --- Major mode for editing ditty source  -*- lexical-binding: t; -*-

;; Author: Thomas Christensen
;; URL: https://codeberg.org/thomasc/ditty
;; Version: 0.1.0
;; Keywords: languages, lisp, ditty
;; Package-Requires: ((emacs "25.1"))

;; This file is free software; you can redistribute it and/or modify
;; it under the terms of the GNU General Public License as published by
;; the Free Software Foundation; either version 3, or (at your option)
;; any later version.

;;; Commentary:

;; Provides syntax highlighting and basic editing support for ditty
;; source files.  Unlike the stock `lisp-mode', this mode knows about
;; ditty's special forms (`define', `set!', `defmacro', `do', `case',
;; `condition-case', `unwind-protect', ...), reader syntax (`pkg:symbol',
;; `:keyword', `#t', `#f', `#\\char'), and its built-in functions.

;;; Code:

(require 'lisp-mode)

;; ============================================================================
;; Customization
;; ============================================================================

(defgroup ditty nil
  "Major mode for editing ditty source."
  :prefix "ditty-"
  :group 'languages)

;; ============================================================================
;; Faces
;; ============================================================================

(defface ditty-special-form-face
  '((t (:inherit font-lock-keyword-face)))
  "Face for ditty special forms."
  :group 'ditty)

(defface ditty-stdlib-face
  '((t (:inherit font-lock-builtin-face)))
  "Face for ditty stdlib macros (defun, when, unless, defvar, ...)."
  :group 'ditty)

(defface ditty-builtin-face
  '((t (:inherit font-lock-builtin-face)))
  "Face for ditty built-in functions (C-level builtins + defalias aliases)."
  :group 'ditty)

(defface ditty-definition-name-face
  '((t (:inherit font-lock-function-name-face)))
  "Face for names introduced by `defun', `define', `defmacro', etc."
  :group 'ditty)

(defface ditty-constant-face
  '((t (:inherit font-lock-constant-face)))
  "Face for ditty constants (`nil', `#t', `#f')."
  :group 'ditty)

(defface ditty-keyword-literal-face
  '((t (:inherit font-lock-constant-face)))
  "Face for `:keyword' self-evaluating literals."
  :group 'ditty)

(defface ditty-character-face
  '((t (:inherit font-lock-string-face)))
  "Face for character literals (`#\\a', `#\\space', `#\\x41', `#\\uHHHH')."
  :group 'ditty)

(defface ditty-package-face
  '((t (:inherit font-lock-type-face)))
  "Face for the package prefix in `pkg:symbol'."
  :group 'ditty)

;; ============================================================================
;; Symbol lists
;; ============================================================================

(defconst ditty--special-forms
  '("quote" "quasiquote" "if" "define" "set!" "lambda" "defmacro"
    "let" "let*" "progn" "do" "cond" "case" "and" "or"
    "condition-case" "unwind-protect")
  "Ditty special forms (from `SPECIAL_FORMS' in include/lisp.h).")

(defconst ditty--stdlib-macros
  '("defun" "when" "unless" "defvar" "defconst" "defalias"
    "with-output-to-string")
  "Ditty stdlib macros defined via Lisp in src/lisp.c.")

(defconst ditty--builtins
  '(;; --- Arithmetic (src/builtins_arithmetic.c) ---
    "+" "-" "*" "/" "quotient" "remainder" "even?" "odd?"
    ;; --- Comparison (src/builtins_comparison.c) ---
    ">" "<" "=" ">=" "<=" "not" "eq?" "equal?" "string=?"
    ;; --- Type predicates (src/builtins_type_predicates.c) ---
    "null?" "atom?" "pair?" "integer?" "boolean?" "number?" "vector?"
    "hash-table?" "string?" "symbol?" "keyword?" "list?" "function?"
    "macro?" "builtin?" "callable?" "regex?"
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
    "regex-compile" "regex-match?" "regex-find" "regex-find-all" "regex-extract"
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
    "open-input-string" "open-output-string"
    "port-peek-char" "port-read-char" "port-write-char"
    "port-write-string"
    "port-position" "port-source" "port-eof?" "string-port?"
    "get-output-string"
    ;; --- Filesystem (src/builtins_filesystem.c) ---
    "home-directory" "expand-path" "getenv" "data-directory"
    "config-directory" "file-exists?" "mkdir"
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
  "Ditty built-in functions registered via REGISTER in src/builtins_*.c,
plus Lisp-level aliases defined via `defalias' in src/lisp.c.")

;; ============================================================================
;; Font-lock
;; ============================================================================

(defun ditty--head-regexp (names)
  "Build a regexp matching one of NAMES immediately after an open paren.
Group 1 is the name."
  (concat "(\\s-*\\(" (regexp-opt names) "\\)\\_>"))

(defconst ditty--definers
  '("defun" "define" "defmacro" "defvar" "defconst" "defalias")
  "Heads that introduce a named definition; the next symbol is the name.")

(defconst ditty--symbol-char
  "[[:alnum:]_!?*+/<=>-]"
  "Character class (as a string fragment) matching a ditty symbol char.
Does not include `:' so that package prefixes can be separated.")

(defconst ditty-font-lock-keywords
  `(;; Character literals: `#\\a', `#\\space', `#\\x41', `#\\u03B1'.
    ("#\\\\\\(?:space\\|newline\\|tab\\|return\\|escape\\|null\\|backspace\\|delete\\|alarm\\|x[0-9a-fA-F]+\\|u[0-9a-fA-F]+\\|.\\)"
     (0 'ditty-character-face))
    ;; Booleans `#t' and `#f'.  `#' has `expression-prefix' syntax, so
    ;; `\\_<' does not fire on it; anchor on preceding whitespace, paren,
    ;; or buffer start.
    ("\\(?:^\\|[ \t\n(')`,@]\\)\\(#[tf]\\)\\_>"
     (1 'ditty-constant-face))
    ;; `nil'.
    ("\\_<nil\\_>"
     (0 'ditty-constant-face))
    ;; `:keyword' self-evaluating literal (word-starting colon).
    (,(concat "\\_<:" ditty--symbol-char "+\\_>")
     (0 'ditty-keyword-literal-face))
    ;; `pkg:symbol' package-qualified reference: paint the `pkg' prefix.
    (,(concat "\\_<\\(" ditty--symbol-char "+\\):"
              ditty--symbol-char "+\\_>")
     (1 'ditty-package-face))
    ;; Special forms.
    (,(ditty--head-regexp ditty--special-forms)
     (1 'ditty-special-form-face))
    ;; Stdlib macros.
    (,(ditty--head-regexp ditty--stdlib-macros)
     (1 'ditty-stdlib-face))
    ;; Built-in functions (in head position: `(FOO ...)').
    (,(ditty--head-regexp ditty--builtins)
     (1 'ditty-builtin-face))
    ;; Name in `(DEFINER NAME ...)' or `(define (NAME ...) ...)'.
    (,(concat "(\\s-*\\(?:"
              (regexp-opt ditty--definers)
              "\\)\\_>[ \t\n]*(?\\s-*\\('?\\_<[^][ \t\n()]+\\_>\\)")
     (1 'ditty-definition-name-face nil t)))
  "Font-lock keywords for `ditty-mode'.")

;; ============================================================================
;; Indentation hints
;; ============================================================================
;; Attach `lisp-indent-function' properties for ditty forms that the
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
                (with-output-to-string . 1)
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
(define-derived-mode ditty-mode lisp-mode "Bloom-Lisp"
  "Major mode for editing ditty source files."
  ;; Replace lisp-mode's font-lock-defaults wholesale so its
  ;; Common-Lisp-oriented keyword list (which can mis-highlight
  ;; ditty forms) does not leak through.
  (setq-local font-lock-defaults
              '(ditty-font-lock-keywords nil nil nil nil))
  ;; Parent `lisp-mode' sets `lisp-indent-function' (the variable) to
  ;; `common-lisp-indent-function', which reads a different property
  ;; (`common-lisp-indent-function') than the numeric-style
  ;; `lisp-indent-function' property we `put' above.  Switch to the
  ;; Emacs-Lisp-style indenter so our puts take effect.
  (setq-local lisp-indent-function 'lisp-indent-function))

(provide 'ditty-mode)

;;; ditty-mode.el ends here
