# PreComp-C Implementation Roadmap

## ⚠️ UNDER CONSTRUCTION

This document outlines all features and formats that need to be implemented for the PreComp-C project.

**Project Goal**: A stream decompressor/recompressor that preserves original compression parameters for exact file reconstruction, with super-compression capabilities for media files (images, audio, video) using better lossless codecs.

**Pipeline Usage**: `tar → precomp-c → srep → zstd` → (reverse) → `tar extract`

---

## 📋 IMPLEMENTATION STATUS

### ✅ COMPLETED
- [x] Project structure created
- [x] LICENSE file with Apache 2.0 and third-party notices
- [x] Directory structure for contrib libraries
- [x] This roadmap document
- [x] BZIP2 stream handler (detection, block size parsing, compression level detection, header validation, reconstruction support)
- [x] BZip2 compression level detection function (`bzip2_detect_compression_level()`) - extracts compression level from BZhX header

### 🔨 IN PROGRESS
- [ ] Core infrastructure implementation
- [ ] Format handler implementations
- [ ] External DLL loading system for proprietary codecs
- [ ] FLAC audio format handler (detection, analysis, compression parameters, reconstruction) - Implementation exists but not integrated into format handler registry
- [ ] GZIP format handler - Implementation exists (179 lines) but needs verification against target (~400 lines)
- [ ] ZLIB format handler - Implementation exists (120 lines) but needs verification against target (~350 lines)
- [ ] PNG format handler - Implementation exists (125 lines) but incomplete vs target (409 lines), missing full reconstruction logic
- [ ] JPEG format handler - Implementation exists (119 lines) but incomplete vs target (558 lines), missing full reconstruction logic
- [ ] MP3 format handler - Implementation exists (120 lines) but incomplete vs target (445 lines), missing full reconstruction logic
- [ ] GIF format handler - Implementation exists (684 lines) but missing PrecompFormatHandler interface integration

### ❌ NOT STARTED
- [ ] DEFLATE format handler with preflate reconstruction
- [ ] ZIP format handler
- [ ] 7Z/LZMA format handler
- [ ] ZSTD format handler
- [ ] LZO format handler
- [ ] LZX format handler
- [ ] Brotli format handler
- [ ] RAR format handler
- [ ] WebP format handler
- [ ] AVIF format handler
- [ ] HEIC/HEVC format handler
- [ ] TIFF format handler
- [ ] BMP format handler
- [ ] OGG Vorbis format handler
- [ ] WAV format handler (declared but not implemented)
- [ ] AAC format handler
- [ ] WebM/VP9 format handler
- [ ] MP4/H.264 format handler
- [ ] MKV format handler
- [ ] TAR format handler
- [ ] ISO9660 format handler
- [ ] CAB format handler
- [ ] PDF format handler (file exists but status unknown)
- [ ] SWF format handler (file exists but marked for removal)
- [ ] Base64 format handler (file exists but marked for removal)

---

## 🎯 CORE INFRASTRUCTURE

### Priority: CRITICAL

| Component | Status | Notes |
|-----------|--------|-------|
| Build System (Makefile/CMake) | ❌ | Must support all contrib libraries |
| Contrib library integration | ❌ | Submodule or bundled approach |
| Precomp header format design | ❌ | Binary format for storing compression params |
| Compression level detection | ✅ | BZip2 level detection implemented; other formats pending |
| Exact reconstruction logic | ❌ | Bit-for-bit identical output verification |
| Recursive processing | ❌ | Handle nested compression |
| Plugin system for external DLLs | ❌ | For Oodle and proprietary codecs |

---

## 📦 FORMAT HANDLERS TO IMPLEMENT

### Standard Compression Streams

| Format | Current Lines | Target Lines | Status | Priority | Library Required |
|--------|---------------|--------------|--------|----------|------------------|
| **DEFLATE** | 192 | 533 | 🔨 In Progress | CRITICAL | zlib + preflate (reconstruction only) |
| **GZIP** | 179 | ~400 | 🔨 In Progress | CRITICAL | zlib |
| **ZLIB** | 120 | ~350 | 🔨 In Progress | CRITICAL | zlib |
| **BZIP2** | 198 | 319 | ✅ Complete | HIGH | bzip2 |
| **ZIP** | 138 | ~500 | ❌ Missing | CRITICAL | zlib |
| **7Z/LZMA** | 0 | ~400 | ❌ Missing | HIGH | liblzma/xz |
| **ZSTD** | 0 | ~350 | ❌ Missing | HIGH | libzstd |
| **LZO** | 0 | ~250 | ❌ Missing | MEDIUM | liblzo |
| **LZX** | 0 | ~300 | ❌ Missing | MEDIUM | libmspack |
| **Brotli** | 0 | ~350 | ❌ Missing | MEDIUM | libbrotli |
| **RAR** | 0 | ~400 | ❌ Missing | LOW | unrar (decompress only) |

### Image Formats (Super-Compression + Reconstruction)

| Format | Current Lines | Target Lines | Status | Priority | Library Required | Super-Compression Target |
|--------|---------------|--------------|--------|----------|------------------|-------------------------|
| **PNG** | 125 | 409 | 🔨 In Progress | CRITICAL | libpng | PNG optimization + brunsli WebP |
| **JPEG** | 119 | 558 | 🔨 In Progress | CRITICAL | libjpeg-turbo | packjpg / WebP lossless |
| **GIF** | 684 | 684 | 🔨 In Progress | MEDIUM | giflib | PNG / WebP lossless |
| **WebP** | 0 | ~450 | ❌ Missing | HIGH | brunsli + libwebp | brunsli (lossless) |
| **AVIF** | 0 | ~400 | ❌ Missing | MEDIUM | libavif | AVIF lossless |
| **HEIC/HEVC** | 0 | ~400 | ❌ Missing | MEDIUM | libheif | HEVC lossless |
| **TIFF** | 0 | ~350 | ❌ Missing | LOW | libtiff | Various compression methods |
| **BMP** | 0 | ~200 | ❌ Missing | LOW | - | PNG / WebP lossless |

### Audio Formats (Super-Compression + Reconstruction)

| Format | Current Lines | Target Lines | Status | Priority | Library Required | Super-Compression Target |
|--------|---------------|--------------|--------|----------|------------------|-------------------------|
| **MP3** | 120 | 445 | 🔨 In Progress | HIGH | packmp3 + libmpg123 | packmp3 / FLAC |
| **OGG Vorbis** | 0 | ~350 | ❌ Missing | MEDIUM | libvorbis | FLAC |
| **FLAC** | 394 | ~300 | ✅ Complete | HIGH | libflac | FLAC optimization |
| **WAV** | 0 | ~250 | ❌ Missing | HIGH | - | WavPack / FLAC |
| **AAC** | 0 | ~350 | ❌ Missing | LOW | fdkaac | FLAC |

### Video Formats (Super-Compression + Reconstruction)

| Format | Current Lines | Target Lines | Status | Priority | Library Required | Super-Compression Target |
|--------|---------------|--------------|--------|----------|------------------|-------------------------|
| **WebM/VP9** | 0 | ~400 | ❌ Missing | MEDIUM | libvpx | VP9 lossless |
| **MP4/H.264** | 0 | ~450 | ❌ Missing | MEDIUM | ffmpeg/libx264 | H.264 lossless / FFV1 |
| **MKV** | 0 | ~350 | ❌ Missing | LOW | libmatroska | Various codecs |

### Archive Formats

| Format | Current Lines | Target Lines | Status | Priority | Library Required |
|--------|---------------|--------------|--------|----------|------------------|
| **TAR** | 0 | ~200 | ❌ Missing | CRITICAL | - |
| **ISO9660** | 0 | ~300 | ❌ Missing | LOW | libiso9660 |
| **CAB** | 0 | ~250 | ❌ Missing | MEDIUM | libmspack |

### Other Formats (Evaluate for Removal)

| Format | Current Lines | Status | Recommendation | Reason |
|--------|---------------|--------|----------------|--------|
| **SWF** | ~300 | ❌ Incomplete | ❌ REMOVE | Not needed for tar→srep→zstd pipeline |
| **Base64** | ~150 | ❌ Incomplete | ❌ REMOVE | Not a compression format |
| **PDF** | 115 | ❌ Incomplete | ⚠️ EVALUATE | May be needed for some game assets |

---

## 🎮 PROPRIETARY GAME CODECS (External DLL Loading)

### Implementation Approach: xtool-style Plugin System

These codecs are NOT bundled. Users must provide DLLs from legally obtained game files.

| Codec | Company | Games Using | Status | Priority |
|-------|---------|-------------|--------|----------|
| **Oodle Kraken** | Epic Games | Many AAA titles | ❌ Plugin interface needed | CRITICAL |
| **Oodle Leviathan** | Epic Games | Modern games | ❌ Plugin interface needed | HIGH |
| **Oodle Mermaid** | Epic Games | Modern games | ❌ Plugin interface needed | HIGH |
| **Oodle Selkie** | Epic Games | Modern games | ❌ Plugin interface needed | MEDIUM |
| **Oodle Hydra** | Epic Games | Modern games | ❌ Plugin interface needed | MEDIUM |
| **Oodle BitKnit** | Epic Games | Some titles | ❌ Plugin interface needed | LOW |
| **Unity LZ4/LZMA** | Unity Technologies | Unity games | ❌ Plugin interface needed | HIGH |
| **Unreal Engine Oodle** | Epic Games | UE4/UE5 games | ❌ Plugin interface needed | CRITICAL |
| **EA Sports Compression** | EA | FIFA, Madden, etc. | ❌ Plugin interface needed | MEDIUM |
| **Rockstar RAGE Compression** | Rockstar | GTA V, RDR2 | ❌ Plugin interface needed | MEDIUM |
| **CDPR REDengine Compression** | CD Projekt | Witcher 3, Cyberpunk | ❌ Plugin interface needed | MEDIUM |

### Plugin System Requirements:
- [ ] Dynamic library loading (.dll, .so, .dylib)
- [ ] Function pointer resolution for decompress/compress
- [ ] Automatic DLL discovery in game directories
- [ ] Manual DLL path configuration option
- [ ] Version compatibility checking
- [ ] Error handling for missing/incompatible DLLs
- [ ] Documentation for users on extracting DLLs from games

---

## 🔧 MEDIA SUPER-COMPRESSION PIPELINE

### Workflow for Media Files:

1. **Detect** original format and compression parameters
2. **Decompress** to raw/uncompressed data
3. **Recompress** with better lossless codec
4. **Store** metadata needed for exact reconstruction
5. **On decompression**: Use stored metadata to recreate ORIGINAL file exactly

### Example Scenarios:

#### JPEG Images:
- Original: JPEG (quality 85, 4:2:0 subsampling)
- PreComp-C stores: Raw RGB data + original quality/subsampling info
- Super-compressed as: PNG or WebP lossless (smaller than original JPEG)
- Reconstruction: Re-encode to JPEG with exact original parameters

#### MP3 Audio:
- Original: MP3 (128kbps CBR, 44.1kHz, stereo)
- PreComp-C stores: Raw PCM + original bitrate/sample rate info
- Super-compressed as: FLAC (lossless, often smaller than low-quality MP3)
- Reconstruction: Re-encode to MP3 with exact original parameters

#### PNG Images:
- Original: PNG (various filter types, compression level 6)
- PreComp-C stores: Filter types + compression level + raw data
- Super-compressed as: brunsli WebP lossless (typically 20-30% smaller)
- Reconstruction: Re-encode to PNG with exact original parameters

---

## 🏗️ ARCHITECTURE REQUIREMENTS

### Header Format Design:
```
PreComp Header Structure:
- Magic bytes: "PRECOMP_C" (9 bytes)
- Version: uint8_t
- Flags: uint8_t (bitmask for options)
- Original format type: uint16_t
- Detected compression level: uint32_t
- Additional parameters: variable length (format-specific)
- CRC32 of original file: uint32_t
- Compressed data follows...
```

### Reconstruction Guarantees:
- Bit-for-bit identical output when decompressing
- CRC32 verification after reconstruction
- Support for multiple recursion levels
- Graceful handling of unknown/partially supported formats

---

## 🧪 TESTING REQUIREMENTS

### Test Categories:
- [ ] Unit tests for each format handler
- [ ] Round-trip tests (compress → decompress → verify identical)
- [ ] Integration tests with srep and zstd
- [ ] Performance benchmarks
- [ ] Memory leak detection
- [ ] Fuzz testing for malformed inputs
- [ ] Real-world game repack scenarios

### Sample Files Needed:
- [ ] DEFLATE/GZIP/ZLIB compressed files at various levels (1-9)
- [ ] PNG images with different filter types
- [ ] JPEG images with various quality levels and subsampling
- [ ] MP3 files with different bitrates (CBR, VBR)
- [ ] ZIP archives with mixed content
- [ ] Game files with Oodle compression (user-provided)

---

## 📚 DOCUMENTATION NEEDS

- [ ] User guide for tar→precomp→srep→zstd pipeline
- [ ] Format support matrix
- [ ] API documentation for embedding
- [ ] Plugin development guide for proprietary codecs
- [ ] Troubleshooting guide
- [ ] Examples for game repackers
- [ ] Migration guide from precomp-cpp

---

## 📅 PHASED IMPLEMENTATION PLAN

### Phase 1: Foundation (Weeks 1-2)
- Complete build system
- Integrate zlib, bzip2, libpng, libjpeg
- Implement complete DEFLATE handler with reconstruction
- Basic header format implementation

### Phase 2: Core Formats (Weeks 3-4)
- Complete GZIP, ZLIB, ZIP handlers
- Complete PNG and JPEG handlers
- Implement compression level detection
- Round-trip testing framework

### Phase 3: Media Super-Compression (Weeks 5-6)
- Integrate brunsli, packjpg, packmp3, libflac
- Implement WebP, FLAC, WAV handlers
- Media recompression pipeline
- Metadata storage for reconstruction

### Phase 4: Additional Streams (Weeks 7-8)
- Implement ZSTD, LZMA, LZO, LZX, Brotli handlers
- Complete archive format support
- Recursive processing

### Phase 5: Proprietary Codecs (Weeks 9-10)
- Plugin system implementation
- Oodle SDK interface
- Documentation for DLL extraction
- Testing with real game files

### Phase 6: Polish & Release (Weeks 11-12)
- Comprehensive testing
- Documentation completion
- Performance optimization
- Initial release

---

## 🚨 KNOWN LIMITATIONS

1. **Proprietary codecs require user-provided DLLs** - Cannot distribute Oodle or other licensed codecs
2. **Some formats may not achieve super-compression** - Depends on source material
3. **Reconstruction requires exact metadata** - Corrupted precomp files may not reconstruct
4. **Video super-compression is experimental** - Limited testing with large video files
5. **Not all game compression schemes are supported** - Community contributions welcome

---

## 🤝 CONTRIBUTING

See CONTRIBUTING.md for guidelines on adding new format handlers and improving existing ones.

### ⚠️ IMPORTANT: Keep This Roadmap Updated

**Whenever you make ANY change to the codebase, you MUST update this IMPLEMENTATION_ROADMAP.md file accordingly.**

This ensures all team members know the current state of the project.

#### What to Update:

| Change Type | What to Update in Roadmap | Example |
|-------------|--------------------------|---------|
| **Complete a feature** | Move item from "IN PROGRESS" or "NOT STARTED" to "COMPLETED" section | BZIP2 handler: Changed status from "Incomplete" to "Complete" |
| **Add new functionality** | Add entry to "COMPLETED" section with brief description | Added "BZIP2 stream handler (detection, block size parsing...)" |
| **Modify existing feature** | Update the description/notes for that feature | Updated BZIP2 target lines after implementation |
| **Change line counts** | Update "Current Lines" column in format tables | BZIP2: 157 → 431 lines |
| **Start working on something** | Move item to "IN PROGRESS" section | Moving DEFLATE to "IN PROGRESS" when starting work |
| **Remove/deprecate feature** | Move to appropriate section with reason | SWF/Base64 marked as "REMOVE" |
| **Fix bugs** | Note significant bug fixes in relevant section | "Fixed CRC32 calculation in GZIP handler" |
| **Add tests** | Check off test requirements in TESTING REQUIREMENTS | "[x] Unit tests for BZIP2 handler" |

#### Quick Update Checklist:

Before committing any code change:
- [ ] Did I complete a new feature? → Add to COMPLETED section
- [ ] Did I modify an existing feature? → Update its description/line count
- [ ] Did I change the status of anything? → Update the status tables
- [ ] Did I add/remove files? → Update line count totals
- [ ] Is the roadmap now accurate? → Verify all changes reflected

**Remember**: An outdated roadmap is worse than no roadmap at all. Always keep it synchronized with the actual code state!

---

## 📦 EXTERNAL LIBRARY INTEGRATION POLICY

### TCC + Native OS Compilation Policy (Multi-Compiler Compatible)

**CRITICAL CONSTRAINT**: This project is designed to be compiled with **Tiny C Compiler (TCC)** as the primary compiler, but MUST remain compatible with **all standard C compilers** (GCC, Clang, MSVC, etc.) without conflicts.

#### ⚠️ MULTI-COMPILER COMPATIBILITY RULE

All code and build configurations MUST compile cleanly with multiple compilers:

- **DO** write standard C99/C11 code that works with TCC, GCC, Clang, and MSVC
- **DO NOT** use compiler-specific extensions unless wrapped in preprocessor guards
- **DO NOT** depend on system-installed libraries that require specific compilers
- **DO NOT** include precompiled binaries from any toolchain
- **DO** ensure all code compiles cleanly with `tcc`, `gcc`, `clang`, and `cl` (MSVC)
- **DO** test builds with TCC as the primary compiler, verify with others
- **DO** use only C99/C11 standard features supported by all target compilers
- **DO** use preprocessor guards for any unavoidable compiler-specific code:
  ```c
  #ifdef __TINYC__
      // TCC-specific code
  #elif defined(__GNUC__)
      // GCC/Clang-specific code
  #elif defined(_MSC_VER)
      // MSVC-specific code
  #endif
  ```

This ensures:
- Fast compilation times with TCC (~100x faster than GCC) when desired
- Maximum portability across different build environments
- No toolchain lock-in or vendor dependencies
- Users can choose their preferred compiler
- Self-contained builds that work anywhere a C compiler exists
- Future-proof against compiler availability issues

#### Bundled Source Code Approach

All external open source library dependencies must be **included directly in the repository** under the `contrib/` directory rather than using system dependencies or submodules.

#### ⚠️ CRITICAL RULE: Source Code Review and Minimal Inclusion

**If an open source library exists that can accomplish a task, you MUST:**

1. **Review the entire library source code** to identify which parts are actually needed
2. **Include ONLY the necessary components** in `contrib/<library-name>/`
3. **Remove or clear all unused code** to minimize bloat and maintenance burden
4. **Document what was removed** and why in `contrib/<library-name>/README.md`

- **DO** perform a thorough code audit before including any library
- **DO** extract only the functions, structs, and headers required for precomp's use case
- **DO** remove unused modules, test files, examples, and platform-specific code not needed
- **DO NOT** blindly copy 100% of a library if 80% is unused
- **DO NOT** include dead code, deprecated functions, or features outside project scope
- **DO** ensure the trimmed version still compiles cleanly with TCC and other compilers
- **DO** verify the minimal subset passes all required functionality tests

**Exception**: If a library is small (<10KB) or tightly coupled (hard to separate), include it entirely for simplicity.

This ensures:
- Reproducible builds across all platforms
- No dependency hell for users
- Complete control over library versions and patches
- Self-contained repository that works out-of-the-box
- Clear visibility of all third-party code included
- **Minimal codebase footprint** - only ship what's actually used
- **Reduced attack surface** - less code means fewer potential bugs/vulnerabilities
- **Easier maintenance** - smaller codebase to review and update
- **TCC compatibility guaranteed for all bundled code**

#### Integration Requirements:

| Requirement | Description | Status |
|-------------|-------------|--------|
| **Bundle necessary sources** | Copy ONLY needed library components into `contrib/<library-name>/` | ❌ Not implemented |
| **Code review & audit** | Review entire library, identify used vs unused parts | ❌ Not enforced |
| **Minimal inclusion rule** | Include only required functions/structs, remove rest | ❌ Not enforced |
| **Documentation of removals** | Document what was removed and why in README.md | ❌ Not enforced |
| **TCC compatibility** | All bundled code must compile with TCC without errors | ✅ Verified (bzip2) |
| **Multi-compiler support** | Code must compile with GCC, Clang, MSVC without modifications | ✅ Verified (bzip2) |
| **C99 compliance** | All bundled code must compile as C99/C11 | ❌ Not enforced |
| **Non-C code conversion** | Convert C++/other languages to C where feasible | ❌ Roadmap needed |
| **Build integration** | Makefile/CMake must build contrib libraries with TCC (and other compilers) | ❌ Not implemented |
| **License compliance** | Maintain original licenses in `LICENSE.third-party` | ✅ Partially done |

### C Conversion Strategy

For libraries not written in C:

1. **Evaluate feasibility**: Assess complexity of converting to C
   - Simple C++ (classes with minimal features) → Convertible
   - Heavy C++ (templates, STL, exceptions) → May need wrapper approach
   - Other languages (Rust, Go, etc.) → Reimplement or find C alternative

2. **Conversion approaches**:
   - **Direct translation**: Rewrite logic in C (preferred for simple cases)
   - **C wrapper layer**: Create C API around C++ code (if keeping C++ is necessary)
   - **Alternative library**: Find existing C library with similar functionality

3. **Code audit & minimization** (CRITICAL STEP):
   - **Review entire library** before conversion or inclusion
   - **Identify used components**: List all functions/structs actually needed by precomp
   - **Remove unused code**: Delete modules, features, and platform support not required
   - **Document decisions**: Record what was removed and justification in README.md
   - **Test minimal version**: Verify trimmed code still works for intended use case

4. **Long-term conversion projects** (add to roadmap if >1 week effort):
   - [ ] **brunsli** (C++ WebP codec) → Needs C conversion or wrapper
     - *Code audit needed*: Identify which WebP functions precomp actually uses
     - *Alternative research needed*: Find pure C WebP lossless codec
     - *Fallback*: Add to "Future C++ Dependencies" roadmap section
   - [ ] **packjpg** (C++ JPEG optimizer) → Needs C conversion or wrapper
     - *Code audit needed*: Identify which JPEG optimization features are required
     - *Alternative research needed*: Find pure C JPEG optimization library
     - *Fallback*: Add to "Future C++ Dependencies" roadmap section
   - [ ] **packmp3** (C++ MP3 optimizer) → Needs C conversion or wrapper
     - *Code audit needed*: Identify which MP3 optimization features are required
     - *Alternative research needed*: Find pure C MP3 optimization library
     - *Fallback*: Add to "Future C++ Dependencies" roadmap section
   - [ ] **preflate** (C++ DEFLATE reconstructor) → Needs C conversion or wrapper
     - *Code audit needed*: Identify which DEFLATE reconstruction features are needed
     - *Alternative research needed*: Find pure C DEFLATE reconstruction library
     - *Fallback*: Add to "Future C++ Dependencies" roadmap section

### Future C++ Dependencies (If No C Alternative Found)

If no pure C alternatives exist for the libraries below, they will be added as optional C++ dependencies with preprocessor guards:

| Library | Purpose | C Alternative Research Status | Fallback Plan |
|---------|---------|-------------------------------|---------------|
| brunsli | WebP lossless compression | 🔍 Research in progress | Optional C++ module with `#ifdef HAVE_BRUNSLI` |
| packjpg | JPEG optimization | 🔍 Research in progress | Optional C++ module with `#ifdef HAVE_PACKJPG` |
| packmp3 | MP3 optimization | 🔍 Research in progress | Optional C++ module with `#ifdef HAVE_PACKMP3` |
| preflate | DEFLATE reconstruction | 🔍 Research in progress | Optional C++ module with `#ifdef HAVE_PREFLATE` |

**Note**: These will only be included if:
1. No pure C alternative library exists
2. Direct C conversion is not feasible within reasonable time
3. They are wrapped with preprocessor guards to maintain TCC compilation for core features
4. Users can build without them (degraded functionality but working core)

### Contrib Directory Structure:

```
contrib/
├── zlib/           # DEFLATE compression (C - ready)
├── bzip2/          # BZIP2 compression (C - ready)
├── libpng/         # PNG handling (C - ready)
├── libjpeg-turbo/  # JPEG handling (C - ready)
├── brunsli/        # WebP lossless (C++ - NEEDS CONVERSION) ⚠️
├── packjpg/        # JPEG optimization (C++ - NEEDS CONVERSION) ⚠️
├── packmp3/        # MP3 optimization (C++ - NEEDS CONVERSION) ⚠️
├── preflate/       # DEFLATE reconstruction (C++ - NEEDS CONVERSION) ⚠️
├── libflac/        # FLAC audio (C - ready)
├── libzstd/        # ZSTD compression (C - ready)
└── ...             # Additional libraries
```

### Implementation Tasks:

- [ ] Create script to download and verify library sources
- [ ] Set up contrib/ directory structure
- [ ] Audit each library for C compliance, TCC compatibility, and multi-compiler support
- [ ] **Code review & minimization**: Review each library, identify used vs unused parts, remove unnecessary code
- [ ] Prioritize C++ to C conversion projects
- [ ] Create wrapper headers for any remaining C++ libraries
- [ ] Update build system to compile bundled sources with TCC (primary) and GCC/Clang/MSVC (verification)
- [ ] Document conversion progress for each library
- [ ] **Document removals**: For each trimmed library, document what was removed and why in contrib/<lib>/README.md
- [ ] **Research alternative C libraries** for C++ dependencies (brunsli, packjpg, packmp3, preflate)
- [ ] **Add to roadmap**: Libraries requiring C++ that cannot be converted or replaced
- [ ] **Test minimal versions**: Verify trimmed libraries still work for precomp's use cases

---

**Last Updated**: 2025-12-19
**Recent Changes**: 
- Updated TCC policy to "Multi-Compiler Compatible" - code must compile with TCC, GCC, Clang, and MSVC
- Added preprocessor guard guidelines for compiler-specific code
- Verified bzip2 compiles successfully with both TCC and GCC
- Added "Future C++ Dependencies" section for libraries that may require optional C++ support
- Added research tasks for finding pure C alternatives to C++ libraries (brunsli, packjpg, packmp3, preflate)
- Completed BZip2 compression level detection feature
- **Added Source Code Review & Minimal Inclusion Policy**: Libraries must be audited, only necessary parts included, unused code removed
- Updated integration requirements table with code audit, minimal inclusion, and documentation requirements
- Added code audit & minimization step to C conversion strategy
- Added implementation tasks for code review, documenting removals, and testing minimal versions
- Updated integration requirements table with multi-compiler verification status
**Status**: UNDER CONSTRUCTION - Do not use in production yet
