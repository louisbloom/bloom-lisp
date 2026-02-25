# Filesystem

Path expansion, environment variables, and filesystem operations.

## `home-directory`

Get the user's home directory path.

### Parameters

None.

### Returns

String with home directory path, or `nil` if home directory cannot be determined.

### Examples

```lisp
(home-directory)                    ; => "/home/alice" (Unix)
(home-directory)                    ; => "C:\\Users\\Alice" (Windows)

; Use in paths
(define config-dir (concat (home-directory) "/.config"))
; => "/home/alice/.config"
```

### Platform Behavior

- **Unix/Linux/macOS**: Uses `$HOME` environment variable
- **Windows**: Uses `%USERPROFILE%` or `%HOMEDRIVE%%HOMEPATH%`

### See Also

- `expand-path` - Expand `~/` prefix in paths

## `expand-path`

Expand `~/` prefix in file paths to the user's home directory.

### Parameters

- `path` - File path (string), may start with `~/`

### Returns

String with expanded path (if path starts with `~/`), or original string (if path does not start with `~/`).

### Examples

```lisp
; Basic expansion
(expand-path "~/config.lisp")
; => "/home/alice/config.lisp" (Unix)
; => "C:\\Users\\Alice\\config.lisp" (Windows)

; Subdirectories
(expand-path "~/Documents/notes.txt")
; => "/home/alice/Documents/notes.txt" (Unix)

; No expansion (no ~/ prefix)
(expand-path "/etc/config")         ; => "/etc/config"
(expand-path "relative/path")       ; => "relative/path"
(expand-path "./local.lisp")        ; => "./local.lisp"

; Just ~ expands to home directory
(expand-path "~")                   ; => "/home/alice"

; Use with file I/O
(define file (open (expand-path "~/my-config.lisp") "r"))
(load (expand-path "~/scripts/init.lisp"))
```

### Notes

- Detects `~/` at start of path
- Replaces `~/` with home directory from `home-directory`
- Handles cross-platform path separators
- Works with both forward and backslashes after `~`

### Errors

Returns error if:

- Home directory cannot be determined
- Argument is not a string

### Use Cases

- Reading/writing user configuration files
- Loading user-specific scripts
- Saving data to user directories
- Cross-platform file operations

### See Also

- `home-directory` - Get home directory path
- `open` - Open file
- `load` - Load Lisp file

## `getenv`

Read an environment variable.

### Parameters

- `name` - Environment variable name (string)

### Returns

String value of the variable, or nil if not set.

### Examples

```lisp
(getenv "HOME")            ; => "/home/alice"
(getenv "NONEXISTENT")     ; => nil
(getenv "PATH")            ; => "/usr/bin:/bin:..."
```

## `data-directory`

Return the platform-specific user data directory for an application.

### Parameters

- `app` - Application name (string)

### Returns

String path. Does not create the directory.

### Platform Behavior

- **Linux/macOS**: `$XDG_DATA_HOME/app` or `~/.local/share/app`
- **Windows**: `%LOCALAPPDATA%\app` or `%APPDATA%\app`

### Examples

```lisp
(data-directory "my-app")  ; => "/home/alice/.local/share/my-app"
(mkdir (data-directory "my-app"))  ; create it if needed
```

### See Also

- `mkdir` - Create directories
- `getenv` - Read environment variables

## `file-exists?`

Check if a file or directory exists.

### Parameters

- `path` - File path (string)

### Returns

#t if path exists, nil otherwise.

### Examples

```lisp
(file-exists? "/tmp")          ; => #t
(file-exists? "/no/such/path") ; => nil
```

## `mkdir`

Create a directory and all parent directories (like mkdir -p).

### Parameters

- `path` - Directory path (string)

### Returns

#t on success. Succeeds silently if directory already exists.

### Examples

```lisp
(mkdir "/tmp/my-app/data")  ; creates /tmp/my-app and /tmp/my-app/data
(mkdir (data-directory "my-app"))  ; create app data dir
```

### See Also

- `data-directory` - Get platform data directory
- `file-exists?` - Check if path exists
