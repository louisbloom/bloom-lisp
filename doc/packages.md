# Packages

Package (namespace) management.

## `in-package`

Set the current package.

### Parameters

- `name` - Package name (symbol). Strings also accepted for convenience.

### Returns

The package name as a symbol.

### Examples

```lisp
(in-package 'math)
(in-package 'user)
```

## `current-package`

Return the current package name as a symbol.

### Examples

```lisp
(current-package) ; => user
```

## `package-symbols`

Return an alist of bindings in the named package.

### Parameters

- `name` - Package name (symbol). Strings also accepted for convenience.

### Returns

An alist of `(symbol . value)` pairs.

### Examples

```lisp
(package-symbols 'user) ; => ((x . 42) ...)
```

## `list-packages`

Return a list of distinct package names as symbols.

### Examples

```lisp
(list-packages) ; => (core user)
```

## `package-save`

Save package bindings to a file as valid Lisp source.

### Parameters

- `filename` - Path to write (string)
- `package` (optional) - Package name (symbol). Defaults to current package.
- `:defun` (optional) - Extract lambdas into named `defun` forms for readability.
- `:format` (optional) - Pretty-print the output (requires lisp-fmt.lisp to be loaded).

### Returns

`#t` on success.

### Examples

```lisp
(define x 42)
(package-save "my-pkg.lisp")
(package-save "math.lisp" 'math)
(package-save "my-pkg.lisp" :defun)
(package-save "math.lisp" 'math :defun)
(package-save "my-pkg.lisp" :defun :format)
```

### Notes

The output file can be loaded with `(load filename)`. Strings are also accepted
for the package name and converted to symbols internally.

With `:defun`, top-level lambdas emit as `(defun name ...)` and nested lambdas
are extracted into separate defun forms referenced by name.

With `:format`, the output is pretty-printed using `format-file` from lisp-fmt.lisp.
Load lisp-fmt.lisp first: `(load "lisp/lisp-fmt.lisp")`.
