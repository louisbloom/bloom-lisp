EMACS ?= emacs

# Install to the same profile prefix as the rest of ditty.
emacslispdir = $(datadir)/emacs/site-lisp/ditty

emacslisp_DATA = ditty-mode.el

EXTRA_DIST = ditty-mode.el ditty-mode-tests.el COPYING

# --- Optional targets (not hooked into the automake build / install / check
#     because they need Emacs at build time, which is not a configure
#     dependency).  Run these by hand when developing the mode. ---------------

.PHONY: byte-compile test elisp-format

byte-compile: ditty-mode.elc

ditty-mode.elc: ditty-mode.el
	$(EMACS) --batch -f batch-byte-compile $(srcdir)/ditty-mode.el

test: ditty-mode.el ditty-mode-tests.el
	$(EMACS) --batch -l $(srcdir)/ditty-mode.el -l $(srcdir)/ditty-mode-tests.el \
	  -f ert-run-tests-batch-and-exit

elisp-format:
	$(EMACS) --batch \
	  --eval '(require (quote package))' \
	  --eval '(package-initialize)' \
	  --eval '(dolist (f (list "$(srcdir)/ditty-mode.el" "$(srcdir)/ditty-mode-tests.el")) (find-file f) (indent-region (point-min) (point-max)) (save-buffer))'

CLEANFILES = *.elc
