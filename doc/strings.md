# Strings

String manipulation functions. All operations are UTF-8 aware.

## `concat`

Concatenate strings together.

### Parameters

- `strings...` - Zero or more strings to concatenate

### Returns

A new string formed by concatenating all arguments. Returns empty string `""` if no arguments provided.

### Examples

```lisp
(concat "Hello" " " "World")     ; => "Hello World"
(concat "foo" "bar" "baz")      ; => "foobarbaz"
(concat)                          ; => ""
(concat "test")                   ; => "test"
```

## `substring`

Extract a substring by character indices (UTF-8 aware).

### Parameters

- `string` - The input string
- `start` - Starting character index (0-based, inclusive)
- `end` - Ending character index (0-based, exclusive)

### Returns

A new string containing characters from `start` to `end-1`. Returns empty string if `start >= end`.

### Examples

```lisp
(substring "Hello" 0 5)          ; => "Hello"
(substring "Hello" 1 4)          ; => "ell"
(substring "Hello, 世界!" 7 9)    ; => "世界" (UTF-8 aware)
```

### Notes

- Indices are **character** positions, not byte positions
- Works correctly with multi-byte UTF-8 characters
- Index out of bounds returns error

## `string-ref`

Get character at index from string (UTF-8 aware).

### Parameters

- `string` - The input string
- `index` - Character index (0-based)

### Returns

Single-character string at the specified index.

### Examples

```lisp
(string-ref "hello" 0)          ; => "h"
(string-ref "hello" 4)          ; => "o"
(string-ref "世界" 0)            ; => "世" (UTF-8)
```

### Notes

- Index is **character** position, not byte position
- Works correctly with multi-byte UTF-8 characters
- Returns error if index out of bounds

### See Also

- `substring` - Extract substring by range
- `length` - Get length of sequence (list, string, or vector)

## `split`

Split string by pattern (supports wildcards).

### Parameters

- `string` - The string to split
- `pattern` - Pattern to split on (supports wildcards: `*`, `?`, `[abc]`)

### Returns

List of strings split by pattern.

### Examples

```lisp
(split "a,b,c" ",")               ; => ("a" "b" "c")
(split "foo*bar*baz" "*")         ; => ("foo" "bar" "baz")
(split "hello world" " ")        ; => ("hello" "world")
```

## `join`

Join a list of strings with a separator.

### Parameters

- `list` - List of strings to join
- `separator` - String to insert between elements

### Returns

A single string with all elements joined.

### Examples

```lisp
(join '("a" "b" "c") ",")       ; => "a,b,c"
(join '("hello" "world") " ")  ; => "hello world"
```

## `number->string`

Convert number to string with optional radix.

### Parameters

- `number` - Integer or float to convert
- `radix` - Optional base (2-36, defaults to 10)

### Returns

String representation of the number.

### Examples

```lisp
(number->string 42)          ; => "42"
(number->string 3.14159)     ; => "3.14159"
(number->string 255 16)      ; => "ff" (hexadecimal)
(number->string 8 2)         ; => "1000" (binary)
```

### Notes

- Floats only supported in base 10
- For other bases, number is converted to integer first

### See Also

- `string->number` - Convert string to number

## `string->number`

Convert string to number with optional radix.

### Parameters

- `string` - String to parse
- `radix` - Optional base (2-36, defaults to 10)

### Returns

Number parsed from string, or `nil` if invalid.

### Examples

```lisp
(string->number "42")           ; => 42
(string->number "3.14")         ; => 3.14
(string->number "ff" 16)        ; => 255 (hexadecimal)
(string->number "#xff")         ; => 255 (auto-detect hex prefix)
(string->number "invalid")      ; => nil
```

### Notes

- Supports prefixes: `#b` (binary), `#o` (octal), `#d` (decimal), `#x` (hex)
- Floats only supported in base 10
- Ignores leading/trailing whitespace

### See Also

- `number->string` - Convert number to string

## `string-contains?`

Test if string contains substring.

### Parameters

- `haystack` - String to search in
- `needle` - Substring to find

### Returns

`#t` if `needle` is found anywhere in `haystack`, `nil` otherwise.

### Examples

```lisp
(string-contains? "hello world" "world")   ; => #t
(string-contains? "hello world" "planet")  ; => nil
(string-contains? "test" "")               ; => #t (empty string)
```

### See Also

- `string-index` - Find position of substring
- `string-prefix?` - Test if string starts with prefix

## `string-index`

Find character index of first occurrence of substring.

### Parameters

- `haystack` - String to search in
- `needle` - Substring to find

### Returns

Character index (0-based) of first occurrence, or `nil` if not found.

### Examples

```lisp
(string-index "hello world" "world")    ; => 6
(string-index "hello" "l")              ; => 2 (first 'l')
(string-index "test" "xyz")             ; => nil
```

### Notes

Returns character index, not byte offset (UTF-8 aware).

### See Also

- `string-contains?` - Test for substring presence

## `string-match?`

Test if string matches wildcard pattern.

### Parameters

- `string` - String to test
- `pattern` - Wildcard pattern

### Returns

`#t` if string matches pattern, `nil` otherwise.

### Pattern Syntax

- `*` - Match zero or more characters
- `?` - Match exactly one character
- `[abc]` - Match any character in set
- `[a-z]` - Match any character in range
- `[!abc]` - Match any character NOT in set

### Examples

```lisp
(string-match? "hello" "h*o")        ; => #t
(string-match? "test.txt" "*.txt")   ; => #t
(string-match? "file" "f?le")        ; => #t
(string-match? "abc" "[a-z]*")       ; => #t
```

### See Also

- `regex-match?` - Full regex pattern matching
- `string-prefix?` - Test for exact prefix

## `string-prefix?`

Test if string starts with prefix.

### Parameters

- `prefix` - Prefix to test for
- `string` - String to test

### Returns

`#t` if `string` starts with `prefix`, `nil` otherwise.

### Examples

```lisp
(string-prefix? "hello" "hello world")   ; => #t
(string-prefix? "world" "hello world")   ; => nil
(string-prefix? "" "anything")           ; => #t (empty prefix)
```

### See Also

- `string-contains?` - Test for substring anywhere
- `string-match?` - Wildcard pattern matching

## `string-replace`

Replace all occurrences of substring in string.

### Parameters

- `string` - The input string
- `old` - Substring to find
- `new` - Substring to replace with

### Returns

New string with all occurrences of `old` replaced by `new`.

### Examples

```lisp
(string-replace "hello world" "world" "universe")
  ; => "hello universe"
(string-replace "hello" "l" "L")
  ; => "heLLo" (all occurrences)
```

## `string-upcase`

Convert string to uppercase (ASCII only).

### Parameters

- `string` - The input string

### Returns

New string with all ASCII letters converted to uppercase.

### Examples

```lisp
(string-upcase "hello world")    ; => "HELLO WORLD"
(string-upcase "Hello123")       ; => "HELLO123"
```

### Notes

Only converts ASCII letters (a-z). Non-ASCII characters unchanged.

## `string-downcase`

Convert string to lowercase (ASCII only).

### Parameters

- `string` - The input string

### Returns

New string with all ASCII letters converted to lowercase.

### Examples

```lisp
(string-downcase "HELLO WORLD")  ; => "hello world"
(string-downcase "Hello123")     ; => "hello123"
```

### Notes

Only converts ASCII letters (A-Z). Non-ASCII characters unchanged.

## `string-trim`

Remove leading and trailing whitespace from a string.

### Parameters

- `string` - The input string

### Returns

New string with leading and trailing whitespace removed.

### Examples

```lisp
(string-trim "  hello  ")      ; => "hello"
(string-trim "\thello\
")      ; => "hello"
(string-trim "hello")           ; => "hello"
(string-trim "   ")             ; => ""
```

### Notes

Whitespace includes space, tab, newline, carriage return, form feed, vertical tab.

## `string<?`

Test if strings are in lexicographic order (less than).

### Parameters

- `str1` - First string
- `str2` - Second string

### Returns

`#t` if `str1` is lexicographically less than `str2`, `nil` otherwise.

### Examples

```lisp
(string<? "apple" "banana")     ; => #t
(string<? "zebra" "ant")        ; => nil
(string<? "abc" "abc")          ; => nil (equal)
```

### Notes

Comparison is byte-by-byte (ASCII/UTF-8 code point order).

### See Also

- `string>?` - Greater than
- `string=?` - Equality

## `string>?`

Test if strings are in reverse lexicographic order (greater than).

### Parameters

- `str1` - First string
- `str2` - Second string

### Returns

`#t` if `str1` is lexicographically greater than `str2`, `nil` otherwise.

### Examples

```lisp
(string>? "banana" "apple")     ; => #t
(string>? "ant" "zebra")        ; => nil
(string>? "abc" "abc")          ; => nil (equal)
```

## `string<=?`

Test if strings are in non-decreasing lexicographic order (less than or equal).

### Parameters

- `str1` - First string
- `str2` - Second string

### Returns

`#t` if `str1` is lexicographically less than or equal to `str2`, `nil` otherwise.

### Examples

```lisp
(string<=? "apple" "banana")    ; => #t
(string<=? "abc" "abc")         ; => #t (equal)
(string<=? "zebra" "ant")       ; => nil
```

## `string>=?`

Test if strings are in non-increasing lexicographic order (greater than or equal).

### Parameters

- `str1` - First string
- `str2` - Second string

### Returns

`#t` if `str1` is lexicographically greater than or equal to `str2`, `nil` otherwise.

### Examples

```lisp
(string>=? "banana" "apple")    ; => #t
(string>=? "abc" "abc")         ; => #t (equal)
(string>=? "ant" "zebra")       ; => nil
```
