/*
 * Precomp C Library - Header for TCC compilation
 * This provides a simplified C API compatible with Tiny C Compiler
 */

#ifndef PRECOMP_LIB_H
#define PRECOMP_LIB_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============ Error Codes ============ */
#define PCOMP_SUCCESS 0
#define PCOMP_ERR_GENERIC 1
#define PCOMP_ERR_NOTHING_DECOMPRESSED 2
#define PCOMP_ERR_DISK_FULL 3
#define PCOMP_ERR_TEMP_FILE_MISSING 4
#define PCOMP_ERR_NO_HEADER 20
#define PCOMP_ERR_INCOMPATIBLE_VERSION 21

/* ============ Logging Levels ============ */
typedef enum {
    PCOMP_LOG_NORMAL = 0,
    PCOMP_LOG_DEBUG = 1
} PcompLogLevel;

/* ============ Opaque Types ============ */
typedef struct PcompInstance PcompInstance;
typedef struct PcompSwitches PcompSwitches;
typedef struct PcompStatistics PcompStatistics;

/* ============ Callback Types ============ */
typedef void (*PcompProgressCallback)(float progress);
typedef void (*PcompLogCallback)(PcompLogLevel level, const char* message);

/* ============ Core Functions ============ */

/* Create/Destroy instance */
PcompInstance* pcomp_create(void);
void pcomp_destroy(PcompInstance* inst);

/* Get configuration structures */
PcompSwitches* pcomp_get_switches(PcompInstance* inst);
PcompStatistics* pcomp_get_statistics(PcompInstance* inst);

/* Set callbacks */
void pcomp_set_progress_callback(PcompInstance* inst, PcompProgressCallback cb);
void pcomp_set_log_callback(PcompLogCallback cb);

/* Set input/output using FILE* */
int pcomp_set_input_file(PcompInstance* inst, FILE* file, const char* name);
int pcomp_set_output_file(PcompInstance* inst, FILE* file, const char* name);

/* Set input/output using generic pointers (for custom streams) */
typedef size_t (*pcomp_read_func)(void* userdata, char* buf, size_t count);
typedef int (*pcomp_get_func)(void* userdata);
typedef int (*pcomp_seek_func)(void* userdata, long long offset, int origin);
typedef long long (*pcomp_tell_func)(void* userdata);
typedef bool (*pcomp_eof_func)(void* userdata);
typedef bool (*pcomp_bad_func)(void* userdata);
typedef void (*pcomp_clear_func)(void* userdata);

int pcomp_set_generic_input(
    PcompInstance* inst,
    void* userdata,
    pcomp_read_func read_fn,
    pcomp_get_func get_fn,
    pcomp_seek_func seek_fn,
    pcomp_tell_func tell_fn,
    pcomp_eof_func eof_fn,
    pcomp_bad_func bad_fn,
    pcomp_clear_func clear_fn,
    const char* name
);

int pcomp_set_generic_output(
    PcompInstance* inst,
    void* userdata,
    size_t (*write_fn)(void*, const char*, size_t),
    int (*put_fn)(void*, int),
    pcomp_seek_func seek_fn,
    pcomp_tell_func tell_fn,
    pcomp_eof_func eof_fn,
    pcomp_bad_func bad_fn,
    pcomp_clear_func clear_fn,
    const char* name
);

/* Main operations */
int pcomp_precompress(PcompInstance* inst);
int pcomp_recompress(PcompInstance* inst);
int pcomp_read_header(PcompInstance* inst, int seek_to_beginning);

/* Get output filename after reading header */
const char* pcomp_get_output_filename(PcompInstance* inst);

/* ============ Switch Configuration ============ */

/* Access individual switch fields */
bool pcomp_switch_get_debug(PcompSwitches* sw);
void pcomp_switch_set_debug(PcompSwitches* sw, bool val);

bool pcomp_switch_get_intense(PcompSwitches* sw);
void pcomp_switch_set_intense(PcompSwitches* sw, bool val);

bool pcomp_switch_get_brute(PcompSwitches* sw);
void pcomp_switch_set_brute(PcompSwitches* sw, bool val);

bool pcomp_switch_get_pdf(PcompSwitches* sw);
void pcomp_switch_set_pdf(PcompSwitches* sw, bool val);

bool pcomp_switch_get_zip(PcompSwitches* sw);
void pcomp_switch_set_zip(PcompSwitches* sw, bool val);

bool pcomp_switch_get_gzip(PcompSwitches* sw);
void pcomp_switch_set_gzip(PcompSwitches* sw, bool val);

bool pcomp_switch_get_png(PcompSwitches* sw);
void pcomp_switch_set_png(PcompSwitches* sw, bool val);

bool pcomp_switch_get_gif(PcompSwitches* sw);
void pcomp_switch_set_gif(PcompSwitches* sw, bool val);

bool pcomp_switch_get_jpg(PcompSwitches* sw);
void pcomp_switch_set_jpg(PcompSwitches* sw, bool val);

bool pcomp_switch_get_mp3(PcompSwitches* sw);
void pcomp_switch_set_mp3(PcompSwitches* sw, bool val);

bool pcomp_switch_get_swf(PcompSwitches* sw);
void pcomp_switch_set_swf(PcompSwitches* sw, bool val);

/* ============ Switch Getters/Setters ============ */

bool pcomp_switch_get_pdf(PcompSwitches* sw);
void pcomp_switch_set_pdf(PcompSwitches* sw, bool val);

bool pcomp_switch_get_zip(PcompSwitches* sw);
void pcomp_switch_set_zip(PcompSwitches* sw, bool val);

bool pcomp_switch_get_gzip(PcompSwitches* sw);
void pcomp_switch_set_gzip(PcompSwitches* sw, bool val);

bool pcomp_switch_get_png(PcompSwitches* sw);
void pcomp_switch_set_png(PcompSwitches* sw, bool val);

bool pcomp_switch_get_gif(PcompSwitches* sw);
void pcomp_switch_set_gif(PcompSwitches* sw, bool val);

bool pcomp_switch_get_jpg(PcompSwitches* sw);
void pcomp_switch_set_jpg(PcompSwitches* sw, bool val);

bool pcomp_switch_get_swf(PcompSwitches* sw);
void pcomp_switch_set_swf(PcompSwitches* sw, bool val);

bool pcomp_switch_get_base64(PcompSwitches* sw);
void pcomp_switch_set_base64(PcompSwitches* sw, bool val);

bool pcomp_switch_get_bzip2(PcompSwitches* sw);
void pcomp_switch_set_bzip2(PcompSwitches* sw, bool val);

bool pcomp_switch_get_mp3(PcompSwitches* sw);
void pcomp_switch_set_mp3(PcompSwitches* sw, bool val);

unsigned int pcomp_switch_get_min_ident_size(PcompSwitches* sw);
void pcomp_switch_set_min_ident_size(PcompSwitches* sw, unsigned int val);

int pcomp_switch_get_max_recursion(PcompSwitches* sw);
void pcomp_switch_set_max_recursion(PcompSwitches* sw, int val);

/* Direct struct access macros for C convenience (optional) */
#define PCOMP_SWITCH_GET_FIELD(sw, field) ((sw)->field)
#define PCOMP_SWITCH_SET_FIELD(sw, field, val) do { (sw)->field = (val); } while(0)

/* Set ignore positions list */
void pcomp_switch_set_ignore_list(PcompSwitches* sw, const long long* list, size_t count);

/* ============ Utility Functions ============ */

/* Get file size helper */
long long pcomp_file_size(const char* filename, int* error_code);

/* Print to terminal (separate from stdout for binary mode) */
void pcomp_print_terminal(const char* fmt, ...);

/* Set std handle binary mode (Windows only, no-op on Unix) */
typedef enum {
    PCOMP_STDIN = 0,
    PCOMP_STDOUT = 1,
    PCOMP_STDERR = 2
} PcompStdHandle;

void pcomp_set_handle_binary_mode(PcompStdHandle handle);

/* Get copyright message */
void pcomp_get_copyright_msg(char* buffer);

/* Get error message string */
const char* pcomp_error_message(int error_code, const char* extra_info);

#ifdef __cplusplus
}
#endif

#endif /* PRECOMP_LIB_H */
