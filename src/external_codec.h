/*
 * PreComp-C - External Codec Plugin Interface
 * 
 * This header defines the interface for loading proprietary compression codecs
 * (like Oodle) dynamically from game files. These codecs are NOT distributed
 * with this software and must be provided by the user.
 *
 * Copyright (c) 2025 PreComp-C Contributors
 * Licensed under Apache License 2.0
 */

#ifndef PRECOMP_EXTERNAL_CODEC_H
#define PRECOMP_EXTERNAL_CODEC_H

#include <stdint.h>
#include <stddef.h>

#ifdef _WIN32
    #define EXTERNAL_CODEC_EXPORT __declspec(dllexport)
    #define EXTERNAL_CODEC_IMPORT __declspec(dllimport)
#else
    #define EXTERNAL_CODEC_EXPORT __attribute__((visibility("default")))
    #define EXTERNAL_CODEC_IMPORT
#endif

/* Error codes */
typedef enum {
    EXTCODEC_OK = 0,
    EXTCODEC_ERROR_INVALID_PARAM = -1,
    EXTCODEC_ERROR_DECOMPRESSION = -2,
    EXTCODEC_ERROR_COMPRESSION = -3,
    EXTCODEC_ERROR_NOT_SUPPORTED = -4,
    EXTCODEC_ERROR_LIBRARY_NOT_FOUND = -5,
    EXTCODEC_ERROR_SYMBOL_NOT_FOUND = -6,
    EXTCODEC_ERROR_VERSION_MISMATCH = -7
} ExtCodecError;

/* Codec types */
typedef enum {
    CODEC_TYPE_OODLE_KRAKEN = 0,
    CODEC_TYPE_OODLE_LEVIATHAN = 1,
    CODEC_TYPE_OODLE_MERMAID = 2,
    CODEC_TYPE_OODLE_SELKIE = 3,
    CODEC_TYPE_OODLE_HYDRA = 4,
    CODEC_TYPE_OODLE_BITKNIT = 5,
    CODEC_TYPE_UNITY_LZ4 = 10,
    CODEC_TYPE_UNITY_LZMA = 11,
    CODEC_TYPE_EA_SPORTS = 20,
    CODEC_TYPE_ROCKSTAR_RAGE = 21,
    CODEC_TYPE_CDPR_REDENGINE = 22,
    CODEC_TYPE_UNKNOWN = 255
} ExtCodecType;

/* Codec information structure */
typedef struct {
    const char* name;
    const char* version;
    const char* vendor;
    ExtCodecType type;
    int supports_decompression;
    int supports_compression;
    int supports_reconstruction;
} ExtCodecInfo;

/* Decompression context */
typedef struct ExtCodecContext ExtCodecContext;

/* Function pointer types for codec operations */
typedef int (*extcodec_init_fn)(ExtCodecContext** ctx, ExtCodecType type);
typedef int (*extcodec_shutdown_fn)(ExtCodecContext* ctx);
typedef int (*extcodec_decompress_fn)(ExtCodecContext* ctx, 
                                       const uint8_t* input, size_t input_size,
                                       uint8_t* output, size_t* output_size);
typedef int (*extcodec_compress_fn)(ExtCodecContext* ctx,
                                     const uint8_t* input, size_t input_size,
                                     uint8_t* output, size_t* output_size,
                                     int compression_level);
typedef int (*extcodec_get_info_fn)(ExtCodecContext* ctx, ExtCodecInfo* info);
typedef int (*extcodec_set_param_fn)(ExtCodecContext* ctx, const char* key, const char* value);

/* Context structure */
struct ExtCodecContext {
    ExtCodecType type;
    void* library_handle;
    void* internal_state;
    
    /* Function pointers loaded from external library */
    extcodec_decompress_fn decompress;
    extcodec_compress_fn compress;
    extcodec_get_info_fn get_info;
    extcodec_set_param_fn set_param;
    
    /* Metadata for reconstruction */
    uint8_t* saved_params;
    size_t params_size;
};

/* Main API functions */
EXTERNAL_CODEC_EXPORT int extcodec_load_library(const char* path, ExtCodecType type);
EXTERNAL_CODEC_EXPORT int extcodec_create_context(ExtCodecContext** ctx, ExtCodecType type);
EXTERNAL_CODEC_EXPORT int extcodec_destroy_context(ExtCodecContext* ctx);
EXTERNAL_CODEC_EXPORT int extcodec_decompress(ExtCodecContext* ctx,
                                               const uint8_t* input, size_t input_size,
                                               uint8_t* output, size_t* output_size);
EXTERNAL_CODEC_EXPORT int extcodec_compress(ExtCodecContext* ctx,
                                             const uint8_t* input, size_t input_size,
                                             uint8_t* output, size_t* output_size,
                                             int compression_level);
EXTERNAL_CODEC_EXPORT int extcodec_save_params_for_reconstruction(ExtCodecContext* ctx,
                                                                   const uint8_t* params,
                                                                   size_t params_size);
EXTERNAL_CODEC_EXPORT int extcodec_load_params_for_reconstruction(ExtCodecContext* ctx,
                                                                   const uint8_t* params,
                                                                   size_t params_size);
EXTERNAL_CODEC_EXPORT const char* extcodec_get_error_string(int error_code);

/* Helper function to find codec DLLs in a directory */
EXTERNAL_CODEC_EXPORT int extcodec_scan_directory(const char* dir_path, ExtCodecType* found_codecs, int* count);

#endif /* PRECOMP_EXTERNAL_CODEC_H */
