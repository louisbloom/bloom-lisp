# Functions

Function introspection and evaluation.

## `function-params`

Return the parameter list of a lambda or macro.

### Parameters

- `func` - A lambda or macro

### Returns

The parameter list of the function.

### Examples

```lisp
(function-params (lambda (x y) (+ x y)))  ; => (x y)
```

## `function-body`

Return the body expression list of a lambda or macro.

### Parameters

- `func` - A lambda or macro

### Returns

The body expressions as a list.

### Examples

```lisp
(function-body (lambda (x) (+ x 1)))  ; => ((+ x 1))
```

## `function-name`

Return the name of a lambda, macro, or builtin, or nil.

### Parameters

- `func` - A lambda, macro, or builtin

### Returns

The name as a string, or nil if anonymous.

### Examples

```lisp
(function-name +)              ; => "+"
(function-name (lambda (x) x)) ; => nil
```

## `documentation`

Get documentation string for function, macro, variable, or built-in bound to symbol.

### Parameters

- `symbol` - Symbol name (quoted)

### Returns

String containing the documentation, or `nil` if:

- No docstring exists
- Symbol is unbound
- Symbol's value is not a function, macro, or built-in

### Examples

```lisp
; User-defined function
(define calculate-area
  (lambda (width height)
    "Calculate the area of a rectangle.

    ## Parameters
    - `width` - Width of the rectangle
    - `height` - Height of the rectangle

    ## Returns
    The area as a number."
    (* width height)))

(documentation 'calculate-area)
; => "Calculate the area of a rectangle.\
\
    ## Parameters..."

; Built-in function
(documentation 'car)
; => "Get the first element of a list (the car).\
\
### Parameters..."

; Macro
(defmacro when (condition . body)
  "Execute BODY when CONDITION is true."
  `(if ,condition (progn ,@body) nil))

(documentation 'when)
; => "Execute BODY when CONDITION is true."

; Undefined symbol
(documentation 'nonexistent)  ; => ERROR: Undefined symbol

; Function without docstring
(define no-doc (lambda (x) (* x 2)))
(documentation 'no-doc)       ; => nil
```

### Notes

- Similar to Emacs Lisp's `documentation` function
- Works with lambdas, macros, and built-in functions
- Docstrings use CommonMark (Markdown) format

### Errors

Returns error if:

- Argument is not a symbol
- Symbol is undefined

### See Also

- `bound?` - Check if symbol is bound
- `set-documentation!` - Set documentation for symbol

## `bound?`

Check if a symbol is bound in the environment.

### Parameters

- `symbol` - Symbol name (quoted)

### Returns

`#t` if the symbol is bound, `#f` otherwise.

### Examples

```lisp
(bound? 'car)              ; => #t (built-in)
(bound? 'nonexistent)      ; => #f
(define my-var 42)
(bound? 'my-var)           ; => #t
```

### Notes

- Used by `defvar` macro to conditionally define variables
- Checks current and parent environments

### See Also

- `defvar` - Define variable only if unbound
- `documentation` - Get documentation for symbol

## `set-documentation!`

Set documentation string for a bound symbol.

### Parameters

- `symbol` - Symbol name (quoted)
- `docstring` - Documentation string (CommonMark format)

### Returns

`#t` on success.

### Examples

```lisp
(define my-var 42)
(set-documentation! 'my-var "The answer to everything.")
(documentation 'my-var)  ; => "The answer to everything."
```

### Notes

- Used by `defvar` and `defconst` macros
- Docstrings use CommonMark (Markdown) format
- Symbol must already be bound

### Errors

Returns error if:

- First argument is not a symbol
- Second argument is not a string
- Symbol is not bound

### See Also

- `documentation` - Get documentation for symbol
- `defvar` - Define variable with optional docstring
- `defconst` - Define constant with optional docstring

## `eval`

Evaluate an expression in the current environment.

### Parameters

- `expression` - A Lisp expression to evaluate

### Returns

The result of evaluating the expression.

### Examples

```lisp
(eval '(+ 1 2))         ; => 3
(eval 'my-function)      ; => #<lambda my-function ...>
```

### Notes

Useful for evaluating quoted expressions and symbols to get their bound values.
