;; Filesystem Builtin Tests
(load "tests/test-helpers.lisp")

;; ============================================================================
;; getenv Tests
;; ============================================================================
(assert-error (getenv) "getenv requires an argument")
(assert-error (getenv 42) "getenv requires a string")

;; HOME should be set on any Unix test environment
(assert-true (string? (getenv "HOME")) "HOME is a string")
(assert-true (> (length (getenv "HOME")) 0) "HOME is non-empty")

;; Nonexistent variable returns nil
(assert-nil (getenv "BLOOM_LISP_NONEXISTENT_VAR_12345")
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
(assert-true (string-prefix? (getenv "HOME") dd)
 "data-directory starts with HOME")

;; ============================================================================
;; file-exists? Tests
;; ============================================================================
(assert-error (file-exists?) "file-exists? requires an argument")
(assert-error (file-exists? 42) "file-exists? requires a string")

;; Home directory exists
(assert-true (file-exists? (getenv "HOME")) "home directory exists")

;; Nonexistent path returns nil
(assert-nil (file-exists? "/no/such/path/bloom-lisp-test-12345")
 "nonexistent path returns nil")

;; ============================================================================
;; mkdir Tests
;; ============================================================================
(assert-error (mkdir) "mkdir requires an argument")
(assert-error (mkdir 42) "mkdir requires a string")

;; Create a temp directory with nested subdirs
(define test-dir (concat (getenv "HOME") "/.cache/bloom-lisp-test/nested/dir"))

(assert-true (mkdir test-dir) "mkdir creates nested directories")
(assert-true (file-exists? test-dir) "created directory exists")
;; Idempotent — succeeds if directory already exists
(assert-true (mkdir test-dir) "mkdir succeeds on existing directory")

;; Clean up
(delete-file (concat (getenv "HOME") "/.cache/bloom-lisp-test/nested/dir"))
(delete-file (concat (getenv "HOME") "/.cache/bloom-lisp-test/nested"))
(delete-file (concat (getenv "HOME") "/.cache/bloom-lisp-test"))

(print "All tests passed!")
