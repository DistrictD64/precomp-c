/*
 * SWF Format Handler - C Header
 * Converted from C++ to C for Tiny C Compiler compatibility
 */

#ifndef PRECOMP_SWF_HANDLER_H
#define PRECOMP_SWF_HANDLER_H

#include "../precomp.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* SWF precompression result */
typedef struct {
    precompression_result base;
} swf_precompression_result;

/* SWF format header data */
typedef struct {
    PrecompFormatHeaderData base;
    long long original_size;
    long long precompressed_size;
} SwfFormatHeaderData;

/* Function declarations */
bool swf_quick_check(unsigned char* buffer, 
                     size_t buffer_size,
                     uintptr_t current_input_id, 
                     long long original_input_pos);

precompression_result* swf_attempt_precompression(Precomp* precomp_mgr, 
                                                   unsigned char* buffer, 
                                                   size_t buffer_size,
                                                   long long input_stream_pos);

PrecompFormatHeaderData* swf_read_format_header(RecursionContext* context, 
                                                 signed char precomp_hdr_flags, 
                                                 SupportedFormats precomp_hdr_format);

void swf_recompress(IStreamLike* precompressed_input, 
                    OStreamLike* recompressed_stream, 
                    PrecompFormatHeaderData* precomp_hdr_data, 
                    SupportedFormats precomp_hdr_format,
                    void* tools);

void swf_write_pre_recursion_data(RecursionContext* context, 
                                   PrecompFormatHeaderData* precomp_hdr_data);

/* Format handler creation */
PrecompFormatHandler* create_swf_handler(void);

#endif /* PRECOMP_SWF_HANDLER_H */
