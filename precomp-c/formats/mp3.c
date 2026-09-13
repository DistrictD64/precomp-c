/*
 * MP3 Format Handler - C Implementation
 * Converted from C++ to C for Tiny C Compiler compatibility
 */

#include "mp3.h"
#include "../precomp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Quick check if data is MP3 */
bool mp3_quick_check(unsigned char* buffer, 
                     size_t buffer_size,
                     uintptr_t current_input_id, 
                     long long original_input_pos) {
    if (buffer_size < 2) return false;
    
    /* MP3 frame sync: FF E0-E3 */
    if (buffer[0] != 0xFF) return false;
    if ((buffer[1] & 0xE0) != 0xE0) return false;
    
    return true;
}

/* Attempt to precompress MP3 data */
precompression_result* mp3_attempt_precompression(Precomp* precomp_mgr, 
                                                   unsigned char* buffer, 
                                                   size_t buffer_size,
                                                   long long input_stream_pos) {
    precompression_result* result = malloc(sizeof(precompression_result));
    if (!result) return NULL;
    
    result->success = false;
    result->compressed_data = NULL;
    result->compressed_size = 0;
    result->original_size = buffer_size;
    
    /* MP3 is already compressed, just pass through */
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

/* Read MP3 format header */
PrecompFormatHeaderData* mp3_read_format_header(RecursionContext* context, 
                                                 signed char precomp_hdr_flags, 
                                                 SupportedFormats precomp_hdr_format) {
    Mp3FormatHeaderData* hdr_data = malloc(sizeof(Mp3FormatHeaderData));
    if (!hdr_data) return NULL;
    
    memset(hdr_data, 0, sizeof(Mp3FormatHeaderData));
    hdr_data->base.format_type = FORMAT_MP3;
    
    IStreamLike* input = context->input;
    hdr_data->original_size = input->read_long_long(input);
    hdr_data->precompressed_size = input->read_long_long(input);
    hdr_data->bitrate = input->read_int(input);
    hdr_data->sample_rate = input->read_int(input);
    hdr_data->channels = input->read_byte(input);
    
    return &hdr_data->base;
}

/* Recompress MP3 data */
void mp3_recompress(IStreamLike* precompressed_input, 
                    OStreamLike* recompressed_stream, 
                    PrecompFormatHeaderData* precomp_hdr_data, 
                    SupportedFormats precomp_hdr_format,
                    void* tools) {
    Mp3FormatHeaderData* hdr = (Mp3FormatHeaderData*)precomp_hdr_data;
    
    long long size_to_read = hdr->precompressed_size;
    
    unsigned char* buffer = malloc(size_to_read);
    if (!buffer) return;
    
    precompressed_input->read_bytes(precompressed_input, buffer, size_to_read);
    recompressed_stream->write_bytes(recompressed_stream, buffer, size_to_read);
    
    free(buffer);
}

/* Write pre-recursion data */
void mp3_write_pre_recursion_data(RecursionContext* context, 
                                   PrecompFormatHeaderData* precomp_hdr_data) {
    Mp3FormatHeaderData* hdr = (Mp3FormatHeaderData*)precomp_hdr_data;
    OStreamLike* output = context->output;
    
    output->write_long_long(output, hdr->original_size);
    output->write_long_long(output, hdr->precompressed_size);
    output->write_int(output, hdr->bitrate);
    output->write_int(output, hdr->sample_rate);
    output->write_byte(output, hdr->channels);
}

/* Create MP3 format handler */
PrecompFormatHandler* create_mp3_handler(void) {
    PrecompFormatHandler* handler = malloc(sizeof(PrecompFormatHandler));
    if (!handler) return NULL;
    
    memset(handler, 0, sizeof(PrecompFormatHandler));
    
    handler->format_type = FORMAT_MP3;
    handler->quick_check = mp3_quick_check;
    handler->attempt_precompression = mp3_attempt_precompression;
    handler->read_format_header = mp3_read_format_header;
    handler->recompress = mp3_recompress;
    handler->write_pre_recursion_data = mp3_write_pre_recursion_data;
    
    return handler;
}
