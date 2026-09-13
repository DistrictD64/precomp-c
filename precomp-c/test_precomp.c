/*
 * Precomp C - Test Program
 * Demonstrates basic usage of the C library
 */

#include "precomp_lib.h"
#include <stdio.h>
#include <stdlib.h>

void progress_callback(float progress) {
    printf("\rProgress: %.1f%%", progress * 100.0f);
    fflush(stdout);
}

void log_callback(PcompLogLevel level, const char* message) {
    if (level == PCOMP_LOG_DEBUG) {
        fprintf(stderr, "[DEBUG] %s\n", message);
    } else {
        fprintf(stderr, "[INFO] %s\n", message);
    }
}

int main(int argc, char* argv[]) {
    PcompInstance* inst;
    PcompSwitches* switches;
    FILE* infile = NULL;
    FILE* outfile = NULL;
    int result;
    
    printf("Precomp C Library - Test Program\n");
    printf("================================\n\n");
    
    /* Get copyright message */
    char copyright[256];
    pcomp_get_copyright_msg(copyright);
    printf("%s\n\n", copyright);
    
    /* Check arguments */
    if (argc < 3) {
        printf("Usage: %s <mode> <input.pcf> <output>\n", argv[0]);
        printf("  mode: 'd' for decompress/recompress, 'c' for compress/precompress\n");
        printf("\nExample:\n");
        printf("  %s d input.pcf output.bin\n", argv[0]);
        printf("  %s c input.bin output.pcf\n", argv[0]);
        return 1;
    }
    
    char mode = argv[1][0];
    const char* input_file = argv[2];
    const char* output_file = argv[3];
    
    /* Create instance */
    inst = pcomp_create();
    if (inst == NULL) {
        fprintf(stderr, "Error: Failed to create Precomp instance\n");
        return 1;
    }
    
    /* Set up callbacks */
    pcomp_set_progress_callback(inst, progress_callback);
    pcomp_set_log_callback(log_callback);
    
    /* Configure switches */
    switches = pcomp_get_switches(inst);
    if (switches != NULL) {
        /* Enable all formats by default */
        pcomp_switch_set_pdf(switches, true);
        pcomp_switch_set_zip(switches, true);
        pcomp_switch_set_gzip(switches, true);
        pcomp_switch_set_png(switches, true);
        pcomp_switch_set_gif(switches, true);
        pcomp_switch_set_jpg(switches, true);
        pcomp_switch_set_mp3(switches, true);
        pcomp_switch_set_bzip2(switches, true);
        
        /* Set minimum identical bytes */
        pcomp_switch_set_min_ident_size(switches, 4);
        
        /* Disable intense/brute mode by default */
        pcomp_switch_set_intense(switches, false);
        pcomp_switch_set_brute(switches, false);
    }
    
    /* Open files */
    if (mode == 'd' || mode == 'D') {
        /* Decompression mode */
        printf("Mode: Decompress/Recompress\n");
        printf("Input:  %s\n", input_file);
        printf("Output: %s\n", output_file);
        
        infile = fopen(input_file, "rb");
        if (infile == NULL) {
            fprintf(stderr, "Error: Cannot open input file '%s'\n", input_file);
            result = -1;
            goto cleanup;
        }
        
        outfile = fopen(output_file, "wb");
        if (outfile == NULL) {
            fprintf(stderr, "Error: Cannot open output file '%s'\n", output_file);
            result = -1;
            goto cleanup;
        }
        
        /* Set input and output */
        pcomp_set_input_file(inst, infile, input_file);
        pcomp_set_output_file(inst, outfile, output_file);
        
        /* Read header first */
        printf("\nReading PCF header...\n");
        result = pcomp_read_header(inst, 1);
        if (result != 0) {
            fprintf(stderr, "Error reading header: %s\n", pcomp_error_message(result, NULL));
            goto cleanup;
        }
        
        /* Get original filename from header */
        const char* orig_name = pcomp_get_output_filename(inst);
        if (orig_name != NULL) {
            printf("Original filename from header: %s\n", orig_name);
        }
        
        /* Recompress */
        printf("\nRecompressing...\n");
        result = pcomp_recompress(inst);
        
    } else if (mode == 'c' || mode == 'C') {
        /* Compression mode */
        printf("Mode: Compress/Precompress\n");
        printf("Input:  %s\n", input_file);
        printf("Output: %s\n", output_file);
        
        infile = fopen(input_file, "rb");
        if (infile == NULL) {
            fprintf(stderr, "Error: Cannot open input file '%s'\n", input_file);
            result = -1;
            goto cleanup;
        }
        
        outfile = fopen(output_file, "wb");
        if (outfile == NULL) {
            fprintf(stderr, "Error: Cannot open output file '%s'\n", output_file);
            result = -1;
            goto cleanup;
        }
        
        /* Set input and output */
        pcomp_set_input_file(inst, infile, input_file);
        pcomp_set_output_file(inst, outfile, output_file);
        
        /* Precompress */
        printf("\nPrecompressing...\n");
        result = pcomp_precompress(inst);
        
    } else {
        fprintf(stderr, "Error: Unknown mode '%c'. Use 'c' for compress or 'd' for decompress.\n", mode);
        result = -1;
        goto cleanup;
    }
    
    /* Check result */
    if (result == 0) {
        printf("\n\nSuccess!\n");
    } else {
        printf("\n\nFailed with error code %d: %s\n", result, pcomp_error_message(result, NULL));
    }
    
cleanup:
    /* Close files */
    if (infile != NULL) fclose(infile);
    if (outfile != NULL) fclose(outfile);
    
    /* Destroy instance */
    pcomp_destroy(inst);
    
    printf("\nDone.\n");
    return (result == 0) ? 0 : 1;
}
