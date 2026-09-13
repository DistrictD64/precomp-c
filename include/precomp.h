/*
 * Precomp Main Header - C Version
 * Converted from C++ to C for Tiny C Compiler compatibility
 */

#ifndef PRECOMP_H
#define PRECOMP_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "precomp_io.h"

/* Supported formats enumeration */
typedef enum {
    F_NONE = 0,
    F_PDF = 1,
    F_ZIP = 2,
    F_GZIP = 3,
    F_PNG = 4,
    F_GIF = 5,
    F_JPG = 6,
    F_MP3 = 7,
    F_SWF = 8,
    F_BASE64 = 9,
    F_BZIP2 = 10,
    F_ZLIB = 11,
    F_BRUTE = 12,
    D_BRUTE = 13,
    F_WAV = 14,
    FORMAT_COUNT
} SupportedFormats;

/* Precompression result base structure */
typedef struct precompression_result {
    SupportedFormats format;
    long long compressed_size;
    long long uncompressed_size;
    bool success;
    
    /* Virtual function pointers */
    void (*dump_header)(struct precompression_result* self, OStreamLike* outfile);
    void (*dump_data)(struct precompression_result* self, OStreamLike* outfile);
} precompression_result;

/* Recursion context */
typedef struct {
    uintmax_t fin_length;
    bool anything_was_used;
    bool non_zlib_was_used;
    IStreamLike* fin;
    OStreamLike* fout;
    int recursion_depth;
} RecursionContext;

/* Format handler tools */
typedef struct {
    void (*progress_callback)(float);
} FormatHandlerTools;

/* Precomp format header data base */
typedef struct PrecompFormatHeaderData {
    SupportedFormats format;
    void (*destroy)(struct PrecompFormatHeaderData* self);
} PrecompFormatHeaderData;

/* Precomp format handler (C-style vtable) */
typedef struct PrecompFormatHandler {
    /* Properties */
    SupportedFormats* header_bytes;
    unsigned int header_bytes_count;
    unsigned int depth_limit;
    bool has_depth_limit;
    
    /* Virtual methods */
    bool (*quick_check)(struct PrecompFormatHandler* self, unsigned char* buffer, 
                        uintptr_t input_id, long long pos);
    precompression_result* (*attempt_precompression)(struct PrecompFormatHandler* self,
                                                      void* precomp_instance,
                                                      unsigned char* buffer,
                                                      long long pos);
    PrecompFormatHeaderData* (*read_format_header)(struct PrecompFormatHandler* self,
                                                    RecursionContext* context,
                                                    signed char flags,
                                                    SupportedFormats format);
    void (*recompress)(struct PrecompFormatHandler* self,
                       IStreamLike* input, OStreamLike* output,
                       PrecompFormatHeaderData* header_data,
                       SupportedFormats format,
                       FormatHandlerTools* tools);
    void (*write_pre_recursion_data)(struct PrecompFormatHandler* self,
                                      RecursionContext* context,
                                      PrecompFormatHeaderData* header_data);
} PrecompFormatHandler;

/* Switches structure (from libprecomp.h adapted) */
typedef struct {
    bool DEBUG_MODE;
    bool verify_precompressed;
    uintmax_t uncompressed_block_length;
    bool intense_mode;
    int intense_mode_depth_limit;
    bool brute_mode;
    int brute_mode_depth_limit;
    bool pdf_bmp_mode;
    bool prog_only;
    bool use_mjpeg;
    bool use_brunsli;
    bool use_packjpg_fallback;
    unsigned int min_ident_size;
    char* working_dir;
    bool use_pdf;
    bool use_zip;
    bool use_gzip;
    bool use_png;
    bool use_gif;
    bool use_jpg;
    bool use_mp3;
    bool use_swf;
    bool use_base64;
    bool use_bzip2;
    bool level_switch_used;
    size_t preflate_meta_block_size;
    bool preflate_verify;
    int max_recursion_depth;
    /* Additional C-specific fields */
    long long* ignore_pos_list;
    size_t ignore_pos_list_count;
} CSwitches;

/* Result statistics */
typedef struct {
    unsigned int recompressed_streams_count;
    unsigned int recompressed_pdf_count;
    unsigned int recompressed_pdf_count_8_bit;
    unsigned int recompressed_pdf_count_24_bit;
    unsigned int recompressed_zip_count;
    unsigned int recompressed_gzip_count;
    unsigned int recompressed_png_count;
    unsigned int recompressed_png_multi_count;
    unsigned int recompressed_gif_count;
    unsigned int recompressed_jpg_count;
    unsigned int recompressed_jpg_prog_count;
    unsigned int recompressed_mp3_count;
    unsigned int recompressed_swf_count;
    unsigned int recompressed_base64_count;
    unsigned int recompressed_bzip2_count;
    unsigned int recompressed_zlib_count;
    unsigned int recompressed_brute_count;
    unsigned int recompressed_wav_count;
    
    unsigned int decompressed_streams_count;
    unsigned int decompressed_pdf_count;
    unsigned int decompressed_pdf_count_8_bit;
    unsigned int decompressed_pdf_count_24_bit;
    unsigned int decompressed_zip_count;
    unsigned int decompressed_gzip_count;
    unsigned int decompressed_png_count;
    unsigned int decompressed_png_multi_count;
    unsigned int decompressed_gif_count;
    unsigned int decompressed_jpg_count;
    unsigned int decompressed_jpg_prog_count;
    unsigned int decompressed_mp3_count;
    unsigned int decompressed_swf_count;
    unsigned int decompressed_base64_count;
    unsigned int decompressed_bzip2_count;
    unsigned int decompressed_zlib_count;
    unsigned int decompressed_brute_count;
    unsigned int decompressed_wav_count;
    
    int max_recursion_depth_used;
    bool max_recursion_depth_reached;
    bool header_already_read;
} CResultStatistics;

/* Main Precomp structure (opaque) */
typedef struct Precomp {
    CSwitches switches;
    RecursionContext recursion_context;
    CResultStatistics statistics;
    IStreamLike* input_stream;
    OStreamLike* output_stream;
    char* input_filename;
    char* output_filename;
    void (*progress_callback)(float);
    PrecompFormatHandler** format_handlers;
    int format_handlers_count;
    int last_detected_level;  /* Last detected compression level */
} Precomp;

/* Function declarations */

/* Constructor/Destructor */
Precomp* PrecompCreate(void);
void PrecompDestroy(Precomp* precomp_mgr);

/* Configuration */
CSwitches* PrecompGetSwitches(Precomp* precomp_mgr);
void PrecompSwitchesSetIgnoreList(CSwitches* switches, const long long* ignore_list, size_t count);
RecursionContext* PrecompGetRecursionContext(Precomp* precomp_mgr);
CResultStatistics* PrecompGetResultStatistics(Precomp* precomp_mgr);

/* I/O Setup */
void PrecompSetInputStream(Precomp* precomp_mgr, IStreamLike* istream, const char* input_filename);
void PrecompSetInputFile(Precomp* precomp_mgr, FILE* fhandle, const char* input_filename);
void PrecompSetOutputStream(Precomp* precomp_mgr, OStreamLike* ostream, const char* output_filename);
void PrecompSetOutputFile(Precomp* precomp_mgr, FILE* fhandle, const char* output_filename);
void PrecompSetProgressCallback(Precomp* precomp_mgr, void (*callback)(float));

/* Operations */
int PrecompPrecompress(Precomp* precomp_mgr);
int PrecompRecompress(Precomp* precomp_mgr);
int PrecompReadHeader(Precomp* precomp_mgr, bool seek_to_beg);
const char* PrecompGetOutputFilename(Precomp* precomp_mgr);

/* Format handlers */
PrecompFormatHandler* create_deflate_handler(void);
PrecompFormatHandler* create_pdf_handler(void);
PrecompFormatHandler* create_png_handler(void);
PrecompFormatHandler* create_jpeg_handler(void);
PrecompFormatHandler* create_gif_handler(void);
PrecompFormatHandler* create_mp3_handler(void);
PrecompFormatHandler* create_bzip2_handler(void);
PrecompFormatHandler* create_wav_handler(void);

#endif /* PRECOMP_H */
