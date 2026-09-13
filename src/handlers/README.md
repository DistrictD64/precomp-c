# Format Handlers Directory

This directory contains format-specific handlers for PreComp-C.

## Structure

```
handlers/
├── streams/      # Compression stream handlers (DEFLATE, GZIP, ZLIB, ZSTD, etc.)
├── media/        # Media format handlers (PNG, JPEG, MP3, WebP, etc.)
└── archives/     # Archive format handlers (ZIP, TAR, 7Z, etc.)
```

## Status

**ALL HANDLERS ARE UNDER CONSTRUCTION**

Each handler needs to implement:
1. Format detection
2. Parameter extraction (compression level, quality, etc.)
3. Decompression to raw data
4. Super-compression (for media formats)
5. Parameter storage for reconstruction
6. Exact reconstruction capability

## Adding a New Handler

See `IMPLEMENTATION_ROADMAP.md` for the complete list of formats to implement.

## Placeholder Files

Placeholder files will be created for each format handler with:
- Basic structure
- Function signatures
- TODO comments
- Copyright headers

