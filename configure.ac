AC_PREREQ([2.69])
AC_INIT([bloom-lisp], [0.1.0], [https://github.com/thomasc/bloom-lisp/issues])
AC_CONFIG_SRCDIR([src/lisp.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([foreign subdir-objects -Wall -Werror])

# Checks for programs
AC_PROG_CC
AC_PROG_RANLIB
AM_PROG_AR

# Check for pkg-config
PKG_PROG_PKG_CONFIG

# Check for required libraries via pkg-config
PKG_CHECK_MODULES([GC], [bdw-gc], [],
    [AC_MSG_ERROR([Boehm GC (bdw-gc) is required. Install with: apt install libgc-dev (Debian/Ubuntu), dnf install gc-devel (Fedora), or brew install bdw-gc (macOS)])])

PKG_CHECK_MODULES([PCRE2], [libpcre2-8], [],
    [AC_MSG_ERROR([PCRE2 is required. Install with: apt install libpcre2-dev (Debian/Ubuntu), dnf install pcre2-devel (Fedora), or brew install pcre2 (macOS)])])

PKG_CHECK_MODULES([BOBA], [bloom-boba], [],
    [AC_MSG_ERROR([bloom-boba is required. Build and install bloom-boba first.])])

# Check for math library
AC_SEARCH_LIBS([floor], [m], [],
    [AC_MSG_ERROR([Math library not found])])

# Checks for header files
AC_CHECK_HEADERS([stdlib.h string.h unistd.h termios.h sys/ioctl.h])

# Checks for typedefs, structures, and compiler characteristics
AC_TYPE_SIZE_T

# Define version macros for config.h
AC_DEFINE_UNQUOTED([BLOOM_LISP_VERSION], ["$PACKAGE_VERSION"], [Full version string])
AC_DEFINE([BLOOM_LISP_VERSION_MAJOR], [0], [Major version number])
AC_DEFINE([BLOOM_LISP_VERSION_MINOR], [0], [Minor version number])
AC_DEFINE([BLOOM_LISP_VERSION_PATCH], [1], [Patch version number])

AC_CONFIG_FILES([
    Makefile
    include/Makefile
    src/Makefile
    repl/Makefile
    tests/Makefile
    bloom-lisp.pc
])

AC_OUTPUT
