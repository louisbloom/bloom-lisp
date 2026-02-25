# Characters

Character type predicates, comparisons, and conversions.

## `char?`

Check if a value is a character.

### Parameters

- `value` - Any value to test

### Returns

`#t` if the value is a character, `#f` otherwise.

### Examples

```lisp
(char? #\a)          ; => #t
(char? #\space)      ; => #t
(char? "a")          ; => #f (string, not character)
(char? 65)           ; => #f (integer, not character)
```

## `char-code`

Return the Unicode code point of a character.

### Parameters

- `char` - A character

### Returns

Integer Unicode code point.

### Examples

```lisp
(char-code #\a)      ; => 97
(char-code #\A)      ; => 65
(char-code #\space)  ; => 32
```

### See Also

- `code-char` - Convert code point to character

## `code-char`

Convert a Unicode code point to a character.

### Parameters

- `integer` - Unicode code point (integer)

### Returns

The character with the given code point.

### Examples

```lisp
(code-char 97)       ; => #\a
(code-char 65)       ; => #\A
(code-char 32)       ; => #\space
```

### See Also

- `char-code` - Convert character to code point

## `char->string`

Convert a character to a one-character string.

### Parameters

- `char` - A character

### Returns

A string containing the single character.

### Examples

```lisp
(char->string #\a)   ; => "a"
(char->string #\space) ; => " "
```

### See Also

- `string->char` - Convert string to character

## `string->char`

Convert a one-character string to a character.

### Parameters

- `string` - A string of length 1

### Returns

The character from the string.

### Examples

```lisp
(string->char "a")   ; => #\a
(string->char " ")   ; => #\space
```

### See Also

- `char->string` - Convert character to string

## `char=?`

Test if two characters are equal.

### Parameters

- `char1` - First character
- `char2` - Second character

### Returns

`#t` if characters are equal, `#f` otherwise.

### Examples

```lisp
(char=? #\a #\a)    ; => #t
(char=? #\a #\A)    ; => #f (case-sensitive)
(char=? #\space #\space) ; => #t
```

## `char<?`

Test if first character is less than second (by code point).

### Parameters

- `char1` - First character
- `char2` - Second character

### Returns

`#t` if `char1` has a lower code point than `char2`, `#f` otherwise.

### Examples

```lisp
(char<? #\a #\b)    ; => #t
(char<? #\b #\a)    ; => #f
(char<? #\A #\a)    ; => #t (uppercase before lowercase in Unicode)
```

## `char>?`

Test if first character is greater than second (by code point).

### Parameters

- `char1` - First character
- `char2` - Second character

### Returns

`#t` if `char1` has a higher code point than `char2`, `#f` otherwise.

### Examples

```lisp
(char>? #\b #\a)    ; => #t
(char>? #\a #\b)    ; => #f
(char>? #\a #\A)    ; => #t
```

## `char<=?`

Test if first character is less than or equal to second (by code point).

### Parameters

- `char1` - First character
- `char2` - Second character

### Returns

`#t` if `char1` code point is less than or equal to `char2`, `#f` otherwise.

### Examples

```lisp
(char<=? #\a #\b)   ; => #t
(char<=? #\a #\a)   ; => #t
(char<=? #\b #\a)   ; => #f
```

## `char>=?`

Test if first character is greater than or equal to second (by code point).

### Parameters

- `char1` - First character
- `char2` - Second character

### Returns

`#t` if `char1` code point is greater than or equal to `char2`, `#f` otherwise.

### Examples

```lisp
(char>=? #\b #\a)   ; => #t
(char>=? #\a #\a)   ; => #t
(char>=? #\a #\b)   ; => #f
```

## `char-upcase`

Convert a character to uppercase.

### Parameters

- `char` - A character

### Returns

The uppercase version of the character, or the character unchanged if not a letter.

### Examples

```lisp
(char-upcase #\a)    ; => #\A
(char-upcase #\A)    ; => #\A
(char-upcase #\1)    ; => #\1 (unchanged)
```

### See Also

- `char-downcase` - Convert to lowercase

## `char-downcase`

Convert a character to lowercase.

### Parameters

- `char` - A character

### Returns

The lowercase version of the character, or the character unchanged if not a letter.

### Examples

```lisp
(char-downcase #\A)  ; => #\a
(char-downcase #\a)  ; => #\a
(char-downcase #\1)  ; => #\1 (unchanged)
```

### See Also

- `char-upcase` - Convert to uppercase

## `char-alphabetic?`

Test if a character is alphabetic.

### Parameters

- `char` - A character

### Returns

`#t` if the character is a letter, `#f` otherwise.

### Examples

```lisp
(char-alphabetic? #\a)    ; => #t
(char-alphabetic? #\Z)    ; => #t
(char-alphabetic? #\1)    ; => #f
(char-alphabetic? #\space) ; => #f
```

## `char-numeric?`

Test if a character is numeric (digit).

### Parameters

- `char` - A character

### Returns

`#t` if the character is a digit (0-9), `#f` otherwise.

### Examples

```lisp
(char-numeric? #\5)       ; => #t
(char-numeric? #\0)       ; => #t
(char-numeric? #\a)       ; => #f
(char-numeric? #\space)   ; => #f
```

## `char-whitespace?`

Test if a character is whitespace.

### Parameters

- `char` - A character

### Returns

`#t` if the character is whitespace (space, tab, newline, etc.), `#f` otherwise.

### Examples

```lisp
(char-whitespace? #\space)    ; => #t
(char-whitespace? #\newline)  ; => #t
(char-whitespace? #\tab)      ; => #t
(char-whitespace? #\a)        ; => #f
```
