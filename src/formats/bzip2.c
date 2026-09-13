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

/* Detect BZip2 compression level from block size indicator */
static int bzip2_detect_compression_level(unsigned char* buffer, size_t buffer_size) {
    if (buffer_size < 4) return -1;
    
    /* BZip2 header format: BZhX where X is block size indicator ('1'-'9') */
    if (buffer[0] != 'B' || buffer[1] != 'Z' || buffer[2] != 'h') {
        return -1;
    }
    
    char level_char = buffer[3];
    if (level_char >= '1' && level_char <= '9') {
        return level_char - '0';
    }
    
    return -1;
}

/* Quick check if data is BZip2 compressed */
bool bzip2_quick_check(struct PrecompFormatHandler* self, 
                       unsigned char* buffer, 
                       uintptr_t input_id, 
                       long long pos) {
    if (buffer == NULL) return false;
    
    /* BZip2 magic: "BZ" followed by compression method digit */
    if (buffer[0] != 'B' || buffer[1] != 'Z') return false;
    if (buffer[2] < 'h' || buffer[2] > '9') return false;
    if (buffer[3] != 'h') return false;
    
    return true;
}

/* Attempt to precompress data using BZip2 */
precompression_result* bzip2_attempt_precompression(struct PrecompFormatHandler* self,
                                                     void* precomp_instance,
                                                     unsigned char* buffer, 
                                                     long long input_stream_pos) {
    Precomp* precomp_mgr = (Precomp*)precomp_instance;
    size_t buffer_size = (size_t)input_stream_pos;  /* Reuse parameter for buffer size in this context */
    
    precompression_result* result = malloc(sizeof(precompression_result));
    if (!result) return NULL;
    
    result->format = F_BZIP2;
    result->success = false;
    result->compressed_size = 0;
    result->uncompressed_size = buffer_size;
    result->dump_header = NULL;
    result->dump_data = NULL;
    
    /* Detect compression level from existing BZip2 stream if present */
    int detected_level = bzip2_detect_compression_level(buffer, buffer_size);
    if (detected_level > 0) {
        /* Store detected level for later use */
        precomp_mgr->last_detected_level = detected_level;
    }
    
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
        result->compressed_size = destSize;
        /* Note: compressed_data would be handled by the caller in the actual implementation */
    } else {
        free(compressed_buffer);
        free(result);
        return NULL;
    }
    
    return result;
}

/* Read BZip2 format header */
PrecompFormatHeaderData* bzip2_read_format_header(struct PrecompFormatHandler* self,
                                                   RecursionContext* context, 
                                                   signed char precomp_hdr_flags, 
                                                   SupportedFormats precomp_hdr_format) {
    Bzip2FormatHeaderData* hdr_data = malloc(sizeof(Bzip2FormatHeaderData));
    if (!hdr_data) return NULL;
    
    memset(hdr_data, 0, sizeof(Bzip2FormatHeaderData));
    hdr_data->base.format = F_BZIP2;
    hdr_data->base.destroy = NULL;
    
    /* Read original size and precompressed size from stream */
    IStreamLike* input = context->fin;
    hdr_data->original_size = input->read_long_long(input);
    hdr_data->precompressed_size = input->read_long_long(input);
    
    return &hdr_data->base;
}

/* Recompress BZip2 data */
void bzip2_recompress(struct PrecompFormatHandler* self,
                      IStreamLike* precompressed_input, 
                      OStreamLike* recompressed_stream, 
                      PrecompFormatHeaderData* precomp_hdr_data, 
                      SupportedFormats precomp_hdr_format,
                      FormatHandlerTools* tools) {
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
void bzip2_write_pre_recursion_data(struct PrecompFormatHandler* self,
                                     RecursionContext* context, 
                                     PrecompFormatHeaderData* precomp_hdr_data) {
    Bzip2FormatHeaderData* hdr = (Bzip2FormatHeaderData*)precomp_hdr_data;
    OStreamLike* output = context->fout;
    
    /* Write original size and precompressed size */
    output->write_long_long(output, hdr->original_size);
    output->write_long_long(output, hdr->precompressed_size);
}

/* Create BZip2 format handler */
PrecompFormatHandler* create_bzip2_handler(void) {
    PrecompFormatHandler* handler = malloc(sizeof(PrecompFormatHandler));
    if (!handler) return NULL;
    
    memset(handler, 0, sizeof(PrecompFormatHandler));
    
    /* Set supported header bytes (BZh1-BZh9) */
    static SupportedFormats header_bytes[] = { F_BZIP2 };
    handler->header_bytes = header_bytes;
    handler->header_bytes_count = 1;
    handler->depth_limit = 0;
    handler->has_depth_limit = false;
    
    handler->quick_check = bzip2_quick_check;
    handler->attempt_precompression = bzip2_attempt_precompression;
    handler->read_format_header = bzip2_read_format_header;
    handler->recompress = bzip2_recompress;
    handler->write_pre_recursion_data = bzip2_write_pre_recursion_data;
    
    return handler;
}
