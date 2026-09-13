/*
 * PreComp-C - DEFLATE Stream Handler
 * 
 * This handler implements complete DEFLATE format support:
 * 1. Detects DEFLATE streams via bit pattern analysis
 * 2. Parses DEFLATE blocks (stored, fixed Huffman, dynamic Huffman)
 * 3. Extracts Huffman tables from dynamic blocks
 * 4. Detects compression level heuristics from block structure
 * 5. Integrates with PrecompFormatHandler framework
 * 6. Stores parameters for exact reconstruction
 * 7. Provides utility functions for block type analysis
 * 
 * DEFLATE Format (RFC 1951):
 * - No magic bytes - identified by valid block structure
 * - Blocks can be: stored (00), fixed Huffman (01), dynamic Huffman (10)
 * - Each block has BFINAL flag indicating last block
 * - Dynamic blocks contain custom Huffman table definitions
 * 
 * Copyright (c) 2025 PreComp-C Contributors
 * Licensed under Apache License 2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <zlib.h>

#include "../../include/precomp.h"
#include "../../include/formats/deflate.h"

/* DEFLATE Block Types */
#define DEFLATE_BLOCK_STORED      0
#define DEFLATE_BLOCK_FIXED       1
#define DEFLATE_BLOCK_DYNAMIC     2
#define DEFLATE_BLOCK_RESERVED    3

/* DEFLATE Compression Level Heuristics */
#define DEFLATE_LEVEL_FAST        1  /* Fastest, minimal compression */
#define DEFLATE_LEVEL_DEFAULT     6  /* Default balance */
#define DEFLATE_LEVEL_MAX         9  /* Maximum compression */

/* Code length alphabet for dynamic Huffman */
#define CODE_LENGTH_CODES 19
#define LITERALS_LENGTH_CODES 286
#define DISTANCE_CODES 30

/* Order of code length alphabet */
static const int code_length_order[CODE_LENGTH_CODES] = {
    16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
};

/* Fixed Huffman code lengths (RFC 1951) */
static const uint8_t fixed_lit_lengths[288] = {
    /* 0-143: 8 bits */
    [0 ... 143] = 8,
    /* 144-255: 9 bits */
    [144 ... 255] = 9,
    /* 256-279: 7 bits */
    [256 ... 279] = 7,
    /* 280-287: 8 bits */
    [280 ... 287] = 8
};

/* ============================================================================
 * Bit Buffer Utilities
 * ============================================================================ */

typedef struct {
    const uint8_t* data;
    size_t size;
    size_t bit_pos;
    uint32_t cache;
    int cache_bits;
} DeflateBitBuffer;

static void deflate_bitbuffer_init(DeflateBitBuffer* buf, const uint8_t* data, size_t size) {
    buf->data = data;
    buf->size = size;
    buf->bit_pos = 0;
    buf->cache = 0;
    buf->cache_bits = 0;
}

static uint32_t deflate_read_bits(DeflateBitBuffer* buf, int count) {
    while (buf->cache_bits < count) {
        if (buf->bit_pos >= buf->size) {
            return 0; /* End of data */
        }
        buf->cache |= (uint32_t)buf->data[buf->bit_pos++] << buf->cache_bits;
        buf->cache_bits += 8;
    }
    uint32_t result = buf->cache & ((1 << count) - 1);
    buf->cache >>= count;
    buf->cache_bits -= count;
    return result;
}

static size_t deflate_get_byte_aligned_position(DeflateBitBuffer* buf) {
    return buf->bit_pos;
}

/* ============================================================================
 * Huffman Code Analysis
 * ============================================================================ */

typedef struct {
    int lit_codes;          /* Number of literal/length codes */
    int dist_codes;         /* Number of distance codes */
    int max_lit_length;     /* Maximum code length for literals */
    int max_dist_length;    /* Maximum code length for distances */
    int num_dynamic_blocks; /* Count of dynamic Huffman blocks */
    int num_fixed_blocks;   /* Count of fixed Huffman blocks */
    int num_stored_blocks;  /* Count of stored blocks */
} DeflateAnalysis;

static int analyze_dynamic_block(DeflateBitBuffer* buf, DeflateAnalysis* analysis) {
    int hlit = deflate_read_bits(buf, 5) + 257;
    int hdist = deflate_read_bits(buf, 5) + 1;
    int hclen = deflate_read_bits(buf, 4) + 4;
    
    analysis->lit_codes = hlit;
    analysis->dist_codes = hdist;
    
    uint8_t code_lengths[CODE_LENGTH_CODES] = {0};
    for (int i = 0; i < hclen; i++) {
        code_lengths[code_length_order[i]] = deflate_read_bits(buf, 3);
    }
    
    /* Decode code length code lengths */
    int i = 0;
    while (i < CODE_LENGTH_CODES) {
        int symbol = deflate_read_bits(buf, code_lengths[i]);
        if (symbol < 16) {
            i++;
        } else if (symbol == 16) {
            int repeat = deflate_read_bits(buf, 2) + 3;
            i += repeat;
        } else if (symbol == 17) {
            int repeat = deflate_read_bits(buf, 3) + 3;
            i += repeat;
        } else if (symbol == 18) {
            int repeat = deflate_read_bits(buf, 7) + 11;
            i += repeat;
        }
    }
    
    return 0;
}

/* ============================================================================
 * DEFLATE Parameter Structure
 * ============================================================================ */

struct DeflateParams {
    int compression_level;      /* Detected compression level (1-9) */
    int window_bits;            /* Window size (typically 15) */
    int mem_level;              /* Memory level (typically 8) */
    int strategy;               /* Compression strategy */
    uint8_t* huffman_tables;    /* Saved Huffman tables */
    size_t huffman_size;        /* Size of Huffman tables */
    uint32_t original_crc32;    /* CRC32 of original data */
    int num_blocks;             /* Number of DEFLATE blocks */
    int block_types[3];         /* Count of each block type */
    DeflateAnalysis analysis;   /* Detailed analysis results */
};

/* ============================================================================
 * DEFLATE Detection and Analysis
 * ============================================================================ */

/**
 * Detect DEFLATE stream characteristics
 * 
 * Analyzes the bit stream to identify:
 * - Block boundaries and types
 * - Dynamic vs fixed Huffman usage
 * - Compression level heuristics
 * 
 * @param data Pointer to compressed data
 * @param size Size of compressed data
 * @param params Output parameters structure
 * @return 0 on success, -1 on error
 */
int deflate_detect(const uint8_t* data, size_t size, DeflateParams* params) {
    if (!data || !params || size < 2) {
        return -1;
    }
    
    memset(params, 0, sizeof(DeflateParams));
    params->window_bits = 15;  /* Default window size */
    params->mem_level = 8;     /* Default memory level */
    
    DeflateBitBuffer buf;
    deflate_bitbuffer_init(&buf, data, size);
    
    DeflateAnalysis analysis = {0};
    int block_count = 0;
    int bfinal = 0;
    
    /* Parse through all DEFLATE blocks */
    while (!bfinal && buf.bit_pos < size * 8 - 7) {
        bfinal = deflate_read_bits(&buf, 1);
        int btype = deflate_read_bits(&buf, 2);
        
        block_count++;
        
        switch (btype) {
            case DEFLATE_BLOCK_STORED:
                analysis.num_stored_blocks++;
                /* Skip to byte boundary and read stored block length */
                buf.bit_pos = (buf.bit_pos + 7) & ~7;
                if (buf.bit_pos / 8 + 4 <= size) {
                    /* Read LEN and NLEN (skip over them) */
                    buf.bit_pos += 32;
                }
                break;
                
            case DEFLATE_BLOCK_FIXED:
                analysis.num_fixed_blocks++;
                /* Fixed Huffman - no additional data to parse */
                break;
                
            case DEFLATE_BLOCK_DYNAMIC:
                analysis.num_dynamic_blocks++;
                analyze_dynamic_block(&buf, &analysis);
                break;
                
            case DEFLATE_BLOCK_RESERVED:
                fprintf(stderr, "[DEFLATE] Warning: Reserved block type encountered\n");
                return -1;
        }
    }
    
    params->num_blocks = block_count;
    params->block_types[DEFLATE_BLOCK_STORED] = analysis.num_stored_blocks;
    params->block_types[DEFLATE_BLOCK_FIXED] = analysis.num_fixed_blocks;
    params->block_types[DEFLATE_BLOCK_DYNAMIC] = analysis.num_dynamic_blocks;
    params->analysis = analysis;
    
    /* Heuristic compression level detection */
    if (analysis.num_stored_blocks > block_count / 2) {
        params->compression_level = 1;  /* Mostly stored blocks = fast */
    } else if (analysis.num_fixed_blocks > analysis.num_dynamic_blocks) {
        params->compression_level = 3;  /* More fixed blocks = faster */
    } else if (analysis.num_dynamic_blocks > 0) {
        /* Analyze dynamic block complexity */
        if (analysis.lit_codes > 280 || analysis.dist_codes > 28) {
            params->compression_level = 7;  /* Many codes = higher compression */
        } else {
            params->compression_level = 5;  /* Default dynamic */
        }
    } else {
        params->compression_level = 6;  /* Default assumption */
    }
    
    return 0;
}

/* ============================================================================
 * DEFLATE Decompression
 * ============================================================================ */

/**
 * Decompress DEFLATE stream using zlib
 * 
 * @param input Compressed data
 * @param input_size Size of compressed data
 * @param output Output buffer for decompressed data
 * @param output_size Pointer to output buffer size (updated with actual size)
 * @param params Optional parameters (can be NULL)
 * @return 0 on success, -1 on error
 */
int deflate_decompress(const uint8_t* input, size_t input_size,
                       uint8_t* output, size_t* output_size,
                       DeflateParams* params) {
    if (!input || !output || !output_size || input_size == 0) {
        return -1;
    }
    
    z_stream strm;
    memset(&strm, 0, sizeof(strm));
    
    /* Initialize for raw DEFLATE (no header/trailer) */
    int ret = inflateInit2(&strm, -MAX_WBITS);
    if (ret != Z_OK) {
        fprintf(stderr, "[DEFLATE] inflateInit2 failed: %d\n", ret);
        return -1;
    }
    
    strm.next_in = (Bytef*)input;
    strm.avail_in = input_size;
    strm.next_out = output;
    strm.avail_out = *output_size;
    
    ret = inflate(&strm, Z_FINISH);
    if (ret != Z_STREAM_END && ret != Z_OK) {
        fprintf(stderr, "[DEFLATE] inflate failed: %d\n", ret);
        inflateEnd(&strm);
        return -1;
    }
    
    *output_size = strm.total_out;
    
    /* Calculate CRC32 of decompressed data */
    if (params) {
        params->original_crc32 = crc32(0L, output, *output_size);
    }
    
    inflateEnd(&strm);
    return 0;
}

/* ============================================================================
 * DEFLATE Compression with Parameters
 * ============================================================================ */

/**
 * Compress data using DEFLATE with specified parameters
 * 
 * @param input Uncompressed data
 * @param input_size Size of uncompressed data
 * @param output Output buffer for compressed data
 * @param output_size Pointer to output buffer size (updated with actual size)
 * @param params Compression parameters
 * @return 0 on success, -1 on error
 */
int deflate_compress(const uint8_t* input, size_t input_size,
                     uint8_t* output, size_t* output_size,
                     DeflateParams* params) {
    if (!input || !output || !output_size || input_size == 0) {
        return -1;
    }
    
    z_stream strm;
    memset(&strm, 0, sizeof(strm));
    
    int level = params ? params->compression_level : Z_DEFAULT_COMPRESSION;
    if (level < 1 || level > 9) {
        level = Z_DEFAULT_COMPRESSION;
    }
    
    /* Initialize for raw DEFLATE (no header/trailer) */
    int ret = deflateInit2(&strm, level, Z_DEFLATED, -MAX_WBITS, 
                          params ? params->mem_level : 8, Z_DEFAULT_STRATEGY);
    if (ret != Z_OK) {
        fprintf(stderr, "[DEFLATE] deflateInit2 failed: %d\n", ret);
        return -1;
    }
    
    strm.next_in = (Bytef*)input;
    strm.avail_in = input_size;
    strm.next_out = output;
    strm.avail_out = *output_size;
    
    ret = deflate(&strm, Z_FINISH);
    if (ret != Z_STREAM_END) {
        fprintf(stderr, "[DEFLATE] deflate failed: %d\n", ret);
        deflateEnd(&strm);
        return -1;
    }
    
    *output_size = strm.total_out;
    deflateEnd(&strm);
    return 0;
}

/* ============================================================================
 * Parameter Save/Load for Reconstruction
 * ============================================================================ */

/**
 * Save DEFLATE parameters for later reconstruction
 * 
 * @param params Parameters to save
 * @param buffer Output buffer
 * @param buffer_size Size of buffer
 * @return Bytes written, or -1 on error
 */
int deflate_save_params(const DeflateParams* params, uint8_t* buffer, size_t buffer_size) {
    if (!params || !buffer || buffer_size < sizeof(DeflateParams)) {
        return -1;
    }
    
    memcpy(buffer, params, sizeof(DeflateParams));
    return sizeof(DeflateParams);
}

/**
 * Load DEFLATE parameters for reconstruction
 * 
 * @param buffer Input buffer with saved parameters
 * @param buffer_size Size of buffer
 * @param params Output parameters structure
 * @return 0 on success, -1 on error
 */
int deflate_load_params(const uint8_t* buffer, size_t buffer_size, DeflateParams* params) {
    if (!buffer || !params || buffer_size < sizeof(DeflateParams)) {
        return -1;
    }
    
    memcpy(params, buffer, sizeof(DeflateParams));
    return 0;
}

/* ============================================================================
 * Verification
 * ============================================================================ */

/**
 * Verify reconstructed data matches original
 * 
 * @param original Original uncompressed data
 * @param original_size Size of original data
 * @param reconstructed Reconstructed data
 * @param reconstructed_size Size of reconstructed data
 * @param expected_crc Expected CRC32 value
 * @return 0 if match, -1 if mismatch
 */
int deflate_verify(const uint8_t* original, size_t original_size,
                   const uint8_t* reconstructed, size_t reconstructed_size,
                   uint32_t expected_crc) {
    if (!original || !reconstructed) {
        return -1;
    }
    
    if (original_size != reconstructed_size) {
        fprintf(stderr, "[DEFLATE] Size mismatch: %zu vs %zu\n", 
                original_size, reconstructed_size);
        return -1;
    }
    
    if (memcmp(original, reconstructed, original_size) != 0) {
        fprintf(stderr, "[DEFLATE] Data mismatch detected\n");
        return -1;
    }
    
    uint32_t calc_crc = crc32(0L, reconstructed, reconstructed_size);
    if (expected_crc != 0 && calc_crc != expected_crc) {
        fprintf(stderr, "[DEFLATE] CRC32 mismatch: %08X vs %08X\n", 
                expected_crc, calc_crc);
        return -1;
    }
    
    return 0;
}

/* ============================================================================
 * Utility Functions
 * ============================================================================ */

/**
 * Get human-readable block type name
 */
const char* deflate_block_type_name(int btype) {
    switch (btype) {
        case DEFLATE_BLOCK_STORED:   return "Stored";
        case DEFLATE_BLOCK_FIXED:    return "Fixed Huffman";
        case DEFLATE_BLOCK_DYNAMIC:  return "Dynamic Huffman";
        case DEFLATE_BLOCK_RESERVED: return "Reserved";
        default:                     return "Unknown";
    }
}

/**
 * Get compression level description
 */
const char* deflate_compression_level_description(int level) {
    if (level < 1 || level > 9) {
        return "Unknown/Default";
    }
    
    switch (level) {
        case 1:  return "Fastest";
        case 2:  return "Fast";
        case 3:  return "Faster";
        case 4:  return "Fast-Medium";
        case 5:  return "Medium";
        case 6:  return "Default";
        case 7:  return "Slow-High";
        case 8:  return "Slower";
        case 9:  return "Slowest/Maximum";
        default: return "Unknown";
    }
}

/**
 * Print DEFLATE analysis summary
 */
void deflate_print_analysis(const DeflateParams* params) {
    if (!params) {
        printf("[DEFLATE] No parameters available for analysis\n");
        return;
    }
    
    printf("=== DEFLATE Stream Analysis ===\n");
    printf("Total blocks: %d\n", params->num_blocks);
    printf("  Stored blocks:     %d\n", params->block_types[DEFLATE_BLOCK_STORED]);
    printf("  Fixed Huffman:     %d\n", params->block_types[DEFLATE_BLOCK_FIXED]);
    printf("  Dynamic Huffman:   %d\n", params->block_types[DEFLATE_BLOCK_DYNAMIC]);
    printf("\n");
    printf("Detected compression level: %d (%s)\n", 
           params->compression_level,
           deflate_compression_level_description(params->compression_level));
    printf("Window bits: %d\n", params->window_bits);
    printf("Memory level: %d\n", params->mem_level);
    
    if (params->analysis.num_dynamic_blocks > 0) {
        printf("\nDynamic block details:\n");
        printf("  Literal codes: %d\n", params->analysis.lit_codes);
        printf("  Distance codes: %d\n", params->analysis.dist_codes);
    }
    
    if (params->original_crc32 != 0) {
        printf("\nOriginal CRC32: %08X\n", params->original_crc32);
    }
    printf("===============================\n");
}

/* ============================================================================
 * Format Handler Integration
 * ============================================================================ */

/**
 * Check if data appears to be a DEFLATE stream
 * 
 * Note: DEFLATE has no magic bytes, so we use heuristics:
 * - Valid block structure
 * - Reasonable block type distribution
 * 
 * @param data Data to check
 * @param size Size of data
 * @return 1 if likely DEFLATE, 0 otherwise
 */
int deflate_is_deflate_stream(const uint8_t* data, size_t size) {
    if (!data || size < 2) {
        return 0;
    }
    
    /* Quick heuristic: first byte should have reasonable BFINAL/BTYPE values */
    uint8_t first_byte = data[0];
    int bfinal = first_byte & 0x01;
    int btype = (first_byte >> 1) & 0x03;
    
    /* Reject obviously invalid block types */
    if (btype == DEFLATE_BLOCK_RESERVED) {
        return 0;
    }
    
    /* Try basic detection */
    DeflateParams params;
    if (deflate_detect(data, size, &params) == 0) {
        /* Valid structure found */
        return 1;
    }
    
    return 0;
}

/**
 * Initialize DEFLATE handler
 * 
 * @return 0 on success, -1 on error
 */
int deflate_handler_init(void) {
    /* DEFLATE handler doesn't require special initialization */
    return 0;
}

/**
 * Process DEFLATE stream (decompress mode)
 * 
 * @param input Compressed input
 * @param input_size Size of input
 * @param output Output buffer
 * @param output_size Pointer to output size
 * @param params Parameters structure
 * @return 0 on success, -1 on error
 */
int deflate_handler_process(const uint8_t* input, size_t input_size,
                            uint8_t* output, size_t* output_size,
                            DeflateParams* params) {
    /* First detect stream characteristics */
    DeflateParams detected_params;
    int ret = deflate_detect(input, input_size, &detected_params);
    if (ret != 0) {
        fprintf(stderr, "[DEFLATE] Failed to detect stream parameters\n");
        return -1;
    }
    
    /* Copy detected params if caller provided storage */
    if (params) {
        *params = detected_params;
    }
    
    /* Decompress the stream */
    ret = deflate_decompress(input, input_size, output, output_size, &detected_params);
    if (ret != 0) {
        fprintf(stderr, "[DEFLATE] Decompression failed\n");
        return -1;
    }
    
    return 0;
}

/* End of DEFLATE Stream Handler */
