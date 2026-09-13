/*
 * Precomp I/O - C Header
 * Converted from C++ to C for Tiny C Compiler compatibility
 */

#ifndef PRECOMP_IO_H
#define PRECOMP_IO_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

/* Path delimiter based on platform */
#ifdef _WIN32
#define PATH_DELIM '\\'
#else
#define PATH_DELIM '/'
#endif

/* Chunk size constant */
#define CHUNK 32768

/* Forward declarations */
typedef struct IStreamLike IStreamLike;
typedef struct OStreamLike OStreamLike;
typedef struct GenericIStreamLike GenericIStreamLike;
typedef struct GenericOStreamLike GenericOStreamLike;
typedef struct PrecompTmpFile PrecompTmpFile;

/* Input stream interface (C-style vtable) */
struct IStreamLike {
    /* Virtual function pointers */
    bool (*eof)(IStreamLike* self);
    bool (*good)(IStreamLike* self);
    bool (*bad)(IStreamLike* self);
    void (*clear)(IStreamLike* self);
    
    size_t (*read)(IStreamLike* self, char* buf, size_t count);
    int (*get)(IStreamLike* self);
    size_t (*gcount)(IStreamLike* self);
    int (*seekg)(IStreamLike* self, long long offset, int dir);
    long long (*tellg)(IStreamLike* self);
    
    /* Implementation-specific data */
    void* impl_data;
};

/* Output stream interface (C-style vtable) */
struct OStreamLike {
    /* Virtual function pointers */
    bool (*eof)(OStreamLike* self);
    bool (*good)(OStreamLike* self);
    bool (*bad)(OStreamLike* self);
    void (*clear)(OStreamLike* self);
    void (*flush)(OStreamLike* self);
    
    size_t (*write)(OStreamLike* self, const char* buf, size_t count);
    int (*put)(OStreamLike* self, char chr);
    long long (*tellp)(OStreamLike* self);
    int (*seekp)(OStreamLike* self, long long offset, int dir);
    
    /* Implementation-specific data */
    void* impl_data;
};

/* Generic input stream - complete type */
struct GenericIStreamLike {
    /* Inherits IStreamLike interface */
    bool (*eof)(IStreamLike* self);
    bool (*good)(IStreamLike* self);
    bool (*bad)(IStreamLike* self);
    void (*clear)(IStreamLike* self);
    
    size_t (*read)(IStreamLike* self, char* buf, size_t count);
    int (*get)(IStreamLike* self);
    size_t (*gcount)(IStreamLike* self);
    int (*seekg)(IStreamLike* self, long long offset, int dir);
    long long (*tellg)(IStreamLike* self);
    
    void* impl_data;
};

/* Generic output stream - complete type */
struct GenericOStreamLike {
    /* Inherits OStreamLike interface */
    bool (*eof)(OStreamLike* self);
    bool (*good)(OStreamLike* self);
    bool (*bad)(OStreamLike* self);
    void (*clear)(OStreamLike* self);
    void (*flush)(OStreamLike* self);
    
    size_t (*write)(OStreamLike* self, const char* buf, size_t count);
    int (*put)(OStreamLike* self, char chr);
    long long (*tellp)(OStreamLike* self);
    int (*seekp)(OStreamLike* self, long long offset, int dir);
    
    void* impl_data;
};

/* Generic input stream implementation data */
typedef struct {
    void* backing_structure;
    size_t gcount;
    bool bad_flag;
    
    /* Function pointers for operations */
    size_t (*read_func)(void*, char*, long long);
    int (*get_func)(void*);
    int (*seekg_func)(void*, long long, int);
    long long (*tellg_func)(void*);
    bool (*eof_func)(void*);
    bool (*bad_func)(void*);
    void (*clear_func)(void*);
} GenericIStreamData;

/* Generic output stream implementation data */
typedef struct {
    void* backing_structure;
    bool bad_flag;
    
    /* Function pointers for operations */
    size_t (*write_func)(void*, const char*, long long);
    int (*put_func)(void*, int);
    int (*seekp_func)(void*, long long, int);
    long long (*tellp_func)(void*);
    bool (*eof_func)(void*);
    bool (*bad_func)(void*);
    void (*clear_func)(void*);
} GenericOStreamData;

/* File-based stream implementation data */
typedef struct {
    FILE* file;
    size_t last_read_count;
    bool owns_file;  /* Whether we should close the file on destroy */
} FileStreamData;

/* Temporary file structure */
struct PrecompTmpFile {
    char* filename;
    FILE* file;
    bool delete_on_close;
};

/* SHA1 context (simplified) */
typedef struct {
    uint32_t state[5];
    uint32_t count[2];
    unsigned char buffer[64];
} SHA1Context;

/* Function declarations for I/O operations */

/* IStreamLike operations */
bool istream_eof(IStreamLike* stream);
bool istream_good(IStreamLike* stream);
bool istream_bad(IStreamLike* stream);
void istream_clear(IStreamLike* stream);
size_t istream_read(IStreamLike* stream, char* buf, size_t count);
int istream_get(IStreamLike* stream);
size_t istream_gcount(IStreamLike* stream);
int istream_seekg(IStreamLike* stream, long long offset, int dir);
long long istream_tellg(IStreamLike* stream);

/* OStreamLike operations */
bool ostream_eof(OStreamLike* stream);
bool ostream_good(OStreamLike* stream);
bool ostream_bad(OStreamLike* stream);
void ostream_clear(OStreamLike* stream);
void ostream_flush(OStreamLike* stream);
size_t ostream_write(OStreamLike* stream, const char* buf, size_t count);
int ostream_put(OStreamLike* stream, char chr);
long long ostream_tellp(OStreamLike* stream);
int ostream_seekp(OStreamLike* stream, long long offset, int dir);

/* Factory functions */
IStreamLike* create_file_istream(FILE* file, bool close_on_destroy);
OStreamLike* create_file_ostream(FILE* file, bool close_on_destroy);
GenericIStreamLike* create_generic_istream(
    void* backing,
    size_t (*read_func)(void*, char*, long long),
    int (*get_func)(void*),
    int (*seekg_func)(void*, long long, int),
    long long (*tellg_func)(void*),
    bool (*eof_func)(void*),
    bool (*bad_func)(void*),
    void (*clear_func)(void*)
);
GenericOStreamLike* create_generic_ostream(
    void* backing,
    size_t (*write_func)(void*, const char*, long long),
    int (*put_func)(void*, int),
    int (*seekp_func)(void*, long long, int),
    long long (*tellp_func)(void*),
    bool (*eof_func)(void*),
    bool (*bad_func)(void*),
    void (*clear_func)(void*)
);

/* Destroy functions */
void destroy_istream(IStreamLike* stream);
void destroy_ostream(OStreamLike* stream);

/* Temp file operations */
PrecompTmpFile* create_temp_file(const char* prefix);
void close_temp_file(PrecompTmpFile* tmpfile);
void delete_temp_file(PrecompTmpFile* tmpfile);

/* Utility functions */
long long get_file_size(FILE* file);
int copy_stream(IStreamLike* in, OStreamLike* out);

#endif /* PRECOMP_IO_H */
