/*
 * Format Handlers Registry - C Header
 */

#ifndef PRECOMP_FORMAT_HANDLERS_H
#define PRECOMP_FORMAT_HANDLERS_H

#include "../precomp.h"

/* Initialize all format handlers */
void init_all_format_handlers(void);

/* Get format handler by type */
PrecompFormatHandler* get_format_handler(SupportedFormats format);

/* Cleanup all format handlers */
void cleanup_all_format_handlers(void);

#endif /* PRECOMP_FORMAT_HANDLERS_H */
