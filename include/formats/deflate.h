/*
 * Deflate Format Handler - C Header
 * Converted from C++ to C for Tiny C Compiler compatibility
 */

#ifndef PRECOMP_DEFLATE_HANDLER_H
#define PRECOMP_DEFLATE_HANDLER_H

#include "../precomp.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* Recompress deflate result structure */
typedef struct {
    long long compressed_stream_size;
    long long uncompressed_stream_size;
    unsigned char* recon_data;
    size_t recon_data_size;
    bool accepted;
    unsigned char* uncompressed_stream_mem;
    size_t uncompressed_stream_mem_size;
    bool zlib_perfect;
    signed char zlib_comp_level;
    signed char zlib_mem_level;
    signed char zlib_window_bits;
} recompress_deflate_result;

/* Deflate precompression result */
typedef struct {
    precompression_result base;
    recompress_deflate_result rdres;
    unsigned char* zlib_header;
    size_t zlib_header_size;
    bool inc_last_hdr_byte;
} deflate_precompression_result;

/* Deflate histogram false positive detector */
typedef struct {
    unsigned char tmp_out[32768];  /* CHUNK size */
    int histogram[256];
    uintptr_t prev_input_id;
    unsigned char prev_first_byte;
    long long prev_deflate_stream_pos;
    int prev_maximum;
    int prev_used;
    int prev_i;
} DeflateHistogramFalsePositiveDetector;

/* Deflate format header data */
typedef struct {
    PrecompFormatHeaderData base;
    recompress_deflate_result rdres;
    unsigned char* stream_hdr;
    size_t stream_hdr_size;
} DeflateFormatHeaderData;

/* Function declarations */
void fin_fget_recon_data(IStreamLike* input, recompress_deflate_result* rdres);

recompress_deflate_result try_recompression_deflate(Precomp* precomp_mgr, 
                                                     IStreamLike* file, 
                                                     long long file_deflate_stream_pos, 
                                                     PrecompTmpFile* tmpfile);

void debug_deflate_detected(RecursionContext* context, 
                            const recompress_deflate_result* rdres, 
                            const char* type, 
                            long long deflate_stream_pos);

bool check_inflate_result(DeflateHistogramFalsePositiveDetector* falsePositiveDetector, 
                          uintptr_t current_input_id, 
                          unsigned char* checkbuf, 
                          size_t checkbuf_size,
                          int windowbits, 
                          const long long deflate_stream_pos, 
                          bool use_brute_parameters);

void fin_fget_deflate_hdr(IStreamLike* input, 
                          recompress_deflate_result* rdres, 
                          signed char flags, 
                          unsigned char* hdr_data, 
                          unsigned int* hdr_length, 
                          const bool inc_last_hdr_byte);

void fin_fget_deflate_rec(IStreamLike* precompressed_input, 
                          OStreamLike* recompressed_stream, 
                          recompress_deflate_result* rdres, 
                          signed char flags, 
                          unsigned char* hdr, 
                          unsigned int* hdr_length, 
                          const bool inc_last);

void recompress_deflate(IStreamLike* precompressed_input, 
                        OStreamLike* recompressed_stream, 
                        DeflateFormatHeaderData* precomp_hdr_data, 
                        const char* filename, 
                        const char* type,
                        void* tools);

PrecompFormatHeaderData* read_deflate_format_header(IStreamLike* precompressed_input, 
                                                     OStreamLike* recompressed_stream, 
                                                     signed char precomp_hdr_flags, 
                                                     bool inc_last_hdr_byte);

/* Format handler creation */
PrecompFormatHandler* create_deflate_handler(void);

/* Helper functions */
void destroy_deflate_precompression_result(deflate_precompression_result* result);
deflate_precompression_result* create_deflate_precompression_result(SupportedFormats format);

#endif /* PRECOMP_DEFLATE_HANDLER_H */
