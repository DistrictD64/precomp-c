/*
 * Deflate Format Handler - C Implementation
 * Converted from C++ to C for Tiny C Compiler compatibility
 */

#include "deflate.h"
#include "../precomp.h"
#include "../contrib/zlib/zlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Quick check if data is deflate compressed */
bool deflate_quick_check(unsigned char* buffer, 
                         size_t buffer_size,
                         uintptr_t current_input_id, 
                         long long original_input_pos) {
    if (buffer_size < 2) return false;
    
    /* Check for valid deflate header */
    unsigned char b0 = buffer[0];
    unsigned char b1 = buffer[1];
    
    /* CMF (Compression Method and flags) */
    int cm = b0 & 0x0F;  /* Compression method (8 = deflate) */
    int cinfo = (b0 >> 4) & 0x0F;  /* Compression info */
    
    if (cm != 8) return false;  /* Must be deflate */
    if (cinfo > 7) return false;  /* Window size check */
    
    /* FLG (Flags) check */
    int fcheck = b1 & 0x1F;
    int fdict = (b1 >> 5) & 0x01;
    int flevel = (b1 >> 6) & 0x03;
    
    /* FCHECK must make (CMF*256 + FLG) divisible by 31 */
    int check = (b0 * 256 + b1) % 31;
    if (check != 0) return false;
    
    return true;
}

/* Attempt to precompress data using deflate */
precompression_result* deflate_attempt_precompression(Precomp* precomp_mgr, 
                                                       unsigned char* buffer, 
                                                       size_t buffer_size,
                                                       long long input_stream_pos) {
    precompression_result* result = malloc(sizeof(precompression_result));
    if (!result) return NULL;
    
    result->success = false;
    result->compressed_data = NULL;
    result->compressed_size = 0;
    result->original_size = buffer_size;
    
    /* Calculate max compressed size */
    z_stream strm;
    memset(&strm, 0, sizeof(strm));
    
    /* Initialize deflate */
    if (deflateInit(&strm, Z_BEST_COMPRESSION) != Z_OK) {
        free(result);
        return NULL;
    }
    
    /* Calculate bound */
    size_t max_compressed_size = deflateBound(&strm, buffer_size);
    unsigned char* compressed_buffer = malloc(max_compressed_size);
    if (!compressed_buffer) {
        deflateEnd(&strm);
        free(result);
        return NULL;
    }
    
    /* Compress */
    strm.next_in = buffer;
    strm.avail_in = buffer_size;
    strm.next_out = compressed_buffer;
    strm.avail_out = max_compressed_size;
    
    int ret = deflate(&strm, Z_FINISH);
    if (ret != Z_STREAM_END) {
        deflateEnd(&strm);
        free(compressed_buffer);
        free(result);
        return NULL;
    }
    
    size_t compressed_size = max_compressed_size - strm.avail_out;
    
    if (compressed_size < buffer_size) {
        result->success = true;
        result->compressed_data = compressed_buffer;
        result->compressed_size = compressed_size;
    } else {
        free(compressed_buffer);
    }
    
    deflateEnd(&strm);
    return result;
}

/* Read Deflate format header */
PrecompFormatHeaderData* deflate_read_format_header(RecursionContext* context, 
                                                     signed char precomp_hdr_flags, 
                                                     SupportedFormats precomp_hdr_format) {
    DeflateFormatHeaderData* hdr_data = malloc(sizeof(DeflateFormatHeaderData));
    if (!hdr_data) return NULL;
    
    memset(hdr_data, 0, sizeof(DeflateFormatHeaderData));
    hdr_data->base.format_type = FORMAT_DEFLATE;
    
    IStreamLike* input = context->input;
    hdr_data->original_size = input->read_long_long(input);
    hdr_data->precompressed_size = input->read_long_long(input);
    
    return &hdr_data->base;
}

/* Recompress Deflate data */
void deflate_recompress(IStreamLike* precompressed_input, 
                        OStreamLike* recompressed_stream, 
                        PrecompFormatHeaderData* precomp_hdr_data, 
                        SupportedFormats precomp_hdr_format,
                        void* tools) {
    DeflateFormatHeaderData* hdr = (DeflateFormatHeaderData*)precomp_hdr_data;
    
    long long size_to_read = hdr->precompressed_size;
    
    unsigned char* buffer = malloc(size_to_read);
    if (!buffer) return;
    
    precompressed_input->read_bytes(precompressed_input, buffer, size_to_read);
    
    /* Decompress with zlib inflate */
    z_stream strm;
    memset(&strm, 0, sizeof(strm));
    
    if (inflateInit(&strm) != Z_OK) {
        free(buffer);
        return;
    }
    
    unsigned char* decompressed = malloc(hdr->original_size);
    if (!decompressed) {
        inflateEnd(&strm);
        free(buffer);
        return;
    }
    
    strm.next_in = buffer;
    strm.avail_in = size_to_read;
    strm.next_out = decompressed;
    strm.avail_out = hdr->original_size;
    
    int ret = inflate(&strm, Z_FINISH);
    if (ret == Z_STREAM_END) {
        size_t decompressed_size = hdr->original_size - strm.avail_out;
        recompressed_stream->write_bytes(recompressed_stream, decompressed, decompressed_size);
    }
    
    inflateEnd(&strm);
    free(buffer);
    free(decompressed);
}

/* Write pre-recursion data */
void deflate_write_pre_recursion_data(RecursionContext* context, 
                                       PrecompFormatHeaderData* precomp_hdr_data) {
    DeflateFormatHeaderData* hdr = (DeflateFormatHeaderData*)precomp_hdr_data;
    OStreamLike* output = context->output;
    
    output->write_long_long(output, hdr->original_size);
    output->write_long_long(output, hdr->precompressed_size);
}

/* Create Deflate format handler */
PrecompFormatHandler* create_deflate_handler(void) {
    PrecompFormatHandler* handler = malloc(sizeof(PrecompFormatHandler));
    if (!handler) return NULL;
    
    memset(handler, 0, sizeof(PrecompFormatHandler));
    
    handler->format_type = FORMAT_DEFLATE;
    handler->quick_check = deflate_quick_check;
    handler->attempt_precompression = deflate_attempt_precompression;
    handler->read_format_header = deflate_read_format_header;
    handler->recompress = deflate_recompress;
    handler->write_pre_recursion_data = deflate_write_pre_recursion_data;
    
    return handler;
}
