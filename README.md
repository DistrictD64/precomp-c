# precomp-c

## ⚠️ UNDER CONSTRUCTION

**This project is currently under active development and is NOT ready for production use.**

See [IMPLEMENTATION_ROADMAP.md](IMPLEMENTATION_ROADMAP.md) for the complete list of features being implemented.

---

A pure C port of precomp (originally precomp-cpp), a free precompression tool for advanced compression. This version is designed to be compiled with Tiny C Compiler (TCC) and other standard C compilers, making it highly portable across different platforms.

**Project Goal**: A stream decompressor/recompressor that preserves original compression parameters for exact file reconstruction, with super-compression capabilities for media files (images, audio, video) using better lossless codecs.

**Pipeline Usage**: `tar → precomp-c → srep → zstd` → (reverse) → `tar extract`

## Features (Planned/Under Development)

- **Pure C99 code** - No C++ dependencies, compatible with TCC
- **Cross-platform** - Works on Windows, Linux, macOS, BSD
- **Exact Reconstruction** - Bit-for-bit identical output when decompressing
- **Media Super-Compression** - Better lossless codecs for images, audio, video
- **External Codec Support** - Plugin system for proprietary game codecs (Oodle, etc.)
- **Multiple formats** - Support for ZIP, GZIP, PNG, JPEG, MP3, WebP, FLAC, ZSTD, LZMA, and more
- **Flexible build system** - Shell scripts, batch files, and Makefile included

## Supported Formats (Target)

### Compression Streams
| Format | Status | Notes |
|--------|--------|-------|
| DEFLATE | ❌ Under Construction | Detect & preserve compression level |
| GZIP | ❌ Under Construction | Full zlib integration |
| ZLIB | ❌ Under Construction | Full zlib integration |
| BZIP2 | ❌ Under Construction | bzip2 library |
| ZIP | ❌ Under Construction | Archive support |
| ZSTD | ❌ Planned | libzstd integration |
| LZMA/XZ | ❌ Planned | liblzma integration |
| LZO | ❌ Planned | liblzo integration |
| LZX | ❌ Planned | libmspack integration |
| Brotli | ❌ Planned | libbrotli integration |

### Media Formats (with Super-Compression)
| Format | Status | Super-Compression Target |
|--------|--------|-------------------------|
| PNG | ❌ Under Construction | brunsli WebP lossless |
| JPEG | ❌ Under Construction | packjpg / WebP lossless |
| GIF | ❌ Under Construction | PNG / WebP lossless |
| WebP | ❌ Planned | brunsli lossless |
| AVIF | ❌ Planned | AVIF lossless |
| HEIC | ❌ Planned | HEVC lossless |
| MP3 | ❌ Under Construction | packmp3 / FLAC |
| OGG Vorbis | ❌ Planned | FLAC |
| FLAC | ❌ Planned | FLAC optimization |
| WAV | ❌ Planned | WavPack / FLAC |

### Game Codecs (External DLL Loading)
| Codec | Status | Notes |
|-------|--------|-------|
| Oodle (Kraken, Leviathan, etc.) | ❌ Planned | Load from game files |
| Unity LZ4/LZMA | ❌ Planned | Load from game files |
| EA Sports | ❌ Planned | Load from game files |
| Rockstar RAGE | ❌ Planned | Load from game files |

## Current Status

**DO NOT USE IN PRODUCTION** - This is a work in progress.

### Completed
- [x] Project structure reorganization
- [x] LICENSE file (Apache 2.0 + third-party notices)
- [x] Implementation roadmap documentation
- [x] External codec plugin interface (placeholder)
- [x] Format handler directory structure

### In Progress
- [ ] Core infrastructure implementation
- [ ] Build system updates
- [ ] Contrib library integration
- [ ] Format handler implementations

### Not Started
- [ ] All format handlers (see IMPLEMENTATION_ROADMAP.md)
- [ ] Testing framework
- [ ] Documentation

For the complete implementation plan, see [IMPLEMENTATION_ROADMAP.md](IMPLEMENTATION_ROADMAP.md).

## Directory Structure

```
precomp-c/
├── src/                    # Source files
│   ├── main.c             # Main entry point
│   ├── precomp.c          # Core precompression logic
│   ├── precomp_io.c       # I/O handling
│   ├── precomp_lib.c      # Library interface
│   ├── precomp_utils.c    # Utility functions
│   └── formats/           # Format handlers
│       ├── base64.c
│       ├── bzip2.c
│       ├── deflate.c
│       ├── format_handlers.c
│       ├── gif.c
│       ├── gzip.c
│       ├── jpeg.c
│       ├── mp3.c
│       ├── pdf.c
│       ├── png.c
│       ├── swf.c
│       ├── zip.c
│       └── zlib.c
├── include/                # Header files
│   ├── config.h           # Configuration
│   ├── precomp.h          # Main header
│   ├── precomp_io.h       # I/O header
│   ├── precomp_lib.h      # Library header
│   ├── precomp_utils.h    # Utils header
│   └── formats/           # Format headers
├── contrib/                # External libraries (bundled)
│   ├── zlib/              # Zlib compression
│   ├── bzip2/             # BZip2 compression
│   ├── giflib/            # GIF library
│   ├── libjpeg/           # JPEG library
│   └── libpng/            # PNG library
├── build/                  # Build output (created during build)
├── build.sh               # Unix/Linux/macOS build script
├── build.bat              # Windows build script
├── Makefile               # GNU Make build file
└── README.md              # This file
```

## Building

### Prerequisites

- **Tiny C Compiler (TCC)** recommended, or any C99-compatible compiler
- **Git** (for cloning and optional TCC auto-setup)
- **curl** or **wget** (for downloading TCC if not installed)
- **make** (optional, for using the Makefile)

The build scripts can automatically download and build TCC if it's not found on your system.

### Quick Start

#### Linux/macOS/BSD

```bash
# Using the shell script
./build.sh              # Build release version
./build.sh debug        # Build with debug symbols
./build.sh clean        # Clean build artifacts

# Or using make
make                    # Build release version
make debug              # Build with debug symbols
make clean              # Clean build

# Install to /usr/local
sudo make install

# Install to custom location
make install PREFIX=/opt/precomp
```

#### Windows (CMD)

```cmd
REM Using the batch script
build.bat               # Build release version
build.bat debug         # Build with debug symbols
build.bat clean         # Clean build artifacts
```

#### Windows (PowerShell/WSL/Git Bash)

```powershell
# Use the Unix shell script
.\build.sh build
.\build.sh clean
```

### Build Options

#### Environment Variables

| Variable | Description | Default |
|----------|-------------|---------|
| `CC` | C compiler to use | `tcc` |
| `CFLAGS` | Additional compiler flags | (none) |
| `SKIP_TCC_SETUP` | Skip automatic TCC setup | `0` |
| `PREFIX` | Installation prefix | `/usr/local` |
| `DEBUG` | Enable debug build | `0` |

#### Examples

```bash
# Use GCC instead of TCC
CC=gcc ./build.sh

# Use Clang with custom flags
CC=clang CFLAGS="-O3 -march=native" ./build.sh

# Skip TCC auto-setup (if already installed)
SKIP_TCC_SETUP=1 ./build.sh

# Debug build with TCC
./build.sh debug

# Custom installation path
make install PREFIX=$HOME/.local
```

### Manual Build with TCC

If you prefer to build manually:

```bash
mkdir build
tcc -Iinclude -Icontrib/zlib -Icontrib/bzip2 \
    -Icontrib/giflib -Icontrib/libjpeg -Icontrib/libpng \
    -o build/precomp src/*.c src/formats/*.c \
    contrib/zlib/*.c contrib/bzip2/*.c contrib/giflib/*.c \
    contrib/libjpeg/*.c contrib/libpng/*.c
```

## Usage

Once built, use precomp like this:

```bash
# Compress a file
./build/precomp -c input.file output.pcp

# Decompress a file
./build/precomp -d input.pcp output.file

# Show help
./build/precomp -h
```

For detailed usage information, run `./build/precomp --help`.

## Platform-Specific Notes

### Linux

- TCC is available in most package managers: `sudo apt install tcc` (Debian/Ubuntu)
- Prebuilt binaries work on most modern distributions

### macOS

- TCC must be built from source (included in build process)
- Requires Xcode Command Line Tools: `xcode-select --install`
- Homebrew alternative: `brew install tinycc`

### Windows

- Download TCC from https://download.savannah.gnu.org/releases/tinycc/
- Add TCC to your PATH, or the build script will look for it locally
- Works on Windows 7 and later

### BSD

- FreeBSD/OpenBSD supported via generic BSD flags
- May need to install git and build tools first

## Troubleshooting

### TCC Not Found

The build scripts will attempt to automatically download and build TCC. If this fails:

1. Install TCC manually from https://download.savannah.gnu.org/releases/tinycc/
2. Add TCC to your PATH
3. Or set `SKIP_TCC_SETUP=1` and specify a compiler with `CC=`

### Build Errors

- Ensure all submodules are initialized: `git submodule update --init --recursive`
- Check that you have write permissions in the build directory
- Try a clean build: `./build.sh clean && ./build.sh`

### Missing Libraries

All required libraries are bundled in the `contrib/` directory. If you encounter missing header errors:

1. Verify the contrib directory exists and contains the libraries
2. Check that the include paths in the build script are correct
3. Try building the contrib libraries separately if needed

## Contributing

Contributions are welcome! Please ensure that:

1. All code is pure C99 (no C++ features)
2. Code is portable across Windows, Linux, macOS, and BSD
3. No new external dependencies are added without discussion
4. Changes are tested with TCC

## License

This project is a C port of precomp-cpp. See the original repository for licensing details:
https://github.com/nicolas-comerci/precomp-cpp

## Acknowledgments

- Original precomp-cpp by Nicolas Comerci
- TCC team for the Tiny C Compiler
- All contributors to zlib, bzip2, libjpeg, libpng, and giflib

## Links

- Original Project: https://github.com/nicolas-comerci/precomp-cpp
- TCC Repository: https://repo.or.cz/tinycc.git
- TCC Downloads: https://download.savannah.gnu.org/releases/tinycc/
