;; Filesystem Builtin Tests
(load "tests/test-helpers.lisp")

;; ============================================================================
;; getenv Tests
;; ============================================================================
(assert-error (getenv) "getenv requires an argument")
(assert-error (getenv 42) "getenv requires a string")

;; HOME/USERPROFILE should be set on any test environment
(define home-dir (or (getenv "HOME") (getenv "USERPROFILE")))

(assert-true (string? home-dir) "home dir env is a string")
(assert-true (> (length home-dir) 0) "home dir env is non-empty")

;; Nonexistent variable returns nil
(assert-nil (getenv "DITTY_NONEXISTENT_VAR_12345")
 "nonexistent var returns nil")

;; ============================================================================
;; data-directory Tests
;; ============================================================================
(assert-error (data-directory) "data-directory requires an argument")
(assert-error (data-directory 42) "data-directory requires a string")

(define dd (data-directory "test-app"))

(assert-true (string? dd) "data-directory returns a string")
(assert-true (string-contains? dd "test-app")
 "data-directory contains app name")
;; On Windows, data-directory uses LOCALAPPDATA/APPDATA which may use
;; different path style than HOME, so check the appropriate base dir.
(define dd-base (or (getenv "LOCALAPPDATA") (getenv "APPDATA") home-dir))
(assert-true (string-prefix? dd-base dd)
 "data-directory starts with expected base dir")

;; ============================================================================
;; config-directory Tests
;; ============================================================================
(assert-error (config-directory) "config-directory requires an argument")
(assert-error (config-directory 42) "config-directory requires a string")

(define cd (config-directory "test-app"))

(assert-true (string? cd) "config-directory returns a string")
(assert-true (string-contains? cd "test-app")
 "config-directory contains app name")
(define cd-base (or (getenv "APPDATA") home-dir))
(assert-true (string-prefix? cd-base cd)
 "config-directory starts with expected base dir")

;; ============================================================================
;; file-exists? Tests
;; ============================================================================
(assert-error (file-exists?) "file-exists? requires an argument")
(assert-error (file-exists? 42) "file-exists? requires a string")

;; Home directory exists
(assert-true (file-exists? home-dir) "home directory exists")

;; Nonexistent path returns nil
(assert-nil (file-exists? (concat home-dir "/no/such/path/ditty-test-12345"))
 "nonexistent path returns nil")

;; ============================================================================
;; mkdir Tests
;; ============================================================================
(assert-error (mkdir) "mkdir requires an argument")
(assert-error (mkdir 42) "mkdir requires a string")

;; Create a temp directory with nested subdirs
(define temp-base (or (getenv "TEMP") (getenv "TMPDIR") home-dir))

(define test-dir (concat temp-base "/ditty-test/nested/dir"))

(assert-true (mkdir test-dir) "mkdir creates nested directories")
(assert-true (file-exists? test-dir) "created directory exists")
;; Idempotent — succeeds if directory already exists
(assert-true (mkdir test-dir) "mkdir succeeds on existing directory")

;; delete-directory tests
(assert-error (delete-file test-dir) "delete-file refuses directories")

(assert-true (eq? (delete-directory test-dir) nil)
 "delete-directory removes empty dir")

(assert-nil (file-exists? test-dir) "deleted directory no longer exists")

;; delete-directory :recursive removes non-empty dirs
(mkdir test-dir)
(mkdir (concat temp-base "/ditty-test/nested/dir2"))

(assert-true (file-exists? (concat temp-base "/ditty-test/nested/dir2"))
 "subdir exists")

(delete-directory (concat temp-base "/ditty-test/nested") :recursive)

(assert-nil (file-exists? (concat temp-base "/ditty-test/nested"))
 "recursive delete removed nested dir")

;; Clean up
(delete-directory (concat temp-base "/ditty-test") :recursive)

(print "All tests passed!")
