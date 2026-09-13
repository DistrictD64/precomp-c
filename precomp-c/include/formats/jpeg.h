/*
 * JPEG Format Handler - C Header
 * Converted from C++ to C for Tiny C Compiler compatibility
 */

#ifndef PRECOMP_JPEG_HANDLER_H
#define PRECOMP_JPEG_HANDLER_H

#include "../precomp.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* JPEG precompression result */
typedef struct {
    precompression_result base;
} jpeg_precompression_result;

/* JPEG format header data */
typedef struct {
    PrecompFormatHeaderData base;
    long long original_size;
    long long precompressed_size;
} JpegFormatHeaderData;

/* Function declarations */
bool jpeg_quick_check(unsigned char* buffer, 
                      size_t buffer_size,
                      uintptr_t current_input_id, 
                      long long original_input_pos);

precompression_result* jpeg_attempt_precompression(Precomp* precomp_mgr, 
                                                    unsigned char* buffer, 
                                                    size_t buffer_size,
                                                    long long input_stream_pos);

PrecompFormatHeaderData* jpeg_read_format_header(RecursionContext* context, 
                                                  signed char precomp_hdr_flags, 
                                                  SupportedFormats precomp_hdr_format);

void jpeg_recompress(IStreamLike* precompressed_input, 
                     OStreamLike* recompressed_stream, 
                     PrecompFormatHeaderData* precomp_hdr_data, 
                     SupportedFormats precomp_hdr_format,
                     void* tools);

void jpeg_write_pre_recursion_data(RecursionContext* context, 
                                    PrecompFormatHeaderData* precomp_hdr_data);

/* Format handler creation */
PrecompFormatHandler* create_jpeg_handler(void);

#endif /* PRECOMP_JPEG_HANDLER_H */
