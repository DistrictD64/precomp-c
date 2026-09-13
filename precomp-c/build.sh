#!/bin/sh
# Cross-platform build script for precomp-c
# Works on Linux, macOS, BSD, and Windows (with sh/bash)
# Automatically detects OS and configures TCC

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
SRC_DIR="$SCRIPT_DIR/src"
INCLUDE_DIR="$SCRIPT_DIR/include"
CONTRIB_DIR="$SCRIPT_DIR/contrib"
TCC_DIR="$SCRIPT_DIR/.tcc"

# Detect OS
detect_os() {
    case "$(uname -s)" in
        Linux*)     echo "linux";;
        Darwin*)    echo "macos";;
        FreeBSD*)   echo "bsd";;
        OpenBSD*)   echo "bsd";;
        CYGWIN*)    echo "windows";;
        MINGW*)     echo "windows";;
        MSYS*)      echo "windows";;
        *)          echo "unknown";;
    esac
}

OS=$(detect_os)
ARCH=$(uname -m)

# Colors for output
if [ -t 1 ]; then
    RED='\033[0;31m'
    GREEN='\033[0;32m'
    YELLOW='\033[1;33m'
    NC='\033[0m'
else
    RED=''
    GREEN=''
    YELLOW=''
    NC=''
fi

log_info() { printf "${GREEN}[INFO]${NC} %s\n" "$1"; }
log_warn() { printf "${YELLOW}[WARN]${NC} %s\n" "$1"; }
log_error() { printf "${RED}[ERROR]${NC} %s\n" "$1"; }

# Check if TCC is available or setup TCC
setup_tcc() {
    if [ "$SKIP_TCC_SETUP" = "1" ]; then
        log_info "Skipping TCC setup"
        return 0
    fi

    # Check if tcc is already in PATH
    if command -v tcc >/dev/null 2>&1; then
        TCC_CMD="tcc"
        log_info "Found system TCC: $(tcc -v 2>&1 | head -1)"
        return 0
    fi

    # Check if local TCC exists
    if [ -f "$TCC_DIR/bin/tcc" ]; then
        TCC_CMD="$TCC_DIR/bin/tcc"
        log_info "Found local TCC: $($TCC_CMD -v 2>&1 | head -1)"
        return 0
    fi

    log_warn "TCC not found. Attempting to setup..."
    
    # Try to download prebuilt TCC
    download_tcc() {
        local tcc_url=""
        local tcc_file=""
        
        case "$OS-$ARCH" in
            linux-x86_64)
                tcc_url="https://download.savannah.gnu.org/releases/tinycc/tcc-0.9.27.tar.bz2"
                ;;
            linux-i686|i686)
                tcc_url="https://download.savannah.gnu.org/releases/tinycc/tcc-0.9.27.tar.bz2"
                ;;
            darwin-x86_64)
                log_warn "No prebuilt TCC for macOS x86_64, will build from source"
                return 1
                ;;
            darwin-arm64|arm64)
                log_warn "No prebuilt TCC for macOS ARM, will build from source"
                return 1
                ;;
            *)
                log_warn "No prebuilt TCC for $OS-$ARCH, will build from source"
                return 1
                ;;
        esac
        
        if [ -n "$tcc_url" ]; then
            log_info "Downloading TCC from $tcc_url"
            cd "$BUILD_DIR"
            if command -v curl >/dev/null 2>&1; then
                curl -L -o tcc.tar.bz2 "$tcc_url"
            elif command -v wget >/dev/null 2>&1; then
                wget -O tcc.tar.bz2 "$tcc_url"
            else
                log_error "Neither curl nor wget found. Cannot download TCC."
                return 1
            fi
            
            if [ -f tcc.tar.bz2 ]; then
                tar -xjf tcc.tar.bz2
                rm tcc.tar.bz2
                cd "$SCRIPT_DIR"
                return 0
            fi
            cd "$SCRIPT_DIR"
        fi
        return 1
    }
    
    # Build TCC from source
    build_tcc() {
        log_info "Building TCC from source..."
        
        if [ ! -d "$BUILD_DIR/tinycc-src" ]; then
            log_info "Cloning TCC repository..."
            git clone --depth 1 https://repo.or.cz/tinycc.git "$BUILD_DIR/tinycc-src" 2>/dev/null || {
                log_error "Failed to clone TCC repository"
                return 1
            }
        fi
        
        log_info "Configuring TCC..."
        cd "$BUILD_DIR/tinycc-src"
        
        # Configure with local installation
        ./configure --prefix="$TCC_DIR" --disable-static --enable-shared 2>/dev/null || {
            log_error "TCC configure failed"
            cd "$SCRIPT_DIR"
            return 1
        }
        
        log_info "Building TCC..."
        make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 2) 2>/dev/null || {
            log_error "TCC build failed"
            cd "$SCRIPT_DIR"
            return 1
        }
        
        log_info "Installing TCC to $TCC_DIR..."
        make install 2>/dev/null || {
            log_error "TCC install failed"
            cd "$SCRIPT_DIR"
            return 1
        }
        
        cd "$SCRIPT_DIR"
        TCC_CMD="$TCC_DIR/bin/tcc"
        log_info "TCC built successfully: $($TCC_CMD -v 2>&1 | head -1)"
        return 0
    }
    
    # Try download first, then build
    if ! download_tcc; then
        if ! build_tcc; then
            log_error "Failed to setup TCC. Please install TCC manually or set SKIP_TCC_SETUP=1"
            return 1
        fi
    fi
    
    return 0
}

# Collect all source files
collect_sources() {
    SOURCES=""
    
    # Core sources
    for src in "$SRC_DIR"/*.c; do
        if [ -f "$src" ]; then
            SOURCES="$SOURCES $src"
        fi
    done
    
    # Format sources
    if [ -d "$SRC_DIR/formats" ]; then
        for src in "$SRC_DIR/formats"/*.c; do
            if [ -f "$src" ]; then
                SOURCES="$SOURCES $src"
            fi
        done
    fi
    
    # Contrib libraries (only necessary C files)
    # Zlib
    if [ -d "$CONTRIB_DIR/zlib" ]; then
        for src in "$CONTRIB_DIR/zlib"/*.c; do
            if [ -f "$src" ]; then
                SOURCES="$SOURCES $src"
            fi
        done
    fi
    
    # Bzip2
    if [ -d "$CONTRIB_DIR/bzip2" ]; then
        for src in "$CONTRIB_DIR/bzip2"/*.c; do
            if [ -f "$src" ]; then
                SOURCES="$SOURCES $src"
            fi
        done
    fi
    
    # GIFLIB
    if [ -d "$CONTRIB_DIR/giflib" ]; then
        for src in "$CONTRIB_DIR/giflib"/*.c; do
            if [ -f "$src" ]; then
                SOURCES="$SOURCES $src"
            fi
        done
    fi
    
    # LibJPEG
    if [ -d "$CONTRIB_DIR/libjpeg" ]; then
        for src in "$CONTRIB_DIR/libjpeg"/*.c; do
            if [ -f "$src" ]; then
                SOURCES="$SOURCES $src"
            fi
        done
    fi
    
    # LibPNG
    if [ -d "$CONTRIB_DIR/libpng" ]; then
        for src in "$CONTRIB_DIR/libpng"/*.c; do
            if [ -f "$src" ]; then
                SOURCES="$SOURCES $src"
            fi
        done
    fi
    
    echo "$SOURCES"
}

# Build function
do_build() {
    local build_type="${1:-release}"
    
    log_info "Building precomp-c for $OS ($ARCH) - $build_type mode"
    
    # Create build directory
    mkdir -p "$BUILD_DIR"
    
    # Setup TCC if needed
    setup_tcc || exit 1
    
    # Use detected TCC or fallback to CC environment variable
    TCC_CMD="${TCC_CMD:-${CC:-tcc}}"
    
    # Compiler flags
    local cflags="-I$INCLUDE_DIR -I$INCLUDE_DIR/formats"
    cflags="$cflags -I$CONTRIB_DIR/zlib -I$CONTRIB_DIR/bzip2"
    cflags="$cflags -I$CONTRIB_DIR/giflib -I$CONTRIB_DIR/libjpeg -I$CONTRIB_DIR/libpng"
    
    if [ "$build_type" = "debug" ]; then
        cflags="$cflags -g -DDEBUG"
    else
        cflags="$cflags -O2"
    fi
    
    # Platform-specific flags
    case "$OS" in
        windows)
            cflags="$cflags -D_WIN32 -D_CRT_SECURE_NO_WARNINGS"
            ;;
        darwin)
            cflags="$cflags -D_DARWIN_C_SOURCE"
            ;;
        linux)
            cflags="$cflags -D_GNU_SOURCE"
            ;;
        bsd)
            cflags="$cflags -D_BSD_SOURCE"
            ;;
    esac
    
    # Collect sources
    SOURCES=$(collect_sources)
    
    if [ -z "$SOURCES" ]; then
        log_error "No source files found!"
        exit 1
    fi
    
    log_info "Compiling $(echo $SOURCES | wc -w | tr -d ' ') source files..."
    
    # Build executable
    local output="$BUILD_DIR/precomp"
    if [ "$OS" = "windows" ]; then
        output="$BUILD_DIR/precomp.exe"
    fi
    
    $TCC_CMD $cflags -o "$output" $SOURCES 2>&1 | tee "$BUILD_DIR/build.log" || {
        log_error "Build failed! Check $BUILD_DIR/build.log for details"
        exit 1
    }
    
    log_info "Build successful! Output: $output"
    log_info "Binary size: $(ls -lh "$output" | awk '{print $5}')"
}

# Clean function
do_clean() {
    log_info "Cleaning build artifacts..."
    rm -rf "$BUILD_DIR"
    log_info "Clean complete"
}

# Install function
do_install() {
    local prefix="${1:-/usr/local}"
    
    if [ ! -f "$BUILD_DIR/precomp" ] && [ ! -f "$BUILD_DIR/precomp.exe" ]; then
        log_error "Build not found. Run './build.sh build' first"
        exit 1
    fi
    
    log_info "Installing to $prefix..."
    
    mkdir -p "$prefix/bin"
    mkdir -p "$prefix/share/precomp"
    
    if [ -f "$BUILD_DIR/precomp.exe" ]; then
        cp "$BUILD_DIR/precomp.exe" "$prefix/bin/"
    else
        cp "$BUILD_DIR/precomp" "$prefix/bin/"
    fi
    
    # Copy example configs if any
    if [ -d "$SCRIPT_DIR/examples" ]; then
        cp -r "$SCRIPT_DIR/examples"/* "$prefix/share/precomp/" 2>/dev/null || true
    fi
    
    log_info "Installation complete"
}

# Help function
show_help() {
    cat << EOF
precomp-c Build Script
======================

Usage: $0 [command] [options]

Commands:
  build       Build precomp (default)
  debug       Build with debug symbols
  release     Build with optimizations (default)
  clean       Remove build artifacts
  install     Install to system (requires sudo)
  help        Show this help message

Options:
  --prefix=PATH   Installation prefix (default: /usr/local)
  --skip-tcc      Skip TCC auto-setup
  --cc=COMPILER   Use specific compiler (default: tcc)

Environment Variables:
  CC              C compiler to use
  CFLAGS          Additional compiler flags
  SKIP_TCC_SETUP  Set to 1 to skip TCC auto-setup
  PREFIX          Installation prefix

Examples:
  $0                    # Build release version
  $0 debug              # Build debug version
  $0 clean              # Clean build
  $0 install --prefix=/opt/precomp
  CC=gcc $0             # Use GCC instead of TCC
  SKIP_TCC_SETUP=1 $0   # Skip TCC setup

EOF
}

# Main
case "${1:-build}" in
    build)
        do_build "release"
        ;;
    debug)
        do_build "debug"
        ;;
    release)
        do_build "release"
        ;;
    clean)
        do_clean
        ;;
    install)
        shift
        PREFIX_ARG=""
        for arg in "$@"; do
            case $arg in
                --prefix=*)
                    PREFIX_ARG="${arg#*=}"
                    ;;
            esac
        done
        do_install "${PREFIX_ARG:-${PREFIX:-/usr/local}}"
        ;;
    help|--help|-h)
        show_help
        ;;
    *)
        log_error "Unknown command: $1"
        show_help
        exit 1
        ;;
esac
