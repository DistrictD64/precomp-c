/*
 * PreComp-C - Format Handler Template Header
 * 
 * Use this template when creating new format handlers.
 * 
 * Copyright (c) 2025 PreComp-C Contributors
 * Licensed under Apache License 2.0
 */

#ifndef PRECOMP_FORMAT_HANDLER_H
#define PRECOMP_FORMAT_HANDLER_H

#include <stdint.h>
#include <stddef.h>

/* Common return codes */
#define PRECOMP_OK                  0
#define PRECOMP_ERROR_FORMAT       -1
#define PRECOMP_ERROR_DECOMPRESSION -2
#define PRECOMP_ERROR_COMPRESSION  -3
#define PRECOMP_ERROR_MEMORY       -4
#define PRECOMP_ERROR_PARAMS       -5
#define PRECOMP_ERROR_NOT_SUPPORTED -6

/* Format detection result */
typedef struct {
    int is_match;           /* 1 if format detected, 0 otherwise */
    int confidence;         /* 0-100 confidence level */
    const char* format_name;
} FormatDetectionResult;

/* Compression parameters for reconstruction */
typedef struct {
    uint32_t format_id;
    uint32_t compression_level;
    uint32_t flags;
    size_t params_size;
    void* params_data;      /* Format-specific parameters */
} CompressionParams;

/* Handler function signatures */
typedef int (*detect_fn)(const uint8_t* data, size_t size, FormatDetectionResult* result);
typedef int (*decompress_fn)(const uint8_t* input, size_t input_size,
                             uint8_t* output, size_t* output_size,
                             CompressionParams* params);
typedef int (*compress_fn)(const uint8_t* input, size_t input_size,
                           uint8_t* output, size_t* output_size,
                           const CompressionParams* params);
typedef int (*extract_params_fn)(const uint8_t* data, size_t size, CompressionParams* params);
typedef void (*free_params_fn)(CompressionParams* params);

/* Handler structure */
typedef struct {
    const char* format_name;
    const char* extension;
    const uint8_t* magic_bytes;
    size_t magic_size;
    
    detect_fn detect;
    decompress_fn decompress;
    compress_fn compress;
    extract_params_fn extract_params;
    free_params_fn free_params;
} FormatHandler;

#endif /* PRECOMP_FORMAT_HANDLER_H */
