EMACS ?= emacs

# Install to the same profile prefix as the rest of bloom-lisp.
emacslispdir = $(datadir)/emacs/site-lisp/bloom-lisp

emacslisp_DATA = bloom-lisp-mode.el

EXTRA_DIST = bloom-lisp-mode.el bloom-lisp-mode-tests.el

# --- Optional targets (not hooked into the automake build / install / check
#     because they need Emacs at build time, which is not a configure
#     dependency).  Run these by hand when developing the mode. ---------------

.PHONY: byte-compile test elisp-format

byte-compile: bloom-lisp-mode.elc

bloom-lisp-mode.elc: bloom-lisp-mode.el
	$(EMACS) --batch -f batch-byte-compile $(srcdir)/bloom-lisp-mode.el

test: bloom-lisp-mode.el bloom-lisp-mode-tests.el
	$(EMACS) --batch -l $(srcdir)/bloom-lisp-mode.el -l $(srcdir)/bloom-lisp-mode-tests.el \
	  -f ert-run-tests-batch-and-exit

elisp-format:
	$(EMACS) --batch \
	  --eval '(require (quote package))' \
	  --eval '(package-initialize)' \
	  --eval '(dolist (f (list "$(srcdir)/bloom-lisp-mode.el" "$(srcdir)/bloom-lisp-mode-tests.el")) (find-file f) (indent-region (point-min) (point-max)) (save-buffer))'

CLEANFILES = *.elc
