/*
 * Precomp Utils - C implementation
 * Converted from C++ to C for Tiny C Compiler compatibility
 */

#include "precomp_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

#ifndef _WIN32
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#else
#include <windows.h>
#endif

/* Static variables */
static int ttyfd = -1;
static int initialized = 0;

/* Initialize utils module */
void precomp_utils_init(void) {
    if (!initialized) {
        initialized = 1;
    }
}

/* Cleanup utils module */
void precomp_utils_cleanup(void) {
    if (ttyfd >= 0) {
#ifndef _WIN32
        close(ttyfd);
#endif
        ttyfd = -1;
    }
    initialized = 0;
}

/* Generate a random 8-digit hex tag for temp files */
char* temp_files_tag(void) {
    static char tag[9];
    static int seeded = 0;
    
    if (!seeded) {
        srand((unsigned int)time(NULL));
        seeded = 1;
    }
    
    for (int i = 0; i < 8; i++) {
        int digit = rand() % 16;
        tag[i] = (digit < 10) ? ('0' + digit) : ('a' + (digit - 10));
    }
    tag[8] = '\0';
    
    return tag;
}

/* Auto-detect thread count (simplified for C) */
unsigned int auto_detected_thread_count(void) {
#ifdef _WIN32
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    return (sysinfo.dwNumberOfProcessors > 0) ? sysinfo.dwNumberOfProcessors : 2;
#else
    /* On Unix, try to get from sysconf, default to 2 */
    long nprocs = sysconf(_SC_NPROCESSORS_ONLN);
    return (nprocs > 0) ? (unsigned int)nprocs : 2;
#endif
}

/* Print to console (for stdout mode separation) */
void print_to_console(const char* format) {
    if (format == NULL) return;
    
#ifdef _WIN32
    /* Windows: use console output */
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD written;
    WriteConsoleA(hConsole, format, (DWORD)strlen(format), &written, NULL);
#else
    /* Unix: write to /dev/tty */
    if (ttyfd < 0) {
        ttyfd = open("/dev/tty", O_RDWR);
    }
    if (ttyfd >= 0) {
        write(ttyfd, format, strlen(format));
    } else {
        /* Fallback to stderr if /dev/tty not available */
        fprintf(stderr, "%s", format);
    }
#endif
}

/* Print to console with printf-style formatting */
void print_to_console_fmt(const char* format, ...) {
    char buffer[4096];
    va_list args;
    
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    print_to_console(buffer);
}

/* Get character with echo */
char get_char_with_echo(void) {
#ifdef _WIN32
    /* Windows: use conio.h if available, or fallback */
    #ifdef __MINGW32__
        #include <conio.h>
        return getch();
    #else
        return fgetc(stdin);
    #endif
#else
    /* Unix: just read from stdin */
    return (char)fgetc(stdin);
#endif
}

/* Get error message string */
const char* precomp_error_msg(int error_nr, const char* extra_info) {
    static char msg_buffer[512];
    const char* base_msg;
    
    switch (error_nr) {
        case ERR_IGNORE_POS_TOO_BIG:
            base_msg = "Ignore position too big";
            break;
        case ERR_IDENTICAL_BYTE_SIZE_TOO_BIG:
            base_msg = "Identical bytes size bigger than 4 GB";
            break;
        case ERR_ONLY_SET_MIN_SIZE_ONCE:
            base_msg = "Minimal identical size can only be set once";
            break;
        case ERR_MORE_THAN_ONE_OUTPUT_FILE:
            base_msg = "More than one output file given";
            break;
        case ERR_MORE_THAN_ONE_INPUT_FILE:
            base_msg = "More than one input file given";
            break;
        case ERR_DONT_USE_SPACE:
            base_msg = "Please don't use a space between the -o switch and the output filename";
            break;
        case ERR_TEMP_FILE_DISAPPEARED:
            base_msg = "Temporary file has disappeared";
            break;
        case ERR_DISK_FULL:
            base_msg = "There is not enough space on disk";
            break;
        case ERR_RECURSION_DEPTH_TOO_BIG:
            base_msg = "Recursion depth too big";
            break;
        case ERR_ONLY_SET_RECURSION_DEPTH_ONCE:
            base_msg = "Recursion depth can only be set once";
            break;
        case ERR_CTRL_C:
            base_msg = "CTRL-C detected";
            break;
        case ERR_INTENSE_MODE_LIMIT_TOO_BIG:
            base_msg = "Intense mode level limit too big";
            break;
        case ERR_BRUTE_MODE_LIMIT_TOO_BIG:
            base_msg = "Brute mode level limit too big";
            break;
        case ERR_DURING_RECOMPRESSION:
            base_msg = "Error recompressing data!";
            break;
        case ERR_NO_PCF_HEADER:
            base_msg = "Input stream has no valid PCF header";
            break;
        case ERR_PCF_HEADER_INCOMPATIBLE_VERSION:
            base_msg = "Input stream was made with an incompatible Precomp version";
            break;
        case ERR_BROTLI_NO_LONGER_SUPPORTED:
            base_msg = "Precompressed stream has a precompressed JPG using Brunsli with Brotli metadata compression, Brotli is no longer supported by precomp";
            break;
        default:
            base_msg = "Unknown error";
            break;
    }
    
    if (extra_info != NULL && strlen(extra_info) > 0) {
        snprintf(msg_buffer, sizeof(msg_buffer), "%s\n%s", base_msg, extra_info);
        return msg_buffer;
    }
    
    return base_msg;
}

/* Get current time in milliseconds */
long long get_time_ms(void) {
#ifdef _WIN32
    return (long long)GetTickCount();
#else
    struct timeval t;
    gettimeofday(&t, NULL);
    return ((long long)t.tv_sec * 1000) + ((long long)t.tv_usec / 1000);
#endif
}
