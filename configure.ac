AC_PREREQ([2.69])
AC_INIT([bloom-lisp],
        m4_esyscmd_s([cat version 2>/dev/null || echo "0.0.0-unknown"]),
        [https://codeberg.org/thomasc/bloom-lisp/issues])
AC_CONFIG_SRCDIR([src/lisp.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([subdir-objects -Wall -Werror])

# Checks for programs
AC_PROG_CC
AC_PROG_RANLIB
AM_PROG_AR

# Detect cross-compilation target
AC_CANONICAL_HOST
case "$host_os" in
  mingw*|msys*) host_windows=yes ;;
  *)            host_windows=no ;;
esac
AM_CONDITIONAL([HOST_WINDOWS], [test "x$host_windows" = "xyes"])

# Check for pkg-config
PKG_PROG_PKG_CONFIG

# Check for required libraries via pkg-config
PKG_CHECK_MODULES([GC], [bdw-gc], [],
    [AC_MSG_ERROR([Boehm GC (bdw-gc) is required. Install with: apt install libgc-dev (Debian/Ubuntu), dnf install gc-devel (Fedora), or brew install bdw-gc (macOS)])])

PKG_CHECK_MODULES([PCRE2], [libpcre2-8], [],
    [AC_MSG_ERROR([PCRE2 is required. Install with: apt install libpcre2-dev (Debian/Ubuntu), dnf install pcre2-devel (Fedora), or brew install pcre2 (macOS)])])

# bloom-boba is optional (not available on Windows cross-builds)
PKG_CHECK_MODULES([BOBA], [bloom-boba],
    [have_bloom_boba=yes],
    [have_bloom_boba=no])
AM_CONDITIONAL([HAVE_BLOOM_BOBA], [test "x$have_bloom_boba" = "xyes"])
if test "x$have_bloom_boba" = "xyes"; then
    AC_DEFINE([HAVE_BLOOM_BOBA], [1], [Define if bloom-boba TUI library is available])
fi

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
