/*
 * Format Handlers Registry - C Implementation
 * Collects all format handlers in one place
 */

#include "../precomp.h"
#include "base64.h"
#include "bzip2.h"
#include "deflate.h"
#include "gif.h"
#include "gzip.h"
#include "jpeg.h"
#include "mp3.h"
#include "pdf.h"
#include "png.h"
#include "swf.h"
#include "wav.h"
#include "zip.h"
#include "zlib.h"
#include <stdlib.h>

/* Array of all format handlers */
static PrecompFormatHandler* g_format_handlers[FORMAT_COUNT] = {NULL};
static int g_handlers_initialized = 0;

/* Initialize all format handlers */
void init_all_format_handlers(void) {
    if (g_handlers_initialized) return;
    
    g_format_handlers[FORMAT_BASE64] = create_base64_handler();
    g_format_handlers[FORMAT_BZIP2] = create_bzip2_handler();
    g_format_handlers[FORMAT_DEFLATE] = create_deflate_handler();
    g_format_handlers[FORMAT_GIF] = create_gif_handler();
    g_format_handlers[FORMAT_GZIP] = create_gzip_handler();
    g_format_handlers[FORMAT_JPEG] = create_jpeg_handler();
    g_format_handlers[FORMAT_MP3] = create_mp3_handler();
    g_format_handlers[FORMAT_PDF] = create_pdf_handler();
    g_format_handlers[FORMAT_PNG] = create_png_handler();
    g_format_handlers[FORMAT_SWF] = create_swf_handler();
    g_format_handlers[FORMAT_WAV] = create_wav_handler();
    g_format_handlers[FORMAT_ZIP] = create_zip_handler();
    g_format_handlers[FORMAT_ZLIB] = create_zlib_handler();
    
    g_handlers_initialized = 1;
}

/* Get format handler by type */
PrecompFormatHandler* get_format_handler(SupportedFormats format) {
    if (!g_handlers_initialized) {
        init_all_format_handlers();
    }
    
    if (format < 0 || format >= FORMAT_COUNT) {
        return NULL;
    }
    
    return g_format_handlers[format];
}

/* Cleanup all format handlers */
void cleanup_all_format_handlers(void) {
    for (int i = 0; i < FORMAT_COUNT; i++) {
        if (g_format_handlers[i]) {
            free(g_format_handlers[i]);
            g_format_handlers[i] = NULL;
        }
    }
    g_handlers_initialized = 0;
}
