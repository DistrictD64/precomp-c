/*
 * Base64 Format Handler - C Header
 * Converted from C++ to C for Tiny C Compiler compatibility
 */

#ifndef PRECOMP_BASE64_HANDLER_H
#define PRECOMP_BASE64_HANDLER_H

#include "../precomp.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* Base64 precompression result */
typedef struct {
    precompression_result base;
    unsigned char* base64_header;
    size_t base64_header_size;
    int line_case;  /* 0, 1, or 2 */
    unsigned int* base64_line_len;
    size_t base64_line_len_count;
} base64_precompression_result;

/* Base64 format header data */
typedef struct {
    PrecompFormatHeaderData base;
    unsigned char* base64_stream_hdr;
    size_t base64_stream_hdr_size;
    unsigned int* base64_line_len;
    size_t base64_line_len_count;
} Base64FormatHeaderData;

/* Function declarations */
bool base64_quick_check(unsigned char* buffer, 
                        size_t buffer_size,
                        uintptr_t current_input_id, 
                        long long original_input_pos);

precompression_result* base64_attempt_precompression(Precomp* precomp_mgr, 
                                                      unsigned char* buffer, 
                                                      size_t buffer_size,
                                                      long long input_stream_pos);

PrecompFormatHeaderData* base64_read_format_header(RecursionContext* context, 
                                                    signed char precomp_hdr_flags, 
                                                    SupportedFormats precomp_hdr_format);

void base64_recompress(IStreamLike* precompressed_input, 
                       OStreamLike* recompressed_stream, 
                       PrecompFormatHeaderData* precomp_hdr_data, 
                       SupportedFormats precomp_hdr_format,
                       void* tools);

void base64_write_pre_recursion_data(RecursionContext* context, 
                                      PrecompFormatHeaderData* precomp_hdr_data);

/* Format handler creation */
PrecompFormatHandler* create_base64_handler(void);

/* Helper functions */
void destroy_base64_precompression_result(base64_precompression_result* result);
base64_precompression_result* create_base64_precompression_result(void);

#endif /* PRECOMP_BASE64_HANDLER_H */
