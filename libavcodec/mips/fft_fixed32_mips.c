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
 * Author:  Nedeljko Babic (nedeljko.babic@imgtec.com)
 *
 * Optimized fixed point MDCT/IMDCT and FFT transforms
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
#include "config.h"
#define CONFIG_FFT_FLOAT 0
#define CONFIG_FFT_FIXED_32 1

#include "libavcodec/fft.h"
#include "libavcodec/fft_table.h"
#define MAX_FFT_SIZE_SRA6   (MAX_FFT_SIZE >> 4)
#define Q31(x)              (int)((x)*2147483648.0 + 0.5)
#define STRINGIFY_(a) #a
#define STRINGIFY(a) STRINGIFY_(a)
/**
 * FFT transform
 */

#if HAVE_INLINE_ASM
#if HAVE_MIPSDSPR2
static void ff_fft_calc_mips_fixed(FFTContext *s, FFTComplex *z) {

    int nbits, n, num_transforms, offset, step;
    int n4;
    FFTSample tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7, tmp8;
    FFTComplex *tmpz;
    FFTComplex *tmpz_n2, *tmpz_n4, *tmpz_n34;
    FFTComplex *tmpz_n2_i, *tmpz_n4_i, *tmpz_n34_i, *tmpz_i;
    FFTSample *w_re_ptr, *w_im_ptr;
    register int mem1, mem2, mem3, mem4, mem5, mem6, mem7, mem8;
    const int fft_size = (1 << s->nbits);
    int32_t m_sqrt1_2 = Q31(M_SQRT1_2);

    num_transforms = (0x2aab >> (16 - s->nbits)) | 1;

    for (n=0; n<num_transforms; n++) {
        offset = fft_offsets_lut[n] << 2;
        tmpz = z + offset;

        tmp1 = tmpz[0].re + tmpz[1].re;
        tmp5 = tmpz[2].re + tmpz[3].re;
        tmp2 = tmpz[0].im + tmpz[1].im;
        tmp6 = tmpz[2].im + tmpz[3].im;
        tmp3 = tmpz[0].re - tmpz[1].re;
        tmp8 = tmpz[2].im - tmpz[3].im;
        tmp4 = tmpz[0].im - tmpz[1].im;
        tmp7 = tmpz[2].re - tmpz[3].re;

        tmpz[0].re = tmp1 + tmp5;
        tmpz[2].re = tmp1 - tmp5;
        tmpz[0].im = tmp2 + tmp6;
        tmpz[2].im = tmp2 - tmp6;
        tmpz[1].re = tmp3 + tmp8;
        tmpz[3].re = tmp3 - tmp8;
        tmpz[1].im = tmp4 - tmp7;
        tmpz[3].im = tmp4 + tmp7;
    }

    if (fft_size < 8)
        return;

    num_transforms = (num_transforms >> 1) | 1;

    for (n=0; n<num_transforms; n++) {
        offset = fft_offsets_lut[n] << 3;
        tmpz = z + offset;

        __asm__ volatile (
            ".set       push                                   \n\t"
            ".set       noreorder                              \n\t"

            "lw        %[mem1],    32(%[tmpz])                 \n\t"     // tmpz[4].re
            "lw        %[mem3],    48(%[tmpz])                 \n\t"     // tmpz[6].re
            "lw        %[mem2],    36(%[tmpz])                 \n\t"     // tmpz[4].im
            "lw        %[mem4],    52(%[tmpz])                 \n\t"     // tmpz[6].im
            "lw        %[mem5],    40(%[tmpz])                 \n\t"     // tmpz[5].re
            "lw        %[mem7],    56(%[tmpz])                 \n\t"     // tmpz[7].re
            "lw        %[mem6],    44(%[tmpz])                 \n\t"     // tmpz[5].im
            "lw        %[mem8],    60(%[tmpz])                 \n\t"     // tmpz[7].im
            "addu      %[tmp1],    %[mem1],    %[mem5]         \n\t"
            "addu      %[tmp3],    %[mem3],    %[mem7]         \n\t"
            "addu      %[tmp2],    %[mem2],    %[mem6]         \n\t"
            "addu      %[tmp4],    %[mem4],    %[mem8]         \n\t"
            "addu      %[tmp5],    %[tmp1],    %[tmp3]         \n\t"
            "subu      %[tmp7],    %[tmp1],    %[tmp3]         \n\t"
            "addu      %[tmp6],    %[tmp2],    %[tmp4]         \n\t"
            "subu      %[tmp8],    %[tmp2],    %[tmp4]         \n\t"
            "subu      %[tmp1],    %[mem1],    %[mem5]         \n\t"
            "subu      %[tmp2],    %[mem2],    %[mem6]         \n\t"
            "subu      %[tmp3],    %[mem3],    %[mem7]         \n\t"
            "subu      %[tmp4],    %[mem4],    %[mem8]         \n\t"
            "lw        %[mem1],    0(%[tmpz])                  \n\t"   // tmpz[0].re
            "lw        %[mem2],    4(%[tmpz])                  \n\t"   // tmpz[0].im
            "lw        %[mem3],    16(%[tmpz])                 \n\t"   // tmpz[2].re
            "lw        %[mem4],    20(%[tmpz])                 \n\t"   // tmpz[2].im
            "subu      %[mem5],    %[mem1],    %[tmp5]         \n\t"
            "addu      %[mem6],    %[mem1],    %[tmp5]         \n\t"
            "subu      %[mem7],    %[mem2],    %[tmp6]         \n\t"
            "addu      %[mem8],    %[mem2],    %[tmp6]         \n\t"
            "sw        %[mem5],    32(%[tmpz])                 \n\t"
            "sw        %[mem6],    0(%[tmpz])                  \n\t"
            "sw        %[mem7],    36(%[tmpz])                 \n\t"
            "sw        %[mem8],    4(%[tmpz])                  \n\t"
            "subu      %[mem5],    %[mem3],    %[tmp8]         \n\t"
            "addu      %[mem6],    %[mem3],    %[tmp8]         \n\t"
            "addu      %[mem7],    %[mem4],    %[tmp7]         \n\t"
            "subu      %[mem8],    %[mem4],    %[tmp7]         \n\t"
            "sw        %[mem5],    48(%[tmpz])                 \n\t"
            "sw        %[mem6],    16(%[tmpz])                 \n\t"
            "sw        %[mem7],    52(%[tmpz])                 \n\t"
            "sw        %[mem8],    20(%[tmpz])                 \n\t"
            "addu      %[mem1],    %[tmp1],    %[tmp2]         \n\t"
            "subu      %[mem2],    %[tmp3],    %[tmp4]         \n\t"
            "subu      %[mem3],    %[tmp2],    %[tmp1]         \n\t"
            "addu      %[mem4],    %[tmp3],    %[tmp4]         \n\t"
            "mulq_rs.w %[tmp5],    %[mem1],    %[m_sqrt1_2]    \n\t"   // tmp5 = (int32_t)(((int64_t)Q31(M_SQRT1_2)*(tmp1 + tmp2) + 0x40000000) >> 31);
            "mulq_rs.w %[tmp7],    %[mem2],    %[m_sqrt1_2]    \n\t"   // tmp7 = (int32_t)(((int64_t)Q31(M_SQRT1_2)*(tmp3 - tmp4) + 0x40000000) >> 31);
            "mulq_rs.w %[tmp6],    %[mem3],    %[m_sqrt1_2]    \n\t"   // tmp6 = (int32_t)(((int64_t)Q31(M_SQRT1_2)*(tmp2 - tmp1) + 0x40000000) >> 31);
            "mulq_rs.w %[tmp8],    %[mem4],    %[m_sqrt1_2]    \n\t"   // tmp8 = (int32_t)(((int64_t)Q31(M_SQRT1_2)*(tmp3 + tmp4) + 0x40000000) >> 31);
            "lw        %[mem5],    8(%[tmpz])                  \n\t"   // tmpz[1].re
            "lw        %[mem6],    12(%[tmpz])                 \n\t"   // tmpz[1].im
            "lw        %[mem7],    24(%[tmpz])                 \n\t"   // tmpz[3].re
            "lw        %[mem8],    28(%[tmpz])                 \n\t"   // tmpz[3].im
            "addu      %[tmp1],    %[tmp5],    %[tmp7]         \n\t"
            "subu      %[tmp3],    %[tmp5],    %[tmp7]         \n\t"
            "addu      %[tmp2],    %[tmp6],    %[tmp8]         \n\t"
            "subu      %[tmp4],    %[tmp6],    %[tmp8]         \n\t"
            "subu      %[mem1],    %[mem5],    %[tmp1]         \n\t"
            "addu      %[tmp1],    %[mem5],    %[tmp1]         \n\t"
            "subu      %[mem2],    %[mem6],    %[tmp2]         \n\t"
            "addu      %[tmp2],    %[mem6],    %[tmp2]         \n\t"
            "subu      %[mem4],    %[mem7],    %[tmp4]         \n\t"
            "addu      %[tmp4],    %[mem7],    %[tmp4]         \n\t"
            "addu      %[mem3],    %[mem8],    %[tmp3]         \n\t"
            "subu      %[tmp3],    %[mem8],    %[tmp3]         \n\t"
            "sw        %[mem1],    40(%[tmpz])                 \n\t"
            "sw        %[tmp1],    8(%[tmpz])                  \n\t"
            "sw        %[mem2],    44(%[tmpz])                 \n\t"
            "sw        %[tmp2],    12(%[tmpz])                 \n\t"
            "sw        %[mem4],    56(%[tmpz])                 \n\t"
            "sw        %[tmp4],    24(%[tmpz])                 \n\t"
            "sw        %[mem3],    60(%[tmpz])                 \n\t"
            "sw        %[tmp3],    28(%[tmpz])                 \n\t"

            ".set      pop                                     \n\t"
            : [tmp1]"=&r"(tmp1), [tmp2]"=&r"(tmp2), [tmp3]"=&r"(tmp3),
              [tmp4]"=&r"(tmp4), [tmp5]"=&r"(tmp5), [tmp6]"=&r"(tmp6),
              [tmp7]"=&r"(tmp7), [tmp8]"=&r"(tmp8), [mem1]"=&r"(mem1),
              [mem2]"=&r"(mem2), [mem3]"=&r"(mem3), [mem4]"=&r"(mem4),
              [mem5]"=&r"(mem5), [mem6]"=&r"(mem6), [mem7]"=&r"(mem7),
              [mem8]"=&r"(mem8)
            : [tmpz]"r"(tmpz), [m_sqrt1_2]"r"(m_sqrt1_2)
            : "memory", "hi", "lo"
        );
    }

    step = 1 << ((MAX_LOG2_NFFT-4) - 2);            // step = (1 << ((MAX_LOG2_NFFT-4) - 4))<<2;
    n4 = 32;

    for (nbits=4; nbits<=s->nbits; nbits++) {
        num_transforms = (num_transforms >> 1) | 1;

        for (n=0; n<num_transforms; n++) {
            offset = fft_offsets_lut[n] << nbits;
            tmpz = z + offset;

            __asm__ volatile (
                ".set       push                                            \n\t"
                ".set       noreorder                                       \n\t"

                "addu       %[tmpz_n4],     %[tmpz],        %[n4]           \n\t"
                "addu       %[tmpz_n2],     %[tmpz_n4],     %[n4]           \n\t"
                "addu       %[tmpz_n34],    %[tmpz_n2],     %[n4]           \n\t"
                "lw         %[mem1],        0(%[tmpz])                      \n\t"
                "lw         %[tmp1],        0(%[tmpz_n2])                   \n\t"
                "lw         %[tmp3],        0(%[tmpz_n34])                  \n\t"
                "lw         %[tmp2],        4(%[tmpz_n2])                   \n\t"
                "lw         %[tmp4],        4(%[tmpz_n34])                  \n\t"
                "lw         %[mem2],        4(%[tmpz])                      \n\t"
                "addu       %[tmp5],        %[tmp1],        %[tmp3]         \n\t"
                "subu       %[tmp1],        %[tmp3]                         \n\t"
                "addu       %[tmp6],        %[tmp2],        %[tmp4]         \n\t"
                "subu       %[tmp2],        %[tmp4]                         \n\t"
                "subu       %[tmp3],        %[mem1],        %[tmp5]         \n\t"
                "addu       %[mem1],        %[mem1],        %[tmp5]         \n\t"
                "subu       %[tmp4],        %[mem2],        %[tmp6]         \n\t"
                "addu       %[mem2],        %[mem2],        %[tmp6]         \n\t"
                "sw         %[tmp3],        0(%[tmpz_n2])                   \n\t"
                "sw         %[mem1],        0(%[tmpz])                      \n\t"
                "sw         %[tmp4],        4(%[tmpz_n2])                   \n\t"
                "sw         %[mem2],        4(%[tmpz])                      \n\t"
                "lw         %[mem1],        0(%[tmpz_n4])                   \n\t"
                "lw         %[mem2],        4(%[tmpz_n4])                   \n\t"
                "subu       %[w_im_ptr],    %[w_tab_sr],    %[step]         \n\t"
                "subu       %[tmp5],        %[mem1],        %[tmp2]         \n\t"
                "addu       %[mem1],        %[mem1],        %[tmp2]         \n\t"
                "addu       %[tmp6],        %[mem2],        %[tmp1]         \n\t"
                "subu       %[mem2],        %[mem2],        %[tmp1]         \n\t"
                "sw         %[tmp5],        0(%[tmpz_n34])                  \n\t"
                "sw         %[mem1],        0(%[tmpz_n4])                   \n\t"
                "sw         %[tmp6],        4(%[tmpz_n34])                  \n\t"
                "sw         %[mem2],        4(%[tmpz_n4])                   \n\t"
                "addu       %[w_re_ptr],    %[w_tab_sr],    %[step]         \n\t"       // w_re_ptr = w_tab_sr + step;
                "addiu      %[w_im_ptr],"   STRINGIFY(MAX_FFT_SIZE_SRA6)   "\n\t"       // w_im_ptr = w_tab_sr + MAX_FFT_SIZE/(4*16);
                "addu       %[tmpz_n4_i],   %[tmpz_n4],     8               \n\t"       // tmpz_n4  + i
                "addu       %[tmpz_n2_i],   %[tmpz_n2],     8               \n\t"       // tmpz_n2  + i
                "addu       %[tmpz_n34_i],  %[tmpz_n34],    8               \n\t"       // tmpz_n34  + i
                "addu       %[tmpz_i],      %[tmpz],        8               \n\t"       // tmpz  + i
            "1:                                                             \n\t"
                "lw         %[tmp1],        0(%[w_re_ptr])                  \n\t"
                "lw         %[tmp2],        0(%[w_im_ptr])                  \n\t"
                "addiu      %[tmpz_i],      %[tmpz_i],      8               \n\t"       // tmpz  + i
                "addiu      %[tmpz_n4_i],   %[tmpz_n4_i],   8               \n\t"       // tmpz_n4  + i
                "lw         %[tmp3],        0(%[tmpz_n2_i])                 \n\t"       // tmpz[ n2+i].re
                "lw         %[tmp4],        4(%[tmpz_n2_i])                 \n\t"       // tmpz[ n2+i].im
                "lw         %[tmp5],        0(%[tmpz_n34_i])                \n\t"       // tmpz[ n34+i].re
                "lw         %[tmp6],        4(%[tmpz_n34_i])                \n\t"       // tmpz[ n34+i].im
                "addu       %[w_re_ptr],    %[step]                         \n\t"
                "subu       %[w_im_ptr],    %[step]                         \n\t"
                "mult       $ac0,           %[tmp1],        %[tmp3]         \n\t"
                "madd       $ac0,           %[tmp2],        %[tmp4]         \n\t"
                "mult       $ac2,           %[tmp1],        %[tmp5]         \n\t"
                "msub       $ac2,           %[tmp2],        %[tmp6]         \n\t"
                "mult       $ac1,           %[tmp1],        %[tmp4]         \n\t"
                "msub       $ac1,           %[tmp2],        %[tmp3]         \n\t"
                "mult       $ac3,           %[tmp1],        %[tmp6]         \n\t"
                "madd       $ac3,           %[tmp2],        %[tmp5]         \n\t"
                "extr_r.w   %[tmp1],        $ac0,           31              \n\t"
                "extr_r.w   %[tmp3],        $ac2,           31              \n\t"
                "extr_r.w   %[tmp2],        $ac1,           31              \n\t"
                "extr_r.w   %[tmp4],        $ac3,           31              \n\t"
                "lw         %[mem1],        -8(%[tmpz_i])                   \n\t"   // tmpz[ i].re
                "lw         %[mem2],        -4(%[tmpz_n4_i])                \n\t"   // tmpz[ n4+i].im
                "addiu      %[tmpz_n2_i],   %[tmpz_n2_i],   8               \n\t"   // tmpz_n2  + i
                "addiu      %[tmpz_n34_i],  %[tmpz_n34_i],  8               \n\t"   // tmpz_n34  + i
                "addu       %[tmp5],        %[tmp1],        %[tmp3]         \n\t"
                "subu       %[tmp1],        %[tmp1],        %[tmp3]         \n\t"
                "addu       %[tmp6],        %[tmp2],        %[tmp4]         \n\t"
                "subu       %[tmp2],        %[tmp2],        %[tmp4]         \n\t"
                "subu       %[tmp3],        %[mem1],        %[tmp5]         \n\t"   // tmpz[ n2+i].re = tmpz[   i].re - tmp5;
                "addu       %[mem1],        %[mem1],        %[tmp5]         \n\t"   // tmpz[    i].re = tmpz[   i].re + tmp5;
                "addu       %[tmp4],        %[mem2],        %[tmp1]         \n\t"   // tmpz[n34+i].im = tmpz[n4+i].im + tmp1;
                "subu       %[mem2],        %[mem2],        %[tmp1]         \n\t"   // tmpz[ n4+i].im = tmpz[n4+i].im - tmp1;
                "sw         %[tmp3],        -8(%[tmpz_n2_i])                \n\t"
                "sw         %[mem1],        -8(%[tmpz_i])                   \n\t"
                "sw         %[mem2],        -4(%[tmpz_n4_i])                \n\t"
                "lw         %[mem1],        -4(%[tmpz_i])                   \n\t"   // tmpz[ i].im
                "lw         %[mem2],        -8(%[tmpz_n4_i])                \n\t"   // tmpz[ n4+i].re
                "sw         %[tmp4],        -4(%[tmpz_n34_i])               \n\t"
                "subu       %[tmp5],        %[mem1],        %[tmp6]         \n\t"   // tmpz[ n2+i].im = tmpz[   i].im - tmp6;
                "addu       %[mem1],        %[mem1],        %[tmp6]         \n\t"   // tmpz[    i].im = tmpz[   i].im + tmp6;
                "subu       %[tmp3],        %[mem2],        %[tmp2]         \n\t"   // tmpz[n34+i].re = tmpz[n4+i].re - tmp2;
                "addu       %[mem2],        %[mem2],        %[tmp2]         \n\t"   // tmpz[ n4+i].re = tmpz[n4+i].re + tmp2;
                "sw         %[tmp5],        -4(%[tmpz_n2_i])                \n\t"
                "sw         %[mem1],        -4(%[tmpz_i])                   \n\t"
                "sw         %[mem2],        -8(%[tmpz_n4_i])                \n\t"
                "bne        %[tmpz_n2],     %[tmpz_n4_i],   1b              \n\t"
                " sw        %[tmp3],        -8(%[tmpz_n34_i])               \n\t"

                ".set       pop                                             \n\t"
                : [tmp1]"=&r"(tmp1), [tmp2]"=&r"(tmp2), [tmp3]"=&r"(tmp3),
                  [tmp4]"=&r"(tmp4), [tmp5]"=&r"(tmp5), [tmp6]"=&r"(tmp6),
                  [mem1]"=&r"(mem1), [mem2]"=&r"(mem2),
                  [tmpz_i]"=&r"(tmpz_i), [tmpz_n2_i]"=&r"(tmpz_n2_i),
                  [tmpz_n4_i]"=&r"(tmpz_n4_i), [tmpz_n34_i]"=&r"(tmpz_n34_i),
                  [tmpz_n2]"=&r"(tmpz_n2), [tmpz_n4]"=&r"(tmpz_n4),
                  [tmpz_n34]"=&r"(tmpz_n34),
                  [w_re_ptr]"=&r"(w_re_ptr), [w_im_ptr]"=&r"(w_im_ptr)
                : [tmpz]"r"(tmpz), [step]"r"(step), [w_tab_sr]"r"(w_tab_sr),
                  [n4]"r"(n4)
                : "memory", "hi", "lo", "$ac1hi", "$ac1lo",
                  "$ac2hi", "$ac2lo", "$ac3hi", "$ac3lo"
            );
        }
        step >>= 1;
        n4   <<= 1;
    }
}
#elif HAVE_MIPSDSPR1
static void ff_fft_calc_mips_fixed(FFTContext *s, FFTComplex *z) {

    int nbits, n, num_transforms, offset, step;
    int n4;
    FFTSample tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7, tmp8;
    FFTComplex *tmpz;
    FFTComplex *tmpz_n2, *tmpz_n4, *tmpz_n34;
    FFTComplex *tmpz_n2_i, *tmpz_n4_i, *tmpz_n34_i, *tmpz_i;
    FFTSample *w_re_ptr, *w_im_ptr;
    register int mem1, mem2, mem3, mem4, mem5, mem6, mem7, mem8;
    const int fft_size = (1 << s->nbits);
    int32_t m_sqrt1_2 = Q31(M_SQRT1_2);

    num_transforms = (0x2aab >> (16 - s->nbits)) | 1;

    for (n=0; n<num_transforms; n++) {
        offset = fft_offsets_lut[n] << 2;
        tmpz = z + offset;

        tmp1 = tmpz[0].re + tmpz[1].re;
        tmp5 = tmpz[2].re + tmpz[3].re;
        tmp2 = tmpz[0].im + tmpz[1].im;
        tmp6 = tmpz[2].im + tmpz[3].im;
        tmp3 = tmpz[0].re - tmpz[1].re;
        tmp8 = tmpz[2].im - tmpz[3].im;
        tmp4 = tmpz[0].im - tmpz[1].im;
        tmp7 = tmpz[2].re - tmpz[3].re;

        tmpz[0].re = tmp1 + tmp5;
        tmpz[2].re = tmp1 - tmp5;
        tmpz[0].im = tmp2 + tmp6;
        tmpz[2].im = tmp2 - tmp6;
        tmpz[1].re = tmp3 + tmp8;
        tmpz[3].re = tmp3 - tmp8;
        tmpz[1].im = tmp4 - tmp7;
        tmpz[3].im = tmp4 + tmp7;
    }

    if (fft_size < 8)
        return;

    num_transforms = (num_transforms >> 1) | 1;

    for (n=0; n<num_transforms; n++) {
        offset = fft_offsets_lut[n] << 3;
        tmpz = z + offset;

        __asm__ volatile (
            ".set      push                                    \n\t"
            ".set      noreorder                               \n\t"

            "lw        %[mem1],    32(%[tmpz])                 \n\t"     // tmpz[4].re
            "lw        %[mem3],    48(%[tmpz])                 \n\t"     // tmpz[6].re
            "lw        %[mem2],    36(%[tmpz])                 \n\t"     // tmpz[4].im
            "lw        %[mem4],    52(%[tmpz])                 \n\t"     // tmpz[6].im
            "lw        %[mem5],    40(%[tmpz])                 \n\t"     // tmpz[5].re
            "lw        %[mem7],    56(%[tmpz])                 \n\t"     // tmpz[7].re
            "lw        %[mem6],    44(%[tmpz])                 \n\t"     // tmpz[5].im
            "lw        %[mem8],    60(%[tmpz])                 \n\t"     // tmpz[7].im
            "addu      %[tmp1],    %[mem1],    %[mem5]         \n\t"
            "addu      %[tmp3],    %[mem3],    %[mem7]         \n\t"
            "addu      %[tmp2],    %[mem2],    %[mem6]         \n\t"
            "addu      %[tmp4],    %[mem4],    %[mem8]         \n\t"
            "addu      %[tmp5],    %[tmp1],    %[tmp3]         \n\t"
            "subu      %[tmp7],    %[tmp1],    %[tmp3]         \n\t"
            "addu      %[tmp6],    %[tmp2],    %[tmp4]         \n\t"
            "subu      %[tmp8],    %[tmp2],    %[tmp4]         \n\t"
            "subu      %[tmp1],    %[mem1],    %[mem5]         \n\t"
            "subu      %[tmp2],    %[mem2],    %[mem6]         \n\t"
            "subu      %[tmp3],    %[mem3],    %[mem7]         \n\t"
            "subu      %[tmp4],    %[mem4],    %[mem8]         \n\t"
            "lw        %[mem1],    0(%[tmpz])                  \n\t"   // tmpz[0].re
            "lw        %[mem2],    4(%[tmpz])                  \n\t"   // tmpz[0].im
            "lw        %[mem3],    16(%[tmpz])                 \n\t"   // tmpz[2].re
            "lw        %[mem4],    20(%[tmpz])                 \n\t"   // tmpz[2].im
            "subu      %[mem5],    %[mem1],    %[tmp5]         \n\t"
            "addu      %[mem6],    %[mem1],    %[tmp5]         \n\t"
            "subu      %[mem7],    %[mem2],    %[tmp6]         \n\t"
            "addu      %[mem8],    %[mem2],    %[tmp6]         \n\t"
            "sw        %[mem5],    32(%[tmpz])                 \n\t"
            "sw        %[mem6],    0(%[tmpz])                  \n\t"
            "sw        %[mem7],    36(%[tmpz])                 \n\t"
            "sw        %[mem8],    4(%[tmpz])                  \n\t"
            "subu      %[mem5],    %[mem3],    %[tmp8]         \n\t"
            "addu      %[mem6],    %[mem3],    %[tmp8]         \n\t"
            "addu      %[mem7],    %[mem4],    %[tmp7]         \n\t"
            "subu      %[mem8],    %[mem4],    %[tmp7]         \n\t"
            "sw        %[mem5],    48(%[tmpz])                 \n\t"
            "sw        %[mem6],    16(%[tmpz])                 \n\t"
            "sw        %[mem7],    52(%[tmpz])                 \n\t"
            "addu      %[mem1],    %[tmp1],    %[tmp2]         \n\t"
            "subu      %[mem2],    %[tmp3],    %[tmp4]         \n\t"
            "subu      %[mem3],    %[tmp2],    %[tmp1]         \n\t"
            "addu      %[mem4],    %[tmp3],    %[tmp4]         \n\t"
            "mult      $ac0,       %[mem1],    %[m_sqrt1_2]    \n\t"
            "mult      $ac1,       %[mem2],    %[m_sqrt1_2]    \n\t"
            "mult      $ac2,       %[mem3],    %[m_sqrt1_2]    \n\t"
            "mult      $ac3,       %[mem4],    %[m_sqrt1_2]    \n\t"
            "sw        %[mem8],    20(%[tmpz])                 \n\t"
            "extr_r.w  %[tmp5],    $ac0,       31              \n\t"   // tmp5 = (int32_t)(((int64_t)Q31(M_SQRT1_2)*(tmp1 + tmp2) + 0x40000000) >> 31);
            "extr_r.w  %[tmp7],    $ac1,       31              \n\t"   // tmp7 = (int32_t)(((int64_t)Q31(M_SQRT1_2)*(tmp3 - tmp4) + 0x40000000) >> 31);
            "extr_r.w  %[tmp6],    $ac2,       31              \n\t"   // tmp6 = (int32_t)(((int64_t)Q31(M_SQRT1_2)*(tmp2 - tmp1) + 0x40000000) >> 31);
            "extr_r.w  %[tmp8],    $ac3,       31              \n\t"   // tmp8 = (int32_t)(((int64_t)Q31(M_SQRT1_2)*(tmp3 + tmp4) + 0x40000000) >> 31);
            "lw        %[mem5],    8(%[tmpz])                  \n\t"   // tmpz[1].re
            "lw        %[mem6],    12(%[tmpz])                 \n\t"   // tmpz[1].im
            "lw        %[mem7],    24(%[tmpz])                 \n\t"   // tmpz[3].re
            "lw        %[mem8],    28(%[tmpz])                 \n\t"   // tmpz[3].im
            "addu      %[tmp1],    %[tmp5],    %[tmp7]         \n\t"
            "subu      %[tmp3],    %[tmp5],    %[tmp7]         \n\t"
            "addu      %[tmp2],    %[tmp6],    %[tmp8]         \n\t"
            "subu      %[tmp4],    %[tmp6],    %[tmp8]         \n\t"
            "subu      %[mem1],    %[mem5],    %[tmp1]         \n\t"
            "addu      %[tmp1],    %[mem5],    %[tmp1]         \n\t"
            "subu      %[mem2],    %[mem6],    %[tmp2]         \n\t"
            "addu      %[tmp2],    %[mem6],    %[tmp2]         \n\t"
            "subu      %[mem4],    %[mem7],    %[tmp4]         \n\t"
            "addu      %[tmp4],    %[mem7],    %[tmp4]         \n\t"
            "addu      %[mem3],    %[mem8],    %[tmp3]         \n\t"
            "subu      %[tmp3],    %[mem8],    %[tmp3]         \n\t"

            "sw        %[mem1],    40(%[tmpz])                 \n\t"
            "sw        %[tmp1],    8(%[tmpz])                  \n\t"
            "sw        %[mem2],    44(%[tmpz])                 \n\t"
            "sw        %[tmp2],    12(%[tmpz])                 \n\t"
            "sw        %[mem4],    56(%[tmpz])                 \n\t"
            "sw        %[tmp4],    24(%[tmpz])                 \n\t"
            "sw        %[mem3],    60(%[tmpz])                 \n\t"
            "sw        %[tmp3],    28(%[tmpz])                 \n\t"

            ".set      pop                                     \n\t"
            : [tmp1]"=&r"(tmp1), [tmp2]"=&r"(tmp2), [tmp3]"=&r"(tmp3),
              [tmp4]"=&r"(tmp4), [tmp5]"=&r"(tmp5), [tmp6]"=&r"(tmp6),
              [tmp7]"=&r"(tmp7), [tmp8]"=&r"(tmp8), [mem1]"=&r"(mem1),
              [mem2]"=&r"(mem2), [mem3]"=&r"(mem3), [mem4]"=&r"(mem4),
              [mem5]"=&r"(mem5), [mem6]"=&r"(mem6), [mem7]"=&r"(mem7),
              [mem8]"=&r"(mem8)
            : [tmpz]"r"(tmpz), [m_sqrt1_2]"r"(m_sqrt1_2)
            : "memory", "hi", "lo", "$ac1hi", "$ac1lo",
              "$ac2hi", "$ac2lo", "$ac3hi", "$ac3lo"
        );
    }

    step = 1 << ((MAX_LOG2_NFFT-4) - 2);            // step = (1 << ((MAX_LOG2_NFFT-4) - 4))<<2;
    n4 = 32;

    for (nbits=4; nbits<=s->nbits; nbits++) {
        num_transforms = (num_transforms >> 1) | 1;

        for (n=0; n<num_transforms; n++) {
            offset = fft_offsets_lut[n] << nbits;
            tmpz = z + offset;

            __asm__ volatile (
                ".set       push                                            \n\t"
                ".set       noreorder                                       \n\t"

                "addu       %[tmpz_n4],     %[tmpz],        %[n4]           \n\t"
                "addu       %[tmpz_n2],     %[tmpz_n4],     %[n4]           \n\t"
                "addu       %[tmpz_n34],    %[tmpz_n2],     %[n4]           \n\t"
                "lw         %[mem1],        0(%[tmpz])                      \n\t"
                "lw         %[tmp1],        0(%[tmpz_n2])                   \n\t"
                "lw         %[tmp3],        0(%[tmpz_n34])                  \n\t"
                "lw         %[tmp2],        4(%[tmpz_n2])                   \n\t"
                "lw         %[tmp4],        4(%[tmpz_n34])                  \n\t"
                "lw         %[mem2],        4(%[tmpz])                      \n\t"
                "addu       %[tmp5],        %[tmp1],        %[tmp3]         \n\t"
                "subu       %[tmp1],        %[tmp3]                         \n\t"
                "addu       %[tmp6],        %[tmp2],        %[tmp4]         \n\t"
                "subu       %[tmp2],        %[tmp4]                         \n\t"
                "subu       %[tmp3],        %[mem1],        %[tmp5]         \n\t"
                "addu       %[mem1],        %[mem1],        %[tmp5]         \n\t"
                "subu       %[tmp4],        %[mem2],        %[tmp6]         \n\t"
                "addu       %[mem2],        %[mem2],        %[tmp6]         \n\t"
                "sw         %[tmp3],        0(%[tmpz_n2])                   \n\t"
                "sw         %[mem1],        0(%[tmpz])                      \n\t"
                "sw         %[tmp4],        4(%[tmpz_n2])                   \n\t"
                "lw         %[mem1],        0(%[tmpz_n4])                   \n\t"
                "sw         %[mem2],        4(%[tmpz])                      \n\t"
                "lw         %[mem2],        4(%[tmpz_n4])                   \n\t"
                "subu       %[w_im_ptr],    %[w_tab_sr],    %[step]         \n\t"
                "subu       %[tmp5],        %[mem1],        %[tmp2]         \n\t"
                "addu       %[mem1],        %[mem1],        %[tmp2]         \n\t"
                "addu       %[tmp6],        %[mem2],        %[tmp1]         \n\t"
                "subu       %[mem2],        %[mem2],        %[tmp1]         \n\t"
                "sw         %[tmp5],        0(%[tmpz_n34])                  \n\t"
                "sw         %[mem1],        0(%[tmpz_n4])                   \n\t"
                "sw         %[tmp6],        4(%[tmpz_n34])                  \n\t"
                "sw         %[mem2],        4(%[tmpz_n4])                   \n\t"
                "addu       %[w_re_ptr],    %[w_tab_sr],    %[step]         \n\t"       // w_re_ptr = w_tab_sr + step;
                "addiu       %[w_im_ptr],"  STRINGIFY(MAX_FFT_SIZE_SRA6)   "\n\t"       // w_im_ptr = w_tab_sr + MAX_FFT_SIZE/(4*16);
                "addu       %[tmpz_n4_i],   %[tmpz_n4],     8               \n\t"       // tmpz_n4  + i
                "addu       %[tmpz_n2_i],   %[tmpz_n2],     8               \n\t"       // tmpz_n2  + i
                "addu       %[tmpz_n34_i],  %[tmpz_n34],    8               \n\t"       // tmpz_n34  + i
                "addu       %[tmpz_i],      %[tmpz],        8               \n\t"       // tmpz  + i
            "1:                                                             \n\t"
                "lw         %[tmp1],        0(%[w_re_ptr])                  \n\t"
                "lw         %[tmp2],        0(%[w_im_ptr])                  \n\t"
                "addiu      %[tmpz_i],      %[tmpz_i],      8               \n\t"       // tmpz  + i
                "addiu      %[tmpz_n4_i],   %[tmpz_n4_i],   8               \n\t"       // tmpz_n4  + i
                "lw         %[tmp3],        0(%[tmpz_n2_i])                 \n\t"       // tmpz[ n2+i].re
                "lw         %[tmp4],        4(%[tmpz_n2_i])                 \n\t"       // tmpz[ n2+i].im
                "lw         %[tmp5],        0(%[tmpz_n34_i])                \n\t"       // tmpz[ n34+i].re
                "lw         %[tmp6],        4(%[tmpz_n34_i])                \n\t"       // tmpz[ n34+i].im
                "addu       %[w_re_ptr],    %[step]                         \n\t"
                "mult       $ac0,           %[tmp1],        %[tmp3]         \n\t"
                "madd       $ac0,           %[tmp2],        %[tmp4]         \n\t"
                "mult       $ac2,           %[tmp1],        %[tmp5]         \n\t"
                "msub       $ac2,           %[tmp2],        %[tmp6]         \n\t"
                "mult       $ac1,           %[tmp1],        %[tmp4]         \n\t"
                "msub       $ac1,           %[tmp2],        %[tmp3]         \n\t"
                "mult       $ac3,           %[tmp1],        %[tmp6]         \n\t"
                "madd       $ac3,           %[tmp2],        %[tmp5]         \n\t"
                "extr_r.w   %[tmp1],        $ac0,           31              \n\t"
                "extr_r.w   %[tmp3],        $ac2,           31              \n\t"
                "extr_r.w   %[tmp2],        $ac1,           31              \n\t"
                "subu       %[w_im_ptr],    %[step]                         \n\t"
                "extr_r.w   %[tmp4],        $ac3,           31              \n\t"
                "lw         %[mem1],        -8(%[tmpz_i])                   \n\t"   // tmpz[ i].re
                "lw         %[mem2],        -4(%[tmpz_n4_i])                \n\t"   // tmpz[ n4+i].im
                "addiu      %[tmpz_n2_i],   %[tmpz_n2_i],   8               \n\t"   // tmpz_n2  + i
                "addiu      %[tmpz_n34_i],  %[tmpz_n34_i],  8               \n\t"   // tmpz_n34  + i
                "addu       %[tmp5],        %[tmp1],        %[tmp3]         \n\t"
                "subu       %[tmp1],        %[tmp1],        %[tmp3]         \n\t"
                "addu       %[tmp6],        %[tmp2],        %[tmp4]         \n\t"
                "subu       %[tmp2],        %[tmp2],        %[tmp4]         \n\t"
                "subu       %[tmp3],        %[mem1],        %[tmp5]         \n\t"   // tmpz[ n2+i].re = tmpz[   i].re - tmp5;
                "addu       %[mem1],        %[mem1],        %[tmp5]         \n\t"   // tmpz[    i].re = tmpz[   i].re + tmp5;
                "addu       %[tmp4],        %[mem2],        %[tmp1]         \n\t"   // tmpz[n34+i].im = tmpz[n4+i].im + tmp1;
                "subu       %[mem2],        %[mem2],        %[tmp1]         \n\t"   // tmpz[ n4+i].im = tmpz[n4+i].im - tmp1;
                "sw         %[tmp3],        -8(%[tmpz_n2_i])                \n\t"
                "sw         %[mem1],        -8(%[tmpz_i])                   \n\t"
                "sw         %[mem2],        -4(%[tmpz_n4_i])                \n\t"
                "lw         %[mem1],        -4(%[tmpz_i])                   \n\t"   // tmpz[ i].im
                "lw         %[mem2],        -8(%[tmpz_n4_i])                \n\t"   // tmpz[ n4+i].re
                "sw         %[tmp4],        -4(%[tmpz_n34_i])               \n\t"
                "subu       %[tmp5],        %[mem1],        %[tmp6]         \n\t"   // tmpz[ n2+i].im = tmpz[   i].im - tmp6;
                "addu       %[mem1],        %[mem1],        %[tmp6]         \n\t"   // tmpz[    i].im = tmpz[   i].im + tmp6;
                "subu       %[tmp3],        %[mem2],        %[tmp2]         \n\t"   // tmpz[n34+i].re = tmpz[n4+i].re - tmp2;
                "addu       %[mem2],        %[mem2],        %[tmp2]         \n\t"   // tmpz[ n4+i].re = tmpz[n4+i].re + tmp2;
                "sw         %[tmp5],        -4(%[tmpz_n2_i])                \n\t"
                "sw         %[mem1],        -4(%[tmpz_i])                   \n\t"
                "sw         %[mem2],        -8(%[tmpz_n4_i])                \n\t"
                "bne        %[tmpz_n2],     %[tmpz_n4_i],   1b              \n\t"
                " sw        %[tmp3],        -8(%[tmpz_n34_i])               \n\t"

                ".set       pop                                             \n\t"
                : [tmp1]"=&r"(tmp1), [tmp2]"=&r"(tmp2), [tmp3]"=&r"(tmp3),
                  [tmp4]"=&r"(tmp4), [tmp5]"=&r"(tmp5), [tmp6]"=&r"(tmp6),
                  [mem1]"=&r"(mem1), [mem2]"=&r"(mem2),
                  [tmpz_i]"=&r"(tmpz_i), [tmpz_n2_i]"=&r"(tmpz_n2_i),
                  [tmpz_n4_i]"=&r"(tmpz_n4_i), [tmpz_n34_i]"=&r"(tmpz_n34_i),
                  [tmpz_n2]"=&r"(tmpz_n2), [tmpz_n4]"=&r"(tmpz_n4),
                  [tmpz_n34]"=&r"(tmpz_n34),
                  [w_re_ptr]"=&r"(w_re_ptr), [w_im_ptr]"=&r"(w_im_ptr)
                : [tmpz]"r"(tmpz), [step]"r"(step), [w_tab_sr]"r"(w_tab_sr),
                  [n4]"r"(n4)
                : "memory", "hi", "lo", "$ac1hi", "$ac1lo",
                  "$ac2hi", "$ac2lo", "$ac3hi", "$ac3lo"
            );
        }
        step >>= 1;
        n4   <<= 1;
    }
}
#elif HAVE_MIPS32R2
static void ff_fft_calc_mips_fixed(FFTContext *s, FFTComplex *z)
{

    int nbits, n, num_transforms, offset, step;
    int n4;
    FFTSample tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7, tmp8;
    FFTComplex *tmpz;
    FFTComplex *tmpz_n2, *tmpz_n4, *tmpz_n34;
    FFTComplex *tmpz_n2_i, *tmpz_n4_i, *tmpz_n34_i, *tmpz_i;
    FFTSample *w_re_ptr, *w_im_ptr;
    register int mem1, mem2, mem3, mem4, mem5, mem6, mem7, mem8;
    const int fft_size = (1 << s->nbits);
    uint32_t m_sqrt1_2 = Q31(M_SQRT1_2);
    uint32_t round = 0x40000000, one = 1;

    num_transforms = (0x2aab >> (16 - s->nbits)) | 1;

    for (n=0; n<num_transforms; n++) {
        offset = fft_offsets_lut[n] << 2;
        tmpz = z + offset;

        tmp1 = tmpz[0].re + tmpz[1].re;
        tmp5 = tmpz[2].re + tmpz[3].re;
        tmp2 = tmpz[0].im + tmpz[1].im;
        tmp6 = tmpz[2].im + tmpz[3].im;
        tmp3 = tmpz[0].re - tmpz[1].re;
        tmp8 = tmpz[2].im - tmpz[3].im;
        tmp4 = tmpz[0].im - tmpz[1].im;
        tmp7 = tmpz[2].re - tmpz[3].re;

        tmpz[0].re = tmp1 + tmp5;
        tmpz[2].re = tmp1 - tmp5;
        tmpz[0].im = tmp2 + tmp6;
        tmpz[2].im = tmp2 - tmp6;
        tmpz[1].re = tmp3 + tmp8;
        tmpz[3].re = tmp3 - tmp8;
        tmpz[1].im = tmp4 - tmp7;
        tmpz[3].im = tmp4 + tmp7;
    }

    if (fft_size < 8)
        return;

    num_transforms = (num_transforms >> 1) | 1;

    for (n=0; n<num_transforms; n++) {
        offset = fft_offsets_lut[n] << 3;
        tmpz = z + offset;

        __asm__ volatile (
            ".set       push                                \n\t"
            ".set       noreorder                           \n\t"

            "lw     %[mem1],    32(%[tmpz])                 \n\t"     // tmpz[4].re
            "lw     %[mem3],    48(%[tmpz])                 \n\t"     // tmpz[6].re
            "lw     %[mem2],    36(%[tmpz])                 \n\t"     // tmpz[4].im
            "lw     %[mem4],    52(%[tmpz])                 \n\t"     // tmpz[6].im
            "lw     %[mem5],    40(%[tmpz])                 \n\t"     // tmpz[5].re
            "lw     %[mem7],    56(%[tmpz])                 \n\t"     // tmpz[7].re
            "lw     %[mem6],    44(%[tmpz])                 \n\t"     // tmpz[5].im
            "lw     %[mem8],    60(%[tmpz])                 \n\t"     // tmpz[7].im
            "addu   %[tmp1],    %[mem1],   %[mem5]          \n\t"
            "addu   %[tmp3],    %[mem3],   %[mem7]          \n\t"
            "addu   %[tmp2],    %[mem2],   %[mem6]          \n\t"
            "addu   %[tmp4],    %[mem4],   %[mem8]          \n\t"
            "addu   %[tmp5],    %[tmp1],   %[tmp3]          \n\t"
            "subu   %[tmp7],    %[tmp1],   %[tmp3]          \n\t"
            "addu   %[tmp6],    %[tmp2],   %[tmp4]          \n\t"
            "subu   %[tmp8],    %[tmp2],   %[tmp4]          \n\t"
            "subu   %[tmp1],    %[mem1],   %[mem5]          \n\t"
            "subu   %[tmp2],    %[mem2],   %[mem6]          \n\t"
            "subu   %[tmp3],    %[mem3],   %[mem7]          \n\t"
            "subu   %[tmp4],    %[mem4],   %[mem8]          \n\t"
            "lw     %[mem1],    0(%[tmpz])                  \n\t"   // tmpz[0].re
            "lw     %[mem2],    4(%[tmpz])                  \n\t"   // tmpz[0].im
            "lw     %[mem3],    16(%[tmpz])                 \n\t"   // tmpz[2].re
            "lw     %[mem4],    20(%[tmpz])                 \n\t"   // tmpz[2].im
            "subu   %[mem5],    %[mem1],    %[tmp5]         \n\t"
            "addu   %[mem6],    %[mem1],    %[tmp5]         \n\t"
            "subu   %[mem7],    %[mem2],    %[tmp6]         \n\t"
            "addu   %[mem8],    %[mem2],    %[tmp6]         \n\t"
            "sw     %[mem5],    32(%[tmpz])                 \n\t"
            "sw     %[mem6],    0(%[tmpz])                  \n\t"
            "subu   %[mem5],    %[mem3],    %[tmp8]         \n\t"
            "addu   %[mem6],    %[mem3],    %[tmp8]         \n\t"
            "addu   %[mem1],    %[tmp1],    %[tmp2]         \n\t"
            "subu   %[mem3],    %[tmp2],    %[tmp1]         \n\t"
            "multu  %[round],   %[one]                      \n\t"
            "madd   %[mem1],    %[m_sqrt1_2]                \n\t"
            "sw     %[mem7],    36(%[tmpz])                 \n\t"
            "sw     %[mem8],    4(%[tmpz])                  \n\t"
            "subu   %[mem2],    %[tmp3],    %[tmp4]         \n\t"
            "mflo   %[tmp1]                                 \n\t"
            "mfhi   %[tmp5]                                 \n\t"   // tmp5 = (int32_t)(((int64_t)Q31(M_SQRT1_2)*(tmp1 + tmp2) + 0x40000000) >> 31);
            "multu  %[round],   %[one]                      \n\t"
            "madd   %[mem2],    %[m_sqrt1_2]                \n\t"
            "addu   %[mem7],    %[mem4],    %[tmp7]         \n\t"
            "subu   %[mem8],    %[mem4],    %[tmp7]         \n\t"
            "addu   %[mem4],    %[tmp3],    %[tmp4]         \n\t"
            "mflo   %[tmp2]                                 \n\t"
            "mfhi   %[tmp7]                                 \n\t"   // tmp7 = (int32_t)(((int64_t)Q31(M_SQRT1_2)*(tmp3 - tmp4) + 0x40000000) >> 31);
            "multu  %[round],   %[one]                      \n\t"
            "madd   %[mem3],    %[m_sqrt1_2]                \n\t"
            "sw     %[mem5],    48(%[tmpz])                 \n\t"
            "sw     %[mem6],    16(%[tmpz])                 \n\t"
            "sw     %[mem7],    52(%[tmpz])                 \n\t"
            "sw     %[mem8],    20(%[tmpz])                 \n\t"
            "mflo   %[tmp3]                                 \n\t"
            "mfhi   %[tmp6]                                 \n\t"   // tmp6 = (int32_t)(((int64_t)Q31(M_SQRT1_2)*(tmp2 - tmp1) + 0x40000000) >> 31);
            "multu  %[round],   %[one]                      \n\t"
            "madd   %[mem4],    %[m_sqrt1_2]                \n\t"
            "lw     %[mem5],    8(%[tmpz])                  \n\t"   // tmpz[1].re
            "lw     %[mem6],    12(%[tmpz])                 \n\t"   // tmpz[1].im
            "lw     %[mem7],    24(%[tmpz])                 \n\t"   // tmpz[3].re
            "lw     %[mem8],    28(%[tmpz])                 \n\t"   // tmpz[3].im
            "mflo   %[tmp4]                                 \n\t"
            "mfhi   %[tmp8]                                 \n\t"   // tmp8 = (int32_t)(((int64_t)Q31(M_SQRT1_2)*(tmp3 + tmp4) + 0x40000000) >> 31);
            "srl    %[tmp1],    31                          \n\t"
            "sll    %[tmp5],    1                           \n\t"
            "or     %[tmp5],    %[tmp1]                     \n\t"
            "srl    %[tmp2],    31                          \n\t"
            "sll    %[tmp7],    1                           \n\t"
            "or     %[tmp7],    %[tmp2]                     \n\t"
            "srl    %[tmp3],    31                          \n\t"
            "sll    %[tmp6],    1                           \n\t"
            "or     %[tmp6],    %[tmp3]                     \n\t"
            "srl    %[tmp4],    31                          \n\t"
            "sll    %[tmp8],    1                           \n\t"
            "or     %[tmp8],    %[tmp4]                     \n\t"
            "addu   %[tmp1],    %[tmp5],    %[tmp7]         \n\t"
            "subu   %[tmp3],    %[tmp5],    %[tmp7]         \n\t"
            "addu   %[tmp2],    %[tmp6],    %[tmp8]         \n\t"
            "subu   %[tmp4],    %[tmp6],    %[tmp8]         \n\t"
            "subu   %[mem1],    %[mem5],    %[tmp1]         \n\t"
            "addu   %[tmp1],    %[mem5],    %[tmp1]         \n\t"
            "subu   %[mem2],    %[mem6],    %[tmp2]         \n\t"
            "addu   %[tmp2],    %[mem6],    %[tmp2]         \n\t"
            "subu   %[mem4],    %[mem7],    %[tmp4]         \n\t"
            "addu   %[tmp4],    %[mem7],    %[tmp4]         \n\t"
            "addu   %[mem3],    %[mem8],    %[tmp3]         \n\t"
            "subu   %[tmp3],    %[mem8],    %[tmp3]         \n\t"

            "sw     %[mem1],    40(%[tmpz])                 \n\t"
            "sw     %[tmp1],    8(%[tmpz])                  \n\t"
            "sw     %[mem2],    44(%[tmpz])                 \n\t"
            "sw     %[tmp2],    12(%[tmpz])                 \n\t"
            "sw     %[mem4],    56(%[tmpz])                 \n\t"
            "sw     %[tmp4],    24(%[tmpz])                 \n\t"
            "sw     %[mem3],    60(%[tmpz])                 \n\t"
            "sw     %[tmp3],    28(%[tmpz])                 \n\t"

            ".set       pop                                 \n\t"
            : [tmp1]"=&r"(tmp1), [tmp2]"=&r"(tmp2), [tmp3]"=&r"(tmp3),
              [tmp4]"=&r"(tmp4), [tmp5]"=&r"(tmp5), [tmp6]"=&r"(tmp6),
              [tmp7]"=&r"(tmp7), [tmp8]"=&r"(tmp8), [mem1]"=&r"(mem1),
              [mem2]"=&r"(mem2), [mem3]"=&r"(mem3), [mem4]"=&r"(mem4),
              [mem5]"=&r"(mem5), [mem6]"=&r"(mem6), [mem7]"=&r"(mem7),
              [mem8]"=&r"(mem8)
            : [tmpz]"r"(tmpz), [m_sqrt1_2]"r"(m_sqrt1_2), [round]"r"(round),
              [one]"r"(one)
            : "memory", "hi", "lo"
        );
    }

    step = 1 << ((MAX_LOG2_NFFT-4) - 2);            // step = (1 << ((MAX_LOG2_NFFT-4) - 4))<<2;
    n4 = 32;

    for (nbits=4; nbits<=s->nbits; nbits++) {
        num_transforms = (num_transforms >> 1) | 1;

        for (n=0; n<num_transforms; n++) {
            offset = fft_offsets_lut[n] << nbits;
            tmpz = z + offset;

            __asm__ volatile (
                ".set       push                                            \n\t"
                ".set       noreorder                                       \n\t"

                "addu       %[tmpz_n4],     %[tmpz],        %[n4]           \n\t"
                "addu       %[tmpz_n2],     %[tmpz_n4],     %[n4]           \n\t"
                "addu       %[tmpz_n34],    %[tmpz_n2],     %[n4]           \n\t"
                "lw         %[mem1],        0(%[tmpz])                      \n\t"
                "lw         %[tmp1],        0(%[tmpz_n2])                   \n\t"
                "lw         %[tmp3],        0(%[tmpz_n34])                  \n\t"
                "lw         %[tmp2],        4(%[tmpz_n2])                   \n\t"
                "lw         %[tmp4],        4(%[tmpz_n34])                  \n\t"
                "lw         %[mem2],        4(%[tmpz])                      \n\t"
                "addu       %[tmp5],        %[tmp1],        %[tmp3]         \n\t"
                "subu       %[tmp1],        %[tmp3]                         \n\t"
                "addu       %[tmp6],        %[tmp2],        %[tmp4]         \n\t"
                "subu       %[tmp2],        %[tmp4]                         \n\t"
                "subu       %[tmp3],        %[mem1],        %[tmp5]         \n\t"
                "addu       %[mem1],        %[mem1],        %[tmp5]         \n\t"
                "subu       %[tmp4],        %[mem2],        %[tmp6]         \n\t"
                "addu       %[mem2],        %[mem2],        %[tmp6]         \n\t"
                "sw         %[tmp3],        0(%[tmpz_n2])                   \n\t"
                "sw         %[mem1],        0(%[tmpz])                      \n\t"
                "sw         %[tmp4],        4(%[tmpz_n2])                   \n\t"
                "sw         %[mem2],        4(%[tmpz])                      \n\t"
                "lw         %[mem1],        0(%[tmpz_n4])                   \n\t"
                "lw         %[mem2],        4(%[tmpz_n4])                   \n\t"
                "subu       %[w_im_ptr],    %[w_tab_sr],    %[step]         \n\t"
                "subu       %[tmp5],        %[mem1],        %[tmp2]         \n\t"
                "addu       %[mem1],        %[mem1],        %[tmp2]         \n\t"
                "addu       %[tmp6],        %[mem2],        %[tmp1]         \n\t"
                "subu       %[mem2],        %[mem2],        %[tmp1]         \n\t"
                "sw         %[tmp5],        0(%[tmpz_n34])                  \n\t"
                "sw         %[mem1],        0(%[tmpz_n4])                   \n\t"
                "sw         %[tmp6],        4(%[tmpz_n34])                  \n\t"
                "sw         %[mem2],        4(%[tmpz_n4])                   \n\t"
                "addu       %[w_re_ptr],    %[w_tab_sr],    %[step]         \n\t"       // w_re_ptr = w_tab_sr + step;
                "addiu      %[w_im_ptr],"   STRINGIFY(MAX_FFT_SIZE_SRA6)   "\n\t"       // w_im_ptr = w_tab_sr + MAX_FFT_SIZE/(4*16);
                "addu       %[tmpz_n4_i],   %[tmpz_n4],     8               \n\t"       // tmpz_n4  + i
                "addu       %[tmpz_n2_i],   %[tmpz_n2],     8               \n\t"       // tmpz_n2  + i
                "addu       %[tmpz_n34_i],  %[tmpz_n34],    8               \n\t"       // tmpz_n34  + i
                "addu       %[tmpz_i],      %[tmpz],        8               \n\t"       // tmpz  + i
            "1:                                                             \n\t"
                "lw         %[tmp1],        0(%[w_re_ptr])                  \n\t"
                "lw         %[tmp2],        0(%[w_im_ptr])                  \n\t"
                "lw         %[tmp3],        0(%[tmpz_n2_i])                 \n\t"       // tmpz[ n2+i].re
                "lw         %[tmp4],        4(%[tmpz_n2_i])                 \n\t"       // tmpz[ n2+i].im
                "mult       %[round],       %[one]                          \n\t"
                "madd       %[tmp1],        %[tmp3]                         \n\t"
                "madd       %[tmp2],        %[tmp4]                         \n\t"
                "addiu      %[tmpz_i],      %[tmpz_i],      8               \n\t"       // tmpz  + i
                "addiu      %[tmpz_n4_i],   %[tmpz_n4_i],   8               \n\t"       // tmpz_n4  + i
                "mflo       %[tmp7]                                         \n\t"
                "mfhi       %[tmp8]                                         \n\t"
                "mult       %[round],       %[one]                          \n\t"
                "madd       %[tmp1],        %[tmp4]                         \n\t"
                "msub       %[tmp2],        %[tmp3]                         \n\t"
                "lw         %[tmp5],        0(%[tmpz_n34_i])                \n\t"       // tmpz[ n34+i].re
                "lw         %[tmp6],        4(%[tmpz_n34_i])                \n\t"       // tmpz[ n34+i].im
                "addu       %[w_re_ptr],    %[step]                         \n\t"
                "subu       %[w_im_ptr],    %[step]                         \n\t"
                "mflo       %[tmp3]                                         \n\t"
                "mfhi       %[tmp4]                                         \n\t"
                "mult       %[round],       %[one]                          \n\t"
                "madd       %[tmp1],        %[tmp5]                         \n\t"
                "msub       %[tmp2],        %[tmp6]                         \n\t"
                "srl        %[tmp7],        31                              \n\t"
                "sll        %[tmp8],        1                               \n\t"
                "or         %[tmp8],        %[tmp7]                         \n\t"
                "srl        %[tmp3],        31                              \n\t"
                "mflo       %[mem1]                                         \n\t"
                "mfhi       %[mem2]                                         \n\t"
                "mult       %[round],       %[one]                          \n\t"
                "madd       %[tmp1],        %[tmp6]                         \n\t"
                "madd       %[tmp2],        %[tmp5]                         \n\t"
                "sll        %[tmp4],        1                               \n\t"
                "or         %[tmp4],        %[tmp3]                         \n\t"
                "srl        %[mem1],        31                              \n\t"
                "sll        %[mem2],        1                               \n\t"
                "mflo       %[tmp5]                                         \n\t"
                "mfhi       %[tmp6]                                         \n\t"
                "or         %[mem2],        %[mem1]                         \n\t"
                "srl        %[tmp5],        31                              \n\t"
                "sll        %[tmp6],        1                               \n\t"
                "or         %[tmp6],        %[tmp5]                         \n\t"
                "addiu      %[tmpz_n2_i],   %[tmpz_n2_i],   8               \n\t"   // tmpz_n2  + i
                "addiu      %[tmpz_n34_i],  %[tmpz_n34_i],  8               \n\t"   // tmpz_n34  + i
                "addu       %[tmp5],        %[tmp8],        %[mem2]         \n\t"
                "subu       %[tmp1],        %[tmp8],        %[mem2]         \n\t"
                "lw         %[mem1],        -8(%[tmpz_i])                   \n\t"   // tmpz[ i].re
                "lw         %[mem2],        -4(%[tmpz_n4_i])                \n\t"   // tmpz[ n4+i].im
                "subu       %[tmp2],        %[tmp4],        %[tmp6]         \n\t"
                "addu       %[tmp6],        %[tmp4],        %[tmp6]         \n\t"
                "subu       %[tmp3],        %[mem1],        %[tmp5]         \n\t"   // tmpz[ n2+i].re = tmpz[   i].re - tmp5;
                "addu       %[mem1],        %[mem1],        %[tmp5]         \n\t"   // tmpz[    i].re = tmpz[   i].re + tmp5;
                "addu       %[tmp4],        %[mem2],        %[tmp1]         \n\t"   // tmpz[n34+i].im = tmpz[n4+i].im + tmp1;
                "subu       %[mem2],        %[mem2],        %[tmp1]         \n\t"   // tmpz[ n4+i].im = tmpz[n4+i].im - tmp1;
                "sw         %[tmp3],        -8(%[tmpz_n2_i])                \n\t"
                "sw         %[mem1],        -8(%[tmpz_i])                   \n\t"
                "sw         %[mem2],        -4(%[tmpz_n4_i])                \n\t"
                "lw         %[mem1],        -4(%[tmpz_i])                   \n\t"   // tmpz[ i].im
                "lw         %[mem2],        -8(%[tmpz_n4_i])                \n\t"   // tmpz[ n4+i].re
                "sw         %[tmp4],        -4(%[tmpz_n34_i])               \n\t"
                "subu       %[tmp5],        %[mem1],        %[tmp6]         \n\t"   // tmpz[ n2+i].im = tmpz[   i].im - tmp6;
                "addu       %[mem1],        %[mem1],        %[tmp6]         \n\t"   // tmpz[    i].im = tmpz[   i].im + tmp6;
                "subu       %[tmp3],        %[mem2],        %[tmp2]         \n\t"   // tmpz[n34+i].re = tmpz[n4+i].re - tmp2;
                "addu       %[mem2],        %[mem2],        %[tmp2]         \n\t"   // tmpz[ n4+i].re = tmpz[n4+i].re + tmp2;
                "sw         %[tmp5],        -4(%[tmpz_n2_i])                \n\t"
                "sw         %[mem1],        -4(%[tmpz_i])                   \n\t"
                "sw         %[mem2],        -8(%[tmpz_n4_i])                \n\t"
                "bne        %[tmpz_n2],     %[tmpz_n4_i],   1b              \n\t"
                " sw        %[tmp3],        -8(%[tmpz_n34_i])               \n\t"

                ".set       pop                                             \n\t"
                : [tmp1]"=&r"(tmp1), [tmp2]"=&r"(tmp2), [tmp3]"=&r"(tmp3),
                  [tmp4]"=&r"(tmp4), [tmp5]"=&r"(tmp5), [tmp6]"=&r"(tmp6),
                  [tmp7]"=&r"(tmp7), [tmp8]"=&r"(tmp8),
                  [mem1]"=&r"(mem1), [mem2]"=&r"(mem2),
                  [tmpz_i]"=&r"(tmpz_i), [tmpz_n2_i]"=&r"(tmpz_n2_i),
                  [tmpz_n4_i]"=&r"(tmpz_n4_i), [tmpz_n34_i]"=&r"(tmpz_n34_i),
                  [tmpz_n2]"=&r"(tmpz_n2), [tmpz_n4]"=&r"(tmpz_n4),
                  [tmpz_n34]"=&r"(tmpz_n34),
                  [w_re_ptr]"=&r"(w_re_ptr), [w_im_ptr]"=&r"(w_im_ptr)
                : [tmpz]"r"(tmpz), [step]"r"(step), [w_tab_sr]"r"(w_tab_sr),
                  [n4]"r"(n4), [round]"r"(round), [one]"r"(one)
                : "memory", "hi", "lo"
            );
        }
        step >>= 1;
        n4   <<= 1;
    }
}
#endif /* HAVE_MIPSDSPR2 */

#if HAVE_MIPSDSPR1 || HAVE_MIPSDSPR2
static void ff_imdct_half_mips_fixed(FFTContext *s, FFTSample *output, const FFTSample *input)
{
    int k, n8, n4, n2, n, j, j1;
    const uint16_t *revtab = s->revtab;
    const FFTSample *tcos = s->tcos;
    const FFTSample *tsin = s->tsin;
    const FFTSample *in1, *in2;
    const FFTSample *cosm, *cosp = tcos;
    const FFTSample *sinm, *sinp = tsin;
    FFTComplex *z1, *z = (FFTComplex *)output;

    n = 1 << s->mdct_bits;
    n2 = n >> 1;
    n4 = n >> 2;
    n8 = n >> 3;

    in1 = input;
    in2 = input + n2 - 1;
    for (k = 0; k < n4; k+=4) {
        int s0, s1, wi, wj;
        int s2, s3, wk, wl;
        int t0, t1, t2, t3;

        __asm__ volatile (
            "lw         %[s0],      0(%[cosp])          \n\t"
            "lw         %[s1],      0(%[sinp])          \n\t"
            "lw         %[wj],      0(%[in2])           \n\t"
            "lw         %[wi],      0(%[in1])           \n\t"
            "lw         %[s2],      4(%[cosp])          \n\t"
            "lw         %[s3],      4(%[sinp])          \n\t"
            "lw         %[wl],      -8(%[in2])          \n\t"
            "lw         %[wk],      8(%[in1])           \n\t"
            "mult       $ac0,       %[s0],      %[wj]   \n\t"
            "mult       $ac1,       %[s0],      %[wi]   \n\t"
            "lh         %[j],       0(%[revtab])        \n\t"
            "lh         %[j1],      2(%[revtab])        \n\t"
            "mult       $ac2,       %[s2],      %[wl]   \n\t"
            "mult       $ac3,       %[s2],      %[wk]   \n\t"
            "msub       $ac0,       %[s1],      %[wi]   \n\t"
            "madd       $ac1,       %[s1],      %[wj]   \n\t"
            "msub       $ac2,       %[s3],      %[wk]   \n\t"
            "madd       $ac3,       %[s3],      %[wl]   \n\t"
            "sll        %[j],       %[j],       3       \n\t"
            "sll        %[j1],      %[j1],      3       \n\t"
            "extr_r.w   %[t0],      $ac0,       31      \n\t"
            "extr_r.w   %[t1],      $ac1,       31      \n\t"
            "extr_r.w   %[t2],      $ac2,       31      \n\t"
            "extr_r.w   %[t3],      $ac3,       31      \n\t"
            "addu       %[j],       %[output],  %[j]    \n\t"
            "addu       %[j1],      %[output],  %[j1]   \n\t"
            "lw         %[s0],      8(%[cosp])          \n\t"
            "lw         %[s1],      8(%[sinp])          \n\t"
            "lw         %[wj],      -16(%[in2])         \n\t"
            "lw         %[wi],      16(%[in1])          \n\t"
            "lw         %[s2],      12(%[cosp])         \n\t"
            "lw         %[s3],      12(%[sinp])         \n\t"
            "lw         %[wl],      -24(%[in2])         \n\t"
            "lw         %[wk],      24(%[in1])          \n\t"
            "sw         %[t0],      0(%[j])             \n\t"
            "sw         %[t1],      4(%[j])             \n\t"
            "sw         %[t2],      0(%[j1])            \n\t"
            "sw         %[t3],      4(%[j1])            \n\t"
            "mult       $ac0,       %[s0],      %[wj]   \n\t"
            "mult       $ac1,       %[s0],      %[wi]   \n\t"
            "lh         %[j],       4(%[revtab])        \n\t"
            "lh         %[j1],      6(%[revtab])        \n\t"
            "mult       $ac2,       %[s2],      %[wl]   \n\t"
            "mult       $ac3,       %[s2],      %[wk]   \n\t"
            "msub       $ac0,       %[s1],      %[wi]   \n\t"
            "madd       $ac1,       %[s1],      %[wj]   \n\t"
            "msub       $ac2,       %[s3],      %[wk]   \n\t"
            "madd       $ac3,       %[s3],      %[wl]   \n\t"
            "sll        %[j],       %[j],       3       \n\t"
            "sll        %[j1],      %[j1],      3       \n\t"
            "extr_r.w   %[t0],      $ac0,       31      \n\t"
            "extr_r.w   %[t1],      $ac1,       31      \n\t"
            "extr_r.w   %[t2],      $ac2,       31      \n\t"
            "extr_r.w   %[t3],      $ac3,       31      \n\t"
            "addu       %[j],       %[output],  %[j]    \n\t"
            "addu       %[j1],      %[output],  %[j1]   \n\t"
            "addu       %[sinp],    %[sinp],    16      \n\t"
            "addu       %[cosp],    %[cosp],    16      \n\t"
            "addu       %[in1],     %[in1],     32      \n\t"
            "addu       %[in2],     %[in2],     -32     \n\t"
            "addu       %[revtab],  %[revtab],  8       \n\t"
            "sw         %[t0],      0(%[j])             \n\t"
            "sw         %[t1],      4(%[j])             \n\t"
            "sw         %[t2],      0(%[j1])            \n\t"
            "sw         %[t3],      4(%[j1])            \n\t"

            : [s0] "=&r" (s0), [s1] "=&r" (s1), [j] "=&r" (j),
              [wi] "=&r" (wi), [wj] "=&r" (wj),
              [t0] "=&r" (t0), [t1] "=&r" (t1),
              [t2] "=&r" (t2), [t3] "=&r" (t3),
              [s2] "=&r" (s2), [s3] "=&r" (s3), [j1] "=&r" (j1),
              [wk] "=&r" (wk), [wl] "=&r" (wl), [revtab] "+r" (revtab),
              [cosp] "+r" (cosp), [sinp] "+r" (sinp),
              [in1] "+r" (in1), [in2] "+r" (in2)
            : [output] "r" (output)
            : "memory", "hi", "lo", "$ac1hi", "$ac1lo",
              "$ac2hi", "$ac2lo", "$ac3hi", "$ac3lo"
        );
    }

    s->fft_calc(s, z);

    cosp = &tcos[n8];
    sinp = &tsin[n8];
    z+=n8;
    cosm = &tcos[n8-1];
    sinm = &tsin[n8-1];
    z1 = (FFTComplex *)(z - 1);

    for (k = 0; k < n8; k+=4) {
        FFTSample r0, i0, r1, i1;
        int s0, s1, wi, wj;
        int s2, s3, wk, wl;

        __asm__ volatile(
            "lw         %[s0],      0(%[sinm])          \n\t"
            "lw         %[wj],      4(%[z1])            \n\t"
            "lw         %[s1],      0(%[cosm])          \n\t"
            "lw         %[wi],      0(%[z1])            \n\t"
            "lw         %[s2],      0(%[sinp])          \n\t"
            "lw         %[wl],      4(%[z])             \n\t"
            "lw         %[s3],      0(%[cosp])          \n\t"
            "lw         %[wk],      0(%[z])             \n\t"
            "mult       $ac0,       %[s0],      %[wj]   \n\t"
            "msub       $ac0,       %[s1],      %[wi]   \n\t"
            "mult       $ac1,       %[s0],      %[wi]   \n\t"
            "madd       $ac1,       %[s1],      %[wj]   \n\t"
            "mult       $ac2,       %[s2],      %[wl]   \n\t"
            "msub       $ac2,       %[s3],      %[wk]   \n\t"
            "mult       $ac3,       %[s2],      %[wk]   \n\t"
            "madd       $ac3,       %[s3],      %[wl]   \n\t"
            "extr_r.w   %[r0],      $ac0,       31      \n\t"
            "extr_r.w   %[i1],      $ac1,       31      \n\t"
            "lw         %[s0],      -4(%[sinm])         \n\t"
            "extr_r.w   %[r1],      $ac2,       31      \n\t"
            "extr_r.w   %[i0],      $ac3,       31      \n\t"
            "lw         %[s1],      -4(%[cosm])         \n\t"
            "lw         %[s2],      4(%[sinp])          \n\t"
            "lw         %[s3],      4(%[cosp])          \n\t"
            "sw         %[r0],      0(%[z1])            \n\t"
            "sw         %[i1],      4(%[z])             \n\t"
            "sw         %[r1],      0(%[z])             \n\t"
            "sw         %[i0],      4(%[z1])            \n\t"
            "lw         %[wj],      -4(%[z1])           \n\t"
            "lw         %[wi],      -8(%[z1])           \n\t"
            "lw         %[wk],      8(%[z])             \n\t"
            "lw         %[wl],      12(%[z])            \n\t"
            "mult       $ac0,       %[s0],      %[wj]   \n\t"
            "msub       $ac0,       %[s1],      %[wi]   \n\t"
            "mult       $ac1,       %[s0],      %[wi]   \n\t"
            "madd       $ac1,       %[s1],      %[wj]   \n\t"
            "mult       $ac2,       %[s2],      %[wl]   \n\t"
            "msub       $ac2,       %[s3],      %[wk]   \n\t"
            "mult       $ac3,       %[s2],      %[wk]   \n\t"
            "madd       $ac3,       %[s3],      %[wl]   \n\t"
            "extr_r.w   %[r0],      $ac0,       31      \n\t"
            "extr_r.w   %[i1],      $ac1,       31      \n\t"
            "lw         %[s0],      -8(%[sinm])         \n\t"
            "extr_r.w   %[r1],      $ac2,       31      \n\t"
            "extr_r.w   %[i0],      $ac3,       31      \n\t"
            "lw         %[s1],      -8(%[cosm])         \n\t"
            "lw         %[s2],      8(%[sinp])          \n\t"
            "lw         %[s3],      8(%[cosp])          \n\t"
            "sw         %[r0],      -8(%[z1])           \n\t"
            "sw         %[i1],      12(%[z])            \n\t"
            "sw         %[r1],      8(%[z])             \n\t"
            "sw         %[i0],      -4(%[z1])           \n\t"
            "lw         %[wj],      -12(%[z1])          \n\t"
            "lw         %[wi],      -16(%[z1])          \n\t"
            "lw         %[wl],      20(%[z])            \n\t"
            "lw         %[wk],      16(%[z])            \n\t"
            "mult       $ac0,       %[s0],      %[wj]   \n\t"
            "msub       $ac0,       %[s1],      %[wi]   \n\t"
            "mult       $ac1,       %[s0],      %[wi]   \n\t"
            "madd       $ac1,       %[s1],      %[wj]   \n\t"
            "mult       $ac2,       %[s2],      %[wl]   \n\t"
            "msub       $ac2,       %[s3],      %[wk]   \n\t"
            "mult       $ac3,       %[s2],      %[wk]   \n\t"
            "madd       $ac3,       %[s3],      %[wl]   \n\t"
            "extr_r.w   %[r0],      $ac0,       31      \n\t"
            "extr_r.w   %[i1],      $ac1,       31      \n\t"
            "lw         %[s0],      -12(%[sinm])        \n\t"
            "extr_r.w   %[r1],      $ac2,       31      \n\t"
            "extr_r.w   %[i0],      $ac3,       31      \n\t"
            "lw         %[s1],      -12(%[cosm])        \n\t"
            "lw         %[s2],      12(%[sinp])         \n\t"
            "lw         %[s3],      12(%[cosp])         \n\t"
            "sw         %[r0],      -16(%[z1])          \n\t"
            "sw         %[i1],      20(%[z])            \n\t"
            "sw         %[r1],      16(%[z])            \n\t"
            "sw         %[i0],      -12(%[z1])          \n\t"
            "lw         %[wj],      -20(%[z1])          \n\t"
            "lw         %[wi],      -24(%[z1])          \n\t"
            "lw         %[wk],      24(%[z])            \n\t"
            "lw         %[wl],      28(%[z])            \n\t"
            "mult       $ac0,       %[s0],      %[wj]   \n\t"
            "msub       $ac0,       %[s1],      %[wi]   \n\t"
            "mult       $ac1,       %[s0],      %[wi]   \n\t"
            "madd       $ac1,       %[s1],      %[wj]   \n\t"
            "mult       $ac2,       %[s2],      %[wl]   \n\t"
            "msub       $ac2,       %[s3],      %[wk]   \n\t"
            "mult       $ac3,       %[s2],      %[wk]   \n\t"
            "madd       $ac3,       %[s3],      %[wl]   \n\t"
            "extr_r.w   %[r0],      $ac0,       31      \n\t"
            "extr_r.w   %[i1],      $ac1,       31      \n\t"
            "addu       %[sinp],    %[sinp],    16      \n\t"
            "extr_r.w   %[r1],      $ac2,       31      \n\t"
            "extr_r.w   %[i0],      $ac3,       31      \n\t"
            "addu       %[cosp],    %[cosp],    16      \n\t"
            "addu       %[sinm],    %[sinm],    -16     \n\t"
            "addu       %[cosm],    %[cosm],    -16     \n\t"
            "sw         %[r0],      -24(%[z1])          \n\t"
            "sw         %[i1],      28(%[z])            \n\t"
            "sw         %[r1],      24(%[z])            \n\t"
            "sw         %[i0],      -20(%[z1])          \n\t"
            "addu       %[z1],      %[z1],      -32     \n\t"
            "addu       %[z],       %[z],       32      \n\t"

            : [r0] "=&r" (r0), [i1] "=&r" (i1),
              [s0] "=&r" (s0), [s1] "=&r" (s1),
              [wi] "=&r" (wi), [wj] "=&r" (wj),
              [r1] "=&r" (r1), [i0] "=&r" (i0),
              [s2] "=&r" (s2), [s3] "=&r" (s3),
              [wk] "=&r" (wk), [wl] "=&r" (wl)
            : [cosm] "r" (cosm), [sinm] "r" (sinm),
              [z] "r" (z), [z1] "r" (z1),
              [cosp] "r" (cosp), [sinp] "r" (sinp)
            : "memory", "hi", "lo", "$ac1hi", "$ac1lo",
              "$ac2hi", "$ac2lo", "$ac3hi", "$ac3lo"
        );
    }
}
#elif HAVE_MIPS32R2
static void ff_imdct_half_mips_fixed(FFTContext *s, FFTSample *output, const FFTSample *input)
{
    int n8, n4, n2, n, j;
    const uint16_t *revtab = s->revtab;
    const FFTSample *tcos = s->tcos;
    const FFTSample *tsin = s->tsin;
    const FFTSample *in1, *in2;
    const FFTSample *cosm, *cosp = tcos;
    const FFTSample *sinm, *sinp = tsin;
    int tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, loop_end;
    int a = 0x40000000;
    int b = 0x1;
    FFTComplex *z1, *z = (FFTComplex *)output;

    n = 1 << s->mdct_bits;
    n2 = n >> 1;
    n4 = n >> 2;
    n8 = n >> 3;

    /* pre rotation */
    in1 = input;
    in2 = input + n2 - 1;
    loop_end = (int)&sinp[n4];

    __asm__ volatile (
        ".set    push                                  \n\t"
        ".set    noreorder                             \n\t"

    "1:                                                \n\t"
        "lw      %[tmp0],     0(%[cosp])               \n\t"
        "lw      %[tmp1],     0(%[sinp])               \n\t"
        "lw      %[tmp2],     0(%[in2])                \n\t"
        "lw      %[tmp3],     0(%[in1])                \n\t"
        "lh      %[j],        0(%[revtab])             \n\t"
        "mult    %[tmp0],     %[tmp2]                  \n\t"
        "msub    %[tmp1],     %[tmp3]                  \n\t"
        "madd    %[a],        %[b]                     \n\t"
        "mfhi    %[tmp4]                               \n\t"
        "mflo    %[tmp5]                               \n\t"
        "sll     %[j],        %[j],         3          \n\t"
        "addu    %[j],        %[output],    %[j]       \n\t"
        "mult    %[tmp0],     %[tmp3]                  \n\t"
        "madd    %[tmp1],     %[tmp2]                  \n\t"
        "madd    %[a],        %[b]                     \n\t"
        "mfhi    %[tmp6]                               \n\t"
        "mflo    %[tmp1]                               \n\t"
        "sll     %[tmp4],     1                        \n\t"
        "srl     %[tmp5],     31                       \n\t"
        "or      %[tmp4],     %[tmp4],      %[tmp5]    \n\t"
        "sll     %[tmp6],     1                        \n\t"
        "srl     %[tmp1],     31                       \n\t"
        "or      %[tmp6],     %[tmp6],      %[tmp1]    \n\t"
        "addu    %[sinp],     %[sinp],      4          \n\t"
        "addu    %[cosp],     %[cosp],      4          \n\t"
        "addu    %[in1],      %[in1],       8          \n\t"
        "addu    %[in2],      %[in2],       -8         \n\t"
        "addu    %[revtab],   %[revtab],    2          \n\t"
        "sw      %[tmp4],     0(%[j])                  \n\t"
        "bne     %[loop_end], %[sinp],      1b         \n\t"
        " sw     %[tmp6],     4(%[j])                  \n\t"

        ".set    pop                                   \n\t"
        : [tmp0] "=&r" (tmp0), [tmp1] "=&r" (tmp1), [j] "=&r" (j),
          [tmp2] "=&r" (tmp2), [tmp3] "=&r" (tmp3),
          [tmp4] "=&r" (tmp4), [tmp5] "=&r" (tmp5),
          [tmp6] "=&r" (tmp6), [revtab] "+r" (revtab),
          [cosp] "+r" (cosp), [sinp] "+r" (sinp),
          [in1] "+r" (in1), [in2] "+r" (in2)
        : [output] "r" (output), [a] "r" (a), [b] "r" (b),
          [loop_end] "r" (loop_end)
        : "memory", "hi", "lo"
    );

    s->fft_calc(s, z);

    cosp = &tcos[n8];
    sinp = &tsin[n8];
    z+=n8;
    cosm = &tcos[n8-1];
    sinm = &tsin[n8-1];
    z1 = (FFTComplex *)(z - 1);
    loop_end = (int)&sinp[n8];

    /* post rotation + reordering */
    __asm__ volatile (
        ".set    push                                \n\t"
        ".set    noreorder                           \n\t"

    "1:                                              \n\t"
        "lw      %[tmp0],     0(%[sinm])             \n\t"
        "lw      %[tmp1],     4(%[z1])               \n\t"
        "lw      %[tmp2],     0(%[cosm])             \n\t"
        "lw      %[tmp3],     0(%[z1])               \n\t"
        "mult    %[tmp0],     %[tmp1]                \n\t"
        "msub    %[tmp2],     %[tmp3]                \n\t"
        "madd    %[a],        %[b]                   \n\t"
        "mfhi    %[tmp4]                             \n\t"
        "mflo    %[tmp5]                             \n\t"
        "mult    %[tmp0],     %[tmp3]                \n\t"
        "madd    %[tmp2],     %[tmp1]                \n\t"
        "madd    %[a],        %[b]                   \n\t"
        "mfhi    %[tmp6]                             \n\t"
        "mflo    %[tmp1]                             \n\t"
        "lw      %[tmp0],     0(%[sinp])             \n\t"
        "lw      %[tmp2],     0(%[cosp])             \n\t"
        "lw      %[tmp3],     0(%[z])                \n\t"
        "sll     %[tmp4],     1                      \n\t"
        "sll     %[tmp6],     1                      \n\t"
        "srl     %[tmp1],     31                     \n\t"
        "or      %[tmp6],     %[tmp6],    %[tmp1]    \n\t"
        "lw      %[tmp1],     4(%[z])                \n\t"
        "srl     %[tmp5],     31                     \n\t"
        "or      %[tmp4],     %[tmp4],    %[tmp5]    \n\t"
        "sw      %[tmp6],     4(%[z])                \n\t"
        "sw      %[tmp4],     0(%[z1])               \n\t"
        "mult    %[tmp0],     %[tmp1]                \n\t"
        "msub    %[tmp2],     %[tmp3]                \n\t"
        "madd    %[a],        %[b]                   \n\t"
        "mfhi    %[tmp4]                             \n\t"
        "mflo    %[tmp5]                             \n\t"
        "mult    %[tmp0],     %[tmp3]                \n\t"
        "madd    %[tmp2],     %[tmp1]                \n\t"
        "madd    %[a],        %[b]                   \n\t"
        "mfhi    %[tmp6]                             \n\t"
        "mflo    %[tmp1]                             \n\t"
        "sll     %[tmp4],     1                      \n\t"
        "srl     %[tmp5],     31                     \n\t"
        "or      %[tmp4],     %[tmp4],    %[tmp5]    \n\t"
        "sll     %[tmp6],     1                      \n\t"
        "srl     %[tmp1],     31                     \n\t"
        "or      %[tmp6],     %[tmp6],    %[tmp1]    \n\t"
        "addu    %[z1],       %[z1],      -8         \n\t"
        "addu    %[z],        %[z],       8          \n\t"
        "addu    %[sinp],     %[sinp],    4          \n\t"
        "addu    %[cosp],     %[cosp],    4          \n\t"
        "addu    %[sinm],     %[sinm],    -4         \n\t"
        "addu    %[cosm],     %[cosm],    -4         \n\t"
        "sw      %[tmp4],     -8(%[z])               \n\t"
        "bne     %[loop_end], %[sinp],    1b         \n\t"
        " sw     %[tmp6],     12(%[z1])              \n\t"

        ".set    pop                                 \n\t"
        : [tmp0] "=&r" (tmp0), [tmp1] "=&r" (tmp1), [tmp2] "=&r" (tmp2),
          [tmp3] "=&r" (tmp3), [tmp4] "=&r" (tmp4), [tmp5] "=&r" (tmp5),
          [tmp6] "=&r" (tmp6), [cosp] "+r" (cosp), [sinp] "+r" (sinp),
          [cosm] "+r" (cosm), [sinm] "+r" (sinm),
          [z] "+r" (z), [z1] "+r" (z1)
        : [a] "r" (a), [b] "r" (b), [loop_end] "r" (loop_end)
        : "memory", "hi", "lo"
    );
}
#endif /* HAVE_MIPSDSPR2 || HAVE_MIPSDSPR1 */
#endif /* HAVE_INLINE_ASM */

av_cold void ff_fft_init_fixed32_mips(FFTContext *s)
{
#if HAVE_INLINE_ASM
#if HAVE_MIPSDSPR2 || HAVE_MIPSDSPR1 || HAVE_MIPS32R2
    s->fft_calc     = ff_fft_calc_mips_fixed;
#if CONFIG_MDCT
    s->imdct_half   = ff_imdct_half_mips_fixed;
#endif
#endif
#endif
}
