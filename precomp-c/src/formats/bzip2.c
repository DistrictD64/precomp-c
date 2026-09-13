/*
 * BZip2 Format Handler - C Implementation
 * Converted from C++ to C for Tiny C Compiler compatibility
 */

#include "bzip2.h"
#include "../precomp.h"
#include "../contrib/bzip2/bzlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Quick check if data is BZip2 compressed */
bool bzip2_quick_check(unsigned char* buffer, 
                       size_t buffer_size,
                       uintptr_t current_input_id, 
                       long long original_input_pos) {
    if (buffer_size < 4) return false;
    
    /* BZip2 magic: "BZ" followed by compression method digit */
    if (buffer[0] != 'B' || buffer[1] != 'Z') return false;
    if (buffer[2] < 'h' || buffer[2] > '9') return false;
    if (buffer[3] != 'h') return false;
    
    return true;
}

/* Attempt to precompress data using BZip2 */
precompression_result* bzip2_attempt_precompression(Precomp* precomp_mgr, 
                                                     unsigned char* buffer, 
                                                     size_t buffer_size,
                                                     long long input_stream_pos) {
    precompression_result* result = malloc(sizeof(precompression_result));
    if (!result) return NULL;
    
    result->success = false;
    result->compressed_data = NULL;
    result->compressed_size = 0;
    result->original_size = buffer_size;
    
    /* Allocate output buffer */
    size_t max_compressed_size = BZ2_compressBound(buffer_size);
    unsigned char* compressed_buffer = malloc(max_compressed_size);
    if (!compressed_buffer) {
        free(result);
        return NULL;
    }
    
    /* Compress with BZip2 */
    unsigned int destSize = max_compressed_size;
    int ret = BZ2_bzBuffToBuffCompress(
        (char*)compressed_buffer, &destSize,
        (char*)buffer, buffer_size,
        9,  /* blockSize100k */
        0,  /* verbosity */
        30  /* workFactor */
    );
    
    if (ret == BZ_OK && destSize < buffer_size) {
        result->success = true;
        result->compressed_data = compressed_buffer;
        result->compressed_size = destSize;
    } else {
        free(compressed_buffer);
    }
    
    return result;
}

/* Read BZip2 format header */
PrecompFormatHeaderData* bzip2_read_format_header(RecursionContext* context, 
                                                   signed char precomp_hdr_flags, 
                                                   SupportedFormats precomp_hdr_format) {
    Bzip2FormatHeaderData* hdr_data = malloc(sizeof(Bzip2FormatHeaderData));
    if (!hdr_data) return NULL;
    
    memset(hdr_data, 0, sizeof(Bzip2FormatHeaderData));
    hdr_data->base.format_type = FORMAT_BZIP2;
    
    /* Read original size and precompressed size from stream */
    IStreamLike* input = context->input;
    hdr_data->original_size = input->read_long_long(input);
    hdr_data->precompressed_size = input->read_long_long(input);
    
    return &hdr_data->base;
}

/* Recompress BZip2 data */
void bzip2_recompress(IStreamLike* precompressed_input, 
                      OStreamLike* recompressed_stream, 
                      PrecompFormatHeaderData* precomp_hdr_data, 
                      SupportedFormats precomp_hdr_format,
                      void* tools) {
    Bzip2FormatHeaderData* hdr = (Bzip2FormatHeaderData*)precomp_hdr_data;
    
    /* Skip the header data we already read */
    long long size_to_read = hdr->precompressed_size;
    
    /* Allocate buffer for recompression */
    unsigned char* buffer = malloc(size_to_read);
    if (!buffer) return;
    
    /* Read compressed data */
    precompressed_input->read_bytes(precompressed_input, buffer, size_to_read);
    
    /* Decompress BZip2 */
    unsigned int destSize = hdr->original_size;
    unsigned char* decompressed = malloc(destSize);
    if (!decompressed) {
        free(buffer);
        return;
    }
    
    int ret = BZ2_bzBuffToBuffDecompress(
        (char*)decompressed, &destSize,
        (char*)buffer, size_to_read,
        0,  /* verbosity */
        0   /* small */
    );
    
    free(buffer);
    
    if (ret == BZ_OK) {
        /* Write decompressed data */
        recompressed_stream->write_bytes(recompressed_stream, decompressed, destSize);
    }
    
    free(decompressed);
}

/* Write pre-recursion data */
void bzip2_write_pre_recursion_data(RecursionContext* context, 
                                     PrecompFormatHeaderData* precomp_hdr_data) {
    Bzip2FormatHeaderData* hdr = (Bzip2FormatHeaderData*)precomp_hdr_data;
    OStreamLike* output = context->output;
    
    /* Write original size and precompressed size */
    output->write_long_long(output, hdr->original_size);
    output->write_long_long(output, hdr->precompressed_size);
}

/* Create BZip2 format handler */
PrecompFormatHandler* create_bzip2_handler(void) {
    PrecompFormatHandler* handler = malloc(sizeof(PrecompFormatHandler));
    if (!handler) return NULL;
    
    memset(handler, 0, sizeof(PrecompFormatHandler));
    
    handler->format_type = FORMAT_BZIP2;
    handler->quick_check = bzip2_quick_check;
    handler->attempt_precompression = bzip2_attempt_precompression;
    handler->read_format_header = bzip2_read_format_header;
    handler->recompress = bzip2_recompress;
    handler->write_pre_recursion_data = bzip2_write_pre_recursion_data;
    
    return handler;
}
