/*
 * Precomp I/O - C Implementation
 * Converted from C++ to C for Tiny C Compiler compatibility
 */

#include "precomp_io.h"
#include "precomp_utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifndef _WIN32
#include <unistd.h>
#else
#include <io.h>
#endif

/* ============== IStreamLike Implementation ============== */

static bool file_istream_eof(IStreamLike* self) {
    FileStreamData* data = (FileStreamData*)self->impl_data;
    return feof(data->file) != 0;
}

static bool file_istream_good(IStreamLike* self) {
    FileStreamData* data = (FileStreamData*)self->impl_data;
    return !feof(data->file) && !ferror(data->file);
}

static bool file_istream_bad(IStreamLike* self) {
    FileStreamData* data = (FileStreamData*)self->impl_data;
    return ferror(data->file) != 0;
}

static void file_istream_clear(IStreamLike* self) {
    FileStreamData* data = (FileStreamData*)self->impl_data;
    clearerr(data->file);
}

static size_t file_istream_read(IStreamLike* self, char* buf, size_t count) {
    FileStreamData* data = (FileStreamData*)self->impl_data;
    size_t result = fread(buf, 1, count, data->file);
    data->last_read_count = result;
    return result;
}

static int file_istream_get(IStreamLike* self) {
    FileStreamData* data = (FileStreamData*)self->impl_data;
    return fgetc(data->file);
}

static size_t file_istream_gcount(IStreamLike* self) {
    FileStreamData* data = (FileStreamData*)self->impl_data;
    return data->last_read_count;
}

static int file_istream_seekg(IStreamLike* self, long long offset, int dir) {
    FileStreamData* data = (FileStreamData*)self->impl_data;
    return fseek(data->file, (long)offset, dir);
}

static long long file_istream_tellg(IStreamLike* self) {
    FileStreamData* data = (FileStreamData*)self->impl_data;
    return (long long)ftell(data->file);
}

static void destroy_file_istream(IStreamLike* self) {
    if (self == NULL) return;
    FileStreamData* data = (FileStreamData*)self->impl_data;
    if (data->owns_file && data->file != NULL) {
        fclose(data->file);
    }
    free(data);
    free(self);
}

IStreamLike* create_file_istream(FILE* file, bool close_on_destroy) {
    if (file == NULL) return NULL;
    
    IStreamLike* stream = (IStreamLike*)malloc(sizeof(IStreamLike));
    if (stream == NULL) return NULL;
    
    FileStreamData* data = (FileStreamData*)malloc(sizeof(FileStreamData));
    if (data == NULL) {
        free(stream);
        return NULL;
    }
    
    data->file = file;
    data->last_read_count = 0;
    data->owns_file = close_on_destroy;
    
    stream->impl_data = data;
    stream->eof = file_istream_eof;
    stream->good = file_istream_good;
    stream->bad = file_istream_bad;
    stream->clear = file_istream_clear;
    stream->read = file_istream_read;
    stream->get = file_istream_get;
    stream->gcount = file_istream_gcount;
    stream->seekg = file_istream_seekg;
    stream->tellg = file_istream_tellg;
    
    return stream;
}

/* ============== OStreamLike Implementation ============== */

static bool file_ostream_eof(OStreamLike* self) {
    FileStreamData* data = (FileStreamData*)self->impl_data;
    return feof(data->file) != 0;
}

static bool file_ostream_good(OStreamLike* self) {
    FileStreamData* data = (FileStreamData*)self->impl_data;
    return !feof(data->file) && !ferror(data->file);
}

static bool file_ostream_bad(OStreamLike* self) {
    FileStreamData* data = (FileStreamData*)self->impl_data;
    return ferror(data->file) != 0;
}

static void file_ostream_clear(OStreamLike* self) {
    FileStreamData* data = (FileStreamData*)self->impl_data;
    clearerr(data->file);
}

static void file_ostream_flush(OStreamLike* self) {
    FileStreamData* data = (FileStreamData*)self->impl_data;
    fflush(data->file);
}

static size_t file_ostream_write(OStreamLike* self, const char* buf, size_t count) {
    FileStreamData* data = (FileStreamData*)self->impl_data;
    return fwrite(buf, 1, count, data->file);
}

static int file_ostream_put(OStreamLike* self, char chr) {
    FileStreamData* data = (FileStreamData*)self->impl_data;
    return fputc((unsigned char)chr, data->file);
}

static long long file_ostream_tellp(OStreamLike* self) {
    FileStreamData* data = (FileStreamData*)self->impl_data;
    return (long long)ftell(data->file);
}

static int file_ostream_seekp(OStreamLike* self, long long offset, int dir) {
    FileStreamData* data = (FileStreamData*)self->impl_data;
    return fseek(data->file, (long)offset, dir);
}

static void destroy_file_ostream(OStreamLike* self) {
    if (self == NULL) return;
    FileStreamData* data = (FileStreamData*)self->impl_data;
    if (data->owns_file && data->file != NULL) {
        fclose(data->file);
    }
    free(data);
    free(self);
}

OStreamLike* create_file_ostream(FILE* file, bool close_on_destroy) {
    if (file == NULL) return NULL;
    
    OStreamLike* stream = (OStreamLike*)malloc(sizeof(OStreamLike));
    if (stream == NULL) return NULL;
    
    FileStreamData* data = (FileStreamData*)malloc(sizeof(FileStreamData));
    if (data == NULL) {
        free(stream);
        return NULL;
    }
    
    data->file = file;
    data->last_read_count = 0;
    data->owns_file = close_on_destroy;
    
    stream->impl_data = data;
    stream->eof = file_ostream_eof;
    stream->good = file_ostream_good;
    stream->bad = file_ostream_bad;
    stream->clear = file_ostream_clear;
    stream->flush = file_ostream_flush;
    stream->write = file_ostream_write;
    stream->put = file_ostream_put;
    stream->tellp = file_ostream_tellp;
    stream->seekp = file_ostream_seekp;
    
    return stream;
}

/* ============== Generic IStream Implementation ============== */

static bool generic_istream_eof(IStreamLike* self) {
    GenericIStreamData* data = (GenericIStreamData*)self->impl_data;
    if (data->eof_func != NULL) {
        return data->eof_func(data->backing_structure);
    }
    return false;
}

static bool generic_istream_good(IStreamLike* self) {
    GenericIStreamData* data = (GenericIStreamData*)self->impl_data;
    return !data->bad_flag && !generic_istream_eof(self);
}

static bool generic_istream_bad(IStreamLike* self) {
    GenericIStreamData* data = (GenericIStreamData*)self->impl_data;
    if (data->bad_func != NULL) {
        return data->bad_func(data->backing_structure);
    }
    return data->bad_flag;
}

static void generic_istream_clear(IStreamLike* self) {
    GenericIStreamData* data = (GenericIStreamData*)self->impl_data;
    data->bad_flag = false;
    if (data->clear_func != NULL) {
        data->clear_func(data->backing_structure);
    }
}

static size_t generic_istream_read(IStreamLike* self, char* buf, size_t count) {
    GenericIStreamData* data = (GenericIStreamData*)self->impl_data;
    if (data->read_func != NULL) {
        size_t result = data->read_func(data->backing_structure, buf, (long long)count);
        data->gcount = result;
        return result;
    }
    return 0;
}

static int generic_istream_get(IStreamLike* self) {
    GenericIStreamData* data = (GenericIStreamData*)self->impl_data;
    if (data->get_func != NULL) {
        return data->get_func(data->backing_structure);
    }
    return -1; /* EOF */
}

static size_t generic_istream_gcount(IStreamLike* self) {
    GenericIStreamData* data = (GenericIStreamData*)self->impl_data;
    return data->gcount;
}

static int generic_istream_seekg(IStreamLike* self, long long offset, int dir) {
    GenericIStreamData* data = (GenericIStreamData*)self->impl_data;
    if (data->seekg_func != NULL) {
        return data->seekg_func(data->backing_structure, offset, dir);
    }
    return -1;
}

static long long generic_istream_tellg(IStreamLike* self) {
    GenericIStreamData* data = (GenericIStreamData*)self->impl_data;
    if (data->tellg_func != NULL) {
        return data->tellg_func(data->backing_structure);
    }
    return -1;
}

static void destroy_generic_istream(IStreamLike* self) {
    if (self == NULL) return;
    free(self->impl_data);
    free(self);
}

GenericIStreamLike* create_generic_istream(
    void* backing,
    size_t (*read_func)(void*, char*, long long),
    int (*get_func)(void*),
    int (*seekg_func)(void*, long long, int),
    long long (*tellg_func)(void*),
    bool (*eof_func)(void*),
    bool (*bad_func)(void*),
    void (*clear_func)(void*)
) {
    if (backing == NULL) return NULL;
    
    GenericIStreamLike* stream = (GenericIStreamLike*)malloc(sizeof(GenericIStreamLike));
    if (stream == NULL) return NULL;
    
    GenericIStreamData* data = (GenericIStreamData*)malloc(sizeof(GenericIStreamData));
    if (data == NULL) {
        free(stream);
        return NULL;
    }
    
    data->backing_structure = backing;
    data->gcount = 0;
    data->bad_flag = false;
    data->read_func = read_func;
    data->get_func = get_func;
    data->seekg_func = seekg_func;
    data->tellg_func = tellg_func;
    data->eof_func = eof_func;
    data->bad_func = bad_func;
    data->clear_func = clear_func;
    
    stream->impl_data = data;
    stream->eof = generic_istream_eof;
    stream->good = generic_istream_good;
    stream->bad = generic_istream_bad;
    stream->clear = generic_istream_clear;
    stream->read = generic_istream_read;
    stream->get = generic_istream_get;
    stream->gcount = generic_istream_gcount;
    stream->seekg = generic_istream_seekg;
    stream->tellg = generic_istream_tellg;
    
    return stream;
}

/* ============== Helper Functions ============== */

bool istream_eof(IStreamLike* stream) {
    if (stream == NULL || stream->eof == NULL) return true;
    return stream->eof(stream);
}

bool istream_good(IStreamLike* stream) {
    if (stream == NULL || stream->good == NULL) return false;
    return stream->good(stream);
}

bool istream_bad(IStreamLike* stream) {
    if (stream == NULL || stream->bad == NULL) return true;
    return stream->bad(stream);
}

void istream_clear(IStreamLike* stream) {
    if (stream == NULL || stream->clear == NULL) return;
    stream->clear(stream);
}

size_t istream_read(IStreamLike* stream, char* buf, size_t count) {
    if (stream == NULL || stream->read == NULL) return 0;
    return stream->read(stream, buf, count);
}

int istream_get(IStreamLike* stream) {
    if (stream == NULL || stream->get == NULL) return -1;
    return stream->get(stream);
}

size_t istream_gcount(IStreamLike* stream) {
    if (stream == NULL || stream->gcount == NULL) return 0;
    return stream->gcount(stream);
}

int istream_seekg(IStreamLike* stream, long long offset, int dir) {
    if (stream == NULL || stream->seekg == NULL) return -1;
    return stream->seekg(stream, offset, dir);
}

long long istream_tellg(IStreamLike* stream) {
    if (stream == NULL || stream->tellg == NULL) return -1;
    return stream->tellg(stream);
}

void destroy_istream(IStreamLike* stream) {
    if (stream == NULL) return;
    /* Call appropriate destructor based on type - simplified */
    destroy_file_istream(stream);
}

/* ============== Temp File Operations ============== */

PrecompTmpFile* create_temp_file(const char* prefix) {
    PrecompTmpFile* tmpfile = (PrecompTmpFile*)malloc(sizeof(PrecompTmpFile));
    if (tmpfile == NULL) return NULL;
    
    /* Create temp filename */
    char* tag = temp_files_tag();
    tmpfile->filename = (char*)malloc(256);
    if (tmpfile->filename == NULL) {
        free(tmpfile);
        return NULL;
    }
    
#ifdef _WIN32
    snprintf(tmpfile->filename, 256, "%s\\%s_%s.tmp", 
             getenv("TEMP") ? getenv("TEMP") : ".", prefix, tag);
#else
    snprintf(tmpfile->filename, 256, "/tmp/%s_%s.tmp", prefix, tag);
#endif
    
    tmpfile->file = fopen(tmpfile->filename, "w+b");
    tmpfile->delete_on_close = true;
    
    if (tmpfile->file == NULL) {
        free(tmpfile->filename);
        free(tmpfile);
        return NULL;
    }
    
    return tmpfile;
}

void close_temp_file(PrecompTmpFile* tmpfile) {
    if (tmpfile == NULL) return;
    if (tmpfile->file != NULL) {
        fclose(tmpfile->file);
        tmpfile->file = NULL;
    }
}

void delete_temp_file(PrecompTmpFile* tmpfile) {
    if (tmpfile == NULL) return;
    close_temp_file(tmpfile);
    if (tmpfile->delete_on_close && tmpfile->filename != NULL) {
        remove(tmpfile->filename);
    }
    free(tmpfile->filename);
    free(tmpfile);
}

/* ============== Utility Functions ============== */

long long get_file_size(FILE* file) {
    if (file == NULL) return -1;
    
    long current_pos = ftell(file);
    fseek(file, 0, SEEK_END);
    long long size = (long long)ftell(file);
    fseek(file, current_pos, SEEK_SET);
    
    return size;
}

int copy_stream(IStreamLike* in, OStreamLike* out) {
    char buffer[CHUNK];
    size_t bytes_read;
    
    if (in == NULL || out == NULL) return -1;
    
    while ((bytes_read = istream_read(in, buffer, CHUNK)) > 0) {
        if (out->write(out, buffer, bytes_read) != bytes_read) {
            return -1;
        }
    }
    
    return istream_bad(in) ? -1 : 0;
}
