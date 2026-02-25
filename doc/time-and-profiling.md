# Time and Profiling

Timing and performance profiling.

## `current-time-ms`

Return current time in milliseconds.

### Returns

Integer representing current monotonic time in milliseconds since an arbitrary epoch.

### Examples

```lisp
(current-time-ms)        ; => 1234567890
(let ((start (current-time-ms)))
  (sleep 100)
  (- (current-time-ms) start))  ; => ~100
```

### Notes

Uses monotonic clock. Intended for measuring elapsed time, not wall-clock time.

## `profile-start`

Start the profiler and clear previous data.

### Returns

`#t`

### Examples

```lisp
(profile-start)
(my-expensive-function)
(profile-stop)
(profile-report)         ; => list of (name calls ms)
```

### Notes

Enables function-level profiling and resets all accumulated data.
While profiling is active, every function call's elapsed time is tracked.

### See Also

- `profile-stop` - Stop profiling
- `profile-report` - View results

## `profile-stop`

Stop the profiler.

### Returns

`#t`

### Notes

Disables profiling. Data is preserved until `profile-start` or `profile-reset`.

### See Also

- `profile-start` - Start profiling
- `profile-report` - View results

## `profile-report`

Return profiling results as a list.

### Returns

List of `(name calls ms)` entries sorted by total time (descending):

- `name` - Function name (string)
- `calls` - Number of calls (integer)
- `ms` - Total time in milliseconds (float)

### Examples

```lisp
(profile-report)
; => (("format-sexp" 12847 1523.45) ("concat" 98234 452.12) ...)
```

### See Also

- `profile-start` - Start profiling
- `profile-reset` - Clear data

## `profile-reset`

Clear all profiling data.

### Returns

`#t`

### Notes

Clears accumulated profiling data without stopping the profiler.

### See Also

- `profile-start` - Start profiling
