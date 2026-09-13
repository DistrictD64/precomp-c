/*
 * main.c - Main entry point for Precomp-C (TCC Compatible)
 * Converted from precomp.cpp
 * 
 * Copyright 2006-2021 Christian Schneider (Original C++ code)
 * Converted to C for TCC compatibility
 */

#include "config.h"
#include "precomp_lib.h"
#include "precomp_io.h"
#include "precomp_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <ctype.h>
#include <errno.h>

#ifdef OS_WINDOWS
#include <io.h>
#include <fcntl.h>
#else
#include <unistd.h>
#include <sys/time.h>
#endif

/* ============ Version Information ============ */
#define V_MAJOR 0
#define V_MINOR 4
#define V_MINOR2 8
#define V_STATE "DEVELOPMENT"
#define V_MSG "USE AT YOUR OWN RISK!"
#ifdef OS_UNIX
  #define V_OS "Unix"
#else
  #define V_OS "Windows"
#endif
#define V_BIT "64-bit"  /* Assume 64-bit for modern systems */

/* ============ Global Variables ============ */
static char input_file_name[4096] = {0};
static char output_file_name[4096] = {0};
static void (*log_output_func)(const char*) = &print_to_console;
static long long sec_time = 0;
static char current_progress_txt[256] = {0};
static int work_sign_var = 0;
static const char work_signs[5] = "|/-\\";

/* ============ Helper Functions ============ */

void print_to_stderr(const char* msg) {
    fprintf(stderr, "%s", msg);
}

char* libprecomp_error_msg(int error_code) {
    static char buf[512];
    snprintf(buf, sizeof(buf), "\nERROR %i: %s", error_code, precomp_error_msg(error_code, NULL));
    return buf;
}

int parse_prefix_text(const char* c, const char* ref) {
    while (*ref && tolower((unsigned char)*c) == *ref) {
        ++c;
        ++ref;
    }
    return *ref == 0;
}

int parse_switch(int* val, const char* c, const char* ref) {
    if (!parse_prefix_text(c, ref)) {
        return 0;
    }
    size_t l = strlen(ref);
    if (c[l] == '+' && !c[l + 1]) {
        *val = 1;
        return 1;
    } else if (c[l] == '-' && !c[l + 1]) {
        *val = 0;
        return 1;
    }
    fprintf(stderr, "ERROR: Only + or - for this switch (%s) allowed\n", c);
    return 0;
}

int parse_int(const char** c, const char* context, int too_big_error_code) {
    if (**c < '0' || **c > '9') {
        fprintf(stderr, "ERROR: Number needed to set %s\n", context);
        return -1;
    }
    int val = *(*c)++ - '0';
    while (**c >= '0' && **c <= '9') {
        if (val >= (2147483647 / 10 - 1)) {  /* INT_MAX approx */
            if (too_big_error_code != 0) {
                fprintf(stderr, "%s", libprecomp_error_msg(too_big_error_code));
                return -1;
            }
            fprintf(stderr, "ERROR: Number too big for %s\n", context);
            return -1;
        }
        val = val * 10 + *(*c)++ - '0';
    }
    return val;
}

long long parse_int64(const char** c, const char* context, int too_big_error_code) {
    if (**c < '0' || **c > '9') {
        fprintf(stderr, "ERROR: Number needed to set %s\n", context);
        return -1;
    }
    long long val = *(*c)++ - '0';
    while (**c >= '0' && **c <= '9') {
        if (val >= (9223372036854775807LL / 10 - 1)) {  /* INT64_MAX approx */
            if (too_big_error_code != 0) {
                fprintf(stderr, "%s", libprecomp_error_msg(too_big_error_code));
                return -1;
            }
            fprintf(stderr, "ERROR: Number too big for %s\n", context);
            return -1;
        }
        val = val * 10 + *(*c)++ - '0';
    }
    return val;
}

int file_exists(const char* filename) {
    FILE* f = fopen(filename, "rb");
    if (f) {
        fclose(f);
        return 1;
    }
    return 0;
}

void ctrl_c_handler(int sig) {
    log_output_func("\n\nCTRL-C detected\n");
    signal(SIGINT, SIG_DFL);
    /* In C we can't throw, so we exit or use setjmp/longjmp */
    exit(1);
}

void printf_time(long long t) {
    log_output_func("Time: ");
    if (t < 1000) {
        char buf[128];
        snprintf(buf, sizeof(buf), "%lli millisecond(s)\n", t);
        log_output_func(buf);
    }
    else if (t < 1000 * 60) {
        char buf[128];
        snprintf(buf, sizeof(buf), "%lli second(s), %lli millisecond(s)\n", t / 1000, t % 1000);
        log_output_func(buf);
    }
    else if (t < 1000 * 60 * 60) {
        char buf[128];
        snprintf(buf, sizeof(buf), "%lli minute(s), %lli second(s)\n", t / (1000 * 60), (t / 1000) % 60);
        log_output_func(buf);
    }
    else if (t < 1000 * 60 * 60 * 24) {
        char buf[128];
        snprintf(buf, sizeof(buf), "%lli hour(s), %lli minute(s), %lli second(s)\n", 
                 t / (1000 * 60 * 60), (t / (1000 * 60)) % 60, (t / 1000) % 60);
        log_output_func(buf);
    }
    else {
        char buf[128];
        snprintf(buf, sizeof(buf), "%lli day(s), %lli hour(s), %lli minute(s)\n",
                 t / (1000 * 60 * 60 * 24), (t / (1000 * 60 * 60)) % 24, (t / (1000 * 60)) % 60);
        log_output_func(buf);
    }
}

char* next_work_sign(void) {
    static char buf[8];
    work_sign_var = (work_sign_var + 1) % 4;
    snprintf(buf, sizeof(buf), "%c     ", work_signs[work_sign_var]);
    return buf;
}

void delete_current_progress_text(void) {
    size_t len = strlen(current_progress_txt);
    char* spaces = malloc(len + 1);
    char* backspaces = malloc(len + 1);
    if (spaces && backspaces) {
        memset(spaces, ' ', len);
        spaces[len] = '\0';
        memset(backspaces, '\b', len);
        backspaces[len] = '\0';
        log_output_func(backspaces);
        log_output_func(spaces);
        log_output_func(backspaces);
    }
    free(spaces);
    free(backspaces);
}

int get_progress_txt(float percent) {
    long long now = get_time_ms();
    if ((now - sec_time) < 250) return 0;

    char new_percent[64];
    snprintf(new_percent, sizeof(new_percent), "%6.2f%% ", percent);
    char* new_work_sign = next_work_sign();

    delete_current_progress_text();
    sec_time = now;
    snprintf(current_progress_txt, sizeof(current_progress_txt), "%s%s", new_percent, new_work_sign);
    return 1;
}

void progress_callback(float percent) {
    if (get_progress_txt(percent)) {
        log_output_func(current_progress_txt);
    }
}

void log_handler(PcompLogLevel level, const char* message) {
    if (level == PCOMP_LOG_NORMAL || level == PCOMP_LOG_DEBUG) {
        log_output_func(message);
    }
}

long long get_time_ms(void) {
#ifdef OS_WINDOWS
    return (long long)clock() * 1000 / CLOCKS_PER_SEC;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000 + tv.tv_usec / 1000;
#endif
}

void packjpg_mp3_dll_msg(void) {
    /* Placeholder - in original this checks for DLL availability */
}

/* ============ Initialization ============ */

typedef enum {
    OP_NONE = 0,
    P_PRECOMPRESS = 1,
    P_RECOMPRESS = 2
} Operation;

Operation init(PcompInstance* precomp_mgr, PcompSwitches* precomp_switches, 
               int argc, char* argv[]) {
    Operation operation = OP_NONE;
    int valid_syntax = 0;
    int comfort_mode = 0;
    int decompress = 0;
    int output_file_given = 0;
    int preserve_extension = 0;
    int verbose = 0;
    int stderr_output = 0;
    int verify = 0;
    int intense_mode = 0;
    int brute_mode = 0;
    int long_help = 0;
    int min_ident_size_set = 0;
    
    /* Initialize switches to defaults using accessor functions */
    pcomp_switch_set_max_recursion(precomp_switches, 10);
    pcomp_switch_set_min_ident_size(precomp_switches, 4);
    pcomp_switch_set_pdf(precomp_switches, true);
    pcomp_switch_set_zip(precomp_switches, true);
    pcomp_switch_set_gzip(precomp_switches, true);
    pcomp_switch_set_png(precomp_switches, true);
    pcomp_switch_set_gif(precomp_switches, true);
    pcomp_switch_set_jpg(precomp_switches, true);
    pcomp_switch_set_swf(precomp_switches, true);
    pcomp_switch_set_base64(precomp_switches, true);
    pcomp_switch_set_bzip2(precomp_switches, true);
    pcomp_switch_set_mp3(precomp_switches, true);
    
    if (argc < 2) {
        return OP_NONE;
    }

    for (int i = 1; i < argc; i++) {
        char* arg = argv[i];
        
        if (arg[0] == '-') {
            char* c = arg + 1;
            
            if (strcmp(c, "comfort") == 0) {
                comfort_mode = 1;
            }
            else if (strcmp(c, "r") == 0) {
                decompress = 1;
            }
            else if (c[0] == 'o' && c[1] != '\0') {
                strncpy(output_file_name, c + 1, sizeof(output_file_name) - 1);
                output_file_given = 1;
            }
            else if (strcmp(c, "e") == 0) {
                preserve_extension = 1;
            }
            else if (strcmp(c, "v") == 0) {
                verbose = 1;
            }
            else if (strcmp(c, "vstderr") == 0) {
                stderr_output = 1;
            }
            else if (strcmp(c, "verify") == 0) {
                verify = 1;
            }
            else if (c[0] == 'd' && c[1] != '\0') {
                const char* num = c + 1;
                int depth = parse_int(&num, "recursion depth", 0);
                if (depth > 0) {
                    pcomp_switch_set_max_recursion(precomp_switches, depth);
                }
            }
            else if (strcmp(c, "intense") == 0) {
                intense_mode = 1;
            }
            else if (strcmp(c, "brute") == 0) {
                brute_mode = 1;
            }
            else if (strcmp(c, "longhelp") == 0) {
                long_help = 1;
            }
            else if (c[0] == 't' && c[1] != '\0') {
                /* Type switch parsing - simplified */
                char mode = c[1];
                char* types = c + 2;
                
                /* Reset all first if using t+ */
                if (mode == '+') {
                    pcomp_switch_set_pdf(precomp_switches, false);
                    pcomp_switch_set_zip(precomp_switches, false);
                    pcomp_switch_set_gzip(precomp_switches, false);
                    pcomp_switch_set_png(precomp_switches, false);
                    pcomp_switch_set_gif(precomp_switches, false);
                    pcomp_switch_set_jpg(precomp_switches, false);
                    pcomp_switch_set_swf(precomp_switches, false);
                    pcomp_switch_set_base64(precomp_switches, false);
                    pcomp_switch_set_bzip2(precomp_switches, false);
                    pcomp_switch_set_mp3(precomp_switches, false);
                    
                    for (char* t = types; *t; t++) {
                        switch (tolower((unsigned char)*t)) {
                            case 'p': pcomp_switch_set_pdf(precomp_switches, true); break;
                            case 'z': pcomp_switch_set_zip(precomp_switches, true); break;
                            case 'g': pcomp_switch_set_gzip(precomp_switches, true); break;
                            case 'n': pcomp_switch_set_png(precomp_switches, true); break;
                            case 'f': pcomp_switch_set_gif(precomp_switches, true); break;
                            case 'j': pcomp_switch_set_jpg(precomp_switches, true); break;
                            case 's': pcomp_switch_set_swf(precomp_switches, true); break;
                            case 'm': pcomp_switch_set_base64(precomp_switches, true); break;
                            case 'b': pcomp_switch_set_bzip2(precomp_switches, true); break;
                            case '3': pcomp_switch_set_mp3(precomp_switches, true); break;
                        }
                    }
                }
                else if (mode == '-') {
                    for (char* t = types; *t; t++) {
                        switch (tolower((unsigned char)*t)) {
                            case 'p': pcomp_switch_set_pdf(precomp_switches, false); break;
                            case 'z': pcomp_switch_set_zip(precomp_switches, false); break;
                            case 'g': pcomp_switch_set_gzip(precomp_switches, false); break;
                            case 'n': pcomp_switch_set_png(precomp_switches, false); break;
                            case 'f': pcomp_switch_set_gif(precomp_switches, false); break;
                            case 'j': pcomp_switch_set_jpg(precomp_switches, false); break;
                            case 's': pcomp_switch_set_swf(precomp_switches, false); break;
                            case 'm': pcomp_switch_set_base64(precomp_switches, false); break;
                            case 'b': pcomp_switch_set_bzip2(precomp_switches, false); break;
                            case '3': pcomp_switch_set_mp3(precomp_switches, false); break;
                        }
                    }
                }
            }
            else if (c[0] == 's' && c[1] != '\0') {
                const char* num = c + 1;
                int size = parse_int(&num, "minimal ident size", 0);
                if (size > 0) {
                    pcomp_switch_set_min_ident_size(precomp_switches, size);
                    min_ident_size_set = 1;
                }
            }
            else {
                fprintf(stderr, "Unknown switch: -%s\n", c);
                return OP_NONE;
            }
        }
        else {
            /* Input file */
            if (input_file_name[0] == '\0') {
                strncpy(input_file_name, arg, sizeof(input_file_name) - 1);
                
                /* Determine operation */
                if (decompress) {
                    operation = P_RECOMPRESS;
                }
                else if (comfort_mode) {
                    /* Check if file has PCF header */
                    FILE* f = fopen(arg, "rb");
                    if (f) {
                        char header[8];
                        if (fread(header, 1, 8, f) == 8) {
                            /* Simple PCF header check - adjust as needed */
                            if (header[0] == 'P' && header[1] == 'C' && 
                                header[2] == 'F' && header[3] == 0x1A) {
                                operation = P_RECOMPRESS;
                            } else {
                                operation = P_PRECOMPRESS;
                            }
                        } else {
                            operation = P_PRECOMPRESS;
                        }
                        fclose(f);
                    } else {
                        operation = P_PRECOMPRESS;
                    }
                }
                else {
                    operation = P_PRECOMPRESS;
                }
                
                /* Set default output file name if not given */
                if (!output_file_given) {
                    if (operation == P_PRECOMPRESS) {
                        if (!preserve_extension) {
                            /* Replace or add .pcf extension */
                            char* dot = strrchr(input_file_name, '.');
                            char* slash = strrchr(input_file_name, PATH_SEP);
                            
                            if (!dot || (slash && dot < slash)) {
                                snprintf(output_file_name, sizeof(output_file_name), 
                                        "%s.pcf", input_file_name);
                            } else {
                                /* Check if already .pcf */
                                if (strcmp(dot, ".pcf") == 0) {
                                    snprintf(output_file_name, sizeof(output_file_name),
                                            "%s_pcf.pcf", input_file_name);
                                } else {
                                    strncpy(output_file_name, input_file_name, 
                                           (dot - input_file_name));
                                    output_file_name[dot - input_file_name] = '\0';
                                    strcat(output_file_name, ".pcf");
                                }
                            }
                        } else {
                            snprintf(output_file_name, sizeof(output_file_name),
                                    "%s.pcf", input_file_name);
                        }
                    } else {
                        /* For decompression, remove .pcf extension */
                        strncpy(output_file_name, input_file_name, sizeof(output_file_name) - 1);
                        char* dot = strrchr(output_file_name, '.');
                        if (dot && strcmp(dot, ".pcf") == 0) {
                            *dot = '\0';
                        }
                    }
                    output_file_given = 1;
                }
            }
            valid_syntax = 1;
        }
    }

    if (!valid_syntax) {
        log_output_func("Usage: precomp [-switches] input_file\n\n");
        if (long_help) {
            log_output_func("Switches (and their <default values>):\n");
        } else {
            log_output_func("Common switches (and their <default values>):\n");
        }
        log_output_func("  comfort      Read input stream for a PCF header and recompress if found\n");
        log_output_func("  r            Recompress PCF file (restore original file)\n");
        log_output_func("  o[filename]  Write output to [filename]\n");
        log_output_func("  e            Preserve original extension for output name\n");
        log_output_func("  v            Verbose (debug) mode\n");
        log_output_func("  vstderr      Output messages to stderr instead of console\n");
        log_output_func("  verify       Verify precompressed data with hash check\n");
        log_output_func("  d[depth]     Set maximal recursion depth <10>\n");
        log_output_func("  intense      Detect raw zLib headers (slower)\n");
        log_output_func("  t[+-][type]  Compression type switch (all enabled)\n");
        log_output_func("               P=PDF, Z=ZIP, G=GZip, N=PNG, F=GIF, J=JPG\n");
        log_output_func("               S=SWF, M=MIME Base64, B=bZip2, 3=MP3\n");
        
        if (!long_help) {
            log_output_func("  longhelp     Show long help\n");
        } else {
            log_output_func("  s[size]      Set minimal identical byte size <4>\n");
            log_output_func("\n");
            log_output_func("Examples:\n");
            log_output_func("  precomp file.dat           # Compress file.dat to file.dat.pcf\n");
            log_output_func("  precomp -r file.pcf        # Decompress file.pcf\n");
            log_output_func("  precomp -t-png file.dat    # Compress without PNG detection\n");
        }
        exit(1);
    }

    /* Configure logging */
    if (stderr_output) {
        log_output_func = &print_to_stderr;
    }
    
    /* Set verbosity */
    if (verbose) {
        /* Enable debug logging */
    }

    /* Print version info */
    char version_info[256];
    snprintf(version_info, sizeof(version_info),
             "Precomp %i.%i.%i %s %s (%s %s) - %s\n\n",
             V_MAJOR, V_MINOR, V_MINOR2, V_STATE, V_MSG, V_OS, V_BIT,
             __DATE__);
    log_output_func(version_info);

    packjpg_mp3_dll_msg();

    return operation;
}

/* ============ Main Function ============ */

int main(int argc, char* argv[]) {
    PcompInstance* precomp_mgr = NULL;
    PcompSwitches* precomp_switches = NULL;
    int return_errorlevel = 0;
    
    /* Register CTRL-C handler */
    signal(SIGINT, ctrl_c_handler);
    
    /* Create instance */
    precomp_mgr = pcomp_create();
    if (!precomp_mgr) {
        fprintf(stderr, "Failed to create Precomp instance\n");
        return 1;
    }
    
    /* Get switches and configure callbacks */
    precomp_switches = pcomp_get_switches(precomp_mgr);
    pcomp_set_progress_callback(precomp_mgr, progress_callback);
    pcomp_set_log_callback(precomp_mgr, log_handler);
    
    /* Initialize and parse arguments */
    Operation op = init(precomp_mgr, *precomp_switches, argc, argv);
    
    if (op == OP_NONE) {
        pcomp_destroy(precomp_mgr);
        return 1;
    }
    
    long long start_time = get_time_ms();
    
    /* Open input file */
    FILE* fin = fopen(input_file_name, "rb");
    if (!fin) {
        fprintf(stderr, "ERROR: Can't open input file \"%s\"\n", input_file_name);
        pcomp_destroy(precomp_mgr);
        return 1;
    }
    pcomp_set_input_file(precomp_mgr, fin, input_file_name);
    
    /* Open output file */
    FILE* fout = fopen(output_file_name, "wb");
    if (!fout) {
        fprintf(stderr, "ERROR: Can't create output file \"%s\"\n", output_file_name);
        fclose(fin);
        pcomp_destroy(precomp_mgr);
        return 1;
    }
    pcomp_set_output_file(precomp_mgr, fout, output_file_name);
    
    log_output_func("Input file: ");
    log_output_func(input_file_name);
    log_output_func("\nOutput file: ");
    log_output_func(output_file_name);
    log_output_func("\n\n");
    
    /* Execute operation */
    switch (op) {
        case P_PRECOMPRESS:
            return_errorlevel = pcomp_precompress(precomp_mgr);
            break;
        case P_RECOMPRESS:
            return_errorlevel = pcomp_recompress(precomp_mgr);
            break;
    }
    
    /* Handle results */
    if (return_errorlevel != 0 && !(return_errorlevel == 2 && op == P_PRECOMPRESS)) {
        log_output_func(libprecomp_error_msg(return_errorlevel));
        log_output_func("\n");
    } else {
        switch (op) {
            case P_PRECOMPRESS:
            case P_RECOMPRESS:
                delete_current_progress_text();
                log_output_func("100.00%\nDone.\n");
                printf_time(get_time_ms() - start_time);
                break;
        }
    }
    
    /* Cleanup */
    fclose(fin);
    fclose(fout);
    pcomp_destroy(precomp_mgr);
    
    return return_errorlevel == 0 ? 0 : (return_errorlevel == 2 ? 0 : 1);
}
