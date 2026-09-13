/*
 * GZip Format Handler - C Implementation
 * Converted from C++ to C for Tiny C Compiler compatibility
 */

#include "gzip.h"
#include "deflate.h"
#include <string.h>
#include <stdlib.h>

#ifndef CHECKBUF_SIZE
#define CHECKBUF_SIZE 4096
#endif

/* Quick check for GZip format */
bool gzip_quick_check(unsigned char* buffer, 
                      size_t buffer_size,
                      uintptr_t current_input_id, 
                      long long original_input_pos) {
    if (buffer_size < 10) return false;
    
    /* Check GZip magic bytes: 0x1F 0x8B */
    if ((buffer[0] == 31) && (buffer[1] == 139)) {
        /* Check compression method (must be 8 = deflate) */
        int compression_method = buffer[2] & 15;
        /* Reserved FLG bits must be zero */
        if ((compression_method == 8) && ((buffer[3] & 224) == 0)) {
            return true;
        }
    }
    return false;
}

/* Attempt precompression of GZip data */
precompression_result* gzip_attempt_precompression(Precomp* precomp_mgr, 
                                                    unsigned char* buffer, 
                                                    size_t buffer_size,
                                                    long long input_stream_pos) {
    if (!precomp_mgr || !buffer || buffer_size < 10) return NULL;
    
    /* Parse GZip flags */
    bool fhcrc = (buffer[3] & 2) == 2;
    bool fextra = (buffer[3] & 4) == 4;
    bool fname = (buffer[3] & 8) == 8;
    bool fcomment = (buffer[3] & 16) == 16;
    
    int header_length = 10;
    bool dont_compress = false;
    int act_checkbuf_pos = 10;
    
    /* Parse extra field */
    if (fextra) {
        if (act_checkbuf_pos + 1 >= (int)buffer_size) {
            dont_compress = true;
        } else {
            int xlen = buffer[act_checkbuf_pos] + (buffer[act_checkbuf_pos + 1] << 8);
            if ((act_checkbuf_pos + xlen) > CHECKBUF_SIZE) {
                dont_compress = true;
            } else {
                act_checkbuf_pos += 2;
                header_length += 2;
                act_checkbuf_pos += xlen;
                header_length += xlen;
            }
        }
    }
    
    /* Parse original filename */
    if (fname && !dont_compress) {
        do {
            if (act_checkbuf_pos >= (int)buffer_size || act_checkbuf_pos >= CHECKBUF_SIZE) {
                dont_compress = true;
                break;
            }
            act_checkbuf_pos++;
            header_length++;
        } while (buffer[act_checkbuf_pos - 1] != 0);
    }
    
    /* Parse comment */
    if (fcomment && !dont_compress) {
        do {
            if (act_checkbuf_pos >= (int)buffer_size || act_checkbuf_pos >= CHECKBUF_SIZE) {
                dont_compress = true;
                break;
            }
            act_checkbuf_pos++;
            header_length++;
        } while (buffer[act_checkbuf_pos - 1] != 0);
    }
    
    /* Parse header CRC */
    if (fhcrc && !dont_compress) {
        act_checkbuf_pos += 2;
        dont_compress = (act_checkbuf_pos > CHECKBUF_SIZE);
        header_length += 2;
    }
    
    if (dont_compress) {
        return create_deflate_precompression_result(D_GZIP);
    }
    
    /* Try decompression using deflate (no zlib header in GZip) */
    deflate_precompression_result* result = try_decompression_deflate_type(
        precomp_mgr, 
        &precomp_mgr->statistics.decompressed_gzip_count, 
        &precomp_mgr->statistics.recompressed_gzip_count,
        D_GZIP, 
        buffer + 2,              /* Skip GZip magic bytes */
        header_length - 2,       /* Rest of header */
        input_stream_pos + header_length, 
        false,                   /* No zlib header */
        "in GZIP", 
        get_tempfile_name(precomp_mgr, "precomp_gzip")
    );
    
    if (result) {
        result->original_size_extra += header_length;
    }
    
    return (precompression_result*)result;
}

/* Read GZip format header during decompression */
PrecompFormatHeaderData* gzip_read_format_header(RecursionContext* context, 
                                                  signed char precomp_hdr_flags, 
                                                  SupportedFormats precomp_hdr_format) {
    if (!context || !context->fin) return NULL;
    
    /* Read deflate format header (no zlib header for GZip) */
    return read_deflate_format_header(context->fin, context->fout, precomp_hdr_flags, false);
}

/* Write pre-recursion data for GZip */
void gzip_write_pre_recursion_data(RecursionContext* context, 
                                    PrecompFormatHeaderData* precomp_hdr_data) {
    if (!context || !context->fout || !precomp_hdr_data) return;
    
    DeflateFormatHeaderData* precomp_deflate_hdr_data = (DeflateFormatHeaderData*)precomp_hdr_data;
    
    /* Write GZip magic bytes */
    ostream_put(context->fout, 31);
    ostream_put(context->fout, 139);
    
    /* Write stored header data (if any) */
    if (precomp_deflate_hdr_data->stream_hdr && precomp_deflate_hdr_data->stream_hdr_size > 0) {
        ostream_write(context->fout, precomp_deflate_hdr_data->stream_hdr, 
                      precomp_deflate_hdr_data->stream_hdr_size);
    }
}

/* Recompress GZip data */
void gzip_recompress(IStreamLike* precompressed_input, 
                     OStreamLike* recompressed_stream, 
                     PrecompFormatHeaderData* precomp_hdr_data, 
                     SupportedFormats precomp_hdr_format,
                     void* tools) {
    if (!precompressed_input || !recompressed_stream || !precomp_hdr_data || !tools) return;
    
    DeflateFormatHeaderData* deflate_hdr = (DeflateFormatHeaderData*)precomp_hdr_data;
    const char* temp_name = get_tools_tempfile_name(tools, "recomp_gzip", true);
    
    recompress_deflate(precompressed_input, recompressed_stream, deflate_hdr, temp_name, "GZIP", tools);
}

/* Create GZip format handler */
PrecompFormatHandler* create_gzip_handler(void) {
    PrecompFormatHandler* handler = (PrecompFormatHandler*)malloc(sizeof(PrecompFormatHandler));
    if (!handler) return NULL;
    
    handler->quick_check = gzip_quick_check;
    handler->attempt_precompression = gzip_attempt_precompression;
    handler->read_format_header = gzip_read_format_header;
    handler->recompress = gzip_recompress;
    handler->write_pre_recursion_data = gzip_write_pre_recursion_data;
    handler->format_type = D_GZIP;
    
    return handler;
}
