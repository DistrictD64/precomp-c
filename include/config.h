/* config.h - Central configuration for Precomp-C (TCC Compatible) */
#ifndef PRECOMP_CONFIG_H
#define PRECOMP_CONFIG_H

/* --- Feature Toggles --- */
#define USE_ZLIB      1
#define USE_BZIP2     1
#define USE_LIBJPEG   1
#define USE_LIBPNG    1
#define USE_GIFLIB    1
/* #define USE_7ZPLUGIN 1 */ /* Uncomment if 7z plugin is converted */

/* --- Platform Detection --- */
#if defined(_WIN32) || defined(_WIN64)
    #define OS_WINDOWS 1
    #define PATH_SEP '\\'
#else
    #define OS_UNIX 1
    #define PATH_SEP '/'
#endif

/* --- Include Paths for Contrib Libraries --- */
/* TCC needs explicit paths for headers if they aren't in system dirs */
#ifdef OS_WINDOWS
    #define CONTRIB_BASE "contrib"
#else
    #define CONTRIB_BASE "contrib"
#endif

/* --- Compiler Specifics for TCC --- */
/* TCC does not support some GCC extensions or C++ features */
#define restrict __restrict__
#define inline __inline__

/* --- Data Types --- */
#include <stdint.h>
#include <stddef.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int32_t  s32;
typedef int64_t  s64;

#endif /* PRECOMP_CONFIG_H */
