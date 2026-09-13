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

### 🔨 IN PROGRESS
- [ ] Core infrastructure implementation
- [ ] Format handler implementations
- [ ] External DLL loading system for proprietary codecs

### ❌ NOT STARTED
- All format handlers (see below)

---

## 🎯 CORE INFRASTRUCTURE

### Priority: CRITICAL

| Component | Status | Notes |
|-----------|--------|-------|
| Build System (Makefile/CMake) | ❌ | Must support all contrib libraries |
| Contrib library integration | ❌ | Submodule or bundled approach |
| Precomp header format design | ❌ | Binary format for storing compression params |
| Compression level detection | ❌ | Analyze streams to detect original settings |
| Exact reconstruction logic | ❌ | Bit-for-bit identical output verification |
| Recursive processing | ❌ | Handle nested compression |
| Plugin system for external DLLs | ❌ | For Oodle and proprietary codecs |

---

## 📦 FORMAT HANDLERS TO IMPLEMENT

### Standard Compression Streams

| Format | Current Lines | Target Lines | Status | Priority | Library Required |
|--------|---------------|--------------|--------|----------|------------------|
| **DEFLATE** | 192 | 533 | ❌ Incomplete | CRITICAL | zlib + preflate (reconstruction only) |
| **GZIP** | 693 | ~400 | ✅ Complete | CRITICAL | zlib |
| **ZLIB** | 439 | ~350 | ✅ Complete | CRITICAL | zlib |
| **BZIP2** | 157 | 319 | ❌ Incomplete | HIGH | bzip2 |
| **ZIP** | 138 | ~500 | ❌ Incomplete | CRITICAL | zlib |
| **7Z/LZMA** | 0 | ~400 | ❌ Missing | HIGH | liblzma/xz |
| **ZSTD** | 0 | ~350 | ❌ Missing | HIGH | libzstd |
| **LZO** | 0 | ~250 | ❌ Missing | MEDIUM | liblzo |
| **LZX** | 0 | ~300 | ❌ Missing | MEDIUM | libmspack |
| **Brotli** | 0 | ~350 | ❌ Missing | MEDIUM | libbrotli |
| **RAR** | 0 | ~400 | ❌ Missing | LOW | unrar (decompress only) |

### Image Formats (Super-Compression + Reconstruction)

| Format | Current Lines | Target Lines | Status | Priority | Library Required | Super-Compression Target |
|--------|---------------|--------------|--------|----------|------------------|-------------------------|
| **PNG** | 125 | 409 | ❌ Incomplete | CRITICAL | libpng | PNG optimization + brunsli WebP |
| **JPEG** | 119 | 558 | ❌ Incomplete | CRITICAL | libjpeg-turbo | packjpg / WebP lossless |
| **GIF** | 684 | 684 | ⚠️ Review needed | MEDIUM | giflib | PNG / WebP lossless |
| **WebP** | 0 | ~450 | ❌ Missing | HIGH | brunsli + libwebp | brunsli (lossless) |
| **AVIF** | 0 | ~400 | ❌ Missing | MEDIUM | libavif | AVIF lossless |
| **HEIC/HEVC** | 0 | ~400 | ❌ Missing | MEDIUM | libheif | HEVC lossless |
| **TIFF** | 0 | ~350 | ❌ Missing | LOW | libtiff | Various compression methods |
| **BMP** | 0 | ~200 | ❌ Missing | LOW | - | PNG / WebP lossless |

### Audio Formats (Super-Compression + Reconstruction)

| Format | Current Lines | Target Lines | Status | Priority | Library Required | Super-Compression Target |
|--------|---------------|--------------|--------|----------|------------------|-------------------------|
| **MP3** | 120 | 445 | ❌ Incomplete | HIGH | packmp3 + libmpg123 | packmp3 / FLAC |
| **OGG Vorbis** | 0 | ~350 | ❌ Missing | MEDIUM | libvorbis | FLAC |
| **FLAC** | 0 | ~300 | ❌ Missing | HIGH | libflac | FLAC optimization |
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

---

**Last Updated**: 2025
**Status**: UNDER CONSTRUCTION - Do not use in production yet
