# Symbols

Symbol and keyword operations.

## `symbol->string`

Convert symbol to string.

### Parameters

- `symbol` - Symbol to convert

### Returns

String containing the symbol's name.

### Examples

```lisp
(symbol->string 'hello)      ; => "hello"
(symbol->string '+)           ; => "+"
(symbol->string 'my-var)      ; => "my-var"

; Using with variables
(define x 'test)
(symbol->string x)            ; => "test"
```

### Errors

Returns error if argument is not a symbol.

### See Also

- `symbol?` - Test if value is symbol
- `string->symbol` - Convert string to symbol

## `string->symbol`

Convert string to interned symbol.

### Parameters

- `string` - String containing symbol name

### Returns

Interned symbol with the given name.

### Examples

```lisp
(string->symbol "hello")     ; => hello
(string->symbol "+")         ; => +
(string->symbol "my-var")    ; => my-var

; Symbols are interned
(eq? (string->symbol "foo") 'foo)  ; => #t
```

### Errors

Returns error if argument is not a string.

### See Also

- `symbol?` - Test if value is symbol
- `symbol->string` - Convert symbol to string

## `keyword-name`

Get the name of a keyword without the leading colon.

### Parameters

- `keyword` - A keyword value

### Returns

A string containing the keyword's name without the colon prefix.

### Examples

```lisp
(keyword-name :foo)      ; => "foo"
(keyword-name :bar-baz)  ; => "bar-baz"
```

### See Also

- `keyword?` - Check if value is keyword
- `symbol->string` - Convert symbol to string
