/*
 * JPEG Format Handler - C Implementation
 * Converted from C++ to C for Tiny C Compiler compatibility
 */

#include "jpeg.h"
#include "../precomp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Quick check if data is JPEG */
bool jpeg_quick_check(unsigned char* buffer, 
                      size_t buffer_size,
                      uintptr_t current_input_id, 
                      long long original_input_pos) {
    if (buffer_size < 2) return false;
    
    /* JPEG SOI marker: FF D8 */
    if (buffer[0] != 0xFF || buffer[1] != 0xD8) return false;
    
    return true;
}

/* Attempt to precompress JPEG data */
precompression_result* jpeg_attempt_precompression(Precomp* precomp_mgr, 
                                                    unsigned char* buffer, 
                                                    size_t buffer_size,
                                                    long long input_stream_pos) {
    precompression_result* result = malloc(sizeof(precompression_result));
    if (!result) return NULL;
    
    result->success = false;
    result->compressed_data = NULL;
    result->compressed_size = 0;
    result->original_size = buffer_size;
    
    /* JPEG is already compressed, just pass through */
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

/* Read JPEG format header */
PrecompFormatHeaderData* jpeg_read_format_header(RecursionContext* context, 
                                                  signed char precomp_hdr_flags, 
                                                  SupportedFormats precomp_hdr_format) {
    JpegFormatHeaderData* hdr_data = malloc(sizeof(JpegFormatHeaderData));
    if (!hdr_data) return NULL;
    
    memset(hdr_data, 0, sizeof(JpegFormatHeaderData));
    hdr_data->base.format_type = FORMAT_JPEG;
    
    IStreamLike* input = context->input;
    hdr_data->original_size = input->read_long_long(input);
    hdr_data->precompressed_size = input->read_long_long(input);
    hdr_data->width = input->read_int(input);
    hdr_data->height = input->read_int(input);
    hdr_data->quality = input->read_int(input);
    
    return &hdr_data->base;
}

/* Recompress JPEG data */
void jpeg_recompress(IStreamLike* precompressed_input, 
                     OStreamLike* recompressed_stream, 
                     PrecompFormatHeaderData* precomp_hdr_data, 
                     SupportedFormats precomp_hdr_format,
                     void* tools) {
    JpegFormatHeaderData* hdr = (JpegFormatHeaderData*)precomp_hdr_data;
    
    long long size_to_read = hdr->precompressed_size;
    
    unsigned char* buffer = malloc(size_to_read);
    if (!buffer) return;
    
    precompressed_input->read_bytes(precompressed_input, buffer, size_to_read);
    recompressed_stream->write_bytes(recompressed_stream, buffer, size_to_read);
    
    free(buffer);
}

/* Write pre-recursion data */
void jpeg_write_pre_recursion_data(RecursionContext* context, 
                                    PrecompFormatHeaderData* precomp_hdr_data) {
    JpegFormatHeaderData* hdr = (JpegFormatHeaderData*)precomp_hdr_data;
    OStreamLike* output = context->output;
    
    output->write_long_long(output, hdr->original_size);
    output->write_long_long(output, hdr->precompressed_size);
    output->write_int(output, hdr->width);
    output->write_int(output, hdr->height);
    output->write_int(output, hdr->quality);
}

/* Create JPEG format handler */
PrecompFormatHandler* create_jpeg_handler(void) {
    PrecompFormatHandler* handler = malloc(sizeof(PrecompFormatHandler));
    if (!handler) return NULL;
    
    memset(handler, 0, sizeof(PrecompFormatHandler));
    
    handler->format_type = FORMAT_JPEG;
    handler->quick_check = jpeg_quick_check;
    handler->attempt_precompression = jpeg_attempt_precompression;
    handler->read_format_header = jpeg_read_format_header;
    handler->recompress = jpeg_recompress;
    handler->write_pre_recursion_data = jpeg_write_pre_recursion_data;
    
    return handler;
}
