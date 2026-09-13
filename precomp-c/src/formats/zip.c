/*
 * ZIP Format Handler - C Implementation
 * Converted from C++ to C for Tiny C Compiler compatibility
 */

#include "zip.h"
#include "deflate.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifndef CHECKBUF_SIZE
#define CHECKBUF_SIZE 4096
#endif

/* Quick check for ZIP format */
bool zip_quick_check(unsigned char* buffer, 
                     size_t buffer_size,
                     uintptr_t current_input_id, 
                     long long original_input_pos) {
    if (buffer_size < 30) return false;
    
    /* Check ZIP local file header signature: PK\x03\x04 */
    if ((buffer[0] == 'P') && (buffer[1] == 'K') &&
        (buffer[2] == 3) && (buffer[3] == 4)) {
        
        /* Extract sizes from ZIP header */
        unsigned int compressed_size = (buffer[21] << 24) + (buffer[20] << 16) + 
                                       (buffer[19] << 8) + buffer[18];
        unsigned int uncompressed_size = (buffer[25] << 24) + (buffer[24] << 16) + 
                                         (buffer[23] << 8) + buffer[22];
        unsigned int filename_length = (buffer[27] << 8) + buffer[26];
        unsigned int extra_field_length = (buffer[29] << 8) + buffer[28];
        
        /* Compression method 8 = Deflate, no data descriptor */
        if ((filename_length + extra_field_length) <= CHECKBUF_SIZE && 
            buffer[8] == 8 && buffer[9] == 0) {
            return true;
        }
    }
    return false;
}

/* Attempt precompression of ZIP data */
precompression_result* zip_attempt_precompression(Precomp* precomp_mgr, 
                                                   unsigned char* buffer, 
                                                   size_t buffer_size,
                                                   long long input_stream_pos) {
    if (!precomp_mgr || !buffer || buffer_size < 30) return NULL;
    
    /* Extract lengths from ZIP header */
    unsigned int filename_length = (buffer[27] << 8) + buffer[26];
    unsigned int extra_field_length = (buffer[29] << 8) + buffer[28];
    unsigned int header_length = 30 + filename_length + extra_field_length;
    
    /* Position of deflate stream (after ZIP header) */
    long long deflate_stream_pos = input_stream_pos + header_length;
    
    /* Try decompression using deflate (no zlib header in ZIP) */
    deflate_precompression_result* result = try_decompression_deflate_type(
        precomp_mgr, 
        &precomp_mgr->statistics.decompressed_zip_count, 
        &precomp_mgr->statistics.recompressed_zip_count,
        D_ZIP, 
        buffer + 4,              /* Skip ZIP signature */
        header_length - 4,       /* Rest of header */
        deflate_stream_pos, 
        false,                   /* No zlib header */
        "in ZIP", 
        get_tempfile_name(precomp_mgr, "decomp_zip")
    );
    
    if (result) {
        /* Add ZIP header size to original size */
        result->original_size_extra += header_length;
    }
    
    return (precompression_result*)result;
}

/* Read ZIP format header during decompression */
PrecompFormatHeaderData* zip_read_format_header(RecursionContext* context, 
                                                 signed char precomp_hdr_flags, 
                                                 SupportedFormats precomp_hdr_format) {
    if (!context || !context->fin) return NULL;
    
    /* Read deflate format header (no zlib header for ZIP) */
    return read_deflate_format_header(context->fin, context->fout, precomp_hdr_flags, false);
}

/* Write pre-recursion data for ZIP */
void zip_write_pre_recursion_data(RecursionContext* context, 
                                   PrecompFormatHeaderData* precomp_hdr_data) {
    if (!context || !context->fout || !precomp_hdr_data) return;
    
    DeflateFormatHeaderData* precomp_deflate_hdr_data = (DeflateFormatHeaderData*)precomp_hdr_data;
    
    /* Write ZIP local file header signature */
    ostream_put(context->fout, 'P');
    ostream_put(context->fout, 'K');
    ostream_put(context->fout, 3);
    ostream_put(context->fout, 4);
    
    /* Write stored header data (if any) */
    if (precomp_deflate_hdr_data->stream_hdr && precomp_deflate_hdr_data->stream_hdr_size > 0) {
        ostream_write(context->fout, precomp_deflate_hdr_data->stream_hdr, 
                      precomp_deflate_hdr_data->stream_hdr_size);
    }
}

/* Recompress ZIP data */
void zip_recompress(IStreamLike* precompressed_input, 
                    OStreamLike* recompressed_stream, 
                    PrecompFormatHeaderData* precomp_hdr_data, 
                    SupportedFormats precomp_hdr_format,
                    void* tools) {
    if (!precompressed_input || !recompressed_stream || !precomp_hdr_data || !tools) return;
    
    DeflateFormatHeaderData* deflate_hdr = (DeflateFormatHeaderData*)precomp_hdr_data;
    const char* temp_name = get_tools_tempfile_name(tools, "recomp_zip", true);
    
    recompress_deflate(precompressed_input, recompressed_stream, deflate_hdr, temp_name, "ZIP", tools);
}

/* Create ZIP format handler */
PrecompFormatHandler* create_zip_handler(void) {
    PrecompFormatHandler* handler = (PrecompFormatHandler*)malloc(sizeof(PrecompFormatHandler));
    if (!handler) return NULL;
    
    handler->quick_check = zip_quick_check;
    handler->attempt_precompression = zip_attempt_precompression;
    handler->read_format_header = zip_read_format_header;
    handler->recompress = zip_recompress;
    handler->write_pre_recursion_data = zip_write_pre_recursion_data;
    handler->format_type = D_ZIP;
    
    return handler;
}
