/*
 * PNG Format Handler - C Implementation
 * Converted from C++ to C for Tiny C Compiler compatibility
 */

#include "png.h"
#include "../precomp.h"
#include "../contrib/libpng/png.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Quick check if data is PNG */
bool png_quick_check(unsigned char* buffer, 
                     size_t buffer_size,
                     uintptr_t current_input_id, 
                     long long original_input_pos) {
    if (buffer_size < 8) return false;
    
    /* PNG signature: 89 50 4E 47 0D 0A 1A 0A */
    const unsigned char png_sig[8] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
    
    return memcmp(buffer, png_sig, 8) == 0;
}

/* Attempt to precompress PNG data */
precompression_result* png_attempt_precompression(Precomp* precomp_mgr, 
                                                   unsigned char* buffer, 
                                                   size_t buffer_size,
                                                   long long input_stream_pos) {
    precompression_result* result = malloc(sizeof(precompression_result));
    if (!result) return NULL;
    
    result->success = false;
    result->compressed_data = NULL;
    result->compressed_size = 0;
    result->original_size = buffer_size;
    
    /* For PNG, we just pass through since it's already compressed */
    /* The actual optimization happens at the IDAT chunk level */
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

/* Read PNG format header */
PrecompFormatHeaderData* png_read_format_header(RecursionContext* context, 
                                                 signed char precomp_hdr_flags, 
                                                 SupportedFormats precomp_hdr_format) {
    PngFormatHeaderData* hdr_data = malloc(sizeof(PngFormatHeaderData));
    if (!hdr_data) return NULL;
    
    memset(hdr_data, 0, sizeof(PngFormatHeaderData));
    hdr_data->base.format_type = FORMAT_PNG;
    
    IStreamLike* input = context->input;
    hdr_data->original_size = input->read_long_long(input);
    hdr_data->precompressed_size = input->read_long_long(input);
    hdr_data->width = input->read_int(input);
    hdr_data->height = input->read_int(input);
    hdr_data->bit_depth = input->read_byte(input);
    hdr_data->color_type = input->read_byte(input);
    
    return &hdr_data->base;
}

/* Recompress PNG data */
void png_recompress(IStreamLike* precompressed_input, 
                    OStreamLike* recompressed_stream, 
                    PrecompFormatHeaderData* precomp_hdr_data, 
                    SupportedFormats precomp_hdr_format,
                    void* tools) {
    PngFormatHeaderData* hdr = (PngFormatHeaderData*)precomp_hdr_data;
    
    long long size_to_read = hdr->precompressed_size;
    
    unsigned char* buffer = malloc(size_to_read);
    if (!buffer) return;
    
    precompressed_input->read_bytes(precompressed_input, buffer, size_to_read);
    
    /* Write back the PNG data */
    recompressed_stream->write_bytes(recompressed_stream, buffer, size_to_read);
    
    free(buffer);
}

/* Write pre-recursion data */
void png_write_pre_recursion_data(RecursionContext* context, 
                                   PrecompFormatHeaderData* precomp_hdr_data) {
    PngFormatHeaderData* hdr = (PngFormatHeaderData*)precomp_hdr_data;
    OStreamLike* output = context->output;
    
    output->write_long_long(output, hdr->original_size);
    output->write_long_long(output, hdr->precompressed_size);
    output->write_int(output, hdr->width);
    output->write_int(output, hdr->height);
    output->write_byte(output, hdr->bit_depth);
    output->write_byte(output, hdr->color_type);
}

/* Create PNG format handler */
PrecompFormatHandler* create_png_handler(void) {
    PrecompFormatHandler* handler = malloc(sizeof(PrecompFormatHandler));
    if (!handler) return NULL;
    
    memset(handler, 0, sizeof(PrecompFormatHandler));
    
    handler->format_type = FORMAT_PNG;
    handler->quick_check = png_quick_check;
    handler->attempt_precompression = png_attempt_precompression;
    handler->read_format_header = png_read_format_header;
    handler->recompress = png_recompress;
    handler->write_pre_recursion_data = png_write_pre_recursion_data;
    
    return handler;
}
