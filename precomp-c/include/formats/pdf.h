/*
 * PDF Format Handler - C Header
 * Converted from C++ to C for Tiny C Compiler compatibility
 */

#ifndef PRECOMP_PDF_HANDLER_H
#define PRECOMP_PDF_HANDLER_H

#include "../precomp.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* PDF precompression result */
typedef struct {
    precompression_result base;
} pdf_precompression_result;

/* PDF format header data */
typedef struct {
    PrecompFormatHeaderData base;
    long long original_size;
    long long precompressed_size;
} PdfFormatHeaderData;

/* Function declarations */
bool pdf_quick_check(unsigned char* buffer, 
                     size_t buffer_size,
                     uintptr_t current_input_id, 
                     long long original_input_pos);

precompression_result* pdf_attempt_precompression(Precomp* precomp_mgr, 
                                                   unsigned char* buffer, 
                                                   size_t buffer_size,
                                                   long long input_stream_pos);

PrecompFormatHeaderData* pdf_read_format_header(RecursionContext* context, 
                                                 signed char precomp_hdr_flags, 
                                                 SupportedFormats precomp_hdr_format);

void pdf_recompress(IStreamLike* precompressed_input, 
                    OStreamLike* recompressed_stream, 
                    PrecompFormatHeaderData* precomp_hdr_data, 
                    SupportedFormats precomp_hdr_format,
                    void* tools);

void pdf_write_pre_recursion_data(RecursionContext* context, 
                                   PrecompFormatHeaderData* precomp_hdr_data);

/* Format handler creation */
PrecompFormatHandler* create_pdf_handler(void);

#endif /* PRECOMP_PDF_HANDLER_H */
