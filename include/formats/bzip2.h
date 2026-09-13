/*
 * BZip2 Format Handler - C Header
 * Converted from C++ to C for Tiny C Compiler compatibility
 */

#ifndef PRECOMP_BZIP2_HANDLER_H
#define PRECOMP_BZIP2_HANDLER_H

#include "../precomp.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* BZip2 precompression result */
typedef struct {
    precompression_result base;
    uint32_t original_crc32;
    uint32_t compressed_size;
    uint8_t block_size_indicator;
    int detected_level;
    bool randomized;
} bzip2_precompression_result;

/* BZip2 format header data */
typedef struct {
    PrecompFormatHeaderData base;
    uint8_t block_size_indicator;  /* '1' through '9' */
    uint32_t block_size;           /* Actual block size in bytes */
    bool randomized;               /* Randomization flag */
    long long original_size;
    long long precompressed_size;
    uint32_t original_crc32;
    int detected_compression_level;
} Bzip2FormatHeaderData;

/* Function declarations */
bool bzip2_quick_check(struct PrecompFormatHandler* self, 
                       unsigned char* buffer, 
                       uintptr_t input_id, 
                       long long pos);

precompression_result* bzip2_attempt_precompression(struct PrecompFormatHandler* self,
                                                     void* precomp_instance,
                                                     unsigned char* buffer, 
                                                     long long pos);

PrecompFormatHeaderData* bzip2_read_format_header(struct PrecompFormatHandler* self,
                                                   RecursionContext* context, 
                                                   signed char precomp_hdr_flags, 
                                                   SupportedFormats precomp_hdr_format);

void bzip2_recompress(struct PrecompFormatHandler* self,
                      IStreamLike* precompressed_input, 
                      OStreamLike* recompressed_stream, 
                      PrecompFormatHeaderData* precomp_hdr_data, 
                      SupportedFormats precomp_hdr_format,
                      FormatHandlerTools* tools);

void bzip2_write_pre_recursion_data(struct PrecompFormatHandler* self,
                                     RecursionContext* context, 
                                     PrecompFormatHeaderData* precomp_hdr_data);

/* Format handler creation */
PrecompFormatHandler* create_bzip2_handler(void);

#endif /* PRECOMP_BZIP2_HANDLER_H */
