/*
 * PDF Format Handler - C Implementation
 * Converted from C++ to C for Tiny C Compiler compatibility
 */

#include "pdf.h"
#include "../precomp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Quick check if data is PDF */
bool pdf_quick_check(unsigned char* buffer, 
                     size_t buffer_size,
                     uintptr_t current_input_id, 
                     long long original_input_pos) {
    if (buffer_size < 5) return false;
    
    /* PDF magic: %PDF- */
    if (memcmp(buffer, "%PDF-", 5) != 0) return false;
    
    return true;
}

/* Attempt to precompress PDF data */
precompression_result* pdf_attempt_precompression(Precomp* precomp_mgr, 
                                                   unsigned char* buffer, 
                                                   size_t buffer_size,
                                                   long long input_stream_pos) {
    precompression_result* result = malloc(sizeof(precompression_result));
    if (!result) return NULL;
    
    result->success = false;
    result->compressed_data = NULL;
    result->compressed_size = 0;
    result->original_size = buffer_size;
    
    /* PDF may contain compressed streams, pass through for now */
    result->success = true;
    result->compressed_data = malloc(buffer_size);
    if (result->compressed_data) {
        memcpy(result->compressed_data, buffer, buffer_size);
        result->compressed_size = buffer_size;
    } else {
        free(result);
        return NULL;
    }
    
    return result;
}

/* Read PDF format header */
PrecompFormatHeaderData* pdf_read_format_header(RecursionContext* context, 
                                                 signed char precomp_hdr_flags, 
                                                 SupportedFormats precomp_hdr_format) {
    PdfFormatHeaderData* hdr_data = malloc(sizeof(PdfFormatHeaderData));
    if (!hdr_data) return NULL;
    
    memset(hdr_data, 0, sizeof(PdfFormatHeaderData));
    hdr_data->base.format_type = FORMAT_PDF;
    
    IStreamLike* input = context->input;
    hdr_data->original_size = input->read_long_long(input);
    hdr_data->precompressed_size = input->read_long_long(input);
    hdr_data->num_objects = input->read_int(input);
    
    return &hdr_data->base;
}

/* Recompress PDF data */
void pdf_recompress(IStreamLike* precompressed_input, 
                    OStreamLike* recompressed_stream, 
                    PrecompFormatHeaderData* precomp_hdr_data, 
                    SupportedFormats precomp_hdr_format,
                    void* tools) {
    PdfFormatHeaderData* hdr = (PdfFormatHeaderData*)precomp_hdr_data;
    
    long long size_to_read = hdr->precompressed_size;
    
    unsigned char* buffer = malloc(size_to_read);
    if (!buffer) return;
    
    precompressed_input->read_bytes(precompressed_input, buffer, size_to_read);
    recompressed_stream->write_bytes(recompressed_stream, buffer, size_to_read);
    
    free(buffer);
}

/* Write pre-recursion data */
void pdf_write_pre_recursion_data(RecursionContext* context, 
                                   PrecompFormatHeaderData* precomp_hdr_data) {
    PdfFormatHeaderData* hdr = (PdfFormatHeaderData*)precomp_hdr_data;
    OStreamLike* output = context->output;
    
    output->write_long_long(output, hdr->original_size);
    output->write_long_long(output, hdr->precompressed_size);
    output->write_int(output, hdr->num_objects);
}

/* Create PDF format handler */
PrecompFormatHandler* create_pdf_handler(void) {
    PrecompFormatHandler* handler = malloc(sizeof(PrecompFormatHandler));
    if (!handler) return NULL;
    
    memset(handler, 0, sizeof(PrecompFormatHandler));
    
    handler->format_type = FORMAT_PDF;
    handler->quick_check = pdf_quick_check;
    handler->attempt_precompression = pdf_attempt_precompression;
    handler->read_format_header = pdf_read_format_header;
    handler->recompress = pdf_recompress;
    handler->write_pre_recursion_data = pdf_write_pre_recursion_data;
    
    return handler;
}
