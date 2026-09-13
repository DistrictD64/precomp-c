/*
 * ZLib Format Handler - C Implementation
 * Converted from C++ to C for Tiny C Compiler compatibility
 */

#include "zlib.h"
#include "deflate.h"
#include <string.h>
#include <stdlib.h>

/* Check if buffer contains valid zlib header */
bool zlib_header_check(unsigned char* buffer, size_t buffer_size) {
    if (buffer_size < 2) return false;
    
    /* Check: ((CMF << 8) + FLG) % 31 == 0 and FDICT bit not set */
    int cmf_flg = (buffer[0] << 8) + buffer[1];
    bool looks_like_zlib_header = ((cmf_flg % 31) == 0) && ((buffer[1] & 32) == 0);
    
    if (!looks_like_zlib_header) return false;
    
    /* Compression method must be 8 (deflate) */
    int compression_method = buffer[0] & 15;
    return compression_method == 8;
}

/* Quick check for zlib format */
bool zlib_quick_check(unsigned char* buffer, 
                      size_t buffer_size,
                      uintptr_t current_input_id, 
                      long long original_input_pos) {
    return zlib_header_check(buffer, buffer_size);
}

/* Attempt precompression of zlib data */
precompression_result* zlib_attempt_precompression(Precomp* precomp_mgr, 
                                                    unsigned char* buffer, 
                                                    size_t buffer_size,
                                                    long long input_stream_pos) {
    if (!precomp_mgr || !buffer || buffer_size < 2) return NULL;
    
    /* Calculate windowbits from zlib header */
    int windowbits = (buffer[0] >> 4) + 8;
    
    /* Deflate stream starts after 2-byte zlib header */
    long long deflate_stream_pos = input_stream_pos + 2;
    
    /* Try decompression with negative windowbits (raw deflate) */
    deflate_precompression_result* result = try_decompression_deflate_type_with_windowbits(
        precomp_mgr, 
        &precomp_mgr->statistics.decompressed_zlib_count, 
        &precomp_mgr->statistics.recompressed_zlib_count,
        D_RAW, 
        buffer, 
        2,                    /* zlib header length */
        deflate_stream_pos, 
        true,                 /* Has zlib header */
        "(intense mode)", 
        get_tempfile_name(precomp_mgr, "original_zlib"),
        -windowbits           /* Negative windowbits for raw deflate */
    );
    
    if (result) {
        result->original_size_extra += 2;
    }
    
    return (precompression_result*)result;
}

/* Read zlib format header during decompression */
PrecompFormatHeaderData* zlib_read_format_header(RecursionContext* context, 
                                                  signed char precomp_hdr_flags, 
                                                  SupportedFormats precomp_hdr_format) {
    if (!context || !context->fin) return NULL;
    
    /* Read deflate format header with zlib header */
    return read_deflate_format_header(context->fin, context->fout, precomp_hdr_flags, true);
}

/* Write pre-recursion data for zlib */
void zlib_write_pre_recursion_data(RecursionContext* context, 
                                    PrecompFormatHeaderData* precomp_hdr_data) {
    if (!context || !context->fout || !precomp_hdr_data) return;
    
    DeflateFormatHeaderData* precomp_deflate_hdr_data = (DeflateFormatHeaderData*)precomp_hdr_data;
    
    /* Write stored zlib header */
    if (precomp_deflate_hdr_data->stream_hdr && precomp_deflate_hdr_data->stream_hdr_size > 0) {
        ostream_write(context->fout, precomp_deflate_hdr_data->stream_hdr, 
                      precomp_deflate_hdr_data->stream_hdr_size);
    }
}

/* Recompress zlib data */
void zlib_recompress(IStreamLike* precompressed_input, 
                     OStreamLike* recompressed_stream, 
                     PrecompFormatHeaderData* precomp_hdr_data, 
                     SupportedFormats precomp_hdr_format,
                     void* tools) {
    if (!precompressed_input || !recompressed_stream || !precomp_hdr_data || !tools) return;
    
    DeflateFormatHeaderData* deflate_hdr = (DeflateFormatHeaderData*)precomp_hdr_data;
    const char* temp_name = get_tools_tempfile_name(tools, "recomp_zlib", true);
    
    recompress_deflate(precompressed_input, recompressed_stream, deflate_hdr, temp_name, "raw zLib", tools);
}

/* Create zlib format handler */
PrecompFormatHandler* create_zlib_handler(void) {
    PrecompFormatHandler* handler = (PrecompFormatHandler*)malloc(sizeof(PrecompFormatHandler));
    if (!handler) return NULL;
    
    handler->quick_check = zlib_quick_check;
    handler->attempt_precompression = zlib_attempt_precompression;
    handler->read_format_header = zlib_read_format_header;
    handler->recompress = zlib_recompress;
    handler->write_pre_recursion_data = zlib_write_pre_recursion_data;
    handler->format_type = D_RAW;
    
    return handler;
}
