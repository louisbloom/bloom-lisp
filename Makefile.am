ACLOCAL_AMFLAGS = -I m4

SUBDIRS = include src flare cli emacs tests

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = ditty.pc

dittydatadir = $(datadir)/ditty/lisp
dittydata_DATA = lisp/lisp-fmt.lisp

EXTRA_DIST = \
    autogen.sh \
    build-aux/git-version.sh \
    version \
    ditty.pc.in \
    README.md \
    LANGUAGE_REFERENCE.md \
    COPYING \
    .clang-format \
    lisp/lisp-fmt.lisp \
    scripts/gen-docstrings.sh \
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
    doc/vectors.md \
    emacs/README.md \
    emacs/COPYING \
    emacs/ditty-mode.el \
    emacs/ditty-mode-tests.el

# Format target — clang-format on C, shfmt on shell, prettier on Markdown,
# and the Lisp formatter (lisp/lisp-fmt.lisp) on .lisp files via ditty.
.PHONY: format
format:
	@if command -v clang-format >/dev/null 2>&1; then \
	    echo "Formatting C with clang-format..."; \
	    find $(top_srcdir)/src $(top_srcdir)/include $(top_srcdir)/cli $(top_srcdir)/flare $(top_srcdir)/tests \
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
	@if command -v emacs >/dev/null 2>&1; then \
	    echo "Formatting Emacs Lisp via emacs..."; \
	    $(MAKE) -C emacs elisp-format srcdir=$(top_srcdir)/emacs; \
	else \
	    echo "emacs not found - skipping Elisp formatting." >&2; \
	fi
	@ditty_bin=""; \
	if [ -x "$(top_builddir)/cli/ditty" ]; then ditty_bin="$(top_builddir)/cli/ditty"; \
	elif command -v ditty >/dev/null 2>&1; then ditty_bin=ditty; fi; \
	if [ -n "$$ditty_bin" ]; then \
	    echo "Formatting Lisp via $$ditty_bin ..."; \
	    find $(top_srcdir)/lisp $(top_srcdir)/tests -name '*.lisp' -print0 \
	        | xargs -0 -r -n 1 "$$ditty_bin" $(top_srcdir)/lisp/lisp-fmt.lisp -- -i; \
	else \
	    echo "ditty not found - skipping Lisp formatting (build first)." >&2; \
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
