/*
 * Precomp C Library - Implementation for TCC
 * Simplified wrapper around the core functionality
 */

#include "precomp_lib.h"
#include "precomp.h"
#include "precomp_utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

/* ============ Internal Structure Definitions ============ */

struct PcompSwitches {
    bool debug_mode;
    bool verify_precompressed;
    uintmax_t block_length;
    bool intense_mode;
    int intense_depth_limit;
    bool brute_mode;
    int brute_depth_limit;
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
    long long* ignore_pos_list;
    size_t ignore_pos_count;
};

struct PcompStatistics {
    unsigned int recompressed_streams;
    unsigned int recompressed_pdf;
    unsigned int recompressed_zip;
    unsigned int recompressed_gzip;
    unsigned int recompressed_png;
    unsigned int recompressed_gif;
    unsigned int recompressed_jpg;
    unsigned int recompressed_mp3;
    unsigned int recompressed_swf;
    unsigned int recompressed_base64;
    unsigned int recompressed_bzip2;
    
    unsigned int decompressed_streams;
    unsigned int decompressed_pdf;
    unsigned int decompressed_zip;
    unsigned int decompressed_gzip;
    unsigned int decompressed_png;
    unsigned int decompressed_gif;
    unsigned int decompressed_jpg;
    unsigned int decompressed_mp3;
    unsigned int decompressed_swf;
    unsigned int decompressed_base64;
    unsigned int decompressed_bzip2;
    
    int max_recursion_used;
    bool max_recursion_reached;
    bool header_read;
};

struct PcompInstance {
    Precomp* internal;
    PcompSwitches switches;
    PcompStatistics stats;
    PcompProgressCallback progress_cb;
    PcompLogCallback log_cb;
};

/* Global log callback */
static PcompLogCallback g_log_callback = NULL;

/* ============ Helper Functions ============ */

static void default_progress_callback(float progress) {
    (void)progress;  /* Unused in default */
}

static void internal_log_callback(PcompLogLevel level, char* msg) {
    if (g_log_callback != NULL) {
        g_log_callback(level, msg);
    }
}

/* ============ Core Functions ============ */

PcompInstance* pcomp_create(void) {
    PcompInstance* inst = (PcompInstance*)malloc(sizeof(PcompInstance));
    if (inst == NULL) return NULL;
    
    memset(inst, 0, sizeof(PcompInstance));
    
    /* Create internal Precomp instance */
    inst->internal = PrecompCreate();
    if (inst->internal == NULL) {
        free(inst);
        return NULL;
    }
    
    /* Initialize switches with defaults */
    inst->switches.debug_mode = false;
    inst->switches.verify_precompressed = true;
    inst->switches.block_length = 100 * 1024 * 1024;
    inst->switches.intense_mode = false;
    inst->switches.intense_depth_limit = 9;
    inst->switches.brute_mode = false;
    inst->switches.brute_depth_limit = 12;
    inst->switches.pdf_bmp_mode = false;
    inst->switches.prog_only = false;
    inst->switches.use_mjpeg = true;
    inst->switches.use_brunsli = true;
    inst->switches.use_packjpg_fallback = true;
    inst->switches.min_ident_size = 4;
    inst->switches.working_dir = NULL;
    inst->switches.use_pdf = true;
    inst->switches.use_zip = true;
    inst->switches.use_gzip = true;
    inst->switches.use_png = true;
    inst->switches.use_gif = true;
    inst->switches.use_jpg = true;
    inst->switches.use_mp3 = true;
    inst->switches.use_swf = true;
    inst->switches.use_base64 = true;
    inst->switches.use_bzip2 = true;
    inst->switches.level_switch_used = false;
    inst->switches.preflate_meta_block_size = 65536;
    inst->switches.preflate_verify = true;
    inst->switches.max_recursion_depth = -1;
    inst->switches.ignore_pos_list = NULL;
    inst->switches.ignore_pos_count = 0;
    
    inst->progress_cb = default_progress_callback;
    inst->log_cb = NULL;
    
    /* Note: PrecompSetLoggingCallback would be implemented in full version */
    /* For now, logging is handled internally */
    
    return inst;
}

void pcomp_destroy(PcompInstance* inst) {
    if (inst == NULL) return;
    
    if (inst->internal != NULL) {
        PrecompDestroy(inst->internal);
    }
    
    if (inst->switches.working_dir != NULL) {
        free(inst->switches.working_dir);
    }
    
    if (inst->switches.ignore_pos_list != NULL) {
        free(inst->switches.ignore_pos_list);
    }
    
    free(inst);
}

PcompSwitches* pcomp_get_switches(PcompInstance* inst) {
    if (inst == NULL) return NULL;
    return &inst->switches;
}

PcompStatistics* pcomp_get_statistics(PcompInstance* inst) {
    if (inst == NULL) return NULL;
    return &inst->stats;
}

void pcomp_set_progress_callback(PcompInstance* inst, PcompProgressCallback cb) {
    if (inst == NULL) return;
    inst->progress_cb = cb;
    
    if (inst->internal != NULL) {
        PrecompSetProgressCallback(inst->internal, cb);
    }
}

void pcomp_set_log_callback(PcompLogCallback cb) {
    g_log_callback = cb;
}

int pcomp_set_input_file(PcompInstance* inst, FILE* file, const char* name) {
    if (inst == NULL || inst->internal == NULL || file == NULL) return -1;
    PrecompSetInputFile(inst->internal, file, name);
    return 0;
}

int pcomp_set_output_file(PcompInstance* inst, FILE* file, const char* name) {
    if (inst == NULL || inst->internal == NULL || file == NULL) return -1;
    PrecompSetOutputFile(inst->internal, file, name);
    return 0;
}

/* Generic input/output would require implementing the stream wrappers */
int pcomp_set_generic_input(PcompInstance* inst, void* userdata,
    pcomp_read_func read_fn, pcomp_get_func get_fn,
    pcomp_seek_func seek_fn, pcomp_tell_func tell_fn,
    pcomp_eof_func eof_fn, pcomp_bad_func bad_fn,
    pcomp_clear_func clear_fn, const char* name) {
    
    if (inst == NULL || inst->internal == NULL) return -1;
    
    /* TODO: Implement generic stream wrapper */
    (void)userdata; (void)read_fn; (void)get_fn; (void)seek_fn;
    (void)tell_fn; (void)eof_fn; (void)bad_fn; (void)clear_fn; (void)name;
    
    return -1;  /* Not yet implemented */
}

int pcomp_set_generic_output(PcompInstance* inst, void* userdata,
    size_t (*write_fn)(void*, const char*, size_t),
    int (*put_fn)(void*, int),
    pcomp_seek_func seek_fn, pcomp_tell_func tell_fn,
    pcomp_eof_func eof_fn, pcomp_bad_func bad_fn,
    pcomp_clear_func clear_fn, const char* name) {
    
    if (inst == NULL || inst->internal == NULL) return -1;
    
    /* TODO: Implement generic stream wrapper */
    (void)userdata; (void)write_fn; (void)put_fn; (void)seek_fn;
    (void)tell_fn; (void)eof_fn; (void)bad_fn; (void)clear_fn; (void)name;
    
    return -1;  /* Not yet implemented */
}

int pcomp_precompress(PcompInstance* inst) {
    if (inst == NULL || inst->internal == NULL) return PCOMP_ERR_GENERIC;
    return PrecompPrecompress(inst->internal);
}

int pcomp_recompress(PcompInstance* inst) {
    if (inst == NULL || inst->internal == NULL) return PCOMP_ERR_GENERIC;
    return PrecompRecompress(inst->internal);
}

int pcomp_read_header(PcompInstance* inst, int seek_to_beginning) {
    if (inst == NULL || inst->internal == NULL) return PCOMP_ERR_NO_HEADER;
    return PrecompReadHeader(inst->internal, seek_to_beginning != 0);
}

const char* pcomp_get_output_filename(PcompInstance* inst) {
    if (inst == NULL || inst->internal == NULL) return NULL;
    return PrecompGetOutputFilename(inst->internal);
}

/* ============ Switch Accessors ============ */

#define SWITCH_GETSET(type, name, field) \
    type pcomp_switch_get_##name(PcompSwitches* sw) { \
        return (sw != NULL) ? sw->field : 0; \
    } \
    void pcomp_switch_set_##name(PcompSwitches* sw, type val) { \
        if (sw != NULL) sw->field = val; \
    }

SWITCH_GETSET(bool, debug, debug_mode)
SWITCH_GETSET(bool, intense, intense_mode)
SWITCH_GETSET(bool, brute, brute_mode)
SWITCH_GETSET(bool, pdf, use_pdf)
SWITCH_GETSET(bool, zip, use_zip)
SWITCH_GETSET(bool, gzip, use_gzip)
SWITCH_GETSET(bool, png, use_png)
SWITCH_GETSET(bool, gif, use_gif)
SWITCH_GETSET(bool, jpg, use_jpg)
SWITCH_GETSET(bool, mp3, use_mp3)
SWITCH_GETSET(bool, swf, use_swf)
SWITCH_GETSET(bool, base64, use_base64)
SWITCH_GETSET(bool, bzip2, use_bzip2)
SWITCH_GETSET(unsigned int, min_ident_size, min_ident_size)
SWITCH_GETSET(int, max_recursion, max_recursion_depth)

void pcomp_switch_set_ignore_list(PcompSwitches* sw, const long long* list, size_t count) {
    if (sw == NULL) return;
    
    if (sw->ignore_pos_list != NULL) {
        free(sw->ignore_pos_list);
    }
    
    if (list == NULL || count == 0) {
        sw->ignore_pos_list = NULL;
        sw->ignore_pos_count = 0;
        return;
    }
    
    sw->ignore_pos_list = (long long*)malloc(count * sizeof(long long));
    if (sw->ignore_pos_list != NULL) {
        memcpy(sw->ignore_pos_list, list, count * sizeof(long long));
        sw->ignore_pos_count = count;
    }
}

/* ============ Utility Functions ============ */

long long pcomp_file_size(const char* filename, int* error_code) {
    if (filename == NULL) {
        if (error_code != NULL) *error_code = -1;
        return -1;
    }
    
    FILE* f = fopen(filename, "rb");
    if (f == NULL) {
        if (error_code != NULL) *error_code = -1;
        return -1;
    }
    
    fseek(f, 0, SEEK_END);
    long long size = (long long)ftell(f);
    fclose(f);
    
    if (error_code != NULL) *error_code = 0;
    return size;
}

void pcomp_print_terminal(const char* fmt, ...) {
    char buffer[4096];
    va_list args;
    
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    
    print_to_console(buffer);
}

void pcomp_set_handle_binary_mode(PcompStdHandle handle) {
#ifdef _WIN32
    HANDLE h = NULL;
    switch (handle) {
        case PCOMP_STDIN:
            h = GetStdHandle(STD_INPUT_HANDLE);
            break;
        case PCOMP_STDOUT:
            h = GetStdHandle(STD_OUTPUT_HANDLE);
            break;
        case PCOMP_STDERR:
            h = GetStdHandle(STD_ERROR_HANDLE);
            break;
    }
    if (h != NULL && h != INVALID_HANDLE_VALUE) {
        SetConsoleMode(h, ENABLE_BINARY_INPUT);
    }
#else
    /* Unix: already binary by default */
    (void)handle;
#endif
}

void pcomp_get_copyright_msg(char* buffer) {
    if (buffer == NULL) return;
    snprintf(buffer, 256, "Precomp C Library - Converted from precomp-cpp");
}

const char* pcomp_error_message(int error_code, const char* extra_info) {
    return precomp_error_msg(error_code, extra_info);
}
