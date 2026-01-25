#!/bin/sh
# Test driver script for bloom-lisp tests
# Usage: run-test.sh bloom-repl test-file.lisp

BLOOM_REPL="${BLOOM_REPL:-./repl/bloom-repl}"

# Called by automake test harness
# Arguments: test-name, source-dir, log-file, trs-file, flags...
if [ "$1" = "--test-name" ]; then
    shift
    test_name="$1"
    shift
    shift  # source-dir
    log_file="$1"
    shift
    trs_file="$1"
    shift
    shift  # --color-tests
    shift  # value
    shift  # --enable-hard-errors
    shift  # value
    shift  # --expect-failure
    shift  # value
    shift  # --
    test_file="$@"

    # Run the test
    output=$("$BLOOM_REPL" "$test_file" 2>&1)
    exit_code=$?

    # Write to log file
    echo "$output" > "$log_file"

    # Check result
    if [ $exit_code -eq 0 ]; then
        echo ":test-result: PASS $test_name" > "$trs_file"
        echo "PASS: $test_name"
        exit 0
    else
        echo ":test-result: FAIL $test_name" > "$trs_file"
        echo "FAIL: $test_name"
        echo "$output"
        exit 1
    fi
fi

# Direct invocation: run-test.sh test-file.lisp
test_file="$1"
if [ -z "$test_file" ]; then
    echo "Usage: $0 test-file.lisp"
    exit 1
fi

"$BLOOM_REPL" "$test_file"
exit $?
