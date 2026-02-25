#!/bin/bash
# gen-docstrings.sh - Generate src/docstrings.gen.h from doc/*.md files
#
# Each .md file uses this structure:
#   # Category Title        (H1 - skipped)
#   Category description    (skipped)
#   ## `function-name`      (H2 - function delimiter)
#   Docstring body...       (captured, H3 converted to H2)
#   ## `next-function`      (next function)
#
# Usage: scripts/gen-docstrings.sh doc > src/docstrings.gen.h

set -e

DOC_DIR="${1:?Usage: $0 <doc-directory>}"

cat <<'HEADER'
/* AUTO-GENERATED from doc/*.md — do not edit */

#ifndef DOCSTRINGS_GEN_H
#define DOCSTRINGS_GEN_H

#include <string.h>

struct builtin_doc {
    const char *name;
    const char *docstring;
};

static const struct builtin_doc builtin_docs[] = {
HEADER

# Process all markdown files in sorted order for deterministic output
for md_file in $(find "$DOC_DIR" -name '*.md' -type f | sort); do
	awk '
    BEGIN {
        name = ""
        nlines = 0
    }

    # Skip H1 headings (category title)
    /^# / { next }

    # H2 heading with backtick-quoted function name
    /^## `[^`]+`/ {
        # Emit previous function if we have one
        if (name != "") {
            emit(name)
        }

        # Extract function name: ## `name` -> name
        n = $0
        sub(/^## `/, "", n)
        sub(/`.*$/, "", n)
        name = n
        nlines = 0
        next
    }

    # Accumulate body lines when inside a function section
    name != "" {
        nlines++
        body[nlines] = $0
    }

    END {
        if (name != "") {
            emit(name)
        }
    }

    function emit(n,    i, start, end, line) {
        # Find first non-blank line
        start = 1
        while (start <= nlines && body[start] == "") start++

        # Find last non-blank line
        end = nlines
        while (end >= start && body[end] == "") end--

        if (start > end) return

        # Emit C struct entry
        printf "    {\"%s\",\n", c_escape(n)

        for (i = start; i <= end; i++) {
            line = body[i]

            # Convert H3 to H2 (### -> ##) for runtime compatibility
            if (line ~ /^### /) {
                sub(/^### /, "## ", line)
            }

            line = c_escape(line)

            if (i < end) {
                printf "     \"%s\\n\"\n", line
            } else {
                printf "     \"%s\"},\n", line
            }
        }

        # Reset for next function
        nlines = 0
        delete body
    }

    function c_escape(s) {
        gsub(/\\/, "\\\\", s)
        gsub(/"/, "\\\"", s)
        return s
    }
    ' "$md_file"
done

cat <<'FOOTER'
    {NULL, NULL}
};

static const char *lookup_builtin_doc(const char *name) {
    for (int i = 0; builtin_docs[i].name != NULL; i++) {
        if (strcmp(builtin_docs[i].name, name) == 0)
            return builtin_docs[i].docstring;
    }
    return NULL;
}

#endif
FOOTER
