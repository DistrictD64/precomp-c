/*
 * WAV Format Handler - C Header
 * Implements detection, analysis, compression parameters, and reconstruction
 */

#ifndef PRECOMP_WAV_HANDLER_H
#define PRECOMP_WAV_HANDLER_H

#include "../precomp.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* WAV precompression result */
typedef struct {
    precompression_result base;
} wav_precompression_result;

/* WAV format header data */
typedef struct {
    PrecompFormatHeaderData base;
    long long original_size;
    long long precompressed_size;
    int sample_rate;
    int bits_per_sample;
    int num_channels;
    int audio_format;      /* 1 = PCM, 3 = IEEE float, etc. */
    long long byte_rate;
    int block_align;
} WavFormatHeaderData;

/* Function declarations */
bool wav_quick_check(unsigned char* buffer, 
                     size_t buffer_size,
                     uintptr_t current_input_id, 
                     long long original_input_pos);

precompression_result* wav_attempt_precompression(Precomp* precomp_mgr, 
                                                   unsigned char* buffer, 
                                                   size_t buffer_size,
                                                   long long input_stream_pos);

PrecompFormatHeaderData* wav_read_format_header(RecursionContext* context, 
                                                 signed char precomp_hdr_flags, 
                                                 SupportedFormats precomp_hdr_format);

void wav_recompress(IStreamLike* precompressed_input, 
                    OStreamLike* recompressed_stream, 
                    PrecompFormatHeaderData* precomp_hdr_data, 
                    SupportedFormats precomp_hdr_format,
                    void* tools);

void wav_write_pre_recursion_data(RecursionContext* context, 
                                   PrecompFormatHeaderData* precomp_hdr_data);

/* Format handler creation */
PrecompFormatHandler* create_wav_handler(void);

#endif /* PRECOMP_WAV_HANDLER_H */

