#ifndef PRECOMP_UTILS_H
#define PRECOMP_UTILS_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

/* Error codes */
#define RETURN_SUCCESS 0
#define ERR_GENERIC_OR_UNKNOWN 1
#define RETURN_NOTHING_DECOMPRESSED 2
#define ERR_DISK_FULL 3
#define ERR_TEMP_FILE_DISAPPEARED 4
#define ERR_IGNORE_POS_TOO_BIG 5
#define ERR_IDENTICAL_BYTE_SIZE_TOO_BIG 6
#define ERR_RECURSION_DEPTH_TOO_BIG 7
#define ERR_ONLY_SET_RECURSION_DEPTH_ONCE 8
#define ERR_ONLY_SET_MIN_SIZE_ONCE 9
#define ERR_DONT_USE_SPACE 10
#define ERR_MORE_THAN_ONE_OUTPUT_FILE 11
#define ERR_MORE_THAN_ONE_INPUT_FILE 12
#define ERR_CTRL_C 13
#define ERR_INTENSE_MODE_LIMIT_TOO_BIG 14
#define ERR_BRUTE_MODE_LIMIT_TOO_BIG 15
#define ERR_DURING_RECOMPRESSION 19
#define ERR_NO_PCF_HEADER 20
#define ERR_PCF_HEADER_INCOMPATIBLE_VERSION 21
#define ERR_BROTLI_NO_LONGER_SUPPORTED 22

/* Logging levels */
typedef enum {
    PRECOMP_NORMAL_LOG = 0,
    PRECOMP_DEBUG_LOG = 1
} PrecompLoggingLevels;

/* PrecompError structure (replaces C++ exception class) */
typedef struct {
    int error_code;
    char* extra_info;
} PrecompError;

/* Function declarations */
char* temp_files_tag(void);
unsigned int auto_detected_thread_count(void);
void print_to_console(const char* format);
void print_to_console_fmt(const char* format, ...);
char get_char_with_echo(void);
const char* precomp_error_msg(int error_nr, const char* extra_info);
long long get_time_ms(void);

/* Memory management helpers */
void precomp_utils_init(void);
void precomp_utils_cleanup(void);

#endif /* PRECOMP_UTILS_H */
