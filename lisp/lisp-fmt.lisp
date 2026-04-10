;;; lisp-fmt.lisp --- Lisp source code formatter
;;;
;;; Usage:
;;;   lisp-repl telnet-lisp/lisp/lisp-fmt.lisp file.lisp       # Format to stdout
;;;   lisp-repl telnet-lisp/lisp/lisp-fmt.lisp -i file.lisp    # In-place edit
;;;
;;; Formatting rules:
;;;   - Line width: 79 characters
;;;   - Indent: 2 spaces
;;;   - List continuations align with first element
;;;   - Comments are preserved (leading and inline)
;;; ============================================================================
;;; Package
;;; ============================================================================
(in-package "lisp-fmt")

;;; ============================================================================
;;; Configuration
;;; ============================================================================
(define *max-column* 79)

(define *indent-size* 2)

;;; Named constants for form-specific formatting
(define *let-binding-offset* 3) ; Characters after "(let* " before first binding

(define *defun-inline-args* 2) ; Max args to keep on first line for defun

(define *max-cached-indent* 80) ; Maximum indent level to cache

;;; ============================================================================
;;; Comment-Preserving Reader
;;; ============================================================================
;;; A pure Lisp reader that preserves comments and attaches them to expressions.
;;; Replaces read-sexp for the formatter to enable comment-aware formatting.
;;; ----------------------------------------------------------------------------
;;; File Reading Utility
;;; ----------------------------------------------------------------------------
(defun strip-trailing-cr (str)
  "Remove trailing carriage return from string (for CRLF handling)."
  (let ((len (length str)))
    (if (and (> len 0) (char=? (string-ref str (- len 1)) #\return))
      (substring str 0 (- len 1))
      str)))

(defun read-file-to-string (filename)
  "Read entire file contents into a string.
   Handles CRLF line endings by stripping trailing CR from each line."
  (let ((file (open filename "r"))
        (lines '())
        (line nil))
    (unwind-protect
      (progn
        (do () ((null? (set! line (read-line file))))
          (set! lines (cons (strip-trailing-cr line) lines)))
        (join (reverse lines) "\n"))
      (close file))))

;;; ----------------------------------------------------------------------------
;;; Reader State Management
;;; ----------------------------------------------------------------------------
;;; State: (port line column)
;;; Uses string port for O(1) character access instead of O(n) string-ref.
(defun make-reader-state (source)
  "Create a reader state for parsing SOURCE string."
  (list (open-input-string source) 1 1))

(defun rs-port (state) (car state))

(defun rs-source (state) (port-source (rs-port state)))

(defun rs-pos (state) (port-position (rs-port state)))

(defun rs-line (state) (cadr state))

(defun rs-col (state) (caddr state))

(defun rs-set-line! (state line) (set-car! (cdr state) line))

(defun rs-set-col! (state col) (set-car! (cddr state) col))

(defun reader-peek (state)
  "Return current character without advancing, or nil at EOF."
  (port-peek-char (rs-port state)))

(defun reader-advance! (state)
  "Advance position by one character, updating line/column. Returns char."
  (let ((ch (port-read-char (rs-port state))))
    (when ch
      (if (char=? ch #\newline)
        (progn (rs-set-line! state (+ (rs-line state) 1)) (rs-set-col! state 1))
        (rs-set-col! state (+ (rs-col state) 1))))
    ch))

(defun reader-at-eof? (state)
  "Check if reader is at end of input."
  (port-eof? (rs-port state)))

;;; ----------------------------------------------------------------------------
;;; Character Utilities
;;; ----------------------------------------------------------------------------
(defun delimiter? (ch)
  "Check if character is a token delimiter."
  (or (null? ch) (char-whitespace? ch) (char=? ch #\() (char=? ch #\))
      (char=? ch #\;) (char=? ch #\")))

;;; ----------------------------------------------------------------------------
;;; Token Data Structure
;;; ----------------------------------------------------------------------------
;;; Token: (type value line col)
(defun make-token (type value line col)
  "Create a token with type, value, and source position."
  (list type value line col))

(defun token-type (tok) (car tok))

(defun token-value (tok) (cadr tok))

(defun token-line (tok) (caddr tok))

(defun token-col (tok) (cadddr tok))

;;; ----------------------------------------------------------------------------
;;; Comment Reading
;;; ----------------------------------------------------------------------------
(defun read-line-comment (state)
  "Read a semicolon comment to end of line. Returns comment string including ;."
  (let ((start (rs-pos state))
        (src (rs-source state)))
    (do ()
      ((let ((ch (reader-peek state))) (or (null? ch) (char=? ch #\newline))))
      (reader-advance! state))
    (substring src start (rs-pos state))))

(defun skip-whitespace (state)
  "Skip whitespace only. Comments are NOT consumed — they become tokens."
  (do ()
    ((let ((ch (reader-peek state))) (not (and ch (char-whitespace? ch)))))
    (reader-advance! state)))

;;; ----------------------------------------------------------------------------
;;; String Token Reading
;;; ----------------------------------------------------------------------------
(defun read-octal-escape (state first-digit)
  "Read an octal escape sequence starting with FIRST-DIGIT."
  (let ((value (- (char-code first-digit) (char-code #\0)))
        (count 1))
    (do ()
      ((or (>= count 3)
           (let ((ch (reader-peek state)))
             (or (null? ch) (not (and (char>=? ch #\0) (char<=? ch #\7)))))))
      (let ((ch (reader-advance! state)))
        (set! value (+ (* value 8) (- (char-code ch) (char-code #\0))))
        (set! count (+ count 1))))
    (code-char value)))

(defun chars-to-string (char-list)
  "Convert list of characters to string efficiently using join.
   This is O(n) instead of O(n²) repeated concatenation."
  (let ((parts '()))
    (do ((remaining char-list (cdr remaining))) ((null? remaining))
      (set! parts (cons (char->string (car remaining)) parts)))
    (join (reverse parts) "")))

(defun read-string-token (state)
  "Read a string literal, handling escape sequences."
  (reader-advance! state) ; skip opening quote
  (let ((chars '()))
    (do () ((let ((ch (reader-peek state))) (or (null? ch) (char=? ch #\"))))
      (let ((ch (reader-advance! state)))
        (if (char=? ch #\\)
          (let ((esc (reader-advance! state)))
            (set! chars
             (cons
              (cond
                ((char=? esc #\n) #\newline)
                ((char=? esc #\t) #\tab)
                ((char=? esc #\r) #\return)
                ((char=? esc #\\) #\\)
                ((char=? esc #\") #\")
                ((and (char>=? esc #\0) (char<=? esc #\7))
                 (read-octal-escape state esc))
                (#t esc)) chars)))
          (set! chars (cons ch chars)))))
    (reader-advance! state) ; skip closing quote
    (chars-to-string (reverse chars))))

;;; ----------------------------------------------------------------------------
;;; Atom Token Reading
;;; ----------------------------------------------------------------------------
(defun read-atom-token (state)
  "Read an atom (symbol or number) token."
  (let ((start (rs-pos state))
        (src (rs-source state)))
    (do () ((delimiter? (reader-peek state))) (reader-advance! state))
    (substring src start (rs-pos state))))

;;; ----------------------------------------------------------------------------
;;; Character Literal Reading
;;; ----------------------------------------------------------------------------
(defun hex-digit? (ch)
  "Check if CH is a hexadecimal digit."
  (and ch
       (or (and (char>=? ch #\0) (char<=? ch #\9))
           (and (char>=? ch #\a) (char<=? ch #\f))
           (and (char>=? ch #\A) (char<=? ch #\F)))))

(defun read-hex-digits (state count)
  "Read COUNT hex digits and return their numeric value."
  (let ((value 0)
        (i 0))
    (do () ((>= i count))
      (let ((ch (reader-advance! state)))
        (cond
          ((and (char>=? ch #\0) (char<=? ch #\9))
           (set! value (+ (* value 16) (- (char-code ch) (char-code #\0)))))
          ((and (char>=? ch #\a) (char<=? ch #\f))
           (set! value
            (+ (* value 16) (+ 10 (- (char-code ch) (char-code #\a))))))
          ((and (char>=? ch #\A) (char<=? ch #\F))
           (set! value
            (+ (* value 16) (+ 10 (- (char-code ch) (char-code #\A)))))))
        (set! i (+ i 1))))
    value))

(defun parse-character-name (name)
  "Parse a character name like 'newline' or 'a' into a character."
  (cond
    ((string=? name "newline") #\newline)
    ((string=? name "space") #\space)
    ((string=? name "tab") #\tab)
    ((string=? name "return") #\return)
    ((string=? name "escape") #\escape)
    ((string=? name "null") #\null)
    ((string=? name "backspace") #\backspace)
    ((string=? name "delete") #\delete)
    ((= (length name) 1) (string-ref name 0))
    (#t (error (concat "Unknown character name: " name)))))

(defun read-character-literal (state)
  "Read a character literal like #\\a or #\\newline.
   Note: Called after # has been consumed. Consumes the backslash and character name.
   Handles hex escapes (#\\x1b) and unicode escapes (#\\u0041) but only if followed
   by valid hex digits. Otherwise treats #\\x and #\\u as regular characters."
  (reader-advance! state) ; skip backslash
  (let ((ch (reader-peek state))
        (src (rs-source state)))
    (cond
      ((null? ch) (error "Unexpected EOF in character literal"))
      ;; Check for hex escape: #\xNN - only if followed by hex digit
      ((and (char=? ch #\x)
            (let ((next-pos (+ (rs-pos state) 1)))
              (and (< next-pos (length src))
                   (hex-digit? (string-ref src next-pos)))))
       (reader-advance! state) (code-char (read-hex-digits state 2)))
      ;; Check for unicode escape: #\uNNNN - only if followed by hex digit
      ((and (char=? ch #\u)
            (let ((next-pos (+ (rs-pos state) 1)))
              (and (< next-pos (length src))
                   (hex-digit? (string-ref src next-pos)))))
       (reader-advance! state) (code-char (read-hex-digits state 4)))
      (#t
       ;; Read at least one character, then continue if not a delimiter
       (let ((start (rs-pos state)))
         (reader-advance! state) ; Read the first character
         ;; Continue reading for named characters like "newline"
         (do () ((delimiter? (reader-peek state))) (reader-advance! state))
         (parse-character-name (substring src start (rs-pos state))))))))

;;; ----------------------------------------------------------------------------
;;; Hash Token Reading (#t, #f, #\, #()
;;; ----------------------------------------------------------------------------
(defun read-hash-token (state line col)
  "Read a # prefixed token."
  (reader-advance! state) ; skip #
  (let ((ch (reader-peek state)))
    (cond
      ((char=? ch #\t) (reader-advance! state)
       (make-token 'boolean #t line col))
      ((char=? ch #\f) (reader-advance! state)
       (make-token 'boolean #f line col))
      ((char=? ch #\() (reader-advance! state)
       (make-token 'vector-start nil line col))
      ((char=? ch #\\)
       (let ((char-val (read-character-literal state)))
         (make-token 'char char-val line col)))
      (#t (error (format nil "Unknown # syntax: #~A at line ~A" ch line))))))

;;; ----------------------------------------------------------------------------
;;; Main Tokenizer
;;; ----------------------------------------------------------------------------
(defun read-token (state)
  "Read the next token. Semicolon comments become 'comment tokens."
  (skip-whitespace state)
  (let* ((line (rs-line state))
         (col (rs-col state))
         (ch (reader-peek state)))
    (cond
      ((null? ch) (make-token 'eof nil line col))
      ((char=? ch #\;)
       (let ((text (read-line-comment state)))
         (make-token 'comment text line col)))
      ((char=? ch #\() (reader-advance! state)
       (make-token 'lparen nil line col))
      ((char=? ch #\)) (reader-advance! state)
       (make-token 'rparen nil line col))
      ((char=? ch #\') (reader-advance! state)
       (make-token 'quote nil line col))
      ((char=? ch #\`) (reader-advance! state)
       (make-token 'backquote nil line col))
      ((char=? ch #\,) (reader-advance! state)
       (if (and (reader-peek state) (char=? (reader-peek state) #\@))
         (progn (reader-advance! state)
           (make-token 'unquote-splicing nil line col))
         (make-token 'unquote nil line col)))
      ((char=? ch #\")
       (let* ((start (rs-pos state))
              (str-val (read-string-token state))
              (original (substring (rs-source state) start (rs-pos state))))
         (make-token 'string (cons str-val original) line col)))
      ((char=? ch #\#) (read-hash-token state line col))
      ((char=? ch #\.)
       (let ((next-pos (+ (rs-pos state) 1))
             (src (rs-source state)))
         (if
           (or (>= next-pos (length src))
               (delimiter? (string-ref src next-pos)))
           (progn (reader-advance! state) (make-token 'dot nil line col))
           (let ((atom-val (read-atom-token state)))
             (make-token 'atom atom-val line col)))))
      (#t
       (let ((atom-val (read-atom-token state)))
         (make-token 'atom atom-val line col))))))

;;; ----------------------------------------------------------------------------
;;; Annotated Expression Data Structure
;;; ----------------------------------------------------------------------------
;;; Wraps an S-expression with source position metadata and optional original
;;; form. Structure: (annotated value original-form start-line last-line)
;;;   - original-form preserves the exact source syntax ("nil" vs "()" vs "#f")
;;;   - start-line is the source line of the expression's first token
;;;   - last-line is the source line of the expression's final token
;;; The formatter uses start-line for vertical-intent preservation (detect
;;; source newlines between sibling elements) and last-line for deciding
;;; inline-vs-standalone placement of following comment nodes.
(defun make-annotated (value original-form start-line last-line)
  "Create an annotated expression."
  (list 'annotated value original-form start-line last-line))

(defun annotated? (obj)
  "Check if OBJ is an annotated expression."
  (and (pair? obj) (eq? (car obj) 'annotated)))

(defun ann-value (ann) (cadr ann))

(defun ann-original-form (ann) (caddr ann))

(defun ann-start-line (ann) (cadddr ann))

(defun ann-last-line (ann) (car (cddr (cddr ann))))

;;; ----------------------------------------------------------------------------
;;; Comment Node — first-class AST peer of expressions
;;; ----------------------------------------------------------------------------
;;; Structure: (comment text line)
;;; Comment nodes are cons'd into list elements alongside annotated
;;; expressions. The formatter decides inline vs. free-standing at render
;;; time by comparing comment.line with the preceding expression's last-line.
(defun make-comment-node (text line)
  "Create a comment node. TEXT includes the leading semicolons."
  (list 'comment text line))

(defun comment-node? (obj)
  "Check if OBJ is a comment node."
  (and (pair? obj) (eq? (car obj) 'comment)))

(defun comment-node-text (n) (cadr n))

(defun comment-node-line (n) (caddr n))

;;; ----------------------------------------------------------------------------
;;; Parser State
;;; ----------------------------------------------------------------------------
;;; Parser uses one-token lookahead.
(defun make-parser-state (reader-state)
  "Create parser state with one-token lookahead.
   State: (reader-state current-token)"
  (let ((first-token (read-token reader-state)))
    (list reader-state first-token)))

(defun parser-reader (pstate) (car pstate))

(defun parser-current (pstate) (cadr pstate))

(defun parser-advance! (pstate)
  "Consume current token and read next. Returns consumed token."
  (let ((current (parser-current pstate)))
    (set-car! (cdr pstate) (read-token (parser-reader pstate)))
    current))

;;; ----------------------------------------------------------------------------
;;; Atom Value Parsing
;;; ----------------------------------------------------------------------------
(defun parse-atom-value (str)
  "Convert atom string to appropriate Lisp value."
  (cond
    ((string=? str "nil") nil)
    ((string=? str "#t") #t)
    ((string=? str "#f") #f)
    (#t (let ((num (string->number str))) (if num num (string->symbol str))))))

;;; ----------------------------------------------------------------------------
;;; Expression Parser
;;; ----------------------------------------------------------------------------
(defun parse-expression (pstate)
  "Parse one expression, returning annotated result.
   Comment tokens are NOT parsed here — parse-list, parse-vector, and
   read-source-with-comments handle them as peer children."
  (let* ((tok (parser-current pstate))
         (type (token-type tok)))
    (cond
      ((eq? type 'eof) nil)
      ((eq? type 'lparen) (parse-list pstate))
      ((eq? type 'quote) (parse-quoted pstate 'quote))
      ((eq? type 'backquote) (parse-quoted pstate 'quasiquote))
      ((eq? type 'unquote) (parse-quoted pstate 'unquote))
      ((eq? type 'unquote-splicing) (parse-quoted pstate 'unquote-splicing))
      ((eq? type 'vector-start) (parse-vector pstate))
      ((eq? type 'atom)
       (let ((line (token-line tok))
             (val (token-value tok)))
         (parser-advance! pstate)
         (make-annotated (parse-atom-value val) val line line)))
      ((eq? type 'string)
       (let ((line (token-line tok))
             (str-pair (token-value tok)))
         (parser-advance! pstate)
         (make-annotated (car str-pair) (cdr str-pair) line line)))
      ((eq? type 'boolean)
       (let ((line (token-line tok))
             (val (token-value tok)))
         (parser-advance! pstate)
         (make-annotated val (if val "#t" "#f") line line)))
      ((eq? type 'char)
       (let ((line (token-line tok))
             (val (token-value tok)))
         (parser-advance! pstate)
         (make-annotated val nil line line)))
      (#t
       (error
        (format nil "Unexpected token type: ~A at line ~A" type
         (token-line tok)))))))

(defun parse-list (pstate)
  "Parse a list expression (a b c) or dotted pair (a . b).
   Comment tokens inside the list become peer comment nodes in the
   elements list, interleaved with annotated expressions."
  (let ((open-line (token-line (parser-current pstate)))
        (elements '())
        (dotted-tail nil)
        (close-line -1))
    (parser-advance! pstate) ; consume lparen
    (do ()
      ((let ((tok (parser-current pstate)))
         (or (eq? (token-type tok) 'rparen) (eq? (token-type tok) 'dot)
             (eq? (token-type tok) 'eof))))
      (let ((tok (parser-current pstate)))
        (cond
          ((eq? (token-type tok) 'comment)
           (parser-advance! pstate)
           (set! elements
            (cons (make-comment-node (token-value tok) (token-line tok))
             elements)))
          (#t (set! elements (cons (parse-expression pstate) elements))))))
    ;; Check for dotted pair
    (when (eq? (token-type (parser-current pstate)) 'dot)
      (parser-advance! pstate) ; consume dot
      (set! dotted-tail (parse-expression pstate)))
    ;; Expect closing paren; capture its line before advancing
    (if (eq? (token-type (parser-current pstate)) 'rparen)
      (progn (set! close-line (token-line (parser-current pstate)))
        (parser-advance! pstate))
      (error
       (format nil "Expected ) at line ~A" (token-line (parser-current pstate)))))
    ;; Build the list from children (annotated + comment nodes mixed)
    (let* ((result (build-list-from-children elements dotted-tail))
           ;; Mark empty lists with "()" to distinguish from nil atom
           (orig-form (if (and (null? result) (null? elements)) "()" nil)))
      (make-annotated result orig-form open-line close-line))))

(defun build-list-from-children (elements dotted-tail)
  "Build a list structure from children (annotated expressions and comment
   nodes mixed together). Elements come in reversed order."
  (let ((result (if dotted-tail (ann-value dotted-tail) nil)))
    (do ((remaining elements (cdr remaining))) ((null? remaining) result)
      (set! result (cons (car remaining) result)))))

(defun parse-quoted (pstate quote-sym)
  "Parse 'expr, `expr, ,expr, or ,@expr."
  (let ((start-line (token-line (parser-current pstate))))
    (parser-advance! pstate) ; consume quote token
    (let* ((inner (parse-expression pstate))
           (last-line (if (annotated? inner) (ann-last-line inner) start-line)))
      (make-annotated (list quote-sym inner) nil start-line last-line))))

(defun list-to-vector (lst)
  "Convert a list to a vector."
  (let* ((len (length lst))
         (vec (make-vector len nil))
         (i 0))
    (do ((remaining lst (cdr remaining))) ((null? remaining) vec)
      (vector-set! vec i (car remaining))
      (set! i (+ i 1)))))

(defun parse-vector (pstate)
  "Parse #(elements...). Comments inside vectors are dropped — there is
   no multi-line vector formatter yet. Filed as a known follow-up."
  (let ((open-line (token-line (parser-current pstate)))
        (elements '())
        (close-line -1))
    (parser-advance! pstate) ; consume vector-start
    (do ()
      ((let ((tok (parser-current pstate)))
         (or (eq? (token-type tok) 'rparen) (eq? (token-type tok) 'eof))))
      (let ((tok (parser-current pstate)))
        (cond
          ((eq? (token-type tok) 'comment) (parser-advance! pstate))
          (#t (set! elements (cons (parse-expression pstate) elements))))))
    (when (eq? (token-type (parser-current pstate)) 'rparen)
      (set! close-line (token-line (parser-current pstate)))
      (parser-advance! pstate))
    (make-annotated (list-to-vector (reverse elements)) nil open-line
     close-line)))

;;; ----------------------------------------------------------------------------
;;; Top-Level Reader
;;; ----------------------------------------------------------------------------
(defun read-source-with-comments (source)
  "Parse a source string into a list of top-level children (annotated
   expressions and comment nodes, in source order)."
  (let* ((rstate (make-reader-state source))
         (pstate (make-parser-state rstate))
         (children '()))
    (do () ((eq? (token-type (parser-current pstate)) 'eof))
      (let ((tok (parser-current pstate)))
        (cond
          ((eq? (token-type tok) 'comment)
           (parser-advance! pstate)
           (set! children
            (cons (make-comment-node (token-value tok) (token-line tok))
             children)))
          (#t
           (let ((expr (parse-expression pstate)))
             (when expr (set! children (cons expr children))))))))
    (reverse children)))

(defun read-file-with-comments (filename)
  "Read all top-level children from a file."
  (read-source-with-comments (read-file-to-string filename)))

;;; ============================================================================
;;; Form-Specific Rules Table
;;; ============================================================================
;;; Rules define how different forms should be formatted.
;;; Each rule is an alist with properties:
;;;   handler   - Symbol of function to call for formatting
;;;   inline    - Number of elements to keep inline (for special-form)
;;;   body-indent - Indent increment for body (default: *indent-size*)
(define *form-rules*
  '((cond (handler . format-cond-form)) (case (handler . format-cond-form))
    (if (handler . format-if-form)) (and (handler . format-and-or-form))
    (or (handler . format-and-or-form))))

(defun get-form-rule (sym prop)
  "Get a property from the form rules table for SYM."
  (let ((form-entry (assoc sym *form-rules*)))
    (if form-entry
      (let ((prop-entry (assoc prop (cdr form-entry))))
        (if prop-entry (cdr prop-entry) nil))
      nil)))

;;; ============================================================================
;;; Indent String Caching
;;; ============================================================================
;;; Memoize indent strings to avoid O(n) string building on every call.
(define *indent-cache* (make-vector *max-cached-indent* nil))

(defun build-indent-string (n)
  "Build an indent string of n spaces (uncached, O(n))."
  (let ((result ""))
    (do ((i 0 (+ i 1))) ((>= i n) result) (set! result (concat result " ")))))

;;; ============================================================================
;;; Line Builder Abstraction
;;; ============================================================================
;;; A line builder accumulates formatted output efficiently using lists
;;; instead of repeated string concatenation (O(n) vs O(n²)).
;;;
;;; State: (base-indent completed-lines current-parts current-col)
(defun lb-create (indent)
  "Create a new line builder starting at given indent."
  (list indent '() '() indent))

(defun lb-base-indent (lb) (car lb))

(defun lb-lines (lb) (cadr lb))

(defun lb-parts (lb) (caddr lb))

(defun lb-col (lb) (cadddr lb))

(defun lb-set-lines! (lb val) (set-car! (cdr lb) val))

(defun lb-set-parts! (lb val) (set-car! (cddr lb) val))

(defun lb-set-col! (lb val) (set-car! (cdr (cddr lb)) val))

(defun lb-append! (lb str)
  "Append a string to the current line."
  (lb-set-parts! lb (cons str (lb-parts lb)))
  (lb-set-col! lb (+ (lb-col lb) (length str))))

(defun lb-append-space! (lb)
  "Append a space to the current line."
  (lb-append! lb " "))

(defun lb-newline! (lb &optional new-indent)
  "Finish current line and start a new one at optional indent."
  (let ((indent (if new-indent new-indent (lb-base-indent lb)))
        (line (join (reverse (lb-parts lb)) "")))
    (lb-set-lines! lb (cons line (lb-lines lb)))
    (lb-set-parts! lb (list (make-indent indent)))
    (lb-set-col! lb indent)))

(defun lb-finish (lb)
  "Return the final formatted string."
  (let* ((parts (lb-parts lb))
         (final-line (if (null? parts) "" (join (reverse parts) "")))
         (lines (lb-lines lb))
         (all-lines
          (if (and (null? lines) (string=? final-line ""))
            '("")
            (reverse (cons final-line lines)))))
    (join all-lines "\n")))

;;; ============================================================================
;;; List Traversal Helpers
;;; ============================================================================
(defun list-has-dotted-tail? (lst)
  "Check if list has a dotted (improper) tail."
  (cond
    ((null? lst) #f)
    ((not (pair? lst)) #t)
    (#t (list-has-dotted-tail? (cdr lst)))))

(defun get-dotted-tail (lst)
  "Get the dotted tail of an improper list, or nil."
  (cond
    ((null? lst) nil)
    ((not (pair? lst)) lst)
    (#t (get-dotted-tail (cdr lst)))))

;;; ============================================================================
;;; S-expression to string conversion (single line)
;;; ============================================================================
(defun string-to-source (str)
  "Convert string to source code representation, preserving newlines.
   Escapes internal quotes and backslashes but keeps actual newlines."
  (let ((parts '())
        (i 0)
        (len (length str)))
    (do () ((>= i len))
      (let ((ch (string-ref str i)))
        (cond
          ((char=? ch #\\) (set! parts (cons "\\\\" parts)))
          ((char=? ch #\") (set! parts (cons "\\\"" parts)))
          (#t (set! parts (cons (char->string ch) parts))))
        (set! i (+ i 1))))
    (concat "\"" (apply string-append (reverse parts)) "\"")))

(defun quote-shorthand? (sexp sym)
  "Check if sexp is (SYM x) - a two-element list starting with SYM."
  (and (pair? sexp) (symbol? (car sexp)) (eq? (car sexp) sym)
       (pair? (cdr sexp)) (null? (cddr sexp))))

(defun sexp-to-string (sexp)
  "Convert s-expression to single-line string representation.
   Comment nodes render as empty strings here; the formatter's multi-line
   branch handles them explicitly, and lists containing comment nodes always
   take the multi-line path via has-nested-comments?."
  (cond
    ((comment-node? sexp) "")
    ((annotated? sexp)
     ;; Check for original-form to preserve nil vs () vs #f distinction
     (let ((orig (ann-original-form sexp)))
       (if orig orig (sexp-to-string (ann-value sexp)))))
    ((null? sexp) "nil")
    ((eq? sexp #t) "#t")
    ((eq? sexp #f) "#f")
    ((number? sexp) (number->string sexp))
    ((string? sexp) (string-to-source sexp))
    ((symbol? sexp) (symbol->string sexp))
    ((char? sexp) (format nil "~S" sexp))
    ((vector? sexp) (vector-to-string sexp))
    ;; Quote/quasiquote shorthand - check before generic list handling
    ((quote-shorthand? sexp 'quote) (concat "'" (sexp-to-string (cadr sexp))))
    ((quote-shorthand? sexp 'quasiquote)
     (concat "`" (sexp-to-string (cadr sexp))))
    ((quote-shorthand? sexp 'unquote) (concat "," (sexp-to-string (cadr sexp))))
    ((quote-shorthand? sexp 'unquote-splicing)
     (concat ",@" (sexp-to-string (cadr sexp))))
    ((list? sexp) (list-to-string sexp))
    ((pair? sexp) (list-to-string sexp))
    (#t (format nil "~A" sexp))))

(defun vector-to-string (vec)
  "Convert vector to string like #(a b c)."
  (let ((parts '())
        (len (length vec)))
    (do ((i 0 (+ i 1))) ((>= i len))
      (set! parts (cons (sexp-to-string (vector-ref vec i)) parts)))
    (concat "#(" (join (reverse parts) " ") ")")))

(defun list-to-string (lst)
  "Convert list to string like (a b c). Comment-node children are skipped —
   lists containing comments always render multi-line via format-list-multiline
   so this single-line path is only a length-lower-bound and a fallback."
  (cond
    ((null? lst) "()")
    ((not (pair? lst))
     ;; Not actually a list - just convert to string
     (sexp-to-string lst))
    (#t
     (let ((parts '())
           (current lst))
       ;; Handle proper list, skipping comment nodes
       (do () ((not (pair? current)))
         (unless (comment-node? (car current))
           (set! parts (cons (sexp-to-string (car current)) parts)))
         (set! current (cdr current)))
       ;; Handle dotted pair
       (if (not (null? current))
         (concat "(" (join (reverse parts) " ") " . " (sexp-to-string current)
          ")")
         (concat "(" (join (reverse parts) " ") ")"))))))

;;; ============================================================================
;;; Length calculation
;;; ============================================================================
(defun sexp-length (sexp)
  "Calculate the string length of an s-expression."
  (length (sexp-to-string sexp)))

;;; ============================================================================
;;; Pretty printing with alignment
;;; ============================================================================
(defun make-indent (n)
  "Create a string of n spaces (cached for common values)."
  (cond
    ((< n 0) "")
    ((< n *max-cached-indent*)
     (let ((cached (vector-ref *indent-cache* n)))
       (if cached
         cached
         (let ((s (build-indent-string n))) (vector-set! *indent-cache* n s) s))))
    (#t (build-indent-string n))))

(defun has-nested-comments? (sexp)
  "Check if sexp is, or contains, any comment node."
  (cond
    ((null? sexp) #f)
    ;; Comment-node arm MUST precede the pair? arm: comment nodes are pairs
    ;; whose car is the symbol 'comment.
    ((comment-node? sexp) #t)
    ((annotated? sexp) (has-nested-comments? (ann-value sexp)))
    ((not (pair? sexp)) #f)
    (#t
     (or (has-nested-comments? (car sexp)) (has-nested-comments? (cdr sexp))))))

(defun has-vertical-intent? (lst)
  "Return #t if any two sibling annotated elements in LST are on different
   source lines. Used to preserve user-chosen vertical layout in data
   literals — if the user wrote each element on its own line, we keep
   them on their own lines instead of flattening to fit *max-column*.
   For quote-shorthand wrappers, recurses into the inner list so that
   '(a\\n b\\n c) is detected as vertical even though the quote wrapper
   itself has only one peer."
  (cond
    ((quote-shorthand? lst 'quote)
     (has-vertical-intent? (unwrap-value (cadr lst))))
    ((quote-shorthand? lst 'quasiquote)
     (has-vertical-intent? (unwrap-value (cadr lst))))
    ((quote-shorthand? lst 'unquote)
     (has-vertical-intent? (unwrap-value (cadr lst))))
    ((quote-shorthand? lst 'unquote-splicing)
     (has-vertical-intent? (unwrap-value (cadr lst))))
    ((not (pair? lst)) #f)
    (#t
     (let ((prev-start -1)
           (found #f))
       (do ((remaining lst (if (pair? remaining) (cdr remaining) nil)))
         ((or found (not (pair? remaining))) found)
         (let ((elem (car remaining)))
           (when (annotated? elem)
             (let ((sl (ann-start-line elem)))
               (when (and (> prev-start 0) (not (= sl prev-start)))
                 (set! found #t))
               (set! prev-start sl)))))))))

(defun fits-on-line? (sexp col)
  "Check if sexp fits on line starting at column col."
  (<= (+ col (sexp-length sexp)) *max-column*))

(defun special-form? (sym)
  "Check if symbol is a special form requiring body indentation."
  (member sym
   '(define defun defvar
      defconst
      defmacro
      lambda
      if
      cond
      case
      when
      unless
      progn
      do
      unwind-protect
      condition-case)))

(defun let-form? (sym)
  "Check if symbol is a let/let* form requiring special binding alignment."
  (member sym '(let let*)))

(defun format-sexp-inner (sexp indent)
  "Format s-expression value (unwrapped). Returns string."
  (cond
    ;; Comment nodes render as their text (first-class AST peer)
    ((comment-node? sexp) (comment-node-text sexp))
    ;; Atoms - just convert to string
    ((not (pair? sexp)) (sexp-to-string sexp))
    ;; Empty list
    ((null? sexp) "()")
    ;; If has nested comments, must use multi-line to preserve them
    ((has-nested-comments? sexp) (format-list-multiline sexp indent))
    ;; Preserve user-chosen vertical layout
    ((has-vertical-intent? sexp) (format-list-multiline sexp indent))
    ;; Try single line if no nested comments and fits
    ((fits-on-line? sexp indent) (sexp-to-string sexp))
    ;; Multi-line formatting needed
    (#t (format-list-multiline sexp indent))))

(defun unwrap-value (sexp)
  "Get the actual value from a possibly-annotated expression."
  (if (annotated? sexp) (ann-value sexp) sexp))

(defun get-expr-head (expr)
  "Get the head symbol of an expression, or nil. Comment nodes have no
   head — they always return nil so clustering logic ignores them."
  (cond
    ((comment-node? expr) nil)
    (#t
     (let ((val (unwrap-value expr)))
       (if
         (and (pair? val) (not (comment-node? val))
              (symbol? (unwrap-value (car val))))
         (unwrap-value (car val))
         nil)))))

(defun defining-form? (sym)
  "Check if SYM is a defining form that should not be clustered."
  (member sym '(define defun defmacro defvar defconst)))

(defun format-sexp (sexp indent)
  "Format s-expression with given base indentation. Returns string.
   Comment nodes render as their text; annotated expressions use
   format-sexp-inner on the unwrapped value (or sexp-to-string for atoms
   to preserve original-form)."
  (cond
    ((comment-node? sexp) (comment-node-text sexp))
    (#t
     (let ((value (unwrap-value sexp)))
       (if (and (annotated? sexp) (not (pair? value)))
         (sexp-to-string sexp)
         (format-sexp-inner value indent))))))

(defun format-list-multiline (lst indent)
  "Format a list across multiple lines."
  (let* ((head-raw (car lst))
         (head (unwrap-value head-raw))
         (head-is-symbol (symbol? head))
         (is-special (and head-is-symbol (special-form? head)))
         (is-let (and head-is-symbol (let-form? head)))
         ;; Check form rules table for custom handler
         (form-handler (and head-is-symbol (get-form-rule head 'handler))))
    (cond
      ;; Custom handler from form rules table
      (form-handler ((eval form-handler) lst indent))
      ;; Let forms: (let ((var val) ...) body...)
      (is-let (format-let-form lst indent))
      ;; Special forms: (define name ...) or (defun name (args) ...)
      (is-special (format-special-form lst indent))
      ;; Quoted list: '(...)
      ((and head-is-symbol (string=? (symbol->string head) "quote"))
       (format-quoted-list lst indent))
      ;; Quasiquoted list: `(...)
      ((and head-is-symbol (string=? (symbol->string head) "quasiquote"))
       (format-quasiquoted-expr lst indent))
      ;; Unquoted expression: ,expr
      ((and head-is-symbol (string=? (symbol->string head) "unquote"))
       (format-unquoted-expr lst indent ","))
      ;; Spliced expression: ,@expr
      ((and head-is-symbol (string=? (symbol->string head) "unquote-splicing"))
       (format-unquoted-expr lst indent ",@"))
      ;; Regular list - align elements under first element
      (#t (format-aligned-list lst indent)))))

(defun format-let-form (lst indent)
  "Format let/let* forms with proper binding alignment.
   (let* ((var1 val1)
          (var2 val2))
     body)
   Comment-node peer children in the bindings list or body are rendered
   at the appropriate indent; inline comments (same source line as the
   previous element) are appended with a space."
  (let* ((head (unwrap-value (car lst)))
         (head-str (symbol->string head))
         (rest (cdr lst))
         (bindings-raw (if (pair? rest) (car rest) '()))
         (bindings (unwrap-value bindings-raw))
         (body (if (pair? rest) (cdr rest) '()))
         (body-indent (+ indent *indent-size*))
         ;; Bindings start after "(let* ("
         (binding-indent (+ indent (length head-str) *let-binding-offset*))
         (lb (lb-create indent))
         (force-break #f)
         (prev-last-line -1))
    ;; Start: (let* (
    (lb-append! lb "(")
    (lb-append! lb head-str)
    (lb-append! lb " (")
    ;; Format bindings with comment-node support
    (if (pair? bindings)
      (let ((first-binding #t))
        (do ((remaining bindings (cdr remaining))) ((not (pair? remaining)))
          (let ((elem (car remaining)))
            (cond
              ((comment-node? elem)
               (let ((cline (comment-node-line elem)))
                 (cond
                   ;; First child comment: attach directly after the ( of
                   ;; bindings with no newline
                   (first-binding (lb-append! lb (comment-node-text elem)))
                   ((and (not force-break) (= cline prev-last-line))
                    (lb-append-space! lb)
                    (lb-append! lb (comment-node-text elem)))
                   (#t (lb-newline! lb binding-indent)
                    (lb-append! lb (comment-node-text elem))))
                 (set! force-break #t)
                 (set! prev-last-line cline)
                 (set! first-binding #f)))
              (#t
               (let ((binding-str (format-sexp elem binding-indent)))
                 (cond
                   (first-binding (lb-append! lb binding-str)
                    (set! first-binding #f))
                   (#t (lb-newline! lb binding-indent)
                    (lb-append! lb binding-str))))
               (set! force-break #f)
               (when (annotated? elem)
                 (set! prev-last-line (ann-last-line elem)))))))
        ;; Close bindings: if last was a comment, put ) on its own line
        (when force-break (lb-newline! lb binding-indent))
        (lb-append! lb ")")
        (lb-newline! lb body-indent))
      ;; Empty bindings
      (progn (lb-append! lb ")") (lb-newline! lb body-indent)))
    ;; Format body forms with comment-node support
    (set! force-break #f)
    (set! prev-last-line -1)
    (let ((first-body #t))
      (do ((remaining body (cdr remaining))) ((not (pair? remaining)))
        (let ((elem (car remaining)))
          (cond
            ((comment-node? elem)
             (let ((cline (comment-node-line elem)))
               (cond
                 ((and (not first-body) (not force-break)
                       (= cline prev-last-line))
                  (lb-append-space! lb)
                  (lb-append! lb (comment-node-text elem)))
                 (#t (unless first-body (lb-newline! lb body-indent))
                  (lb-append! lb (comment-node-text elem))))
               (set! force-break #t)
               (set! prev-last-line cline)
               (set! first-body #f)))
            (#t (unless first-body (lb-newline! lb body-indent))
             (lb-append! lb (format-sexp elem body-indent))
             (set! force-break #f)
             (when (annotated? elem) (set! prev-last-line (ann-last-line elem)))
             (set! first-body #f))))))
    ;; Close let form: if last child was a comment, newline first
    (when force-break (lb-newline! lb indent))
    (lb-append! lb ")")
    (lb-finish lb)))

(defun format-special-form (lst indent)
  "Format special forms like define, defun, let, etc.
   Comment-node peer children are rendered inline if on the same line as
   the previous element, otherwise on a new line at body-indent."
  (let* ((head-raw (car lst))
         (head (unwrap-value head-raw))
         (head-str
          (if (symbol? head) (symbol->string head) (sexp-to-string head)))
         (rest (cdr lst))
         (body-indent (+ indent *indent-size*))
         (lb (lb-create indent))
         (elem-count 0)
         (force-break #f)
         (prev-last-line
          (if (annotated? head-raw) (ann-last-line head-raw) -1)))
    (lb-append! lb "(")
    (lb-append! lb head-str)
    (do ((remaining rest (if (pair? remaining) (cdr remaining) nil)))
      ((not (pair? remaining))
       ;; Handle dotted tail
       (when (not (null? remaining)) (lb-append! lb " . ")
         (lb-append! lb (sexp-to-string remaining))))
      (let ((elem (car remaining)))
        (cond
          ((comment-node? elem)
           (let ((cline (comment-node-line elem)))
             (cond
               ((and (not force-break) (= cline prev-last-line))
                (lb-append-space! lb)
                (lb-append! lb (comment-node-text elem)))
               (#t (lb-newline! lb body-indent)
                (lb-append! lb (comment-node-text elem))))
             (set! force-break #t)
             (set! prev-last-line cline)))
          (#t (set! elem-count (+ elem-count 1))
           (let* ((elem-single-len (sexp-length elem))
                  (try-len (+ (lb-col lb) 1 elem-single-len))
                  (nested-comments? (has-nested-comments? elem)))
             ;; First *defun-inline-args* elements try to stay on same line,
             ;; but only if no force-break from a preceding comment and
             ;; the element itself has no nested comments.
             (if
               (and (not force-break) (not nested-comments?)
                    (<= elem-count *defun-inline-args*)
                    (<= try-len *max-column*))
               (progn (lb-append-space! lb)
                 (lb-append! lb (format-sexp elem (lb-col lb))))
               (progn (lb-newline! lb body-indent)
                 (lb-append! lb (format-sexp elem body-indent)))))
           (set! force-break #f)
           (when (annotated? elem) (set! prev-last-line (ann-last-line elem)))))))
    ;; If the last child was a comment, close ) on a new line so ; doesn't
    ;; swallow the ).
    (when force-break (lb-newline! lb indent))
    (lb-append! lb ")")
    (lb-finish lb)))

(defun format-quoted-list (lst indent)
  "Format quoted list like '(a b c ...)."
  (let* ((inner (cadr lst))
         ;; Format with indent+1 to account for the quote character
         (inner-fmt (format-sexp inner (+ indent 1))))
    ;; Always keep quote attached to content
    (concat "'" inner-fmt)))

(defun format-quasiquoted-expr (lst indent)
  "Format quasiquoted expression like `(...)."
  (let* ((inner (cadr lst))
         ;; Format with indent+1 to account for the backtick character
         (inner-fmt (format-sexp inner (+ indent 1))))
    (concat "`" inner-fmt)))

(defun format-unquoted-expr (lst indent prefix)
  "Format unquoted expression with PREFIX (, or ,@)."
  (let* ((inner (cadr lst))
         ;; Format with indent + prefix length
         (inner-fmt (format-sexp inner (+ indent (length prefix)))))
    (concat prefix inner-fmt)))

(defun format-cond-form (lst indent)
  "Format cond/case forms with each clause on its own line.
   Comment-node peer clauses render inline if on the same line as the
   previous clause, otherwise on a new line at clause-indent."
  (let* ((head-raw (car lst))
         (head (unwrap-value head-raw))
         (head-str (symbol->string head))
         (clauses (cdr lst))
         (clause-indent (+ indent *indent-size*))
         (lb (lb-create indent))
         (force-break #f)
         (prev-last-line
          (if (annotated? head-raw) (ann-last-line head-raw) -1)))
    (lb-append! lb "(")
    (lb-append! lb head-str)
    (do ((remaining clauses (cdr remaining))) ((not (pair? remaining)))
      (let ((elem (car remaining)))
        (cond
          ((comment-node? elem)
           (let ((cline (comment-node-line elem)))
             (cond
               ((and (not force-break) (= cline prev-last-line))
                (lb-append-space! lb)
                (lb-append! lb (comment-node-text elem)))
               (#t (lb-newline! lb clause-indent)
                (lb-append! lb (comment-node-text elem))))
             (set! force-break #t)
             (set! prev-last-line cline)))
          (#t (lb-newline! lb clause-indent)
           (lb-append! lb (format-sexp elem clause-indent))
           (set! force-break #f)
           (when (annotated? elem) (set! prev-last-line (ann-last-line elem)))))))
    (when force-break (lb-newline! lb indent))
    (lb-append! lb ")")
    (lb-finish lb)))

(defun format-if-form (lst indent)
  "Format if forms with condition inline, then/else on separate lines.
   Comment-node peer children are emitted at their natural position;
   inline if on the same line as the previous element, otherwise on a
   new line at body-indent."
  (let* ((head-raw (car lst))
         (rest (cdr lst))
         (body-indent (+ indent *indent-size*))
         (lb (lb-create indent))
         (force-break #f)
         (prev-last-line
          (if (annotated? head-raw) (ann-last-line head-raw) -1))
         (value-count 0))
    (lb-append! lb "(if")
    (do ((remaining rest (cdr remaining))) ((not (pair? remaining)))
      (let ((elem (car remaining)))
        (cond
          ((comment-node? elem)
           (let ((cline (comment-node-line elem)))
             (cond
               ((and (not force-break) (= cline prev-last-line))
                (lb-append-space! lb)
                (lb-append! lb (comment-node-text elem)))
               (#t (lb-newline! lb body-indent)
                (lb-append! lb (comment-node-text elem))))
             (set! force-break #t)
             (set! prev-last-line cline)))
          (#t (set! value-count (+ value-count 1))
           (cond
             ((= value-count 1)
              ;; Condition: try inline after "(if "
              (let ((cond-str (format-sexp elem (lb-col lb))))
                (if
                  (and (not force-break)
                       (<= (+ (lb-col lb) 1 (length cond-str)) *max-column*))
                  (progn (lb-append-space! lb) (lb-append! lb cond-str))
                  (progn (lb-newline! lb body-indent)
                    (lb-append! lb (format-sexp elem body-indent))))))
             (#t (lb-newline! lb body-indent)
              (lb-append! lb (format-sexp elem body-indent))))
           (set! force-break #f)
           (when (annotated? elem) (set! prev-last-line (ann-last-line elem)))))))
    (when force-break (lb-newline! lb indent))
    (lb-append! lb ")")
    (lb-finish lb)))

(defun format-and-or-form (lst indent)
  "Format and/or forms with arguments aligned under first argument.
   Comment-node peer children render inline or on a new line at
   arg-indent based on source-line match with the previous element."
  (let* ((head-raw (car lst))
         (head (unwrap-value head-raw))
         (head-str (symbol->string head))
         (args (cdr lst))
         ;; Arguments align under first arg: (or + space = 4, (and + space = 5
         (arg-indent (+ indent 1 (length head-str) 1))
         (lb (lb-create indent))
         (force-break #f)
         (prev-last-line
          (if (annotated? head-raw) (ann-last-line head-raw) -1)))
    (lb-append! lb "(")
    (lb-append! lb head-str)
    (do ((remaining args (cdr remaining)) (first-arg #t #f))
      ((not (pair? remaining)))
      (let ((elem (car remaining)))
        (cond
          ((comment-node? elem)
           (let ((cline (comment-node-line elem)))
             (cond
               ((and (not first-arg) (not force-break) (= cline prev-last-line))
                (lb-append-space! lb)
                (lb-append! lb (comment-node-text elem)))
               (#t (lb-newline! lb arg-indent)
                (lb-append! lb (comment-node-text elem))))
             (set! force-break #t)
             (set! prev-last-line cline)))
          (#t
           (let ((arg-str (format-sexp elem arg-indent))
                 (arg-single-len (sexp-length elem)))
             (if first-arg
               ;; First arg: try to keep on same line
               (if
                 (and (not force-break)
                      (<= (+ (lb-col lb) 1 arg-single-len) *max-column*))
                 (progn (lb-append-space! lb) (lb-append! lb arg-str))
                 (progn (lb-newline! lb arg-indent) (lb-append! lb arg-str)))
               ;; Subsequent args
               (if
                 (or force-break
                     (> (+ (lb-col lb) 1 arg-single-len) *max-column*))
                 (progn (lb-newline! lb arg-indent) (lb-append! lb arg-str))
                 (progn (lb-append-space! lb) (lb-append! lb arg-str)))))
           (set! force-break #f)
           (when (annotated? elem) (set! prev-last-line (ann-last-line elem)))))))
    (when force-break (lb-newline! lb indent))
    (lb-append! lb ")")
    (lb-finish lb)))

(defun format-aligned-list (lst indent)
  "Format list with elements aligned under first element.
   Comment-node peer children render inline if on the same line as the
   previous element, otherwise on a new line at elem-indent.
   Vertical intent is preserved: if two sibling elements were on
   different source lines, the formatter keeps them on different output
   lines."
  (let* ((elem-indent (+ indent 1)) ; After opening paren
         (lb (lb-create elem-indent))
         (dotted-tail nil)
         (force-break #f)
         (prev-last-line -1))
    ;; Start with opening paren
    (lb-set-parts! lb '("("))
    (lb-set-col! lb (+ indent 1))
    ;; Process each element
    (do
      ((remaining lst (if (pair? remaining) (cdr remaining) nil))
       (first-elem #t #f))
      ((not (pair? remaining))
       (if (not (null? remaining)) (set! dotted-tail remaining)))
      (let ((elem (car remaining)))
        (cond
          ((comment-node? elem)
           (let ((cline (comment-node-line elem)))
             (cond
               ;; First child comment: attach directly after ( with no break
               (first-elem (lb-append! lb (comment-node-text elem)))
               ;; Same source line as previous element → inline with space
               ((and (not force-break) (= cline prev-last-line))
                (lb-append-space! lb)
                (lb-append! lb (comment-node-text elem)))
               ;; Otherwise: new line at elem-indent
               (#t (lb-newline! lb elem-indent)
                (lb-append! lb (comment-node-text elem))))
             (set! force-break #t)
             (set! prev-last-line cline)))
          (#t
           (let* ((elem-single-len (sexp-length elem))
                  (space-needed (if first-elem 0 1))
                  (try-col (if first-elem (lb-col lb) (+ (lb-col lb) 1)))
                  (elem-str (format-sexp elem try-col))
                  (is-multiline (string-contains? elem-str "\n"))
                  ;; Vertical-intent preservation: if the element's start
                  ;; line is strictly after prev's last line in source, the
                  ;; user wrote them on different lines → force newline.
                  (source-gap?
                   (and (annotated? elem) (> prev-last-line 0)
                        (> (ann-start-line elem) prev-last-line))))
             (if
               (and (not first-elem)
                    (or force-break source-gap?
                        (> (+ (lb-col lb) space-needed elem-single-len)
                         *max-column*)
                        is-multiline))
               ;; New line needed
               (let ((elem-str-aligned
                      (if is-multiline (format-sexp elem elem-indent) elem-str)))
                 (lb-newline! lb elem-indent)
                 (lb-append! lb elem-str-aligned)
                 (let ((first-newline (string-index elem-str-aligned "\n")))
                   (when first-newline
                     (lb-set-col! lb (+ elem-indent first-newline)))))
               ;; Fits on current line
               (progn (unless first-elem (lb-append-space! lb))
                 (lb-append! lb elem-str))))
           (set! force-break #f)
           (when (annotated? elem) (set! prev-last-line (ann-last-line elem)))))))
    ;; Handle dotted tail - respect force-break too
    (when dotted-tail
      (let* ((tail-single-len (sexp-length dotted-tail))
             (tail-needed (+ 3 tail-single-len)))
        (if (or force-break (> (+ (lb-col lb) tail-needed 1) *max-column*))
          (progn (lb-newline! lb elem-indent) (lb-append! lb ". ")
            (lb-append! lb (format-sexp dotted-tail elem-indent)))
          (progn (lb-append! lb " . ")
            (lb-append! lb (format-sexp dotted-tail (lb-col lb)))))))
    ;; If the last child was a comment, close ) on a new line so ; doesn't
    ;; swallow the ).
    (when force-break (lb-newline! lb indent))
    (lb-append! lb ")")
    (lb-finish lb)))

;;; ============================================================================
;;; File processing
;;; ============================================================================
(defun should-cluster? (prev-head curr-head)
  "Check if two consecutive expressions should be clustered (no blank line).
   Returns #t if both have the same head symbol and neither is a defining form."
  (and prev-head curr-head (eq? prev-head curr-head)
       (not (defining-form? prev-head))))

(defun top-level-separator (prev elem)
  "Return the separator string between two top-level children.
   Rules:
   - Same-line comment after expression: \" \" (inline).
   - Two comments with source gap >= 2 lines: blank line between.
   - Two adjacent comments: single newline (tight cluster).
   - Comment → expression: single newline (comment belongs to next).
   - Expression → comment with source gap >= 2: blank line; else single.
   - Expression → expression: existing clustering logic."
  (cond
    ;; Same-line comment after expression → inline
    ((and (comment-node? elem) (annotated? prev)
          (= (comment-node-line elem) (ann-last-line prev)))
     " ")
    ;; Two comments: blank if source had gap
    ((and (comment-node? prev) (comment-node? elem))
     (if (>= (- (comment-node-line elem) (comment-node-line prev)) 2)
       "\n\n"
       "\n"))
    ;; Comment → expression: single newline (attach comment to next)
    ((comment-node? prev) "\n")
    ;; Expression → comment: blank if source had gap
    ((comment-node? elem)
     (if
       (>=
        (- (comment-node-line elem)
         (if (annotated? prev) (ann-last-line prev) 0))
        2)
       "\n\n"
       "\n"))
    ;; Expression → expression: clustering
    (#t
     (if (should-cluster? (get-expr-head prev) (get-expr-head elem))
       "\n"
       "\n\n"))))

(defun format-source-string (source)
  "Format a source string (read from a file or constructed in-memory).
   Returns the formatted string with a trailing newline.
   Maintains a 'logical prev' that skips over same-line inline comments
   so that a top-level expression followed by an inline trailing comment
   still gets the normal expression-to-expression separator before the
   next top-level form."
  (let ((children (read-source-with-comments source))
        (output "")
        (prev nil)
        (expr-num 0))
    (do ((remaining children (cdr remaining))) ((null? remaining))
      (set! expr-num (+ expr-num 1))
      (condition-case err
        (let* ((elem (car remaining))
               (formatted
                (if (comment-node? elem)
                  (comment-node-text elem)
                  (format-sexp elem 0)))
               (inline-after-prev
                (and (comment-node? elem) (annotated? prev)
                     (= (comment-node-line elem) (ann-last-line prev)))))
          (cond
            ((null? prev) (set! output formatted))
            (#t
             (set! output
              (concat output (top-level-separator prev elem) formatted))))
          ;; Inline same-line comments don't shift the logical prev.
          (unless inline-after-prev (set! prev elem)))
        (error (princ "Error formatting expression #") (princ expr-num)
         (princ ": ") (princ (error-message err)) (terpri)
         (signal 'format-error (error-message err)))))
    (concat output "\n")))

(defun format-file (filename)
  "Format a Lisp file and return formatted content as string."
  (format-source-string (read-file-to-string filename)))

(defun format-file-inplace (filename)
  "Format a Lisp file in-place. Skip write if no changes."
  (let* ((original (read-file-to-string filename))
         (formatted (format-source-string original)))
    ;; Compare trimmed versions to handle trailing newline differences
    (if (string=? (string-trim original) (string-trim formatted))
      (progn (princ filename) (princ " (unchanged)") (terpri))
      (let ((file (open filename "w")))
        (write-line file formatted)
        (close file)
        (princ "Formatted: ")
        (princ filename)
        (terpri)))))

;;; ============================================================================
;;; Main entry point
;;; ============================================================================
(defun print-usage ()
  "Print usage information."
  (princ "Usage: lisp-repl lisp/lisp-fmt.lisp [-i] file.lisp ...\n")
  (princ "  -i, --inplace  Edit files in place\n")
  (princ "  -h, --help     Show this help\n"))

(defun main ()
  "Main entry point - process command line arguments."
  (let ((args *command-line-args*)
        (inplace #f)
        (files '()))
    ;; Show usage if no arguments given
    (if (null? args)
      (print-usage)
      (progn
        ;; Parse arguments
        (do ((remaining args (cdr remaining))) ((null? remaining))
          (let ((arg (car remaining)))
            (cond
              ((or (string=? arg "-h") (string=? arg "--help")) (print-usage)
               (exit 0))
              ((string=? arg "-i") (set! inplace #t))
              ((string=? arg "--inplace") (set! inplace #t))
              (#t (set! files (cons arg files))))))
        ;; Process files (show usage if none given)
        (set! files (reverse files))
        (if (null? files)
          (print-usage)
          (do ((remaining files (cdr remaining))) ((null? remaining))
            (let ((file (car remaining)))
              (if inplace
                (format-file-inplace file)
                (princ (format-file file))))))))))

(when (and (bound? '*command-line-args*) *command-line-args*) (main))

