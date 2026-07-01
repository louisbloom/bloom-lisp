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

- `config-directory` - Get platform config directory
- `mkdir` - Create directories
- `getenv` - Read environment variables

## `config-directory`

Return the platform-specific user config directory for an application.

### Parameters

- `app` - Application name (string)

### Returns

String path. Does not create the directory.

### Platform Behavior

- **Linux/macOS**: `$XDG_CONFIG_HOME/app` or `~/.config/app`
- **Windows**: `%APPDATA%\app`

### Examples

```lisp
(config-directory "my-app")  ; => "/home/alice/.config/my-app"
(mkdir (config-directory "my-app"))  ; create it if needed
```

### See Also

- `data-directory` - Get platform data directory
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
- `config-directory` - Get platform config directory
- `file-exists?` - Check if path exists

## `delete-directory`

Delete a directory. Like Emacs Lisp's `delete-directory`.

### Parameters

- `path` - Directory path (string)
- `:recursive` - Optional keyword; if present, delete directory and all its contents

### Returns

`nil` on success, error if deletion fails.

### Examples

```lisp
(delete-directory "/tmp/empty-dir")                  ; remove empty directory
(delete-directory "/tmp/parent-dir" :recursive)     ; remove directory and all contents
```

### Notes

Without `:recursive`, only empty directories can be removed (like `rmdir`).
With `:recursive`, the directory and all files and subdirectories inside it are deleted.
Use `delete-file` for files — `delete-file` will refuse to delete directories.

### Errors

- Error if path is not a directory
- Error if directory is not empty and `:recursive` is not specified
- Error if deletion fails (permission denied, etc.)

### See Also

- `delete-file` - Delete a file
- `mkdir` - Create a directory
- `file-exists?` - Check if path exists

## `system-type`

Return a symbol identifying the operating system family. Emacs Lisp style.

### Parameters

None.

### Returns

Symbol identifying the OS:

| Symbol       | Platform                             |
| ------------ | ------------------------------------ |
| `windows-nt` | Native Windows (MSVC/MinGW)          |
| `msys`       | MSYS2 / Cygwin (case-insensitive FS) |
| `darwin`     | macOS                                |
| `gnu/linux`  | Linux                                |
| `freebsd`    | FreeBSD                              |
| `netbsd`     | NetBSD                               |
| `openbsd`    | OpenBSD                              |
| `unknown`    | Unrecognized platform                |

### Examples

```lisp
(system-type)        ; => gnu/linux (on Linux)
(system-type)        ; => windows-nt (on native Windows)
(system-type)        ; => msys (on MSYS2/Cygwin)

;; Branch on platform
(if (eq? (system-type) 'windows-nt)
    (load "windows-init.lisp")
    (load "unix-init.lisp"))

;; Guard case-sensitive-filesystem assertions
(unless (eq? (system-type) 'msys)
  (assert-false (file-exists? "MIXED-CASE-File")
    "case-sensitive filesystem distinguishes case"))
```

### Notes

- `msys` covers both Cygwin and MSYS2 (including UCRT64 and MINGW64
  shells). The filesystem is case-insensitive in these environments,
  which affects tests that assert file case sensitivity.
- On native Windows builds (not through MSYS2/Cygwin), `windows-nt` is
  returned instead.

### See Also

- `home-directory` - Get home directory (platform-specific)
- `config-directory` - Get config directory (platform-specific)
- `data-directory` - Get data directory (platform-specific)
