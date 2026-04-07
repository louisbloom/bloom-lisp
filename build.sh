#!/bin/bash
# Build Script for bloom-lisp interpreter

set -e # Exit on any error
set -u # Treat unset variables as errors

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Configuration
PROJECT_NAME="bloom-lisp"
PROJECT_ROOT="$(pwd)"
BUILD_DIR="build"
INSTALL_PREFIX="$HOME/.local"
ENABLE_DEBUG=true
PARALLEL_JOBS=$(nproc)

# Logging functions
log_info() {
	echo -e "${GREEN}[INFO]${NC} $1"
}

log_warn() {
	echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
	echo -e "${RED}[ERROR]${NC} $1"
}

# Compute version from git tags
get_version() {
	if git describe --tags --match 'v*' HEAD >/dev/null 2>&1; then
		# v0.1 → "0.1", v0.1-5-gabc1234 → "0.1.5-abc1234"
		git describe --tags --match 'v*' HEAD | sed 's/^v//;s/-\([0-9]*\)-g/.\1-/'
	else
		# No tags: 0.0.N-HASH
		local count
		count=$(git rev-list --count HEAD)
		local hash
		hash=$(git rev-parse --short HEAD)
		echo "0.0.${count}-${hash}"
	fi
}

# Setup local dependencies (user-installed packages in ~/.local)
setup_local_deps() {
	local local_pkgconfig="$HOME/.local/lib/pkgconfig"
	local local_lib="$HOME/.local/lib"

	if [ -d "$local_pkgconfig" ]; then
		if [ -f "$local_pkgconfig/bloom-boba.pc" ]; then
			log_info "Found bloom-boba in $local_pkgconfig"
			export PKG_CONFIG_PATH="${local_pkgconfig}${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
			export LD_LIBRARY_PATH="${local_lib}${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
		fi
	fi
}

# Check if running on Fedora 41+
check_os() {
	log_info "Checking operating system..."
	if [ -f /etc/fedora-release ]; then
		FEDORA_VERSION=$(grep -oP 'Fedora release \K\d+' /etc/fedora-release)
		if [ "$FEDORA_VERSION" -ge 41 ]; then
			log_info "Running on Fedora $FEDORA_VERSION"
		else
			log_warn "Running on Fedora $FEDORA_VERSION (expected Fedora 41+)"
		fi
	else
		log_warn "Not running on Fedora. Some dependencies may differ."
	fi
}

# Generate configure script
generate_configure() {
	log_info "Generating configure script..."

	# Write version file for configure.ac
	get_version >version
	log_info "Version: $(cat version)"

	# Clean up any existing generated files
	log_info "Cleaning up generated files..."
	rm -f configure aclocal.m4
	rm -rf autom4te.cache

	if [ -f configure ]; then
		log_info "Removing existing configure script"
		rm -f configure
	fi

	autoreconf -fvi

	if [ $? -ne 0 ]; then
		log_error "Failed to generate configure script"
		return 1
	fi

	log_info "Configure script generated successfully"
	return 0
}

# Configure the build
configure_build() {
	log_info "Configuring build..."

	# Create build directory if it doesn't exist
	if [ -d "$BUILD_DIR" ]; then
		log_info "Removing existing build directory"
		rm -rf "$BUILD_DIR"
	fi

	mkdir -p "$BUILD_DIR"
	cd "$BUILD_DIR"

	# Build configure command
	CONFIGURE_CMD="../configure --prefix=$INSTALL_PREFIX"

	if [ "$ENABLE_DEBUG" = true ]; then
		CONFIGURE_CMD="$CONFIGURE_CMD CFLAGS='-O0 -g3 -DDEBUG'"
	fi

	log_info "Running: $CONFIGURE_CMD"
	eval $CONFIGURE_CMD

	if [ $? -ne 0 ]; then
		log_error "Configure failed"
		if [ -f "config.log" ]; then
			log_error "Last 50 lines of config.log:"
			tail -50 config.log
		fi
		cd ..
		return 1
	fi

	cd ..
	log_info "Build configured successfully"
	return 0
}

# Generate docstrings header from markdown files
generate_docstrings() {
	log_info "Generating docstrings from markdown..."
	scripts/gen-docstrings.sh doc >src/docstrings.gen.h
	if [ $? -ne 0 ]; then
		log_error "Docstring generation failed"
		return 1
	fi
	log_info "Docstrings generated successfully"
	return 0
}

# Build the project
build_project() {
	local use_bear=${1:-false}

	log_info "Building project..."

	cd "$BUILD_DIR"

	if [ "$use_bear" = true ]; then
		log_info "Generating compile_commands.json with bear..."
		bear --output ../compile_commands.json -- make -j"$PARALLEL_JOBS"
	else
		log_info "Running make with $PARALLEL_JOBS parallel jobs"
		make -j"$PARALLEL_JOBS"
	fi

	if [ $? -ne 0 ]; then
		log_error "Build failed"
		cd ..
		return 1
	fi

	cd ..
	log_info "Project built successfully"
	return 0
}

# Install the project (optional)
install_project() {
	log_info "Installing project to $INSTALL_PREFIX..."

	cd "$BUILD_DIR"

	if ! make install; then
		log_error "Installation failed"
		cd ..
		return 1
	fi

	cd ..
	log_info "Project installed successfully"
	return 0
}

# Cross-compile for Windows using mingw64
build_mingw64() {
	log_info "Cross-compiling for Windows (mingw64)..."

	local MINGW_BUILD_DIR="build-mingw64"
	local MINGW_HOST="x86_64-w64-mingw32"
	local MINGW_CC="${MINGW_HOST}-gcc"
	local MINGW_PKG_CONFIG="${MINGW_HOST}-pkg-config"
	local MINGW_SYSROOT="/usr/x86_64-w64-mingw32/sys-root/mingw"
	local DEPS_DIR="deps"
	local GC_VERSION="8.2.6"
	local GC_URL="https://github.com/ivmai/bdwgc/releases/download/v${GC_VERSION}/gc-${GC_VERSION}.tar.gz"
	local GC_DIR="${DEPS_DIR}/gc-${GC_VERSION}"
	local GC_TARBALL="${DEPS_DIR}/gc-${GC_VERSION}.tar.gz"
	local AO_VERSION="7.8.2"
	local AO_URL="https://github.com/ivmai/libatomic_ops/releases/download/v${AO_VERSION}/libatomic_ops-${AO_VERSION}.tar.gz"
	local AO_TARBALL="${DEPS_DIR}/libatomic_ops-${AO_VERSION}.tar.gz"

	# Check for cross-compiler
	if ! command -v "$MINGW_CC" &>/dev/null; then
		log_error "mingw64 cross-compiler not found: $MINGW_CC"
		log_error "Install with: sudo dnf install mingw64-gcc"
		exit 1
	fi

	# Check for required mingw64 packages
	local missing_pkgs=()
	for pkg in mingw64-pcre2; do
		if ! rpm -q "$pkg" &>/dev/null; then
			missing_pkgs+=("$pkg")
		fi
	done
	if [ ${#missing_pkgs[@]} -gt 0 ]; then
		log_error "Missing mingw64 packages: ${missing_pkgs[*]}"
		log_error "Install with: sudo dnf install ${missing_pkgs[*]}"
		exit 1
	fi

	# --- Cross-compile Boehm GC ---
	mkdir -p "$DEPS_DIR"

	if [ ! -f "$GC_TARBALL" ]; then
		log_info "Downloading Boehm GC ${GC_VERSION}..."
		curl -L -o "$GC_TARBALL" "$GC_URL"
		if [ $? -ne 0 ]; then
			log_error "Failed to download Boehm GC"
			rm -f "$GC_TARBALL"
			exit 1
		fi
	fi

	if [ ! -d "$GC_DIR" ]; then
		log_info "Extracting Boehm GC..."
		tar -xzf "$GC_TARBALL" -C "$DEPS_DIR"
	fi

	# Download libatomic_ops (required by GC for Windows)
	if [ ! -f "$AO_TARBALL" ]; then
		log_info "Downloading libatomic_ops ${AO_VERSION}..."
		curl -L -o "$AO_TARBALL" "$AO_URL"
		if [ $? -ne 0 ]; then
			log_error "Failed to download libatomic_ops"
			rm -f "$AO_TARBALL"
			exit 1
		fi
	fi

	if [ ! -d "${GC_DIR}/libatomic_ops" ]; then
		log_info "Extracting libatomic_ops into GC source tree..."
		tar -xzf "$AO_TARBALL" -C "$DEPS_DIR"
		mv "${DEPS_DIR}/libatomic_ops-${AO_VERSION}" "${GC_DIR}/libatomic_ops"
	fi

	local GC_PREFIX
	GC_PREFIX="${PROJECT_ROOT}/${DEPS_DIR}/gc-mingw64-prefix"
	if [ ! -f "${GC_PREFIX}/lib/libgc.a" ]; then
		log_info "Cross-compiling Boehm GC for mingw64..."
		local GC_BUILD="${GC_DIR}/build-mingw64"
		mkdir -p "$GC_BUILD"
		cd "$GC_DIR"
		if [ ! -f configure ]; then
			autoreconf -fvi
		fi
		cd "${PROJECT_ROOT}/${GC_BUILD}"
		"${PROJECT_ROOT}/${GC_DIR}/configure" \
			--host="${MINGW_HOST}" \
			--prefix="${GC_PREFIX}" \
			--disable-shared \
			--enable-static \
			--enable-threads=win32 \
			--enable-munmap \
			CFLAGS="-O2"
		if [ $? -ne 0 ]; then
			log_error "Boehm GC configure failed"
			cd "$PROJECT_ROOT"
			exit 1
		fi
		make -j"$PARALLEL_JOBS"
		if [ $? -ne 0 ]; then
			log_error "Boehm GC build failed"
			cd "$PROJECT_ROOT"
			exit 1
		fi
		make install
		cd "$PROJECT_ROOT"

		# Generate pkg-config file
		mkdir -p "${GC_PREFIX}/lib/pkgconfig"
		cat >"${GC_PREFIX}/lib/pkgconfig/bdw-gc.pc" <<-GCPC
			prefix=${GC_PREFIX}
			libdir=\${prefix}/lib
			includedir=\${prefix}/include

			Name: Boehm-Demers-Weiser Conservative Garbage Collector
			Description: A garbage collector for C and C++
			Version: ${GC_VERSION}
			Libs: -L\${libdir} -lgc
			Cflags: -I\${includedir}
		GCPC
		log_info "Boehm GC cross-compiled: ${GC_PREFIX}/lib/libgc.a"
	else
		log_info "Using cached Boehm GC: ${GC_PREFIX}/lib/libgc.a"
	fi

	# --- Generate configure script ---
	cd "$PROJECT_ROOT"
	if ! generate_configure; then
		exit 1
	fi

	# --- Generate docstrings (host tool, text output) ---
	if ! generate_docstrings; then
		exit 1
	fi

	# --- Configure for cross-compilation ---
	if [ -d "$MINGW_BUILD_DIR" ]; then
		rm -rf "$MINGW_BUILD_DIR"
	fi
	mkdir -p "$MINGW_BUILD_DIR"
	cd "$MINGW_BUILD_DIR"

	local CONFIGURE_CMD="../configure"
	CONFIGURE_CMD="$CONFIGURE_CMD --host=${MINGW_HOST}"
	CONFIGURE_CMD="$CONFIGURE_CMD --prefix=${MINGW_SYSROOT}"
	CONFIGURE_CMD="$CONFIGURE_CMD PKG_CONFIG=${MINGW_PKG_CONFIG}"
	# Pass GC flags directly to avoid sysroot path mangling by mingw pkg-config
	CONFIGURE_CMD="$CONFIGURE_CMD GC_CFLAGS=-I${GC_PREFIX}/include"
	CONFIGURE_CMD="$CONFIGURE_CMD GC_LIBS=\"-L${GC_PREFIX}/lib -lgc\""

	if [ "$ENABLE_DEBUG" = true ]; then
		CONFIGURE_CMD="$CONFIGURE_CMD CFLAGS='-O0 -g3 -DDEBUG'"
	fi

	log_info "Running: $CONFIGURE_CMD"
	eval "$CONFIGURE_CMD"

	if [ $? -ne 0 ]; then
		log_error "Cross-compilation configure failed"
		if [ -f "config.log" ]; then
			log_error "Last 50 lines of config.log:"
			tail -50 config.log
		fi
		cd "$PROJECT_ROOT"
		exit 1
	fi

	cd "$PROJECT_ROOT"

	# --- Build ---
	cd "$MINGW_BUILD_DIR"
	log_info "Building with $PARALLEL_JOBS parallel jobs..."
	make -j"$PARALLEL_JOBS"

	if [ $? -ne 0 ]; then
		log_error "Cross-compilation build failed"
		cd "$PROJECT_ROOT"
		exit 1
	fi
	cd "$PROJECT_ROOT"

	# --- Collect DLLs alongside the binary ---
	log_info "Collecting runtime DLLs..."
	local DLL_DIR="${MINGW_SYSROOT}/bin"
	local EXE_DIR="${MINGW_BUILD_DIR}/repl"
	local DLLS=(
		libpcre2-8-0.dll
		libgcc_s_seh-1.dll
		libwinpthread-1.dll
	)

	for dll in "${DLLS[@]}"; do
		if [ -f "${DLL_DIR}/${dll}" ]; then
			cp "${DLL_DIR}/${dll}" "${EXE_DIR}/"
		else
			log_warn "DLL not found: ${DLL_DIR}/${dll}"
		fi
	done

	log_info "Cross-compilation complete!"
	log_info "Binary: ${EXE_DIR}/bloom-repl.exe"
}

# Format source files
format_sources() {
	log_info "Formatting source files..."

	# Format C files with clang-format
	if command -v clang-format &>/dev/null; then
		log_info "Formatting C files with clang-format..."
		find src/ include/ repl/ -name "*.c" -o -name "*.h" | xargs clang-format -i
	else
		log_warn "clang-format not found. Skipping C formatting."
	fi

	# Format shell scripts with shfmt
	if command -v shfmt &>/dev/null; then
		log_info "Formatting shell scripts with shfmt..."
		shfmt -w build.sh tests/run-test.sh
	else
		log_warn "shfmt not found. Skipping shell script formatting."
	fi

	# Format markdown files with prettier
	if command -v prettier &>/dev/null; then
		log_info "Formatting markdown files with prettier..."
		find . -name "*.md" | xargs prettier --write
	else
		log_warn "prettier not found. Skipping markdown formatting."
	fi

	# Format Lisp files with lisp-fmt.lisp
	log_info "Formatting Lisp files with lisp-fmt.lisp..."
	local repl_path=""
	if [ -x "$BUILD_DIR/repl/bloom-repl" ]; then
		repl_path="$BUILD_DIR/repl/bloom-repl"
	elif [ -x "$INSTALL_PREFIX/bin/bloom-repl" ]; then
		repl_path="$INSTALL_PREFIX/bin/bloom-repl"
	elif command -v bloom-repl &>/dev/null; then
		repl_path="bloom-repl"
	fi

	if [ -n "$repl_path" ]; then
		find lisp/ tests/ -name "*.lisp" | while read -r file; do
			"$repl_path" lisp/lisp-fmt.lisp -- -i "$file"
		done
	else
		log_warn "bloom-repl not found. Skipping Lisp formatting. Build the project first."
	fi

	log_info "Formatting completed."
}

# Default build action
do_build() {
	check_os
	setup_local_deps

	if [ "${INSTALL_ONLY:-false}" = true ]; then
		log_info "Installing project only (--install flag used)"

		if ! generate_configure; then
			exit 1
		fi

		if ! configure_build; then
			exit 1
		fi

		if ! generate_docstrings; then
			exit 1
		fi

		log_info "Installing project to $INSTALL_PREFIX"
		if ! install_project; then
			log_error "Installation failed"
			exit 1
		fi
	else
		if ! generate_configure; then
			exit 1
		fi

		if ! configure_build; then
			exit 1
		fi

		if ! generate_docstrings; then
			exit 1
		fi

		if [ "${USE_BEAR:-false}" = true ]; then
			if ! build_project true; then
				exit 1
			fi
		else
			if ! build_project; then
				exit 1
			fi
		fi

		# Install by default after building
		if ! install_project; then
			exit 1
		fi
	fi

	log_info "Build process completed successfully!"
	log_info "Build directory: $BUILD_DIR"
	log_info "To run the REPL: ./$BUILD_DIR/repl/bloom-repl"
}

# Main execution
main() {
	local ACTION="build"

	# Parse all arguments first, then dispatch
	while [[ $# -gt 0 ]]; do
		case $1 in
		--install)
			INSTALL_ONLY=true
			shift
			;;
		--bear)
			USE_BEAR=true
			shift
			;;
		--no-debug)
			ENABLE_DEBUG=false
			shift
			;;
		--prefix=*)
			INSTALL_PREFIX="${1#*=}"
			shift
			;;
		--mingw64)
			ACTION=mingw64
			shift
			;;
		--format)
			ACTION=format
			shift
			;;
		--help)
			echo "Usage: $0 [options]"
			echo ""
			echo "Default: configure, build, and install to $HOME/.local"
			echo ""
			echo "Options:"
			echo "  --install         Only configure and install (skip build)"
			echo "  --bear            Generate compile_commands.json using bear"
			echo "  --no-debug        Disable debug build"
			echo "  --mingw64         Cross-compile for Windows using mingw64"
			echo "  --format          Format source files (C, shell, markdown, Lisp)"
			echo "  --prefix=PATH     Set installation prefix (default: $HOME/.local)"
			echo "  --help            Show this help message"
			exit 0
			;;
		*)
			log_error "Unknown option: $1"
			echo "Use --help for usage information"
			exit 1
			;;
		esac
	done

	# Dispatch action
	case "$ACTION" in
	build)
		log_info "Starting build process for $PROJECT_NAME"
		do_build
		;;
	mingw64)
		log_info "Starting mingw64 cross-compilation for $PROJECT_NAME"
		build_mingw64
		;;
	format)
		format_sources
		;;
	esac
}

# Run main function with all arguments
main "$@"
