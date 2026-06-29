AC_PREREQ([2.69])
AC_INIT([ditty],
        m4_esyscmd_s([cat version 2>/dev/null || echo "0.0.0-unknown"]),
        [https://codeberg.org/thomasc/ditty/issues])
AC_CONFIG_SRCDIR([src/lisp.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.14 foreign subdir-objects -Wall -Werror])
AM_SILENT_RULES([yes])

dnl Recompute the version at configure time so a rebuild after new commits
dnl picks up the current version without needing to re-run autoreconf. Falls
dnl back to PACKAGE_VERSION (set at autoreconf time via m4_esyscmd_s above)
dnl for tarball builds where build-aux/git-version.sh has nothing to read.
DITTY_VERSION=`"$srcdir/build-aux/git-version.sh" "$srcdir" 2>/dev/null`
AS_IF([test -z "$DITTY_VERSION"],
      [DITTY_VERSION="$PACKAGE_VERSION"])
AC_SUBST([DITTY_VERSION])
AC_MSG_NOTICE([ditty version: $DITTY_VERSION])

# Checks for programs
# Save CFLAGS before AC_PROG_CC so we can detect whether the user
# set them or whether AC_PROG_CC applied its default (-g -O2).
user_cflags="$CFLAGS"
AC_PROG_CC
AC_PROG_RANLIB
AM_PROG_AR

# Build mode selection (--enable-release > --enable-debug > default ASan)
AC_ARG_ENABLE([debug],
  [AS_HELP_STRING([--enable-debug],
    [unsanitized debug build (-O0 -g3, no ASan)])],
  [enable_debug=$enableval],
  [enable_debug=no])

AC_ARG_ENABLE([release],
  [AS_HELP_STRING([--enable-release],
    [optimized release build (-O3 -DNDEBUG)])],
  [enable_release=$enableval],
  [enable_release=no])

if test "x$enable_release" = "xyes"; then
  CFLAGS="-O3 -DNDEBUG"
  AC_MSG_NOTICE([Release mode: CFLAGS set to -O3 -DNDEBUG])
elif test "x$enable_debug" = "xyes"; then
  CFLAGS="-O0 -g3"
  AC_MSG_NOTICE([Debug mode: CFLAGS set to -O0 -g3])
elif test "x$user_cflags" = "x"; then
  # No user CFLAGS — default to ASan+UBSan (matches monorepo Makefile)
  CFLAGS="-O1 -g3 -fno-omit-frame-pointer -fno-common -fsanitize=address,undefined -fno-sanitize-recover=undefined"
  LDFLAGS="-fsanitize=address,undefined $LDFLAGS"
  AC_MSG_NOTICE([ASan+UBSan mode (default): CFLAGS=$CFLAGS])
fi

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

# boba is optional (not available on Windows cross-builds)
PKG_CHECK_MODULES([BOBA], [boba],
    [have_boba=yes],
    [have_boba=no])
AM_CONDITIONAL([HAVE_BOBA], [test "x$have_boba" = "xyes"])
if test "x$have_boba" = "xyes"; then
    AC_DEFINE([HAVE_BOBA], [1], [Define if boba TUI library is available])
fi

# Check for math library
AC_SEARCH_LIBS([floor], [m], [],
    [AC_MSG_ERROR([Math library not found])])

# Checks for header files
AC_CHECK_HEADERS([stdlib.h string.h unistd.h termios.h sys/ioctl.h])

# Checks for typedefs, structures, and compiler characteristics
AC_TYPE_SIZE_T

# DITTY_VERSION comes from the compile-time-generated ditty_version.h
# (see src/Makefile.am); MAJOR/MINOR/PATCH stay here as hardcoded constants.
AC_DEFINE([DITTY_VERSION_MAJOR], [0], [Major version number])
AC_DEFINE([DITTY_VERSION_MINOR], [0], [Minor version number])
AC_DEFINE([DITTY_VERSION_PATCH], [1], [Patch version number])

AC_CONFIG_FILES([
    Makefile
    include/Makefile
    src/Makefile
    flare/Makefile
    cli/Makefile
    emacs/Makefile
    tests/Makefile
    ditty.pc
])

AC_OUTPUT
