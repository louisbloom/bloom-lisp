#!/bin/bash
# make-dist-zip.sh — assemble a portable Windows ZIP of ditty.
#
# Produces a self-contained ZIP that runs without MSYS2 installed.
# Layout inside the ZIP:
#
#   ditty-<version>/
#     ditty.exe               — REPL executable
#     flare.exe               — syntax highlighter
#     ditty.cmd               — launcher (sets DITTY_LISP_PATH, then runs exe)
#     README-PORTABLE.txt     — quick-start instructions
#     share/
#       ditty/lisp/lisp-fmt.lisp
#       emacs/site-lisp/ditty-mode.el
#     *.dll                   — all runtime DLL dependencies
#
# Usage (invoked from the top-level Makefile):
#   make dist-portable
#
# Or directly:
#   scripts/make-dist-zip.sh [BUILD_DIR] [SRC_DIR]
#
# BUILD_DIR defaults to "build", SRC_DIR defaults to the repo root.
# The resulting file is ditty-<version>-windows-<arch>.zip.

set -eu

# Resolve paths
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SRC_DIR="${2:-$(cd "$SCRIPT_DIR/.." && pwd)}"
BUILD_DIR="${1:-$SRC_DIR/build}"

# Get version from the same script configure uses
VERSION="$("$SRC_DIR/build-aux/git-version.sh" "$SRC_DIR" 2>/dev/null || echo "0.0.0-unknown")"

# --- Binaries ---------------------------------------------------------------
# libtool wraps the real binary in .libs/ — use that if it exists.
REAL_DITTY="$BUILD_DIR/cli/ditty.exe"
if [ -f "$BUILD_DIR/cli/.libs/ditty.exe" ]; then
	REAL_DITTY="$BUILD_DIR/cli/.libs/ditty.exe"
fi

REAL_FLARE="$BUILD_DIR/flare/flare.exe"
if [ -f "$BUILD_DIR/flare/.libs/flare.exe" ]; then
	REAL_FLARE="$BUILD_DIR/flare/.libs/flare.exe"
fi

# Detect target architecture from the built binary
ARCH=$(file "$REAL_DITTY" 2>/dev/null | grep -oq 'ARM64\|aarch64' && echo "aarch64" || echo "x86_64")
PKG_NAME="ditty-${VERSION}-windows-${ARCH}"
STAGE_DIR="$SRC_DIR/$PKG_NAME"
ZIP_FILE="$SRC_DIR/${PKG_NAME}.zip"

echo "==> Packaging ditty $VERSION"

# Fresh staging directory
rm -rf "$STAGE_DIR"
mkdir -p "$STAGE_DIR/share/ditty/lisp" \
	"$STAGE_DIR/share/emacs/site-lisp"

echo "==> Copying ditty.exe (from $REAL_DITTY)"
cp "$REAL_DITTY" "$STAGE_DIR/ditty.exe"

echo "==> Copying flare.exe (from $REAL_FLARE)"
cp "$REAL_FLARE" "$STAGE_DIR/flare.exe"

# --- Data files -------------------------------------------------------------
echo "==> Copying data files (lisp, emacs)"
if [ -f "$SRC_DIR/lisp/lisp-fmt.lisp" ]; then
	cp "$SRC_DIR/lisp/lisp-fmt.lisp" \
		"$STAGE_DIR/share/ditty/lisp/lisp-fmt.lisp"
fi

if [ -f "$SRC_DIR/emacs/ditty-mode.el" ]; then
	cp "$SRC_DIR/emacs/ditty-mode.el" \
		"$STAGE_DIR/share/emacs/site-lisp/ditty-mode.el"
fi

# --- Runtime DLLs ----------------------------------------------------------
echo "==> Bundling runtime DLLs"
# Recursively resolve DLL dependencies via ldd, filtering out Windows system
# DLLs (those live in System32 and are always present on the user's machine).
# We use a worklist: start with the exe, run ldd on each binary, collect any
# non-system DLLs we haven't seen yet, then process those too.
SYSTEM_DLLS='ADVAPI32|KERNEL32|msvcrt|USER32|GDI32|SHELL32|ole32|OLE32|COMDLG32|SHLWAPI|WS2_32|WINSPOOL|VERSION|WINMM|IMM32|powrprof|PSAPI|DNSAPI|IPHLPAPI|WINHTTP|Secur32|SSPICLI|BCRYPT|NCRYPT|NTDLL|RPCRT4|SETUPAPI|CFGMG32|DEVOBJ|dwmapi|dwrite|D3D|DXGI|windows.storage|processthreadsapi|kernelbase|ucrtbase|msvcp|api-ms-win|win32u|combase|SHCORE|clbcat|propsys|MMDevApi|USERENV|AUTHZ|cryptbase|wldap32|wkscli|netutils|SAMLIB|secur32|SensApi|dpapi|apphelp|sechost|gdi32full|OLEAUT32|USP10|cfgmgr32|schannel|msvcp_win|gpapi|devobj|xtajit'

# Use temp files for the queue and seen-set to avoid subshell variable loss
QUEUE="$(mktemp)"
SEEN="$(mktemp)"
echo "$REAL_DITTY" >"$QUEUE"
echo "$REAL_FLARE" >>"$QUEUE"

while [ -s "$QUEUE" ]; do
	current="$(head -1 "$QUEUE")"
	# Remove first line from queue
	tail -n +2 "$QUEUE" >"$QUEUE.tmp" && mv "$QUEUE.tmp" "$QUEUE"

	[ -z "$current" ] && continue
	[ ! -f "$current" ] && continue

	# Process all DLL dependencies of the current binary
	# timeout prevents ldd from hanging on certain DLLs (ARM64)
	echo "  ldd: $(basename "$current")"
	LDD_OUT="$(timeout 10 ldd "$current" 2>&1)"
	LDD_RC=$?
	if [ "$LDD_RC" -ne 0 ]; then
		echo "  WARNING: ldd timed out or failed (rc=$LDD_RC) on: $current"
		continue
	fi
	for dll in $(echo "$LDD_OUT" |
		grep -i '\.dll' | awk '{print $3}'); do
		[ -z "$dll" ] && continue
		[ ! -f "$dll" ] && continue
		base="$(basename "$dll")"
		# Skip Windows system DLLs
		if echo "$base" | grep -iqE "^($SYSTEM_DLLS)"; then
			continue
		fi
		# Skip if already seen
		if grep -qxF "$base" "$SEEN" 2>/dev/null; then
			continue
		fi
		echo "$base" >>"$SEEN"
		cp "$dll" "$STAGE_DIR/"
		echo "  bundled: $base"
		# Enqueue for recursive resolution
		echo "$dll" >>"$QUEUE"
	done
done

rm -f "$QUEUE" "$SEEN"

# --- Launcher script -------------------------------------------------------
echo "==> Writing launcher script"
cat >"$STAGE_DIR/ditty.cmd" <<'LAUNCHER'
@echo off
:: ditty.cmd — launcher for portable ditty
:: Sets DITTY_LISP_PATH so the REPL can find lisp-fmt.lisp,
:: then starts ditty.exe. All paths are relative to this script.
setlocal
set "HERE=%~dp0"
set "DITTY_LISP_PATH=%HERE%share\ditty\lisp"
for %%I in ("%HERE%ditty.exe") do set "DITTY_EXE=%%~fI"
"%DITTY_EXE%" %*
endlocal
LAUNCHER

# --- README ----------------------------------------------------------------
echo "==> Writing README-PORTABLE.txt"
cat >"$STAGE_DIR/README-PORTABLE.txt" <<'README'
Ditty (Portable)
================

This is a self-contained build of ditty — no MSYS2 installation required.

Quick start:
  1. Extract the ZIP anywhere.
  2. Double-click ditty.cmd (or ditty.exe directly).

ditty.cmd sets up DITTY_LISP_PATH so the REPL can find lisp-fmt.lisp.
If you run ditty.exe directly, the Lisp formatter will still work if
ditty is installed system-wide.

flare.exe is a syntax-highlighting CLI (cat-like):
  flare somefile.lisp

Emacs integration:
  Add share\emacs\site-lisp to your load-path, then (require 'ditty-mode).

To uninstall:
  Delete the folder. ditty stores no data outside its own directory.
README

# --- Create ZIP ------------------------------------------------------------
echo "==> Creating ZIP: $(basename "$ZIP_FILE")"
rm -f "$ZIP_FILE"
# 7z is a native Windows binary — it needs Windows-style paths. zip (if
# present) is an MSYS2 binary and prefers POSIX paths. Convert as needed.
if command -v 7z >/dev/null 2>&1; then
	win_zip="$(cygpath -w "$ZIP_FILE" 2>/dev/null || echo "$ZIP_FILE")"
	# cd to SRC_DIR so 7z stores relative paths (just the pkg name)
	win_pkg="$(cygpath -w "$PKG_NAME" 2>/dev/null || echo "$PKG_NAME")"
	(cd "$SRC_DIR" && 7z a -tzip -mx=9 "$win_zip" "$win_pkg" >/dev/null)
elif command -v zip >/dev/null 2>&1; then
	(cd "$SRC_DIR" && zip -r -9 "$ZIP_FILE" "$PKG_NAME" >/dev/null)
else
	echo "ERROR: Neither 7z nor zip found — cannot create ZIP" >&2
	echo " staged directory left at: $STAGE_DIR" >&2
	exit 1
fi

# Clean up staging dir
rm -rf "$STAGE_DIR"

echo "==> Done: $ZIP_FILE"
echo "    Size: $(du -h "$ZIP_FILE" | cut -f1)"
