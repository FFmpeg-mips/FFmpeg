/*
 * Format Conversion Utils
 * Copyright (c) 2000, 2001 Fabrice Bellard
 * Copyright (c) 2002-2004 Michael Niedermayer <michaelni@gmx.at>
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "avcodec.h"
#include "fmtconvert.h"
#include "libavutil/common.h"

static void int32_to_float_fmul_scalar_c(float *dst, const int *src, float mul, int len){
    int i;
    for(i=0; i<len; i++)
        dst[i] = src[i] * mul;
}

static void int32_to_fixed_fmul_scalar_c(int16_t *dst, const int *src, int mul, int len) {
    int i;
    for(i=0; i<len; i++)
    dst[i] = (src[i] * mul + 0x8000) >> 16;
}

static av_always_inline int float_to_int16_one(const float *src){
    return av_clip_int16(lrintf(*src));
}

static av_always_inline int fixed_to_int16_one(const int *src)
{
    return av_clip_int16_c(*src);
}

static void float_to_int16_c(int16_t *dst, const float *src, long len)
{
    int i;
    for(i=0; i<len; i++)
        dst[i] = float_to_int16_one(src+i);
}

static void float_to_int16_interleave_c(int16_t *dst, const float **src,
                                        long len, int channels)
{
    int i,j,c;
    if(channels==2){
        for(i=0; i<len; i++){
            dst[2*i]   = float_to_int16_one(src[0]+i);
            dst[2*i+1] = float_to_int16_one(src[1]+i);
        }
    }else{
        for(c=0; c<channels; c++)
            for(i=0, j=c; i<len; i++, j+=channels)
                dst[j] = float_to_int16_one(src[c]+i);
    }
}

static void fixed_to_int16_interleave_c(int16_t *dst, const int **src,
                                        long len, int channels)
{
    int i,j,c;
    if(channels==2) {
        for(i=0; i<len; i++) {
            dst[2*i] = fixed_to_int16_one(src[0]+i);
            dst[2*i+1] = fixed_to_int16_one(src[1]+i);
        }
    }
    else {
        if(channels==6) {
            for(i=0; i<len; i++) {
                dst[6*i] = fixed_to_int16_one(src[0]+i);
                dst[6*i+1] = fixed_to_int16_one(src[1]+i);
                dst[6*i+2] = fixed_to_int16_one(src[2]+i);
                dst[6*i+3] = fixed_to_int16_one(src[3]+i);
                dst[6*i+4] = fixed_to_int16_one(src[4]+i);
                dst[6*i+5] = fixed_to_int16_one(src[5]+i);
            }
        }
        else {
            for(c=0; c<channels; c++)
                for(i=0, j=c; i<len; i++, j+=channels)
                    dst[j] = fixed_to_int16_one(src[c]+i);
        }
    }
}

void ff_float_interleave_c(float *dst, const float **src, unsigned int len,
                           int channels)
{
    int j, c;
    unsigned int i;
    if (channels == 2) {
        for (i = 0; i < len; i++) {
            dst[2*i]   = src[0][i];
            dst[2*i+1] = src[1][i];
        }
    } else if (channels == 1 && len < INT_MAX / sizeof(float)) {
        memcpy(dst, src[0], len * sizeof(float));
    } else {
        for (c = 0; c < channels; c++)
            for (i = 0, j = c; i < len; i++, j += channels)
                dst[j] = src[c][i];
    }
}

void ff_fixed_interleave_c(int *dst, const int **src, unsigned int len,
                           int channels)
{
    int j, c;
    unsigned int i;
    if (channels == 6) {
        for (i = 0; i < len; i++) {
            dst[6*i]   = src[0][i];
            dst[6*i+1] = src[1][i];
            dst[6*i+2] = src[2][i];
            dst[6*i+3] = src[3][i];
            dst[6*i+4] = src[4][i];
            dst[6*i+5] = src[5][i];
        }
    }
    else if (channels == 2) {
        for (i = 0; i < len; i++) {
            dst[2*i]   = src[0][i];
            dst[2*i+1] = src[1][i];
        }
    } else if (channels == 1 && len < INT_MAX / sizeof(int)) {
        memcpy(dst, src[0], len * sizeof(int));
    } else {
        for (c = 0; c < channels; c++)
            for (i = 0, j = c; i < len; i++, j += channels)
                dst[j] = src[c][i];
    }
}

av_cold void ff_fmt_convert_init(FmtConvertContext *c, AVCodecContext *avctx)
{
    c->int32_to_float_fmul_scalar = int32_to_float_fmul_scalar_c;
    c->int32_to_fixed_fmul_scalar = int32_to_fixed_fmul_scalar_c;
    c->fixed_to_int16_interleave  = fixed_to_int16_interleave_c;
    c->fixed_interleave           = ff_fixed_interleave_c;
    c->float_to_int16             = float_to_int16_c;
    c->float_to_int16_interleave  = float_to_int16_interleave_c;
    c->float_interleave           = ff_float_interleave_c;

    if (ARCH_ARM) ff_fmt_convert_init_arm(c, avctx);
    if (HAVE_ALTIVEC) ff_fmt_convert_init_altivec(c, avctx);
    if (ARCH_X86) ff_fmt_convert_init_x86(c, avctx);
    if (HAVE_MIPSFPU) ff_fmt_convert_init_mips(c);
    if (HAVE_MIPSDSPR1) ff_fmt_convert_init_mips_fixed(c, avctx);
}

/* ffdshow custom code */
void float_interleave(float *dst, const float **src, long len, int channels)
{
    int i,j,c;
    if(channels==2){
        for(i=0; i<len; i++){
            dst[2*i]   = src[0][i] / 32768.0f;
            dst[2*i+1] = src[1][i] / 32768.0f;
        }
    }else{
        for(c=0; c<channels; c++)
            for(i=0, j=c; i<len; i++, j+=channels)
                dst[j] = src[c][i] / 32768.0f;
    }
}

void float_interleave_noscale(float *dst, const float **src, long len, int channels)
{
    int i,j,c;
    if(channels==2){
        for(i=0; i<len; i++){
            dst[2*i]   = src[0][i];
            dst[2*i+1] = src[1][i];
        }
    }else{
        for(c=0; c<channels; c++)
            for(i=0, j=c; i<len; i++, j+=channels)
                dst[j] = src[c][i];
    }
}
