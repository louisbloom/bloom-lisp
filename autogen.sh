#!/bin/sh
# Bootstrap script for bloom-lisp
# Run this to generate configure and Makefile.in files

set -e

echo "Generating build system files..."
autoreconf --install --force

echo ""
echo "Now run:"
echo "  ./configure"
echo "  make"
echo "  make check"
