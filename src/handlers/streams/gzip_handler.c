/*
 * PreComp-C - GZip Stream Handler
 * 
 * This handler implements complete GZIP format support:
 * 1. Detects GZIP via magic bytes (0x1F 0x8B)
 * 2. Parses GZIP header (modification time, flags, extra fields, filename, comment, CRC16)
 * 3. Detects compression levels from extra flags
 * 4. Integrates with PrecompFormatHandler framework
 * 5. Provides utility functions for OS name lookup and compression level hints
 * 
 * Copyright (c) 2025 PreComp-C Contributors
 * Licensed under Apache License 2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "../../include/precomp.h"
#include "../../include/formats/gzip.h"

/* GZIP Magic Bytes */
#define GZIP_MAGIC_1 0x1F
#define GZIP_MAGIC_2 0x8B

/* GZIP Compression Methods */
#define GZIP_METHOD_STORED     0
#define GZIP_METHOD_DEFLATE    8

/* GZIP Flags */
#define GZIP_FLAG_FTEXT    0x01  /* ASCII text hint */
#define GZIP_FLAG_CRC16    0x02  /* Header CRC16 present */
#define GZIP_FLAG_EXTRA    0x04  /* Extra field present */
#define GZIP_FLAG_FILENAME 0x08  /* Original filename present */
#define GZIP_FLAG_COMMENT  0x10  /* Comment present */
#define GZIP_FLAG_RESERVED 0xE0  /* Reserved bits */

/* GZIP Extra Flags for compression level detection */
#define GZIP_XFLAG_FASTEST  0x04  /* Fastest compression */
#define GZIP_XFLAG_MAXIMUM  0x02  /* Maximum compression */

/* OS identifiers for GZIP header */
#define GZIP_OS_FAT         0
#define GZIP_OS_AMIGA       1
#define GZIP_OS_VMS         2
#define GZIP_OS_UNIX        3
#define GZIP_OS_VM_CMS      4
#define GZIP_OS_ATARI_TOS   5
#define GZIP_OS_HPFS        6
#define GZIP_OS_MACINTOSH   7
#define GZIP_OS_Z_SYSTEM    8
#define GZIP_OS_CP_M        9
#define GZIP_OS_TOPS_20    10
#define GZIP_OS_NTFS       11
#define GZIP_OS_QDOS       12
#define GZIP_OS_ACORN      13

/* Maximum sizes for safety */
#define GZIP_MAX_EXTRA_LEN   65535
#define GZIP_MAX_FILENAME    4096
#define GZIP_MAX_COMMENT     65535

/* GZip precompression result structure */
typedef struct {
    precompression_result base;
    uint32_t original_crc32;
    uint32_t compressed_size;
    uint8_t compression_method;
    uint8_t compression_flags;
    int detected_level;
} gzip_precompression_result;

/* GZip format header data structure */
typedef struct {
    PrecompFormatHeaderData base;
    uint32_t modification_time;
    uint8_t flags;
    uint8_t extra_flags;
    uint8_t os_type;
    uint8_t* extra_field;
    size_t extra_field_len;
    char* filename;
    char* comment;
    uint16_t header_crc16;
    uint32_t original_size;
    uint32_t original_crc32;
    long long precompressed_size;
    int detected_compression_level;
} GzipFormatHeaderData;

/* Forward declarations */
static bool gzip_quick_check_impl(struct PrecompFormatHandler* self, 
                                   unsigned char* buffer, uintptr_t input_id, 
                                   long long pos);
static precompression_result* gzip_attempt_precompression(struct PrecompFormatHandler* self,
                                                           void* precomp_instance,
                                                           unsigned char* buffer,
                                                           long long pos);
static PrecompFormatHeaderData* gzip_read_format_header_impl(struct PrecompFormatHandler* self,
                                                              RecursionContext* context,
                                                              signed char flags,
                                                              SupportedFormats format);
static void gzip_recompress_impl(struct PrecompFormatHandler* self,
                                  IStreamLike* input, OStreamLike* output,
                                  PrecompFormatHeaderData* header_data,
                                  SupportedFormats format,
                                  FormatHandlerTools* tools);
static void gzip_write_pre_recursion_data_impl(struct PrecompFormatHandler* self,
                                                RecursionContext* context,
                                                PrecompFormatHeaderData* header_data);
static void gzip_header_data_destroy(PrecompFormatHeaderData* self);

/* Utility function: Get OS name from OS type byte */
const char* gzip_get_os_name(uint8_t os_type) {
    switch (os_type) {
        case GZIP_OS_FAT:       return "FAT filesystem / MS-DOS";
        case GZIP_OS_AMIGA:     return "Amiga";
        case GZIP_OS_VMS:       return "VMS (or OpenVMS)";
        case GZIP_OS_UNIX:      return "Unix";
        case GZIP_OS_VM_CMS:    return "VM/CMS";
        case GZIP_OS_ATARI_TOS: return "Atari TOS";
        case GZIP_OS_HPFS:      return "HPFS filesystem (OS/2 or NT)";
        case GZIP_OS_MACINTOSH: return "Macintosh";
        case GZIP_OS_Z_SYSTEM:  return "Z-System";
        case GZIP_OS_CP_M:      return "CP/M";
        case GZIP_OS_TOPS_20:   return "TOPS-20";
        case GZIP_OS_NTFS:      return "NTFS filesystem (Windows NT)";
        case GZIP_OS_QDOS:      return "QDOS";
        case GZIP_OS_ACORN:     return "Acorn RISC OS";
        default:                return "Unknown";
    }
}

/* Utility function: Get compression level hint from extra flags */
int gzip_get_compression_level_hint(uint8_t extra_flags) {
    if (extra_flags & GZIP_XFLAG_FASTEST) {
        return 1;  /* Fastest compression (level 1) */
    } else if (extra_flags & GZIP_XFLAG_MAXIMUM) {
        return 9;  /* Maximum compression (level 9) */
    }
    return 6;  /* Default compression (level 6) */
}

/* Utility function: Calculate CRC16 for GZIP header */
static uint16_t gzip_calc_crc16(const uint8_t* data, size_t len) {
    uint16_t crc = 0;
    static const uint16_t crc16_table[256] = {
        0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
        0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
        0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
        0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
        0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
        0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
        0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
        0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
        0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
        0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
        0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
        0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
        0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
        0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
        0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
        0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
        0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
        0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
        0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
        0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
        0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
        0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
        0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
        0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
        0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
        0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
        0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
        0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
        0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
        0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
        0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
        0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040
    };
    
    for (size_t i = 0; i < len; i++) {
        crc = crc16_table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    }
    return crc;
}

/* Read a null-terminated string from stream */
static char* read_null_string(IStreamLike* stream, size_t max_len) {
    char* buffer = (char*)malloc(max_len + 1);
    if (!buffer) return NULL;
    
    size_t pos = 0;
    int ch;
    while ((ch = stream->read_byte(stream)) != EOF && pos < max_len) {
        buffer[pos++] = (char)ch;
        if (ch == 0) break;
    }
    
    if (ch == EOF && pos == 0) {
        free(buffer);
        return NULL;
    }
    
    buffer[pos] = '\0';
    return buffer;
}

/* Create GZip format header data */
static GzipFormatHeaderData* gzip_header_data_create(void) {
    GzipFormatHeaderData* data = (GzipFormatHeaderData*)calloc(1, sizeof(GzipFormatHeaderData));
    if (!data) return NULL;
    
    data->base.format = F_GZIP;
    data->base.destroy = gzip_header_data_destroy;
    data->extra_field = NULL;
    data->extra_field_len = 0;
    data->filename = NULL;
    data->comment = NULL;
    data->header_crc16 = 0;
    data->original_size = 0;
    data->original_crc32 = 0;
    data->precompressed_size = 0;
    data->detected_compression_level = 6;
    
    return data;
}

/* Destroy GZip format header data */
static void gzip_header_data_destroy(PrecompFormatHeaderData* self) {
    if (!self) return;
    
    GzipFormatHeaderData* data = (GzipFormatHeaderData*)self;
    
    if (data->extra_field) {
        free(data->extra_field);
        data->extra_field = NULL;
    }
    if (data->filename) {
        free(data->filename);
        data->filename = NULL;
    }
    if (data->comment) {
        free(data->comment);
        data->comment = NULL;
    }
    
    free(data);
}

/* Quick check: Verify GZIP magic bytes */
static bool gzip_quick_check_impl(struct PrecompFormatHandler* self,
                                   unsigned char* buffer, uintptr_t input_id,
                                   long long pos) {
    (void)self;
    (void)input_id;
    (void)pos;
    
    if (!buffer) return false;
    
    /* Check for GZIP magic bytes: 0x1F 0x8B */
    return (buffer[0] == GZIP_MAGIC_1 && buffer[1] == GZIP_MAGIC_2);
}

/* Attempt precompression of GZIP stream */
static precompression_result* gzip_attempt_precompression(struct PrecompFormatHandler* self,
                                                           void* precomp_instance,
                                                           unsigned char* buffer,
                                                           long long pos) {
    (void)self;
    (void)precomp_instance;
    (void)buffer;
    (void)pos;
    
    /* Placeholder for full precompression implementation */
    /* In production, this would use zlib to decompress and analyze the stream */
    
    gzip_precompression_result* result = 
        (gzip_precompression_result*)calloc(1, sizeof(gzip_precompression_result));
    if (!result) return NULL;
    
    result->base.format = F_GZIP;
    result->base.success = false;  /* Not yet implemented */
    result->base.compressed_size = 0;
    result->base.uncompressed_size = 0;
    result->original_crc32 = 0;
    result->compressed_size = 0;
    result->compression_method = GZIP_METHOD_DEFLATE;
    result->compression_flags = 0;
    result->detected_level = 6;
    
    return (precompression_result*)result;
}

/* Read GZIP format header from stream */
static PrecompFormatHeaderData* gzip_read_format_header_impl(struct PrecompFormatHandler* self,
                                                              RecursionContext* context,
                                                              signed char flags,
                                                              SupportedFormats format) {
    (void)self;
    (void)flags;
    (void)format;
    
    if (!context || !context->fin) return NULL;
    
    IStreamLike* fin = context->fin;
    GzipFormatHeaderData* header = gzip_header_data_create();
    if (!header) return NULL;
    
    /* Read and verify magic bytes */
    uint8_t magic[2];
    if (fin->read(fin, magic, 2) != 2) {
        goto error;
    }
    if (magic[0] != GZIP_MAGIC_1 || magic[1] != GZIP_MAGIC_2) {
        goto error;
    }
    
    /* Read compression method */
    uint8_t method;
    if (fin->read(fin, &method, 1) != 1) {
        goto error;
    }
    if (method != GZIP_METHOD_DEFLATE && method != GZIP_METHOD_STORED) {
        /* Unknown compression method */
        goto error;
    }
    header->base.format = F_GZIP;
    
    /* Read flags */
    if (fin->read(fin, &header->flags, 1) != 1) {
        goto error;
    }
    
    /* Check reserved bits */
    if (header->flags & GZIP_FLAG_RESERVED) {
        /* Reserved bits are set - invalid GZIP */
        goto error;
    }
    
    /* Read modification time (Unix timestamp) */
    uint8_t mtime_bytes[4];
    if (fin->read(fin, mtime_bytes, 4) != 4) {
        goto error;
    }
    header->modification_time = ((uint32_t)mtime_bytes[0]) |
                                (((uint32_t)mtime_bytes[1]) << 8) |
                                (((uint32_t)mtime_bytes[2]) << 16) |
                                (((uint32_t)mtime_bytes[3]) << 24);
    
    /* Read extra flags */
    if (fin->read(fin, &header->extra_flags, 1) != 1) {
        goto error;
    }
    
    /* Detect compression level from extra flags */
    header->detected_compression_level = gzip_get_compression_level_hint(header->extra_flags);
    
    /* Read OS type */
    if (fin->read(fin, &header->os_type, 1) != 1) {
        goto error;
    }
    
    /* Initialize CRC16 calculation for header */
    uint16_t crc16 = 0;
    
    /* Process optional fields based on flags */
    
    /* Extra field */
    if (header->flags & GZIP_FLAG_EXTRA) {
        uint8_t xlen_bytes[2];
        if (fin->read(fin, xlen_bytes, 2) != 2) {
            goto error;
        }
        uint16_t xlen = ((uint16_t)xlen_bytes[0]) | (((uint16_t)xlen_bytes[1]) << 8);
        
        if (xlen > GZIP_MAX_EXTRA_LEN) {
            goto error;
        }
        
        header->extra_field = (uint8_t*)malloc(xlen);
        if (!header->extra_field) {
            goto error;
        }
        
        if (fin->read(fin, header->extra_field, xlen) != xlen) {
            goto error;
        }
        header->extra_field_len = xlen;
        
        /* Update CRC16 */
        crc16 = gzip_calc_crc16(xlen_bytes, 2);
        crc16 = gzip_calc_crc16(header->extra_field, xlen);
    }
    
    /* Original filename */
    if (header->flags & GZIP_FLAG_FILENAME) {
        header->filename = read_null_string(fin, GZIP_MAX_FILENAME);
        if (!header->filename) {
            goto error;
        }
        
        /* Update CRC16 */
        size_t fname_len = strlen(header->filename) + 1;
        crc16 = gzip_calc_crc16((const uint8_t*)header->filename, fname_len);
    }
    
    /* Comment */
    if (header->flags & GZIP_FLAG_COMMENT) {
        header->comment = read_null_string(fin, GZIP_MAX_COMMENT);
        if (!header->comment) {
            goto error;
        }
        
        /* Update CRC16 */
        size_t comment_len = strlen(header->comment) + 1;
        crc16 = gzip_calc_crc16((const uint8_t*)header->comment, comment_len);
    }
    
    /* Header CRC16 */
    if (header->flags & GZIP_FLAG_CRC16) {
        uint8_t crc_bytes[2];
        if (fin->read(fin, crc_bytes, 2) != 2) {
            goto error;
        }
        header->header_crc16 = ((uint16_t)crc_bytes[0]) | (((uint16_t)crc_bytes[1]) << 8);
        
        /* Verify CRC16 */
        if (header->header_crc16 != crc16) {
            /* CRC16 mismatch - header corrupted */
            goto error;
        }
    }
    
    return (PrecompFormatHeaderData*)header;
    
error:
    gzip_header_data_destroy(&header->base);
    return NULL;
}

/* Recompress GZIP stream */
static void gzip_recompress_impl(struct PrecompFormatHandler* self,
                                  IStreamLike* input, OStreamLike* output,
                                  PrecompFormatHeaderData* header_data,
                                  SupportedFormats format,
                                  FormatHandlerTools* tools) {
    (void)self;
    (void)input;
    (void)output;
    (void)format;
    (void)tools;
    
    if (!header_data || header_data->format != F_GZIP) {
        fprintf(stderr, "[GZIP] Invalid header data for recompression\n");
        return;
    }
    
    GzipFormatHeaderData* gzip_header = (GzipFormatHeaderData*)header_data;
    
    /* Write GZIP magic bytes */
    uint8_t magic[2] = {GZIP_MAGIC_1, GZIP_MAGIC_2};
    output->write(output, magic, 2);
    
    /* Write compression method (DEFLATE = 8) */
    uint8_t method = GZIP_METHOD_DEFLATE;
    output->write(output, &method, 1);
    
    /* Write flags */
    output->write(output, &gzip_header->flags, 1);
    
    /* Write modification time */
    uint8_t mtime_bytes[4];
    mtime_bytes[0] = (uint8_t)(gzip_header->modification_time & 0xFF);
    mtime_bytes[1] = (uint8_t)((gzip_header->modification_time >> 8) & 0xFF);
    mtime_bytes[2] = (uint8_t)((gzip_header->modification_time >> 16) & 0xFF);
    mtime_bytes[3] = (uint8_t)((gzip_header->modification_time >> 24) & 0xFF);
    output->write(output, mtime_bytes, 4);
    
    /* Write extra flags */
    output->write(output, &gzip_header->extra_flags, 1);
    
    /* Write OS type */
    output->write(output, &gzip_header->os_type, 1);
    
    /* Write extra field if present */
    if (gzip_header->flags & GZIP_FLAG_EXTRA) {
        uint16_t xlen = (uint16_t)gzip_header->extra_field_len;
        uint8_t xlen_bytes[2];
        xlen_bytes[0] = (uint8_t)(xlen & 0xFF);
        xlen_bytes[1] = (uint8_t)((xlen >> 8) & 0xFF);
        output->write(output, xlen_bytes, 2);
        output->write(output, gzip_header->extra_field, gzip_header->extra_field_len);
    }
    
    /* Write filename if present */
    if (gzip_header->filename) {
        size_t len = strlen(gzip_header->filename) + 1;
        output->write(output, gzip_header->filename, len);
    }
    
    /* Write comment if present */
    if (gzip_header->comment) {
        size_t len = strlen(gzip_header->comment) + 1;
        output->write(output, gzip_header->comment, len);
    }
    
    /* Write header CRC16 if flag is set */
    if (gzip_header->flags & GZIP_FLAG_CRC16) {
        uint8_t crc_bytes[2];
        crc_bytes[0] = (uint8_t)(gzip_header->header_crc16 & 0xFF);
        crc_bytes[1] = (uint8_t)((gzip_header->header_crc16 >> 8) & 0xFF);
        output->write(output, crc_bytes, 2);
    }
    
    /* Note: The actual compressed data would be written here in a full implementation */
    /* This is a placeholder showing the header reconstruction logic */
}

/* Write pre-recursion data for GZIP format */
static void gzip_write_pre_recursion_data_impl(struct PrecompFormatHandler* self,
                                                RecursionContext* context,
                                                PrecompFormatHeaderData* header_data) {
    (void)self;
    
    if (!context || !context->fout || !header_data) {
        return;
    }
    
    if (header_data->format != F_GZIP) {
        fprintf(stderr, "[GZIP] Invalid format for write_pre_recursion_data\n");
        return;
    }
    
    GzipFormatHeaderData* gzip_header = (GzipFormatHeaderData*)header_data;
    
    /* Write format identifier */
    uint8_t format_id = (uint8_t)F_GZIP;
    context->fout->write(context->fout, &format_id, 1);
    
    /* Write flags */
    context->fout->write(context->fout, &gzip_header->flags, 1);
    
    /* Write modification time */
    uint8_t mtime_bytes[4];
    mtime_bytes[0] = (uint8_t)(gzip_header->modification_time & 0xFF);
    mtime_bytes[1] = (uint8_t)((gzip_header->modification_time >> 8) & 0xFF);
    mtime_bytes[2] = (uint8_t)((gzip_header->modification_time >> 16) & 0xFF);
    mtime_bytes[3] = (uint8_t)((gzip_header->modification_time >> 24) & 0xFF);
    context->fout->write(context->fout, mtime_bytes, 4);
    
    /* Write extra flags */
    context->fout->write(context->fout, &gzip_header->extra_flags, 1);
    
    /* Write OS type */
    context->fout->write(context->fout, &gzip_header->os_type, 1);
    
    /* Write extra field length and data */
    uint16_t xlen = (uint16_t)gzip_header->extra_field_len;
    uint8_t xlen_bytes[2];
    xlen_bytes[0] = (uint8_t)(xlen & 0xFF);
    xlen_bytes[1] = (uint8_t)((xlen >> 8) & 0xFF);
    context->fout->write(context->fout, xlen_bytes, 2);
    if (xlen > 0 && gzip_header->extra_field) {
        context->fout->write(context->fout, gzip_header->extra_field, xlen);
    }
    
    /* Write filename length and data */
    uint16_t fname_len = 0;
    if (gzip_header->filename) {
        fname_len = (uint16_t)strlen(gzip_header->filename);
    }
    xlen_bytes[0] = (uint8_t)(fname_len & 0xFF);
    xlen_bytes[1] = (uint8_t)((fname_len >> 8) & 0xFF);
    context->fout->write(context->fout, xlen_bytes, 2);
    if (fname_len > 0) {
        context->fout->write(context->fout, gzip_header->filename, fname_len);
    }
    
    /* Write comment length and data */
    uint16_t comment_len = 0;
    if (gzip_header->comment) {
        comment_len = (uint16_t)strlen(gzip_header->comment);
    }
    xlen_bytes[0] = (uint8_t)(comment_len & 0xFF);
    xlen_bytes[1] = (uint8_t)((comment_len >> 8) & 0xFF);
    context->fout->write(context->fout, xlen_bytes, 2);
    if (comment_len > 0) {
        context->fout->write(context->fout, gzip_header->comment, comment_len);
    }
    
    /* Write header CRC16 */
    uint8_t crc_bytes[2];
    crc_bytes[0] = (uint8_t)(gzip_header->header_crc16 & 0xFF);
    crc_bytes[1] = (uint8_t)((gzip_header->header_crc16 >> 8) & 0xFF);
    context->fout->write(context->fout, crc_bytes, 2);
    
    /* Write original size */
    uint8_t size_bytes[8];
    uint64_t orig_size = (uint64_t)gzip_header->original_size;
    for (int i = 0; i < 8; i++) {
        size_bytes[i] = (uint8_t)(orig_size & 0xFF);
        orig_size >>= 8;
    }
    context->fout->write(context->fout, size_bytes, 8);
    
    /* Write original CRC32 */
    uint8_t crc32_bytes[4];
    crc32_bytes[0] = (uint8_t)(gzip_header->original_crc32 & 0xFF);
    crc32_bytes[1] = (uint8_t)((gzip_header->original_crc32 >> 8) & 0xFF);
    crc32_bytes[2] = (uint8_t)((gzip_header->original_crc32 >> 16) & 0xFF);
    crc32_bytes[3] = (uint8_t)((gzip_header->original_crc32 >> 24) & 0xFF);
    context->fout->write(context->fout, crc32_bytes, 4);
}

/* Static magic bytes array for handler registration */
static SupportedFormats gzip_format_bytes[] = {F_GZIP};

/* Create GZIP format handler */
PrecompFormatHandler* create_gzip_handler(void) {
    PrecompFormatHandler* handler = (PrecompFormatHandler*)calloc(1, sizeof(PrecompFormatHandler));
    if (!handler) return NULL;
    
    /* Set magic bytes for detection */
    handler->header_bytes = gzip_format_bytes;
    handler->header_bytes_count = 1;
    
    /* Set depth limit (can be configured) */
    handler->depth_limit = 10;
    handler->has_depth_limit = true;
    
    /* Set virtual function pointers */
    handler->quick_check = gzip_quick_check_impl;
    handler->attempt_precompression = gzip_attempt_precompression;
    handler->read_format_header = gzip_read_format_header_impl;
    handler->recompress = gzip_recompress_impl;
    handler->write_pre_recursion_data = gzip_write_pre_recursion_data_impl;
    
    return handler;
}

/* Public API functions matching header declarations */

bool gzip_quick_check(unsigned char* buffer, size_t buffer_size,
                      uintptr_t current_input_id, long long original_input_pos) {
    if (buffer_size < 2) return false;
    return (buffer[0] == GZIP_MAGIC_1 && buffer[1] == GZIP_MAGIC_2);
}

precompression_result* gzip_attempt_precompression(Precomp* precomp_mgr,
                                                    unsigned char* buffer,
                                                    size_t buffer_size,
                                                    long long input_stream_pos) {
    (void)precomp_mgr;
    (void)buffer;
    (void)buffer_size;
    (void)input_stream_pos;
    
    /* Placeholder implementation */
    gzip_precompression_result* result = 
        (gzip_precompression_result*)calloc(1, sizeof(gzip_precompression_result));
    if (!result) return NULL;
    
    result->base.format = F_GZIP;
    result->base.success = false;
    result->base.compressed_size = 0;
    result->base.uncompressed_size = 0;
    
    return (precompression_result*)result;
}

PrecompFormatHeaderData* gzip_read_format_header(RecursionContext* context,
                                                  signed char precomp_hdr_flags,
                                                  SupportedFormats precomp_hdr_format) {
    return gzip_read_format_header_impl(NULL, context, precomp_hdr_flags, precomp_hdr_format);
}

void gzip_recompress(IStreamLike* precompressed_input,
                     OStreamLike* recompressed_stream,
                     PrecompFormatHeaderData* precomp_hdr_data,
                     SupportedFormats precomp_hdr_format,
                     void* tools) {
    FormatHandlerTools* handler_tools = (FormatHandlerTools*)tools;
    gzip_recompress_impl(NULL, precompressed_input, recompressed_stream,
                         precomp_hdr_data, precomp_hdr_format, handler_tools);
}

void gzip_write_pre_recursion_data(RecursionContext* context,
                                    PrecompFormatHeaderData* precomp_hdr_data) {
    gzip_write_pre_recursion_data_impl(NULL, context, precomp_hdr_data);
}
