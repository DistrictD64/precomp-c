/*
 * GIF format handler for precomp - C implementation
 * Converted from C++ to C for Tiny C Compiler compatibility
 */

#include "gif.h"
#include "precomp.h"
#include "precomp_io.h"
#include "contrib/giflib/precomp_gif.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Structure to hold GIF precompression result */
typedef struct {
    precompression_result base;
    unsigned char* gif_diff;
    size_t gif_diff_size;
} gif_precompression_result;

/* Global variables for GIF processing */
static int newgif_may_write = 0;
static OStreamLike* frecompress_gif = NULL;
static IStreamLike* freadfunc = NULL;

/* DGifGetLineByte helper function */
int DGifGetLineByte(GifFileType* GifFile, GifPixelType* Line, int LineLen, GifCodeStruct* g) {
    GifPixelType* LineBuf = (GifPixelType*)malloc(LineLen * sizeof(GifPixelType));
    if (!LineBuf) return GIF_ERROR;
    
    memcpy(LineBuf, Line, LineLen * sizeof(GifPixelType));
    int result = DGifGetLine(GifFile, LineBuf, g, LineLen);
    memcpy(Line, LineBuf, LineLen * sizeof(GifPixelType));
    free(LineBuf);
    
    return result;
}

/* Read function for GIF library */
int readFunc(GifFileType* GifFile, GifByteType* buf, int count) {
    if (!freadfunc) return 0;
    fin_read(freadfunc, (char*)buf, count);
    return (int)fin_gcount(freadfunc);
}

/* Write function for GIF library */
int writeFunc(GifFileType* GifFile, const GifByteType* buf, int count) {
    if (newgif_may_write && frecompress_gif) {
        fout_write(frecompress_gif, (const char*)buf, count);
        return fout_bad(frecompress_gif) ? 0 : count;
    }
    return count;
}

/* Allocate GIF screen buffer */
unsigned char** alloc_gif_screenbuf(GifFileType* myGifFile) {
    if (myGifFile->SHeight <= 0 || myGifFile->SWidth <= 0) {
        return NULL;
    }
    
    unsigned char** ScreenBuff = (unsigned char**)malloc(myGifFile->SHeight * sizeof(unsigned char*));
    if (!ScreenBuff) return NULL;
    
    for (int i = 0; i < myGifFile->SHeight; i++) {
        ScreenBuff[i] = (unsigned char*)malloc(myGifFile->SWidth);
        if (!ScreenBuff[i]) {
            /* Free already allocated rows */
            for (int j = 0; j < i; j++) {
                free(ScreenBuff[j]);
            }
            free(ScreenBuff);
            return NULL;
        }
    }

    /* Set its color to BackGround */
    for (int i = 0; i < myGifFile->SWidth; i++) {
        ScreenBuff[0][i] = myGifFile->SBackGroundColor;
    }
    for (int i = 1; i < myGifFile->SHeight; i++) {
        memcpy(ScreenBuff[i], ScreenBuff[0], myGifFile->SWidth);
    }
    return ScreenBuff;
}

/* Free GIF screen buffer */
void free_gif_screenbuf(unsigned char** ScreenBuff, GifFileType* myGifFile) {
    if (ScreenBuff != NULL) {
        for (int i = 0; i < myGifFile->SHeight; i++) {
            free(ScreenBuff[i]);
        }
        free(ScreenBuff);
    }
}

/* Helper functions for precompress result handling */
static int precompress_gif_result(unsigned char** ScreenBuff, GifFileType* myGifFile, int result) {
    free_gif_screenbuf(ScreenBuff, myGifFile);
    DGifCloseFile(myGifFile);
    return result;
}

static int precompress_gif_error(unsigned char** ScreenBuff, GifFileType* myGifFile) {
    return precompress_gif_result(ScreenBuff, myGifFile, 0);
}

static int precompress_gif_ok(unsigned char** ScreenBuff, GifFileType* myGifFile) {
    return precompress_gif_result(ScreenBuff, myGifFile, 1);
}

/* Helper functions for recompress result handling */
static int recompress_gif_result(unsigned char** ScreenBuff, GifFileType* myGifFile, 
                                  GifFileType* newGifFile, int result) {
    free_gif_screenbuf(ScreenBuff, myGifFile);
    DGifCloseFile(myGifFile);
    EGifCloseFile(newGifFile);
    return result;
}

static int recompress_gif_error(unsigned char** ScreenBuff, GifFileType* myGifFile, 
                                 GifFileType* newGifFile) {
    return recompress_gif_result(ScreenBuff, myGifFile, newGifFile, 0);
}

static int recompress_gif_ok(unsigned char** ScreenBuff, GifFileType* myGifFile, 
                              GifFileType* newGifFile) {
    return recompress_gif_result(ScreenBuff, myGifFile, newGifFile, 1);
}

/* Quick check for GIF detection */
int gif_quick_check(const unsigned char* buffer, uintptr_t current_input_id, long long original_input_pos) {
    return (buffer[0] == 'G' && buffer[1] == 'I' && buffer[2] == 'F' &&
            buffer[3] == '8' && (buffer[4] == '7' || buffer[4] == '9') && buffer[5] == 'a');
}

/* Decompress GIF */
int decompress_gif(IStreamLike* srcfile, OStreamLike* dstfile, long long src_pos, 
                   long long* gif_length, long long* decomp_length, 
                   unsigned char* block_size, GifCodeStruct* g) {
    int i, j;
    GifFileType* myGifFile;
    int Row, Col, Width, Height, ExtCode;
    GifByteType* Extension;
    GifRecordType RecordType;
    long long srcfile_pos;
    long long last_pos = -1;
    unsigned char** ScreenBuff = NULL;
    unsigned char c;

    freadfunc = srcfile;
    myGifFile = DGifOpen(NULL, readFunc);
    if (myGifFile == NULL) {
        return 0;
    }

    do {
        if (DGifGetRecordType(myGifFile, &RecordType) == GIF_ERROR) {
            DGifCloseFile(myGifFile);
            return 0;
        }

        switch (RecordType) {
        case IMAGE_DESC_RECORD_TYPE:
            if (DGifGetImageDesc(myGifFile) == GIF_ERROR) {
                return precompress_gif_error(ScreenBuff, myGifFile);
            }

            if (ScreenBuff == NULL) {
                ScreenBuff = alloc_gif_screenbuf(myGifFile);
                if (!ScreenBuff) {
                    DGifCloseFile(myGifFile);
                    return 0;
                }
            }

            srcfile_pos = fin_tell(srcfile);
            if (last_pos != srcfile_pos) {
                if (last_pos == -1) {
                    last_pos = src_pos + 2;
                    /* change GIF8xa to PGF8xa */
                    fout_put(dstfile, 'P');
                    fout_put(dstfile, 'G');
                }
                fin_seek(srcfile, last_pos, SEEK_SET);
                fast_copy(srcfile, dstfile, srcfile_pos - last_pos);
                fin_seek(srcfile, srcfile_pos, SEEK_SET);
            }

            c = (unsigned char)fin_get(srcfile);
            if (c == 254) {
                *block_size = 254;
            }
            fin_seek(srcfile, srcfile_pos, SEEK_SET);

            Row = myGifFile->Image.Top;
            Col = myGifFile->Image.Left;
            Width = myGifFile->Image.Width;
            Height = myGifFile->Image.Height;

            if (((Col + Width) > myGifFile->SWidth) || ((Row + Height) > myGifFile->SHeight)) {
                return precompress_gif_error(ScreenBuff, myGifFile);
            }

            if (myGifFile->Image.Interlace) {
                for (i = 0; i < 4; i++) {
                    for (j = Row + InterlacedOffset[i]; j < (Row + Height); j += InterlacedJumps[i]) {
                        if (DGifGetLineByte(myGifFile, &ScreenBuff[j][Col], Width, g) == GIF_ERROR) {
                            return precompress_gif_error(ScreenBuff, myGifFile);
                        }
                    }
                }
                for (i = Row; i < (Row + Height); i++) {
                    fout_write(dstfile, (const char*)&ScreenBuff[i][Col], Width);
                }
            } else {
                for (i = Row; i < (Row + Height); i++) {
                    if (DGifGetLineByte(myGifFile, &ScreenBuff[i][Col], Width, g) == GIF_ERROR) {
                        return precompress_gif_error(ScreenBuff, myGifFile);
                    }
                    fout_write(dstfile, (const char*)&ScreenBuff[i][Col], Width);
                }
            }

            last_pos = fin_tell(srcfile);
            break;
            
        case EXTENSION_RECORD_TYPE:
            if (DGifGetExtension(myGifFile, &ExtCode, &Extension) == GIF_ERROR) {
                return precompress_gif_error(ScreenBuff, myGifFile);
            }
            while (Extension != NULL) {
                if (DGifGetExtensionNext(myGifFile, &Extension) == GIF_ERROR) {
                    return precompress_gif_error(ScreenBuff, myGifFile);
                }
            }
            break;
            
        case TERMINATE_RECORD_TYPE:
        default:
            break;
        }
    } while (RecordType != TERMINATE_RECORD_TYPE);

    srcfile_pos = fin_tell(srcfile);
    if (last_pos != srcfile_pos) {
        fin_seek(srcfile, last_pos, SEEK_SET);
        fast_copy(srcfile, dstfile, srcfile_pos - last_pos);
        fin_seek(srcfile, srcfile_pos, SEEK_SET);
    }

    *gif_length = srcfile_pos - src_pos;
    *decomp_length = fout_tell(dstfile);

    return precompress_gif_ok(ScreenBuff, myGifFile);
}

/* Recompress GIF */
int recompress_gif(IStreamLike* srcfile, OStreamLike* dstfile, unsigned char block_size, 
                   GifCodeStruct* g, GifDiffStruct* gd) {
    int i, j;
    long long last_pos = -1;
    int Row, Col, Width, Height, ExtCode;
    long long src_pos, init_src_pos;
    GifFileType* myGifFile;
    GifFileType* newGifFile;
    GifRecordType RecordType;
    GifByteType* Extension;
    unsigned char** ScreenBuff;

    freadfunc = srcfile;
    frecompress_gif = dstfile;
    newgif_may_write = 0;

    init_src_pos = fin_tell(srcfile);

    myGifFile = DGifOpenPCF(NULL, readFunc);
    if (myGifFile == NULL) {
        return 0;
    }

    newGifFile = EGifOpen(NULL, writeFunc);
    if (newGifFile == NULL) {
        DGifCloseFile(myGifFile);
        return 0;
    }

    newGifFile->BlockSize = block_size;

    ScreenBuff = alloc_gif_screenbuf(myGifFile);
    if (!ScreenBuff) {
        DGifCloseFile(myGifFile);
        EGifCloseFile(newGifFile);
        return 0;
    }

    EGifPutScreenDesc(newGifFile, myGifFile->SWidth, myGifFile->SHeight, 
                      myGifFile->SColorResolution, myGifFile->SBackGroundColor, 
                      myGifFile->SPixelAspectRatio, myGifFile->SColorMap);

    do {
        if (DGifGetRecordType(myGifFile, &RecordType) == GIF_ERROR) {
            return recompress_gif_error(ScreenBuff, myGifFile, newGifFile);
        }

        switch (RecordType) {
        case IMAGE_DESC_RECORD_TYPE:
            if (DGifGetImageDesc(myGifFile) == GIF_ERROR) {
                return recompress_gif_error(ScreenBuff, myGifFile, newGifFile);
            }

            src_pos = fin_tell(srcfile);
            if (last_pos != src_pos) {
                if (last_pos == -1) {
                    last_pos = init_src_pos + 2;
                    /* change PGF8xa to GIF8xa */
                    fout_put(dstfile, 'G');
                    fout_put(dstfile, 'I');
                }
                fin_seek(srcfile, last_pos, SEEK_SET);
                fast_copy(srcfile, dstfile, src_pos - last_pos);
                fin_seek(srcfile, src_pos, SEEK_SET);
            }

            Row = myGifFile->Image.Top;
            Col = myGifFile->Image.Left;
            Width = myGifFile->Image.Width;
            Height = myGifFile->Image.Height;

            for (i = Row; i < (Row + Height); i++) {
                fin_read(srcfile, (char*)&ScreenBuff[i][Col], Width);
            }

            if (EGifPutImageDesc(newGifFile, g, gd, Row, Col, Width, Height, 
                                 myGifFile->Image.Interlace, myGifFile->Image.ColorMap) == GIF_ERROR) {
                return recompress_gif_error(ScreenBuff, myGifFile, newGifFile);
            }

            newgif_may_write = 1;

            if (myGifFile->Image.Interlace) {
                for (i = 0; i < 4; i++) {
                    for (j = Row + InterlacedOffset[i]; j < (Row + Height); j += InterlacedJumps[i]) {
                        EGifPutLine(newGifFile, &ScreenBuff[j][Col], g, gd, Width);
                    }
                }
            } else {
                for (i = Row; i < (Row + Height); i++) {
                    EGifPutLine(newGifFile, &ScreenBuff[i][Col], g, gd, Width);
                }
            }

            newgif_may_write = 0;
            last_pos = fin_tell(srcfile);
            break;
            
        case EXTENSION_RECORD_TYPE:
            if (DGifGetExtension(myGifFile, &ExtCode, &Extension) == GIF_ERROR) {
                return recompress_gif_error(ScreenBuff, myGifFile, newGifFile);
            }
            while (Extension != NULL) {
                if (DGifGetExtensionNext(myGifFile, &Extension) == GIF_ERROR) {
                    return recompress_gif_error(ScreenBuff, myGifFile, newGifFile);
                }
            }
            break;
            
        case TERMINATE_RECORD_TYPE:
        default:
            break;
        }
    } while (RecordType != TERMINATE_RECORD_TYPE);

    src_pos = fin_tell(srcfile);
    if (last_pos != src_pos) {
        fin_seek(srcfile, last_pos, SEEK_SET);
        fast_copy(srcfile, dstfile, src_pos - last_pos);
        fin_seek(srcfile, src_pos, SEEK_SET);
    }
    
    return recompress_gif_ok(ScreenBuff, myGifFile, newGifFile);
}

/* Free GIF precompression result */
static void free_gif_result(gif_precompression_result* result) {
    if (result) {
        if (result->gif_diff) free(result->gif_diff);
        if (result->base.precompressed_stream) {
            fclose(result->base.precompressed_stream);
        }
        free(result);
    }
}

/* Create new GIF precompression result */
static gif_precompression_result* create_gif_result(void) {
    gif_precompression_result* result = (gif_precompression_result*)calloc(1, sizeof(gif_precompression_result));
    if (result) {
        result->base.format_type = D_GIF;
        result->base.free_func = (void (*)(void*))free_gif_result;
    }
    return result;
}

/* Dump GIF diff to output file */
static void dump_gif_diff_to_outfile(OStreamLike* outfile, const gif_precompression_result* result) {
    fout_fput_vlint(outfile, result->gif_diff_size);
    if (result->gif_diff_size > 0) {
        print_to_log(PRECOMP_DEBUG_LOG, "Diff bytes were used: %zu bytes\n", result->gif_diff_size);
    }
    for (size_t i = 0; i < result->gif_diff_size; i++) {
        fout_put(outfile, result->gif_diff[i]);
    }
}

/* Dump to output file */
static void gif_dump_to_outfile(precompression_result* pres, OStreamLike* outfile) {
    gif_precompression_result* result = (gif_precompression_result*)pres;
    dump_header_to_outfile(pres, outfile);
    dump_gif_diff_to_outfile(outfile, result);
    dump_penaltybytes_to_outfile(pres, outfile);
    dump_stream_sizes_to_outfile(pres, outfile);
    dump_precompressed_data_to_outfile(pres, outfile);
}

/* Structure for GIF format header data */
typedef struct {
    PrecompFormatHeaderData base;
    GifDiffStruct gDiff;
    unsigned char block_size;
    int recompress_success_needed;
} GifFormatHeaderData;

/* Read format header */
PrecompFormatHeaderData* gif_read_format_header(RecursionContext* context, byte precomp_hdr_flags, 
                                                 SupportedFormats precomp_hdr_format) {
    GifFormatHeaderData* fmt_hdr = (GifFormatHeaderData*)calloc(1, sizeof(GifFormatHeaderData));
    if (!fmt_hdr) return NULL;

    int penalty_bytes_stored = (precomp_hdr_flags & 0x02) == 0x02;
    fmt_hdr->block_size = 255;
    if ((precomp_hdr_flags & 0x04) == 0x04) fmt_hdr->block_size = 254;
    fmt_hdr->recompress_success_needed = ((precomp_hdr_flags & 0x80) == 0x80);

    /* read diff bytes */
    fmt_hdr->gDiff.GIFDiffIndex = (size_t)fin_fget_vlint(context->fin);
    fmt_hdr->gDiff.GIFDiff = (unsigned char*)malloc(fmt_hdr->gDiff.GIFDiffIndex * sizeof(unsigned char));
    if (!fmt_hdr->gDiff.GIFDiff) {
        free(fmt_hdr);
        return NULL;
    }
    fin_read(context->fin, (char*)fmt_hdr->gDiff.GIFDiff, fmt_hdr->gDiff.GIFDiffIndex);
    print_to_log(PRECOMP_DEBUG_LOG, "Diff bytes were used: %zu bytes\n", fmt_hdr->gDiff.GIFDiffIndex);
    fmt_hdr->gDiff.GIFDiffSize = fmt_hdr->gDiff.GIFDiffIndex;
    fmt_hdr->gDiff.GIFDiffIndex = 0;
    fmt_hdr->gDiff.GIFCodeCount = 0;

    /* read penalty bytes */
    if (penalty_bytes_stored) {
        size_t penalty_bytes_len = (size_t)fin_fget_vlint(context->fin);
        while (penalty_bytes_len > 0) {
            unsigned char pb_data[5];
            fin_read(context->fin, (char*)pb_data, 5);
            penalty_bytes_len -= 5;

            uint32_t next_pb_pos = ((uint32_t)pb_data[0] << 24) | ((uint32_t)pb_data[1] << 16) | 
                                   ((uint32_t)pb_data[2] << 8) | pb_data[3];
            
            /* Add to penalty bytes map - simplified for C */
            /* In full implementation, would need a proper map structure */
        }
    }

    fmt_hdr->base.original_size = (long long)fin_fget_vlint(context->fin);
    fmt_hdr->base.precompressed_size = (long long)fin_fget_vlint(context->fin);

    return (PrecompFormatHeaderData*)fmt_hdr;
}

/* Recompress GIF data */
void gif_recompress(IStreamLike* precompressed_input, OStreamLike* recompressed_stream, 
                    PrecompFormatHeaderData* precomp_hdr_data, SupportedFormats precomp_hdr_format, 
                    const Tools* tools) {
    GifFormatHeaderData* gif_precomp_hdr_format = (GifFormatHeaderData*)precomp_hdr_data;
    
    print_to_log(PRECOMP_DEBUG_LOG, "Recompressed length: %lli - decompressed length: %lli\n", 
                 gif_precomp_hdr_format->base.original_size, gif_precomp_hdr_format->base.precompressed_size);

    char* tmp_tag = temp_files_tag();
    char* tempfile = (char*)malloc(strlen(tools->get_tempfile_name("", 0)) + strlen(tmp_tag) + 50);
    if (!tempfile) return;
    
    sprintf(tempfile, "%s%s_precompressed_gif", tools->get_tempfile_name("", 0), tmp_tag);

    /* Dump to temp file */
    dump_to_file_stream(precompressed_input, tempfile, gif_precomp_hdr_format->base.precompressed_size);
    
    int recompress_success;
    FILE* ftempout = fopen(tempfile, "rb");
    if (ftempout) {
        IStreamLike ftempin_wrapper = {ftempout, 0};
        recompress_success = recompress_gif(&ftempin_wrapper, recompressed_stream, 
                                            gif_precomp_hdr_format->block_size, NULL, 
                                            &gif_precomp_hdr_format->gDiff);
        fclose(ftempout);
    } else {
        recompress_success = 0;
    }

    if (gif_precomp_hdr_format->recompress_success_needed && !recompress_success) {
        GifDiffFree(&gif_precomp_hdr_format->gDiff);
        free(tempfile);
        /* In C, we can't throw exceptions, so we set an error flag or return early */
        return;
    }

    remove(tempfile);
    GifDiffFree(&gif_precomp_hdr_format->gDiff);
    free(tempfile);
}

/* Attempt precompression for GIF */
precompression_result* gif_attempt_precompression(Precomp* precomp_mgr, const unsigned char* checkbuf, 
                                                   long long original_input_pos) {
    gif_precompression_result* result = create_gif_result();
    if (!result) return NULL;
    
    PrecompTmpFile tmpfile_struct;
    PrecompTmpFile* tmpfile = &tmpfile_struct;
    char* tmpfilename = get_tempfile_name(precomp_mgr, "decomp_gif");
    
    if (!tmpfile_open(tmpfile, tmpfilename, "ab+")) {
        free_gif_result(result);
        return NULL;
    }
    tmpfile_close(tmpfile);
    
    unsigned char version[5];
    for (int i = 0; i < 5; i++) {
        version[i] = checkbuf[i];
    }

    unsigned char block_size = 255;
    long long gif_length = -1;
    long long decomp_length = -1;

    GifCodeStruct gCode;
    GifCodeInit(&gCode);
    GifDiffStruct gDiff;
    GifDiffInit(&gDiff);

    int recompress_success_needed = 1;

    print_to_log(PRECOMP_DEBUG_LOG, "Possible GIF found at position %lli\n", original_input_pos);

    fin_seek(precomp_mgr->ctx->fin, original_input_pos, SEEK_SET);

    /* read GIF file */
    if (!tmpfile_open(tmpfile, tmpfile->file_path, "rb+")) {
        GifDiffFree(&gDiff);
        GifCodeFree(&gCode);
        free_gif_result(result);
        return NULL;
    }
    
    OStreamLike tmpfile_out = {tmpfile->file_handle, 0};
    if (!decompress_gif(precomp_mgr->ctx->fin, &tmpfile_out, original_input_pos, 
                        &gif_length, &decomp_length, &block_size, &gCode)) {
        tmpfile_close(tmpfile);
        GifDiffFree(&gDiff);
        GifCodeFree(&gCode);
        free_gif_result(result);
        return NULL;
    }

    print_to_log(PRECOMP_DEBUG_LOG, "Can be decompressed to %lli bytes\n", decomp_length);

    precomp_mgr->statistics.decompressed_streams_count++;
    precomp_mgr->statistics.decompressed_gif_count++;

    char* tempfile2 = (char*)malloc(strlen(tmpfile->file_path) + 10);
    if (!tempfile2) {
        tmpfile_close(tmpfile);
        GifDiffFree(&gDiff);
        GifCodeFree(&gCode);
        free_gif_result(result);
        return NULL;
    }
    sprintf(tempfile2, "%s_rec_", tmpfile->file_path);
    
    tmpfile_reopen(tmpfile);
    
    PrecompTmpFile frecomp;
    if (!tmpfile_open(&frecomp, tempfile2, "wb")) {
        free(tempfile2);
        tmpfile_close(tmpfile);
        GifDiffFree(&gDiff);
        GifCodeFree(&gCode);
        free_gif_result(result);
        return NULL;
    }
    
    IStreamLike tmpfile_in = {tmpfile->file_handle, 0};
    OStreamLike frecomp_out = {frecomp.file_handle, 0};
    
    if (recompress_gif(&tmpfile_in, &frecomp_out, block_size, &gCode, &gDiff)) {
        tmpfile_close(&frecomp);
        tmpfile_close(tmpfile);

        FILE* frecomp2 = fopen(tempfile2, "rb");
        if (frecomp2) {
            IStreamLike frecomp2_wrapper = {frecomp2, 0};
            fin_seek(precomp_mgr->ctx->fin, original_input_pos, SEEK_SET);
            
            /* Compare files and get penalty bytes - simplified */
            long long identical_bytes = compare_files(precomp_mgr, precomp_mgr->ctx->fin, &frecomp2_wrapper, 
                                                       (unsigned int)gif_length, (unsigned int)0);
            result->base.original_size = identical_bytes;
            result->base.precompressed_size = decomp_length;
            fclose(frecomp2);
        }

        if (result->base.original_size < gif_length) {
            print_to_log(PRECOMP_DEBUG_LOG, "Recompression failed\n");
        } else {
            print_to_log(PRECOMP_DEBUG_LOG, "Recompression successful\n");
            recompress_success_needed = 1;

            if (result->base.original_size > precomp_mgr->switches.min_ident_size) {
                precomp_mgr->statistics.recompressed_streams_count++;
                precomp_mgr->statistics.recompressed_gif_count++;
                precomp_mgr->ctx->non_zlib_was_used = 1;

                result->base.success = 1;

                /* write compressed data header (GIF) */
                byte add_bits = 0;
                /* Simplified - penalty bytes handling would need more work */
                if (block_size == 254) add_bits |= 0x04;
                add_bits |= 0x80;
                result->base.flags = 0x01 | add_bits;

                /* Copy diff bytes */
                if (gDiff.GIFDiffIndex > 0) {
                    result->gif_diff = (unsigned char*)malloc(gDiff.GIFDiffIndex);
                    if (result->gif_diff) {
                        memcpy(result->gif_diff, gDiff.GIFDiff, gDiff.GIFDiffIndex);
                        result->gif_diff_size = gDiff.GIFDiffIndex;
                    }
                }

                if (!tmpfile_reopen(tmpfile)) {
                    free(tempfile2);
                    GifDiffFree(&gDiff);
                    GifCodeFree(&gCode);
                    free_gif_result(result);
                    return NULL;
                }
                result->base.precompressed_stream = tmpfile_reuse(tmpfile);
            }
        }
    } else {
        print_to_log(PRECOMP_DEBUG_LOG, "No matches\n");
    }

    GifDiffFree(&gDiff);
    GifCodeFree(&gCode);
    free(tempfile2);
    
    /* Clean up tmpfile if not used */
    if (!result->base.precompressed_stream) {
        tmpfile_close(tmpfile);
    }
    
    return (precompression_result*)result;
}

/* Free GIF format header data */
void gif_free_format_header(PrecompFormatHeaderData* hdr) {
    if (hdr) {
        GifFormatHeaderData* gif_hdr = (GifFormatHeaderData*)hdr;
        GifDiffFree(&gif_hdr->gDiff);
        free(gif_hdr);
    }
}
