/*
 * SWF Format Handler - C Implementation
 * Converted from C++ to C for Tiny C Compiler compatibility
 */

#include "swf.h"
#include "zlib.h"
#include "deflate.h"
#include <string.h>
#include <stdlib.h>

/* Quick check for SWF format */
bool swf_quick_check(unsigned char* buffer, 
                     size_t buffer_size,
                     uintptr_t current_input_id, 
                     long long original_input_pos) {
    if (buffer_size < 11) return false;
    
    /* CWS = Compressed SWF file */
    bool cws_hdr = (buffer[0] == 'C') && (buffer[1] == 'W') && (buffer[2] == 'S');
    
    if (!cws_hdr) return false;
    
    /* Check zlib header starting at offset 8 */
    return zlib_header_check(buffer + 8, buffer_size - 8);
}

/* Attempt precompression of SWF data */
precompression_result* swf_attempt_precompression(Precomp* precomp_mgr, 
                                                   unsigned char* buffer, 
                                                   size_t buffer_size,
                                                   long long input_stream_pos) {
    if (!precomp_mgr || !buffer || buffer_size < 11) return NULL;
    
    deflate_precompression_result* result = create_deflate_precompression_result(D_SWF);
    if (!result) return NULL;
    
    /* SWF compressed stream starts at offset 10 (skip CWS and zlib header) */
    long long deflate_stream_pos = input_stream_pos + 10;
    
    /* Try decompression using deflate */
    result = try_decompression_deflate_type(
        precomp_mgr, 
        &precomp_mgr->statistics.decompressed_swf_count, 
        &precomp_mgr->statistics.recompressed_swf_count,
        D_SWF, 
        buffer + 3,  /* Skip CWS header */
        7,           /* zlib header length */
        deflate_stream_pos, 
        true,        /* Has zlib header */
        "in SWF", 
        get_tempfile_name(precomp_mgr, "original_swf")
    );
    
    if (result) {
        result->original_size_extra += 10;
    }
    
    return (precompression_result*)result;
}

/* Read SWF format header during decompression */
PrecompFormatHeaderData* swf_read_format_header(RecursionContext* context, 
                                                 signed char precomp_hdr_flags, 
                                                 SupportedFormats precomp_hdr_format) {
    if (!context || !context->fin) return NULL;
    
    return read_deflate_format_header(context->fin, context->fout, precomp_hdr_flags, true);
}

/* Recompress SWF data */
void swf_recompress(IStreamLike* precompressed_input, 
                    OStreamLike* recompressed_stream, 
                    PrecompFormatHeaderData* precomp_hdr_data, 
                    SupportedFormats precomp_hdr_format,
                    void* tools) {
    if (!precompressed_input || !recompressed_stream || !precomp_hdr_data || !tools) return;
    
    DeflateFormatHeaderData* precomp_deflate_hdr_data = (DeflateFormatHeaderData*)precomp_hdr_data;
    
    /* Write CWS header */
    ostream_put(recompressed_stream, 'C');
    ostream_put(recompressed_stream, 'W');
    ostream_put(recompressed_stream, 'S');
    
    /* Write zlib header */
    if (precomp_deflate_hdr_data->stream_hdr) {
        ostream_write(recompressed_stream, precomp_deflate_hdr_data->stream_hdr, 
                      precomp_deflate_hdr_data->stream_hdr_size);
    }
    
    /* Recompress deflate stream */
    const char* temp_name = get_tools_tempfile_name(tools, "recomp_swf", true);
    recompress_deflate(precompressed_input, recompressed_stream, precomp_deflate_hdr_data, temp_name, "SWF", tools);
}

/* Write pre-recursion data for SWF */
void swf_write_pre_recursion_data(RecursionContext* context, 
                                   PrecompFormatHeaderData* precomp_hdr_data) {
    /* SWF uses standard deflate format header data, no extra data needed */
    (void)context;
    (void)precomp_hdr_data;
}

/* Create SWF format handler */
PrecompFormatHandler* create_swf_handler(void) {
    PrecompFormatHandler* handler = (PrecompFormatHandler*)malloc(sizeof(PrecompFormatHandler));
    if (!handler) return NULL;
    
    handler->quick_check = swf_quick_check;
    handler->attempt_precompression = swf_attempt_precompression;
    handler->read_format_header = swf_read_format_header;
    handler->recompress = swf_recompress;
    handler->write_pre_recursion_data = swf_write_pre_recursion_data;
    handler->format_type = D_SWF;
    
    return handler;
}
