# Regular Expressions

PCRE2-based regular expression matching and replacement.

Patterns may be supplied as strings or as compiled regex objects produced by
`regex-compile`. Reuse a compiled regex when matching the same pattern many
times to avoid recompilation overhead.

## `regex-compile`

Compile a PCRE2 pattern into a reusable regex object.

### Parameters

- `pattern` - PCRE2 regular expression pattern (string)

### Returns

A compiled regex object usable in any regex operation. Pattern errors are
reported here rather than at match time.

### Examples

```lisp
(define digits (regex-compile "\\d+"))
(regex-match? digits "abc123")     ; => #t
(regex-find digits "x42y99")       ; => "42"
(regex-find-all digits "a1b22c3")  ; => ("1" "22" "3")
```

### See Also

- `regex?` - Test if a value is a compiled regex
- `regex-valid?` - Test if a pattern compiles

## `regex?`

Test if a value is a compiled regex object.

### Parameters

- `value` - Any value

### Returns

`#t` if `value` was produced by `regex-compile`, `nil` otherwise. Pattern
strings are _not_ regex objects.

### Examples

```lisp
(regex? (regex-compile "\\d+"))  ; => #t
(regex? "\\d+")                  ; => nil
```

## `regex-match?`

Test if regex pattern matches string.

### Parameters

- `pattern` - PCRE2 regular expression pattern (string or compiled regex)
- `string` - String to match against

### Returns

`#t` if pattern matches anywhere in string, `nil` otherwise.

### Examples

```lisp
(regex-match? "[0-9]+" "abc123")      ; => #t
(regex-match? "^[0-9]+$" "abc123")    ; => nil (not all digits)
(regex-match? "hello" "hello world")  ; => #t
```

### See Also

- `regex-find` - Return the matched text
- `regex-find-all` - Find all matches
- `string-match?` - Wildcard pattern matching

## `regex-find`

Find first regex match in string.

### Parameters

- `pattern` - PCRE2 regular expression pattern (string or compiled regex)
- `string` - String to search

### Returns

First matching substring, or `nil` if no match found.

### Examples

```lisp
(regex-find "[0-9]+" "abc123def456")  ; => "123"
(regex-find "\\d+" "no digits")       ; => nil
(regex-find "h.llo" "hello world")    ; => "hello"
```

### See Also

- `regex-match?` - Test if pattern matches
- `regex-find-all` - Find all matches
- `regex-extract` - Extract capture groups

## `regex-find-all`

Find all regex matches in string.

### Parameters

- `pattern` - PCRE2 regular expression pattern (string or compiled regex)
- `string` - String to search

### Returns

List of all matching substrings (in order), or empty list if no matches.

### Examples

```lisp
(regex-find-all "[0-9]+" "a1b22c333")  ; => ("1" "22" "333")
(regex-find-all "\\w+" "hello world")  ; => ("hello" "world")
(regex-find-all "x" "no match")        ; => ()
```

### See Also

- `regex-find` - Find first match only
- `regex-split` - Split string by pattern

## `regex-extract`

Extract capture groups from regex match.

### Parameters

- `pattern` - PCRE2 regular expression with capture groups `(...)`
- `string` - String to match against

### Returns

List of captured substrings (excluding group 0), or `nil` if no match.

### Examples

```lisp
(regex-extract "(\\d+)-(\\d+)" "12-34")     ; => ("12" "34")
(regex-extract "(\\w+)@(\\w+)" "user@host")  ; => ("user" "host")
(regex-extract "(no)(match)" "text")       ; => nil
```

### Notes

Only returns capture groups (not the full match). Use parentheses `()` in pattern to define groups.

## `regex-replace`

Replace all regex matches in string.

### Parameters

- `pattern` - PCRE2 regular expression pattern (string or compiled regex)
- `string` - String to search and replace in
- `replacement` - Replacement string (can use `$1`, `$2` for captures)

### Returns

New string with all matches replaced.

### Examples

```lisp
(regex-replace "[0-9]+" "a1b2c3" "X")        ; => "aXbXcX"
(regex-replace "(\\w+)" "hello" "[$1]")     ; => "[hello]"
(regex-replace "\\s+" "a  b  c" " ")        ; => "a b c"
```

### Notes

Always replaces ALL occurrences. Use `$1`, `$2`, etc. to reference capture groups.

### See Also

- `regex-replace-all` - Alias for this function
- `string-replace` - Literal string replacement (no regex)

## `regex-replace-all`

Replace all regex matches in string. Alias for `regex-replace`.

### Parameters

- `pattern` - PCRE2 regular expression pattern (string or compiled regex)
- `string` - String to search and replace in
- `replacement` - Replacement string (can use `$1`, `$2` for captures)

### Returns

New string with all matches replaced.

### Examples

```lisp
(regex-replace-all "[0-9]+" "a1b2c3" "X")  ; => "aXbXcX"
```

### See Also

- `regex-replace` - Same function
- `string-replace` - Literal string replacement (no regex)

## `regex-split`

Split string by regex pattern.

### Parameters

- `pattern` - PCRE2 regular expression pattern (string or compiled regex)
- `string` - String to split

### Returns

List of substrings split by pattern matches.

### Examples

```lisp
(regex-split "[,;]" "a,b;c")        ; => ("a" "b" "c")
(regex-split "\\s+" "a  b   c")    ; => ("a" "b" "c")
(regex-split "\\d+" "a1b2c")       ; => ("a" "b" "c")
```

### See Also

- `split` - Split by literal string or wildcard
- `regex-find-all` - Find all matches (doesn't split)

## `regex-escape`

Escape special regex characters in string.

### Parameters

- `string` - String to escape

### Returns

New string with regex metacharacters escaped.

### Examples

```lisp
(regex-escape "a.b")         ; => "a\\.b"
(regex-escape "$100")        ; => "\\$100"
(regex-escape "(test)")      ; => "\\(test\\)"
```

### Notes

Escapes these characters: `. ^ $ * + ? ( ) [ ] { } | \`

### Use Case

Use this when you need to match user input literally in a regex pattern.

## `regex-valid?`

Test if regex pattern is valid.

### Parameters

- `pattern` - PCRE2 regular expression pattern (string or compiled regex)

### Returns

`#t` if pattern compiles successfully, `nil` if invalid.

### Examples

```lisp
(regex-valid? "[0-9]+")      ; => #t
(regex-valid? "[0-9")        ; => nil (unclosed bracket)
(regex-valid? "(?P<name>\\w+)")  ; => #t (named capture)
```

### Use Case

Validate user-provided regex patterns before use.
