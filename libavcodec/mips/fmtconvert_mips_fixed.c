/*
 * Copyright (c) 2012
 *      MIPS Technologies, Inc., California.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the MIPS Technologies, Inc., nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE MIPS TECHNOLOGIES, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE MIPS TECHNOLOGIES, INC. BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Author:  Zoran Lukic (zlukic@mips.com)
 * Author:  Nedeljko Babic (nbabic@mips.com)
 *
 * Format Conversion Utils optimized for MIPS fixed-point architecture
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

/**
 * @file
 * Reference: libavcodec/fmtconvert.c
 */

#include "libavcodec/fmtconvert.h"

#if HAVE_INLINE_ASM
static void int32_to_fixed_fmul_scalar_mips(int16_t *dst, const int *src,
                                            int mul, int len)
{
    int i;
    int16_t temp1, temp3, temp5, temp7, temp9, temp11, temp13, temp15;

    for (i=0; i<len; i+=8) {
        __asm__ volatile (
            "lw     %[temp1],   0(%[src_i])         \n\t"
            "lw     %[temp3],   4(%[src_i])         \n\t"
            "lw     %[temp5],   8(%[src_i])         \n\t"
            "lw     %[temp7],   12(%[src_i])        \n\t"
            "lw     %[temp9],   16(%[src_i])        \n\t"
            "lw     %[temp11],  20(%[src_i])        \n\t"
            "lw     %[temp13],  24(%[src_i])        \n\t"
            "lw     %[temp15],  28(%[src_i])        \n\t"
            "mul    %[temp1],   %[temp1],   %[mul]  \n\t"
            "mul    %[temp3],   %[temp3],   %[mul]  \n\t"
            "mul    %[temp5],   %[temp5],   %[mul]  \n\t"
            "mul    %[temp7],   %[temp7],   %[mul]  \n\t"
            "mul    %[temp9],   %[temp9],   %[mul]  \n\t"
            "mul    %[temp11],  %[temp11],  %[mul]  \n\t"
            "mul    %[temp13],  %[temp13],  %[mul]  \n\t"
            "mul    %[temp15],  %[temp15],  %[mul]  \n\t"
            "shra_r.w   %[temp1],   %[temp1],   0x10    \n\t"
            "shra_r.w   %[temp3],   %[temp3],   0x10    \n\t"
            "shra_r.w   %[temp5],   %[temp5],   0x10    \n\t"
            "shra_r.w   %[temp7],   %[temp7],   0x10    \n\t"
            "shra_r.w   %[temp9],   %[temp9],   0x10    \n\t"
            "shra_r.w   %[temp11],  %[temp11],  0x10    \n\t"
            "shra_r.w   %[temp13],  %[temp13],  0x10    \n\t"
            "shra_r.w   %[temp15],  %[temp15],  0x10    \n\t"
            "sh     %[temp1],   0(%[dst_i])         \n\t"
            "sh     %[temp3],   2(%[dst_i])         \n\t"
            "sh     %[temp5],   4(%[dst_i])         \n\t"
            "sh     %[temp7],   6(%[dst_i])         \n\t"
            "sh     %[temp9],   8(%[dst_i])         \n\t"
            "sh     %[temp11],  10(%[dst_i])        \n\t"
            "sh     %[temp13],  12(%[dst_i])        \n\t"
            "sh     %[temp15],  14(%[dst_i])        \n\t"

            : [temp1] "=r" (temp1),   [temp11] "=r" (temp11),
              [temp13] "=r" (temp13), [temp15] "=r" (temp15),
              [temp3] "=r" (temp3),   [temp5] "=r" (temp5),
              [temp7] "=r" (temp7),   [temp9] "=r" (temp9)
            : [dst_i] "r" (dst+i),  [src_i] "r" (src+i),
              [mul] "r" (mul)
            : "memory"
        );
    }
}

static inline int16_t fixed_to_int16_one_mips(const int *src)
{
    int ret;

    __asm__ volatile (
        "lw         %[ret], 0(%[src])           \n\t"
        "shll_s.w   %[ret], %[ret],     16      \n\t"
        "sra        %[ret], 16                  \n\t"
        :[ret]"=&r"(ret)
        :[src]"r"(src)
        :"memory"
    );

    return ret;
}

static void fixed_to_int16_interleave_mips(int16_t *dst, const int **src,
                                    long len, int channels)
{
    int i,j,c;
    int temp, temp1, temp2, temp3;

    if(channels==2) {
        int *src_i= &src[0][0], *src_i1=&src[1][0];
        for(i=0; i<len; i+=8) {
            __asm__ volatile (
                "lw     %[temp],    0(%[src_i])             \n\t"
                "lw     %[temp1],   0(%[src_i1])            \n\t"
                "lw         %[temp2],   4(%[src_i])         \n\t"
                "lw         %[temp3],   4(%[src_i1])        \n\t"
                "shll_s.w   %[temp],    %[temp],    16      \n\t"
                "shll_s.w   %[temp1],   %[temp1],   16      \n\t"
                "shll_s.w   %[temp2],   %[temp2],   16      \n\t"
                "shll_s.w   %[temp3],   %[temp3],   16      \n\t"
                "sra        %[temp],    16                  \n\t"
                "sra        %[temp1],   16                  \n\t"
                "sra        %[temp2],   16                  \n\t"
                "sra        %[temp3],   16                  \n\t"
                "sh         %[temp],    0(%[dst])           \n\t"
                "sh         %[temp1],   2(%[dst])           \n\t"
                "sh         %[temp2],   4(%[dst])           \n\t"
                "sh         %[temp3],   6(%[dst])           \n\t"

                "lw         %[temp],    8(%[src_i])         \n\t"
                "lw         %[temp1],   8(%[src_i1])        \n\t"
                "lw         %[temp2],   12(%[src_i])        \n\t"
                "lw         %[temp3],   12(%[src_i1])       \n\t"
                "shll_s.w   %[temp],    %[temp],    16      \n\t"
                "shll_s.w   %[temp1],   %[temp1],   16      \n\t"
                "shll_s.w   %[temp2],   %[temp2],   16      \n\t"
                "shll_s.w   %[temp3],   %[temp3],   16      \n\t"
                "sra        %[temp],    16                  \n\t"
                "sra        %[temp1],   16                  \n\t"
                "sra        %[temp2],   16                  \n\t"
                "sra        %[temp3],   16                  \n\t"
                "sh         %[temp],    8(%[dst])           \n\t"
                "sh         %[temp1],   10(%[dst])          \n\t"
                "sh         %[temp2],   12(%[dst])          \n\t"
                "sh         %[temp3],   14(%[dst])          \n\t"

                "lw         %[temp],    16(%[src_i])        \n\t"
                "lw         %[temp1],   16(%[src_i1])       \n\t"
                "lw         %[temp2],   20(%[src_i])        \n\t"
                "lw         %[temp3],   20(%[src_i1])       \n\t"
                "shll_s.w   %[temp],    %[temp],    16      \n\t"
                "shll_s.w   %[temp1],   %[temp1],   16      \n\t"
                "shll_s.w   %[temp2],   %[temp2],   16      \n\t"
                "shll_s.w   %[temp3],   %[temp3],   16      \n\t"
                "sra        %[temp],    16                  \n\t"
                "sra        %[temp1],   16                  \n\t"
                "sra        %[temp2],   16                  \n\t"
                "sra        %[temp3],   16                  \n\t"
                "sh         %[temp],    16(%[dst])          \n\t"
                "sh         %[temp1],   18(%[dst])          \n\t"
                "sh         %[temp2],   20(%[dst])          \n\t"
                "sh         %[temp3],   22(%[dst])          \n\t"

                "lw         %[temp],    24(%[src_i])        \n\t"
                "lw         %[temp1],   24(%[src_i1])       \n\t"
                "lw         %[temp2],   28(%[src_i])        \n\t"
                "lw         %[temp3],   28(%[src_i1])       \n\t"
                "shll_s.w   %[temp],    %[temp],    16      \n\t"
                "shll_s.w   %[temp1],   %[temp1],   16      \n\t"
                "shll_s.w   %[temp2],   %[temp2],   16      \n\t"
                "shll_s.w   %[temp3],   %[temp3],   16      \n\t"
                "addiu      %[src_i],   32                  \n\t"
                "addiu      %[src_i1],  32                  \n\t"
                "sra        %[temp],    16                  \n\t"
                "sra        %[temp1],   16                  \n\t"
                "sra        %[temp2],   16                  \n\t"
                "sra        %[temp3],   16                  \n\t"
                "sh         %[temp],    24(%[dst])          \n\t"
                "sh         %[temp1],   26(%[dst])          \n\t"
                "sh         %[temp2],   28(%[dst])          \n\t"
                "sh         %[temp3],   30(%[dst])          \n\t"
                "addiu      %[dst],     32                  \n\t"
                :[temp]"=&r"(temp), [temp1]"=&r"(temp1),
                 [temp2]"=&r"(temp2), [temp3]"=&r"(temp3),
                 [dst]"+r"(dst), [src_i]"+r"(src_i), [src_i1]"+r"(src_i1)
                :
                :"memory"
            );
        }
    }
    else {
        if(channels==6) {
            int *src_i = &src[0][0], *src_i1 = &src[1][0], *src_i2 = &src[2][0], *src_i3 = &src[3][0];
            int *src_i4 = &src[4][0], *src_i5 = &src[5][0];

            for(i=0; i<len; i+=2) {
                __asm__ volatile (
                    "lw     %[temp],    0(%[src_i])             \n\t"
                    "lw     %[temp1],   0(%[src_i1])            \n\t"
                    "lw     %[temp2],   0(%[src_i2])            \n\t"
                    "lw     %[temp3],   0(%[src_i3])            \n\t"
                    "shll_s.w   %[temp],    %[temp],    16      \n\t"
                    "shll_s.w   %[temp1],   %[temp1],   16      \n\t"
                    "shll_s.w   %[temp2],   %[temp2],   16      \n\t"
                    "shll_s.w   %[temp3],   %[temp3],   16      \n\t"
                    "sra        %[temp],    16                  \n\t"
                    "sra        %[temp1],   16                  \n\t"
                    "sra        %[temp2],   16                  \n\t"
                    "sra        %[temp3],   16                  \n\t"
                    "sh         %[temp],    0(%[dst])           \n\t"
                    "sh         %[temp1],   2(%[dst])           \n\t"
                    "sh         %[temp2],   4(%[dst])           \n\t"
                    "sh         %[temp3],   6(%[dst])           \n\t"

                    "lw         %[temp],    0(%[src_i4])        \n\t"
                    "lw         %[temp1],   0(%[src_i5])        \n\t"
                    "lw         %[temp2],   4(%[src_i])         \n\t"
                    "lw         %[temp3],   4(%[src_i1])        \n\t"
                    "shll_s.w   %[temp],    %[temp],    16      \n\t"
                    "shll_s.w   %[temp1],   %[temp1],   16      \n\t"
                    "shll_s.w   %[temp2],   %[temp2],   16      \n\t"
                    "shll_s.w   %[temp3],   %[temp3],   16      \n\t"
                    "sra        %[temp],    16                  \n\t"
                    "sra        %[temp1],   16                  \n\t"
                    "sra        %[temp2],   16                  \n\t"
                    "sra        %[temp3],   16                  \n\t"
                    "sh         %[temp],    8(%[dst])           \n\t"
                    "sh         %[temp1],   10(%[dst])          \n\t"
                    "sh         %[temp2],   12(%[dst])          \n\t"
                    "sh         %[temp3],   14(%[dst])          \n\t"

                    "addiu      %[src_i],   8                   \n\t"
                    "addiu      %[src_i1],  8                   \n\t"

                    "lw         %[temp],    4(%[src_i2])        \n\t"
                    "lw         %[temp1],   4(%[src_i3])        \n\t"
                    "lw         %[temp2],   4(%[src_i4])        \n\t"
                    "lw         %[temp3],   4(%[src_i5])        \n\t"
                    "shll_s.w   %[temp],    %[temp],    16      \n\t"
                    "shll_s.w   %[temp1],   %[temp1],   16      \n\t"
                    "shll_s.w   %[temp2],   %[temp2],   16      \n\t"
                    "shll_s.w   %[temp3],   %[temp3],   16      \n\t"
                    "sra        %[temp],    16                  \n\t"
                    "sra        %[temp1],   16                  \n\t"
                    "sra        %[temp2],   16                  \n\t"
                    "sra        %[temp3],   16                  \n\t"
                    "sh         %[temp],    16(%[dst])          \n\t"
                    "sh         %[temp1],   18(%[dst])          \n\t"
                    "sh         %[temp2],   20(%[dst])          \n\t"
                    "sh         %[temp3],   22(%[dst])          \n\t"

                    "addiu      %[src_i2],  8                   \n\t"
                    "addiu      %[src_i3],  8                   \n\t"
                    "addiu      %[src_i4],  8                   \n\t"
                    "addiu      %[src_i5],  8                   \n\t"
                    "addiu      %[dst],     24                  \n\t"
                    :[temp]"=&r"(temp), [temp1]"=&r"(temp1),
                     [temp2]"=&r"(temp2), [temp3]"=&r"(temp3),
                     [dst]"+r"(dst), [src_i]"+r"(src_i), [src_i1]"+r"(src_i1),
                     [src_i2]"+r"(src_i2), [src_i3]"+r"(src_i3),
                     [src_i4]"+r"(src_i4), [src_i5]"+r"(src_i5)
                    :
                    :"memory"
                );
            }
        }
        else {
            for(c=0; c<channels; c++)
                for(i=0, j=c; i<len; i++, j+=channels)
                    dst[j] = fixed_to_int16_one_mips(src[c]+i);
        }
    }
}
#endif

void ff_fmt_convert_init_mips_fixed(FmtConvertContext *c, AVCodecContext *avctx) {
#if HAVE_INLINE_ASM
    c->int32_to_fixed_fmul_scalar = int32_to_fixed_fmul_scalar_mips;
    c->fixed_to_int16_interleave  = fixed_to_int16_interleave_mips;
#endif
}
