#!/bin/sh
# Test wrapper script for bloom-lisp tests
# Runs tests from the project root so (load "tests/...") works

# Get the directory containing this script
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
# Project root is one level up
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# Find the bloom-repl binary
if [ -n "$BLOOM_REPL" ] && [ -x "$BLOOM_REPL" ]; then
	REPL="$BLOOM_REPL"
elif [ -x "$PROJECT_ROOT/build/repl/bloom-repl" ]; then
	REPL="$PROJECT_ROOT/build/repl/bloom-repl"
elif [ -x "$PROJECT_ROOT/repl/bloom-repl" ]; then
	REPL="$PROJECT_ROOT/repl/bloom-repl"
else
	echo "ERROR: bloom-repl not found" >&2
	exit 1
fi

# Get the test file path
TEST_FILE="$1"
if [ -z "$TEST_FILE" ]; then
	echo "Usage: $0 <test-file.lisp>" >&2
	exit 1
fi

# Resolve to absolute path from current working directory
if [ -f "$TEST_FILE" ]; then
	TEST_FILE="$(cd "$(dirname "$TEST_FILE")" && pwd)/$(basename "$TEST_FILE")"
fi

# Strip project root prefix to get a path relative to project root
TEST_FILE="${TEST_FILE#"$PROJECT_ROOT/"}"

# Ensure tests/ prefix
case "$TEST_FILE" in
tests/*) ;; # Already has tests/ prefix
*) TEST_FILE="tests/$TEST_FILE" ;;
esac

# Run from project root
cd "$PROJECT_ROOT" || exit 1
exec "$REPL" "$TEST_FILE"
