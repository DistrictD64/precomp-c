/*
 * Base64 format handler for precomp - C implementation
 * Converted from C++ to C for Tiny C Compiler compatibility
 */

#include "base64.h"
#include "precomp.h"
#include "precomp_io.h"
#include "deflate.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/stat.h>

/* Base64 alphabet */
static const char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/* Structure to hold base64 precompression result */
typedef struct {
    precompression_result base;
    unsigned char* base64_header;
    size_t base64_header_size;
    int line_case;
    unsigned int* base64_line_len;
    size_t base64_line_len_count;
} base64_precompression_result;

/* Decode a single base64 character */
unsigned char base64_char_decode(unsigned char c) {
    if ((c >= 'A') && (c <= 'Z')) {
        return (c - 'A');
    }
    if ((c >= 'a') && (c <= 'z')) {
        return (c - 'a' + 26);
    }
    if ((c >= '0') && (c <= '9')) {
        return (c - '0' + 52);
    }
    if (c == '+') return 62;
    if (c == '/') return 63;
    if (c == '=') return 64; /* padding */
    return 65; /* invalid */
}

/* Dump base64 header to output file */
static void dump_base64_header(OStreamLike* outfile, const base64_precompression_result* result) {
    /* write "header", but change first char to prevent re-detection */
    fout_fput_vlint(outfile, result->base64_header_size);
    fout_put(outfile, result->base64_header[0] - 1);
    fout_write(outfile, (const char*)(result->base64_header + 1), result->base64_header_size - 1);

    fout_fput_vlint(outfile, result->base64_line_len_count);
    if (result->line_case == 2) {
        for (size_t i = 0; i < result->base64_line_len_count; i++) {
            fout_put(outfile, (unsigned char)result->base64_line_len[i]);
        }
    } else {
        fout_put(outfile, (unsigned char)result->base64_line_len[0]);
        if (result->line_case == 1) {
            fout_put(outfile, (unsigned char)result->base64_line_len[result->base64_line_len_count - 1]);
        }
    }
}

/* Free base64 precompression result */
static void free_base64_result(base64_precompression_result* result) {
    if (result) {
        if (result->base64_header) free(result->base64_header);
        if (result->base64_line_len) free(result->base64_line_len);
        if (result->base.precompressed_stream) {
            fclose(result->base.precompressed_stream);
        }
        free(result);
    }
}

/* Create new base64 precompression result */
static base64_precompression_result* create_base64_result(void) {
    base64_precompression_result* result = (base64_precompression_result*)calloc(1, sizeof(base64_precompression_result));
    if (result) {
        result->base.format_type = D_BASE64;
        result->base.free_func = (void (*)(void*))free_base64_result;
    }
    return result;
}

/* Dump to output file */
static void base64_dump_to_outfile(precompression_result* pres, OStreamLike* outfile) {
    base64_precompression_result* result = (base64_precompression_result*)pres;
    dump_header_to_outfile(pres, outfile);
    dump_base64_header(outfile, result);
    dump_penaltybytes_to_outfile(pres, outfile);
    dump_stream_sizes_to_outfile(pres, outfile);
    dump_precompressed_data_to_outfile(pres, outfile);
}

/* Quick check for base64 detection */
int base64_quick_check(const unsigned char* buffer, uintptr_t current_input_id, long long original_input_pos) {
    if ((*(buffer + 1) == 'o') && (*(buffer + 2) == 'n') && 
        (*(buffer + 3) == 't') && (*(buffer + 4) == 'e')) {
        unsigned char cte_detect[33];
        for (int i = 0; i < 33; i++) {
            cte_detect[i] = tolower(*(buffer + i));
        }
        return memcmp(cte_detect, "content-transfer-encoding: base64", 33) == 0;
    }
    return 0;
}

/* Re-encode base64 data */
void base64_reencode(IStreamLike* file_in, OStreamLike* file_out, 
                     const unsigned int* base64_line_len, size_t line_count,
                     long long max_in_count, long long max_byte_count) {
    int line_nr = 0;
    unsigned int act_line_len = 0;
    long long avail_in;
    unsigned char a, b, c;
    int i;
    long long act_byte_count = 0;
    long long remaining_bytes = max_in_count;
    
    unsigned char* in_buf = (unsigned char*)malloc(DIV3CHUNK);
    if (!in_buf) return;
    
    do {
        if (remaining_bytes > DIV3CHUNK) {
            fin_read(file_in, (char*)in_buf, DIV3CHUNK);
            avail_in = fin_gcount(file_in);
        } else {
            fin_read(file_in, (char*)in_buf, remaining_bytes);
            avail_in = fin_gcount(file_in);
        }
        remaining_bytes -= avail_in;

        /* make sure avail_in mod 3 = 0, pad with 0 bytes */
        while ((avail_in % 3) != 0) {
            in_buf[avail_in] = 0;
            avail_in++;
        }

        for (i = 0; i < (avail_in / 3); i++) {
            a = in_buf[i * 3];
            b = in_buf[i * 3 + 1];
            c = in_buf[i * 3 + 2];
            
            if (act_byte_count < max_byte_count) fout_put(file_out, b64[a >> 2]);
            act_byte_count++;
            act_line_len++;
            if (act_line_len == base64_line_len[line_nr]) {
                if (act_byte_count < max_byte_count) fout_put(file_out, 13);
                act_byte_count++;
                if (act_byte_count < max_byte_count) fout_put(file_out, 10);
                act_byte_count++;
                act_line_len = 0;
                line_nr++;
                if (line_nr >= (int)line_count) break;
            }
            
            if (line_nr >= (int)line_count) break;
            
            if (act_byte_count < max_byte_count) fout_put(file_out, b64[((a & 0x03) << 4) | (b >> 4)]);
            act_byte_count++;
            act_line_len++;
            if (act_line_len == base64_line_len[line_nr]) {
                if (act_byte_count < max_byte_count) fout_put(file_out, 13);
                act_byte_count++;
                if (act_byte_count < max_byte_count) fout_put(file_out, 10);
                act_byte_count++;
                act_line_len = 0;
                line_nr++;
                if (line_nr >= (int)line_count) break;
            }
            
            if (line_nr >= (int)line_count) break;
            
            if (act_byte_count < max_byte_count) fout_put(file_out, b64[((b & 0x0F) << 2) | (c >> 6)]);
            act_byte_count++;
            act_line_len++;
            if (act_line_len == base64_line_len[line_nr]) {
                if (act_byte_count < max_byte_count) fout_put(file_out, 13);
                act_byte_count++;
                if (act_byte_count < max_byte_count) fout_put(file_out, 10);
                act_byte_count++;
                act_line_len = 0;
                line_nr++;
                if (line_nr >= (int)line_count) break;
            }
            
            if (line_nr >= (int)line_count) break;
            
            if (act_byte_count < max_byte_count) fout_put(file_out, b64[c & 63]);
            act_byte_count++;
            act_line_len++;
            if (act_line_len == base64_line_len[line_nr]) {
                if (act_byte_count < max_byte_count) fout_put(file_out, 13);
                act_byte_count++;
                if (act_byte_count < max_byte_count) fout_put(file_out, 10);
                act_byte_count++;
                act_line_len = 0;
                line_nr++;
                if (line_nr >= (int)line_count) break;
            }
        }
        if (line_nr >= (int)line_count) break;
    } while ((remaining_bytes > 0) && (avail_in > 0));
    
    free(in_buf);
}

/* Compare two files */
unsigned long long compare_files(Precomp* precomp_mgr, IStreamLike* file1, IStreamLike* file2, 
                                  unsigned int pos1, unsigned int pos2) {
    unsigned char input_bytes1[COMP_CHUNK];
    unsigned char input_bytes2[COMP_CHUNK];
    long long same_byte_count = 0;
    long long size1, size2, minsize;
    int i;
    int endNow = 0;

    fin_seek(file1, pos1, SEEK_SET);
    fin_seek(file2, pos2, SEEK_SET);

    do {
        call_progress_callback(precomp_mgr);

        fin_read(file1, (char*)input_bytes1, COMP_CHUNK);
        size1 = fin_gcount(file1);
        fin_read(file2, (char*)input_bytes2, COMP_CHUNK);
        size2 = fin_gcount(file2);

        minsize = (size1 < size2) ? size1 : size2;
        for (i = 0; i < minsize; i++) {
            if (input_bytes1[i] != input_bytes2[i]) {
                endNow = 1;
                break;
            }
            same_byte_count++;
        }
    } while ((minsize == COMP_CHUNK) && (!endNow));

    return same_byte_count;
}

/* Try decompression of base64 */
precompression_result* try_decompression_base64(Precomp* precomp_mgr, long long original_input_pos, 
                                                 int base64_header_length, const unsigned char* checkbuf) {
    base64_precompression_result* result = create_base64_result();
    if (!result) return NULL;
    
    PrecompTmpFile tmpfile_struct;
    PrecompTmpFile* tmpfile = &tmpfile_struct;
    char* tmpfilename = get_tempfile_name(precomp_mgr, "decomp_base64");
    
    if (!tmpfile_open(tmpfile, tmpfilename, "ab+")) {
        free_base64_result(result);
        return NULL;
    }

    long long base64_stream_pos = original_input_pos + base64_header_length;

    /* try to decode at current position */
    fin_seek(precomp_mgr->ctx->fin, base64_stream_pos, SEEK_SET);

    unsigned char base64_data[CHUNK >> 2];
    unsigned char in_buf[CHUNK];
    
    long long avail_in = 0;
    unsigned char a, b, c, d;
    int cr_count = 0;
    int decoding_failed = 0;
    int stream_finished = 0;
    unsigned int act_line_len = 0;
    uint16_t k = 0;

    do {
        fin_read(precomp_mgr->ctx->fin, (char*)in_buf, CHUNK);
        avail_in = fin_gcount(precomp_mgr->ctx->fin);
        
        for (long long i = 0; i < (avail_in >> 2); i++) {
            /* are these valid base64 chars? */
            int valid_chars = 1;
            for (int j = (int)(i << 2); j < (int)((i << 2) + 4); j++) {
                c = base64_char_decode(in_buf[j]);
                if (c < 64) {
                    base64_data[k] = c;
                    k++;
                    cr_count = 0;
                    act_line_len++;
                    continue;
                }
                if ((in_buf[j] == 13) || (in_buf[j] == 10)) {
                    if (in_buf[j] == 13) {
                        cr_count++;
                        if (cr_count == 2) {
                            stream_finished = 1;
                            break;
                        }
                        if (result->base64_line_len_count < 65534) {
                            unsigned int* new_lens = (unsigned int*)realloc(result->base64_line_len, 
                                (result->base64_line_len_count + 1) * sizeof(unsigned int));
                            if (new_lens) {
                                result->base64_line_len = new_lens;
                                result->base64_line_len[result->base64_line_len_count++] = act_line_len;
                            }
                        } else {
                            stream_finished = 1;
                        }
                        act_line_len = 0;
                    }
                    continue;
                } else {
                    cr_count = 0;
                }
                stream_finished = 1;
                if (result->base64_line_len_count < 65534) {
                    unsigned int* new_lens = (unsigned int*)realloc(result->base64_line_len, 
                        (result->base64_line_len_count + 1) * sizeof(unsigned int));
                    if (new_lens) {
                        result->base64_line_len = new_lens;
                        result->base64_line_len[result->base64_line_len_count++] = act_line_len;
                    }
                }
                act_line_len = 0;
                /* "=" -> Padding */
                if (in_buf[j] == '=') {
                    while ((k % 4) != 0) {
                        base64_data[k] = 0;
                        k++;
                    }
                    break;
                }
                /* "-" -> base64 end */
                if (in_buf[j] == '-') break;
                /* invalid char found -> decoding failed */
                decoding_failed = 1;
                break;
            }
            if (decoding_failed) break;

            for (int j = 0; j < (k >> 2); j++) {
                a = base64_data[(j << 2)];
                b = base64_data[(j << 2) + 1];
                c = base64_data[(j << 2) + 2];
                d = base64_data[(j << 2) + 3];
                tmpfile_put(tmpfile, (a << 2) | (b >> 4));
                tmpfile_put(tmpfile, ((b << 4) & 0xFF) | (c >> 2));
                tmpfile_put(tmpfile, ((c << 6) & 0xFF) | d);
            }
            if (stream_finished) break;
            for (int j = 0; j < (k % 4); j++) {
                base64_data[j] = base64_data[((k >> 2) << 2) + j];
            }
            k = k % 4;
        }
    } while ((avail_in == CHUNK) && (!decoding_failed) && (!stream_finished));

    /* if one of the lines is longer than 255 characters -> decoding failed */
    for (size_t i = 0; i < result->base64_line_len_count; i++) {
        if (result->base64_line_len[i] > 255) {
            decoding_failed = 1;
            break;
        }
    }

    if (decoding_failed) {
        tmpfile_close(tmpfile);
        return (precompression_result*)result;
    }

    int line_case = 0; /* one length for all lines */
    /* check line case */
    if (result->base64_line_len_count != 1) {
        for (size_t i = 1; i < (result->base64_line_len_count - 1); i++) {
            if (result->base64_line_len[i] != result->base64_line_len[0]) {
                line_case = 2; /* save complete line length list */
                break;
            }
        }
        if (line_case == 0) {
            /* check last line */
            if (result->base64_line_len[result->base64_line_len_count - 1] != result->base64_line_len[0]) {
                line_case = 1; /* first length for all lines, second length for last line */
            }
        }
    }

    precomp_mgr->statistics.decompressed_streams_count++;
    precomp_mgr->statistics.decompressed_base64_count++;

    tmpfile_close(tmpfile);
    
    /* Get file size */
    struct stat st;
    long long decoded_size = 0;
    if (stat(tmpfile->file_path, &st) == 0) {
        decoded_size = st.st_size;
    }

    print_to_log(PRECOMP_DEBUG_LOG, "Possible Base64-Stream (line_case %i, line_count %zu) found at position %lli\n", 
                 line_case, result->base64_line_len_count, original_input_pos);
    print_to_log(PRECOMP_DEBUG_LOG, "Can be decoded to %lli bytes\n", decoded_size);

    /* try to re-encode Base64 data */
    if (!tmpfile_reopen(tmpfile)) {
        free_base64_result(result);
        return NULL;
    }

    char* frecomp_filename = (char*)malloc(strlen(tmpfile->file_path) + 10);
    if (!frecomp_filename) {
        free_base64_result(result);
        return NULL;
    }
    sprintf(frecomp_filename, "%s_rec", tmpfile->file_path);
    remove(frecomp_filename);
    
    PrecompTmpFile frecomp;
    if (!tmpfile_open(&frecomp, frecomp_filename, "wb+")) {
        free(frecomp_filename);
        free_base64_result(result);
        return NULL;
    }
    
    base64_reencode(tmpfile, &frecomp, result->base64_line_len, result->base64_line_len_count, 
                    0x7FFFFFFFFFFFFFFFLL, 0x7FFFFFFFFFFFFFFFLL);

    long long compressed_size = compare_files(precomp_mgr, precomp_mgr->ctx->fin, &frecomp, 
                                               base64_stream_pos, 0);

    tmpfile_close(&frecomp);
    remove(frecomp_filename);
    free(frecomp_filename);

    if (compressed_size > precomp_mgr->switches.min_ident_size) {
        precomp_mgr->statistics.recompressed_streams_count++;
        precomp_mgr->statistics.recompressed_base64_count++;
        print_to_log(PRECOMP_DEBUG_LOG, "Match: encoded to %lli bytes\n", compressed_size);

        result->base.success = 1;

        /* write compressed data header (Base64) */
        result->base.flags = 0x01 | (line_case << 2);
        
        /* Copy header */
        result->base64_header = (unsigned char*)malloc(base64_header_length);
        if (result->base64_header) {
            memcpy(result->base64_header, checkbuf, base64_header_length);
            result->base64_header_size = base64_header_length;
        }

        result->base.original_size = compressed_size;
        result->base.precompressed_size = decoded_size;

        /* write decompressed data */
        if (!tmpfile_reopen(tmpfile)) {
            free_base64_result(result);
            return NULL;
        }
        result->base.precompressed_stream = tmpfile_reuse(tmpfile);
    } else {
        print_to_log(PRECOMP_DEBUG_LOG, "No match\n");
    }
    
    /* Clean up tmpfile if not used */
    if (!result->base.precompressed_stream) {
        tmpfile_close(tmpfile);
    }
    
    return (precompression_result*)result;
}

/* Attempt precompression for base64 */
precompression_result* base64_attempt_precompression(Precomp* precomp_mgr, const unsigned char* checkbuf, 
                                                      long long original_input_pos) {
    /* search for double CRLF, all between is "header" */
    int base64_header_length = 33;
    int found_double_crlf = 0;
    
    do {
        if ((*(checkbuf + base64_header_length) == 13) && (*(checkbuf + base64_header_length + 1) == 10)) {
            if ((*(checkbuf + base64_header_length + 2) == 13) && (*(checkbuf + base64_header_length + 3) == 10)) {
                found_double_crlf = 1;
                base64_header_length += 4;
                /* skip additional CRLFs */
                while ((*(checkbuf + base64_header_length) == 13) && 
                       (*(checkbuf + base64_header_length + 1) == 10)) {
                    base64_header_length += 2;
                }
                break;
            }
        }
        base64_header_length++;
    } while (base64_header_length < (CHECKBUF_SIZE - 2));

    if (found_double_crlf) {
        precompression_result* result = try_decompression_base64(precomp_mgr, original_input_pos, 
                                                                  base64_header_length, checkbuf);
        if (result) {
            result->original_size_extra += base64_header_length;
        }
        return result;
    }
    return NULL;
}

/* Structure for base64 format header data */
typedef struct {
    PrecompFormatHeaderData base;
    unsigned char* base64_stream_hdr;
    size_t base64_stream_hdr_size;
    unsigned int* base64_line_len;
    size_t base64_line_len_count;
} Base64FormatHeaderData;

/* Read format header */
PrecompFormatHeaderData* base64_read_format_header(RecursionContext* context, byte precomp_hdr_flags, 
                                                    SupportedFormats precomp_hdr_format) {
    Base64FormatHeaderData* fmt_hdr = (Base64FormatHeaderData*)calloc(1, sizeof(Base64FormatHeaderData));
    if (!fmt_hdr) return NULL;

    int line_case = (int)(precomp_hdr_flags & 0x0C);

    /* restore Base64 "header" */
    size_t base64_header_length = fin_fget_vlint(context->fin);

    print_to_log(PRECOMP_DEBUG_LOG, "Base64 header length: %zu\n", base64_header_length);
    
    fmt_hdr->base64_stream_hdr = (unsigned char*)malloc(base64_header_length);
    if (!fmt_hdr->base64_stream_hdr) {
        free(fmt_hdr);
        return NULL;
    }
    fmt_hdr->base64_stream_hdr_size = base64_header_length;
    fin_read(context->fin, (char*)fmt_hdr->base64_stream_hdr, base64_header_length);

    /* read line length list */
    size_t line_count = fin_fget_vlint(context->fin);
    fmt_hdr->base64_line_len_count = line_count;
    
    fmt_hdr->base64_line_len = (unsigned int*)calloc(line_count, sizeof(unsigned int));
    if (!fmt_hdr->base64_line_len) {
        free(fmt_hdr->base64_stream_hdr);
        free(fmt_hdr);
        return NULL;
    }

    if (line_case == 2) {
        for (size_t i = 0; i < line_count; i++) {
            fmt_hdr->base64_line_len[i] = (unsigned int)fin_get(context->fin);
        }
    } else {
        fmt_hdr->base64_line_len[0] = (unsigned int)fin_get(context->fin);
        for (size_t i = 1; i < line_count; i++) {
            fmt_hdr->base64_line_len[i] = fmt_hdr->base64_line_len[0];
        }
        if (line_case == 1) {
            fmt_hdr->base64_line_len[line_count - 1] = (unsigned int)fin_get(context->fin);
        }
    }

    fmt_hdr->base.original_size = fin_fget_vlint(context->fin);
    fmt_hdr->base.precompressed_size = fin_fget_vlint(context->fin);

    if ((precomp_hdr_flags & 0x80) == 0x80) {
        fmt_hdr->base.recursion_data_size = fin_fget_vlint(context->fin);
    }
    
    return (PrecompFormatHeaderData*)fmt_hdr;
}

/* Write pre-recursion data */
void base64_write_pre_recursion_data(RecursionContext* context, PrecompFormatHeaderData* precomp_hdr_data) {
    Base64FormatHeaderData* precomp_b64_hdr_data = (Base64FormatHeaderData*)precomp_hdr_data;
    
    /* Write Base64 header */
    fout_put(context->fout, precomp_b64_hdr_data->base64_stream_hdr[0] + 1);
    fout_write(context->fout, (const char*)(precomp_b64_hdr_data->base64_stream_hdr + 1), 
               precomp_b64_hdr_data->base64_stream_hdr_size - 1);
}

/* Recompress base64 data */
void base64_recompress(IStreamLike* precompressed_input, OStreamLike* recompressed_stream, 
                       PrecompFormatHeaderData* precomp_hdr_data, SupportedFormats precomp_hdr_format, 
                       const Tools* tools) {
    Base64FormatHeaderData* precomp_b64_hdr_data = (Base64FormatHeaderData*)precomp_hdr_data;
    
    print_to_log(PRECOMP_DEBUG_LOG, "Decompressed data - Base64\n");

    if (precomp_b64_hdr_data->base.recursion_data_size > 0) {
        print_to_log(PRECOMP_DEBUG_LOG, "Recursion data length: %lli\n", precomp_b64_hdr_data->base.recursion_data_size);
    } else {
        print_to_log(PRECOMP_DEBUG_LOG, "Encoded length: %lli - decoded length: %lli\n", 
                     precomp_b64_hdr_data->base.original_size, precomp_b64_hdr_data->base.precompressed_size);
    }

    base64_reencode(precompressed_input, recompressed_stream, precomp_b64_hdr_data->base64_line_len, 
                    precomp_b64_hdr_data->base64_line_len_count, 
                    precomp_b64_hdr_data->base.original_size, precomp_b64_hdr_data->base.precompressed_size);
}

/* Free base64 format header data */
void base64_free_format_header(PrecompFormatHeaderData* hdr) {
    if (hdr) {
        Base64FormatHeaderData* b64_hdr = (Base64FormatHeaderData*)hdr;
        if (b64_hdr->base64_stream_hdr) free(b64_hdr->base64_stream_hdr);
        if (b64_hdr->base64_line_len) free(b64_hdr->base64_line_len);
        free(b64_hdr);
    }
}
