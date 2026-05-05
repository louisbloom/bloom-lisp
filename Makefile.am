ACLOCAL_AMFLAGS = -I m4

SUBDIRS = include src repl tests

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = bloom-lisp.pc

bloomlispdir = $(datadir)/bloom-lisp/lisp
bloomlisp_DATA = lisp/lisp-fmt.lisp

EXTRA_DIST = \
    autogen.sh \
    build-aux/git-version.sh \
    version \
    bloom-lisp.pc.in \
    README.md \
    LANGUAGE_REFERENCE.md \
    COPYING \
    .clang-format \
    lisp/lisp-fmt.lisp \
    scripts/gen-docstrings.sh \
    scripts/build-mingw64.sh \
    doc/arithmetic.md \
    doc/characters.md \
    doc/comparison.md \
    doc/errors.md \
    doc/file-io.md \
    doc/filesystem.md \
    doc/functions.md \
    doc/hash-tables.md \
    doc/lists.md \
    doc/packages.md \
    doc/printing.md \
    doc/regex.md \
    doc/string-ports.md \
    doc/strings.md \
    doc/symbols.md \
    doc/time-and-profiling.md \
    doc/type-predicates.md \
    doc/vectors.md

# Format target — clang-format on C, shfmt on shell, prettier on Markdown,
# and the Lisp formatter (lisp/lisp-fmt.lisp) on .lisp files via bloom-repl.
.PHONY: format
format:
	@if command -v clang-format >/dev/null 2>&1; then \
	    echo "Formatting C with clang-format..."; \
	    find $(top_srcdir)/src $(top_srcdir)/include $(top_srcdir)/repl $(top_srcdir)/tests \
	        \( -name '*.c' -o -name '*.h' \) -print0 \
	        | xargs -0 clang-format -i; \
	else \
	    echo "clang-format not found - skipping C formatting." >&2; \
	fi
	@if command -v shfmt >/dev/null 2>&1; then \
	    echo "Formatting shell scripts with shfmt..."; \
	    find $(top_srcdir) -maxdepth 2 -name '*.sh' -print0 2>/dev/null \
	        | xargs -0 -r shfmt -w; \
	else \
	    echo "shfmt not found - skipping shell script formatting." >&2; \
	fi
	@if command -v prettier >/dev/null 2>&1; then \
	    echo "Formatting Markdown with prettier..."; \
	    find $(top_srcdir) -maxdepth 2 -name '*.md' -print0 \
	        | xargs -0 prettier --write; \
	else \
	    echo "prettier not found - skipping Markdown formatting." >&2; \
	fi
	@repl=""; \
	if [ -x "$(top_builddir)/repl/bloom-repl" ]; then repl="$(top_builddir)/repl/bloom-repl"; \
	elif command -v bloom-repl >/dev/null 2>&1; then repl=bloom-repl; fi; \
	if [ -n "$$repl" ]; then \
	    echo "Formatting Lisp via $$repl ..."; \
	    find $(top_srcdir)/lisp $(top_srcdir)/tests -name '*.lisp' -print0 \
	        | xargs -0 -r -n 1 "$$repl" $(top_srcdir)/lisp/lisp-fmt.lisp -- -i; \
	else \
	    echo "bloom-repl not found - skipping Lisp formatting (build first)." >&2; \
	fi

# Bear target — produce compile_commands.json for clangd by intercepting
# the build. Cleans first so bear sees every compile invocation.
.PHONY: bear
bear:
	@command -v bear >/dev/null 2>&1 || { \
	    echo "bear not installed - install via your package manager." >&2; \
	    exit 1; \
	}
	$(MAKE) clean
	bear --output $(top_srcdir)/compile_commands.json -- $(MAKE)
