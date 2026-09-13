/*
 * WAV Format Handler - C Implementation
 * Implements detection, analysis, compression parameters, and reconstruction
 * 
 * WAV files can be super-compressed by:
 * 1. Extracting raw PCM audio data
 * 2. Storing original format metadata (sample rate, bit depth, channels)
 * 3. Compressing with WavPack or FLAC (often smaller than original WAV)
 * 4. On reconstruction: re-encode to exact original WAV format
 */

#include "wav.h"
#include "../precomp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* WAV file constants */
#define WAV_RIFF_TAG 0x52494646  /* 'RIFF' */
#define WAV_WAVE_TAG 0x57415645  /* 'WAVE' */
#define WAV_FMT_TAG  0x666D7420  /* 'fmt ' */
#define WAV_DATA_TAG 0x64617461  /* 'data' */

/* Helper function to read little-endian 16-bit value */
static uint16_t read_le16(const unsigned char* buf) {
    return (uint16_t)(buf[0] | (buf[1] << 8));
}

/* Helper function to read little-endian 32-bit value */
static uint32_t read_le32(const unsigned char* buf) {
    return (uint32_t)(buf[0] | (buf[1] << 8) | (buf[2] << 16) | ((uint32_t)buf[3] << 24));
}

/* Quick check if data is WAV */
bool wav_quick_check(unsigned char* buffer, 
                     size_t buffer_size,
                     uintptr_t current_input_id, 
                     long long original_input_pos) {
    if (buffer_size < 12) return false;
    
    /* Check RIFF header: 'RIFF' .... 'WAVE' */
    if (read_le32(buffer) != WAV_RIFF_TAG) return false;
    if (read_le32(buffer + 8) != WAV_WAVE_TAG) return false;
    
    return true;
}

/* Attempt to precompress WAV data */
precompression_result* wav_attempt_precompression(Precomp* precomp_mgr, 
                                                   unsigned char* buffer, 
                                                   size_t buffer_size,
                                                   long long input_stream_pos) {
    precompression_result* result = malloc(sizeof(precompression_result));
    if (!result) return NULL;
    
    result->success = false;
    result->compressed_data = NULL;
    result->compressed_size = 0;
    result->original_size = buffer_size;
    
    if (buffer_size < 44) {
        /* File too small for valid WAV */
        free(result);
        return NULL;
    }
    
    /* Parse WAV header to find data chunk */
    size_t pos = 12;  /* Skip RIFF header (4 bytes tag + 4 bytes size + 4 bytes WAVE) */
    size_t data_offset = 0;
    size_t data_size = 0;
    
    while (pos + 8 <= buffer_size) {
        uint32_t chunk_tag = read_le32(buffer + pos);
        uint32_t chunk_size = read_le32(buffer + pos + 4);
        
        pos += 8;
        
        if (chunk_tag == WAV_DATA_TAG) {
            data_offset = pos;
            data_size = chunk_size;
            break;
        }
        
        /* Skip to next chunk (handle odd sizes with padding byte) */
        pos += chunk_size;
        if (chunk_size % 2 == 1 && pos < buffer_size) {
            pos++;
        }
        
        if (pos > buffer_size) break;
    }
    
    if (data_offset == 0 || data_size == 0) {
        /* No data chunk found */
        free(result);
        return NULL;
    }
    
    /* For now, just pass through the WAV data as-is */
    /* TODO: Implement actual super-compression using WavPack/FLAC */
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

/* Read WAV format header during decompression */
PrecompFormatHeaderData* wav_read_format_header(RecursionContext* context, 
                                                 signed char precomp_hdr_flags, 
                                                 SupportedFormats precomp_hdr_format) {
    WavFormatHeaderData* hdr_data = malloc(sizeof(WavFormatHeaderData));
    if (!hdr_data) return NULL;
    
    memset(hdr_data, 0, sizeof(WavFormatHeaderData));
    hdr_data->base.format_type = FORMAT_WAV;
    
    IStreamLike* input = context->input;
    hdr_data->original_size = input->read_long_long(input);
    hdr_data->precompressed_size = input->read_long_long(input);
    hdr_data->sample_rate = input->read_int(input);
    hdr_data->bits_per_sample = input->read_int(input);
    hdr_data->num_channels = input->read_int(input);
    hdr_data->audio_format = input->read_int(input);
    hdr_data->byte_rate = input->read_long_long(input);
    hdr_data->block_align = input->read_int(input);
    
    return &hdr_data->base;
}

/* Recompress WAV data */
void wav_recompress(IStreamLike* precompressed_input, 
                    OStreamLike* recompressed_stream, 
                    PrecompFormatHeaderData* precomp_hdr_data, 
                    SupportedFormats precomp_hdr_format,
                    void* tools) {
    WavFormatHeaderData* hdr = (WavFormatHeaderData*)precomp_hdr_data;
    
    long long size_to_read = hdr->precompressed_size;
    
    unsigned char* buffer = malloc(size_to_read);
    if (!buffer) return;
    
    precompressed_input->read_bytes(precompressed_input, buffer, size_to_read);
    recompressed_stream->write_bytes(recompressed_stream, buffer, size_to_read);
    
    free(buffer);
}

/* Write pre-recursion data for WAV format */
void wav_write_pre_recursion_data(RecursionContext* context, 
                                   PrecompFormatHeaderData* precomp_hdr_data) {
    WavFormatHeaderData* hdr = (WavFormatHeaderData*)precomp_hdr_data;
    OStreamLike* output = context->output;
    
    output->write_long_long(output, hdr->original_size);
    output->write_long_long(output, hdr->precompressed_size);
    output->write_int(output, hdr->sample_rate);
    output->write_int(output, hdr->bits_per_sample);
    output->write_int(output, hdr->num_channels);
    output->write_int(output, hdr->audio_format);
    output->write_long_long(output, hdr->byte_rate);
    output->write_int(output, hdr->block_align);
}

/* Create WAV format handler */
PrecompFormatHandler* create_wav_handler(void) {
    PrecompFormatHandler* handler = malloc(sizeof(PrecompFormatHandler));
    if (!handler) return NULL;
    
    memset(handler, 0, sizeof(PrecompFormatHandler));
    
    handler->format_type = FORMAT_WAV;
    handler->quick_check = wav_quick_check;
    handler->attempt_precompression = wav_attempt_precompression;
    handler->read_format_header = wav_read_format_header;
    handler->recompress = wav_recompress;
    handler->write_pre_recursion_data = wav_write_pre_recursion_data;
    
    return handler;
}

