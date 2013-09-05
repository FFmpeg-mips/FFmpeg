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
 * Author:  Branimir Vasic (branimir.vasic@imgtec.com)
 * Author:  Zoran Lukic (zoran.lukic@imgtec.com)
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
 * Reference: libavutil/fixed_dsp.c
 */

#include "config.h"
#include "libavutil/fixed_dsp.h"

#if HAVE_INLINE_ASM
#if (HAVE_MIPSDSPR1 || HAVE_MIPSDSPR2)
static void vector_q31mul_window_mips(int *dst, const int *src0, const int *src1, const int *win, int len)
{
    int i;
    int s0,s1, wi, wj, tmp, tmp1, *dst1;
    const int *win1;

    src1+= len-1;
    win1 = win + 2*len-1;
    dst1 = dst + 2*len-1;

    for (i=len; i>0; i-=8) {
        __asm__ volatile (
            "lw         %[s0],      0(%[src0])          \n\t"
            "lw         %[wj],      0(%[win1])          \n\t"
            "lw         %[s1],      0(%[src1])          \n\t"
            "lw         %[wi],      0(%[win])           \n\t"
            "mult       $ac0,       %[s0],      %[wj]   \n\t"
            "msub       $ac0,       %[s1],      %[wi]   \n\t"
            "mult       $ac1,       %[s0],      %[wi]   \n\t"
            "madd       $ac1,       %[s1],      %[wj]   \n\t"
            "lw         %[s0],      4(%[src0])          \n\t"
            "lw         %[wj],      -4(%[win1])         \n\t"
            "extr_r.w   %[tmp],     $ac0,       31      \n\t"
            "lw         %[s1],      -4(%[src1])         \n\t"
            "extr_r.w   %[tmp1],    $ac1,       31      \n\t"
            "lw         %[wi],      4(%[win])           \n\t"
            "mult       $ac0,       %[s0],      %[wj]   \n\t"
            "msub       $ac0,       %[s1],      %[wi]   \n\t"
            "mult       $ac1,       %[s0],      %[wi]   \n\t"
            "madd       $ac1,       %[s1],      %[wj]   \n\t"
            "sw         %[tmp],     0(%[dst])           \n\t"
            "sw         %[tmp1],    0(%[dst1])          \n\t"
            "lw         %[s0],      8(%[src0])          \n\t"
            "extr_r.w   %[tmp],     $ac0,       31      \n\t"
            "lw         %[wj],      -8(%[win1])         \n\t"
            "extr_r.w   %[tmp1],    $ac1,       31      \n\t"
            "lw         %[s1],      -8(%[src1])         \n\t"
            "lw         %[wi],      8(%[win])           \n\t"
            "mult       $ac0,       %[s0],      %[wj]   \n\t"
            "msub       $ac0,       %[s1],      %[wi]   \n\t"
            "sw         %[tmp],     4(%[dst])           \n\t"
            "mult       $ac1,       %[s0],      %[wi]   \n\t"
            "madd       $ac1,       %[s1],      %[wj]   \n\t"
            "sw         %[tmp1],    -4(%[dst1])         \n\t"
            "lw         %[s0],      12(%[src0])         \n\t"
            "lw         %[wj],      -12(%[win1])        \n\t"
            "extr_r.w   %[tmp],     $ac0,       31      \n\t"
            "extr_r.w   %[tmp1],    $ac1,       31      \n\t"
            "lw         %[s1],      -12(%[src1])        \n\t"
            "lw         %[wi],      12(%[win])          \n\t"
            "mult       $ac0,       %[s0],      %[wj]   \n\t"
            "msub       $ac0,       %[s1],      %[wi]   \n\t"
            "mult       $ac1,       %[s0],      %[wi]   \n\t"
            "madd       $ac1,       %[s1],      %[wj]   \n\t"
            "sw         %[tmp],     8(%[dst])           \n\t"
            "sw         %[tmp1],    -8(%[dst1])         \n\t"
            "extr_r.w   %[tmp],     $ac0,       31      \n\t"
            "lw         %[s0],      16(%[src0])         \n\t"
            "extr_r.w   %[tmp1],    $ac1,       31      \n\t"
            "lw         %[wj],      -16(%[win1])        \n\t"
            "lw         %[s1],      -16(%[src1])        \n\t"
            "lw         %[wi],      16(%[win])          \n\t"
            "mult       $ac0,       %[s0],      %[wj]   \n\t"
            "sw         %[tmp],     12(%[dst])          \n\t"
            "msub       $ac0,       %[s1],      %[wi]   \n\t"
            "sw         %[tmp1],    -12(%[dst1])        \n\t"
            "mult       $ac1,       %[s0],      %[wi]   \n\t"
            "madd       $ac1,       %[s1],      %[wj]   \n\t"
            "lw         %[s0],      20(%[src0])         \n\t"
            "lw         %[wj],      -20(%[win1])        \n\t"
            "extr_r.w   %[tmp],     $ac0,       31      \n\t"
            "lw         %[s1],      -20(%[src1])        \n\t"
            "extr_r.w   %[tmp1],    $ac1,       31      \n\t"
            "lw         %[wi],      20(%[win])          \n\t"
            "mult       $ac0,       %[s0],      %[wj]   \n\t"
            "msub       $ac0,       %[s1],      %[wi]   \n\t"
            "mult       $ac1,       %[s0],      %[wi]   \n\t"
            "madd       $ac1,       %[s1],      %[wj]   \n\t"
            "sw         %[tmp],     16(%[dst])          \n\t"
            "sw         %[tmp1],    -16(%[dst1])        \n\t"
            "lw         %[s0],      24(%[src0])         \n\t"
            "extr_r.w   %[tmp],     $ac0,       31      \n\t"
            "lw         %[wj],      -24(%[win1])        \n\t"
            "extr_r.w   %[tmp1],    $ac1,       31      \n\t"
            "lw         %[s1],      -24(%[src1])        \n\t"
            "lw         %[wi],      24(%[win])          \n\t"
            "mult       $ac0,       %[s0],      %[wj]   \n\t"
            "msub       $ac0,       %[s1],      %[wi]   \n\t"
            "sw         %[tmp],     20(%[dst])          \n\t"
            "mult       $ac1,       %[s0],      %[wi]   \n\t"
            "madd       $ac1,       %[s1],      %[wj]   \n\t"
            "sw         %[tmp1],    -20(%[dst1])        \n\t"
            "lw         %[s0],      28(%[src0])         \n\t"
            "lw         %[wj],      -28(%[win1])        \n\t"
            "extr_r.w   %[tmp],     $ac0,       31      \n\t"
            "extr_r.w   %[tmp1],    $ac1,       31      \n\t"
            "lw         %[s1],      -28(%[src1])        \n\t"
            "lw         %[wi],      28(%[win])          \n\t"
            "mult       $ac0,       %[s0],      %[wj]   \n\t"
            "msub       $ac0,       %[s1],      %[wi]   \n\t"
            "mult       $ac1,       %[s0],      %[wi]   \n\t"
            "madd       $ac1,       %[s1],      %[wj]   \n\t"
            "sw         %[tmp],     24(%[dst])          \n\t"
            "sw         %[tmp1],    -24(%[dst1])        \n\t"
            "extr_r.w   %[tmp],     $ac0,       31      \n\t"
            "extr_r.w   %[tmp1],    $ac1,       31      \n\t"
            "addiu      %[src0],    %[src0],    32      \n\t"
            "addiu      %[src1],    %[src1],    -32     \n\t"
            "addiu      %[win],     %[win],     32      \n\t"
            "addiu      %[win1],    %[win1],    -32     \n\t"
            "sw         %[tmp],     28(%[dst])          \n\t"
            "addiu      %[dst],     %[dst],     32      \n\t"
            "sw         %[tmp1],    -28(%[dst1])        \n\t"
            "addiu      %[dst1],    %[dst1],    -32     \n\t"

            : [src0] "+r" (src0), [src1] "+r" (src1),
              [win] "+r" (win), [win1] "+r" (win1),
              [dst] "+r" (dst), [dst1] "+r" (dst1),
              [tmp] "=&r" (tmp), [tmp1] "=&r" (tmp1),
              [s0] "=&r" (s0), [s1] "=&r" (s1),
              [wi] "=&r" (wi), [wj] "=&r" (wj)
            :
            : "memory", "hi", "lo", "$ac1hi", "$ac1lo"
        );
    }
}
#else
#if HAVE_MIPS32R2
static void vector_q31mul_window_mips(int *dst, const int *src0, const int *src1, const int *win, int len)
{
    int i;
    int s0,s1, wi, wj, tmp, tmp1, *dst1;
    const int *win1;
    long long c1 = 0x80000000;

    src1+= len-1;
    win1 = win + 2*len-1;
    dst1 = dst + 2*len-1;

    for (i=len; i>0; i--) {
        __asm__ volatile (
            "lw         %[s0],      0(%[src0])          \n\t"
            "lw         %[wj],      0(%[win1])          \n\t"
            "lw         %[s1],      0(%[src1])          \n\t"
            "lw         %[wi],      0(%[win])           \n\t"
            "mthi       $0                              \n\t"
            "sll        %[s0],      %[s0],      1       \n\t"
            "sll        %[s1],      %[s1],      1       \n\t"
            "mtlo       %[c1]                           \n\t"
            "madd       %[s0],      %[wj]               \n\t"
            "msub       %[s1],      %[wi]               \n\t"
            "mfhi       %[tmp]                          \n\t"
            "mthi       $0                              \n\t"
            "mtlo       %[c1]                           \n\t"
            "madd       %[s0],      %[wi]               \n\t"
            "madd       %[s1],      %[wj]               \n\t"
            "mfhi       %[tmp1]                         \n\t"
            "addiu      %[src0],    %[src0],    4       \n\t"
            "addiu      %[src1],    %[src1],    -4      \n\t"
            "addiu      %[win],     %[win],     4       \n\t"
            "addiu      %[win1],    %[win1],    -4      \n\t"
            "sw         %[tmp],     0(%[dst])           \n\t"
            "addiu      %[dst],     %[dst],     4       \n\t"
            "sw         %[tmp1],    0(%[dst1])          \n\t"
            "addiu      %[dst1],    %[dst1],    -4      \n\t"

            : [src0] "+r" (src0), [src1] "+r" (src1),
              [win] "+r" (win), [win1] "+r" (win1),
              [dst] "+r" (dst), [dst1] "+r" (dst1),
              [tmp] "=&r" (tmp), [tmp1] "=&r" (tmp1),
              [s0] "=&r" (s0), [s1] "=&r" (s1),
              [wi] "=&r" (wi), [wj] "=&r" (wj)
            : [c1] "r" (c1)
            : "memory", "hi", "lo"
        );
    }
}
#endif /* HAVE_MIPS32R2 */
#endif

#if HAVE_MIPSDSPR2
static void vector_imul_add_mips(int *dst, const int *src0, const int *src1, const int *src2, int len)
{
    int i;
    int s0, s1, s2;
    int s3, s4, s5;
    int s6, s7, s8;
    int s9, s10, s11;

    for(i=0; i<len; i+=16) {
        __asm__ volatile (
            "lw         %[s0],      0(%[src0])          \n\t"
            "lw         %[s1],      0(%[src1])          \n\t"
            "lw         %[s2],      0(%[src2])          \n\t"
            "lw         %[s3],      4(%[src0])          \n\t"
            "lw         %[s4],      4(%[src1])          \n\t"
            "mulq_rs.w  %[s0],      %[s0],      %[s1]   \n\t"
            "lw         %[s5],      4(%[src2])          \n\t"
            "mulq_rs.w  %[s3],      %[s3],      %[s4]   \n\t"
            "lw         %[s6],      8(%[src0])          \n\t"
            "lw         %[s7],      8(%[src1])          \n\t"
            "addu       %[s0],      %[s0],      %[s2]   \n\t"
            "lw         %[s8],      8(%[src2])          \n\t"
            "sw         %[s0],      0(%[dst])           \n\t"
            "mulq_rs.w  %[s6],      %[s6],      %[s7]   \n\t"
            "addu       %[s3],      %[s3],      %[s5]   \n\t"
            "lw         %[s9],      12(%[src0])         \n\t"
            "lw         %[s10],     12(%[src1])         \n\t"
            "sw         %[s3],      4(%[dst])           \n\t"
            "lw         %[s11],     12(%[src2])         \n\t"
            "mulq_rs.w  %[s9],      %[s9],      %[s10]  \n\t"
            "addu       %[s6],      %[s6],      %[s8]   \n\t"
            "lw         %[s0],      16(%[src0])         \n\t"
            "lw         %[s1],      16(%[src1])         \n\t"
            "sw         %[s6],      8(%[dst])           \n\t"
            "lw         %[s2],      16(%[src2])         \n\t"
            "mulq_rs.w  %[s0],      %[s0],      %[s1]   \n\t"
            "addu       %[s9],      %[s9],      %[s11]  \n\t"
            "lw         %[s3],      20(%[src0])         \n\t"
            "lw         %[s4],      20(%[src1])         \n\t"
            "sw         %[s9],      12(%[dst])          \n\t"
            "lw         %[s5],      20(%[src2])         \n\t"
            "mulq_rs.w  %[s3],      %[s3],      %[s4]   \n\t"
            "addu       %[s0],      %[s0],      %[s2]   \n\t"
            "lw         %[s6],      24(%[src0])         \n\t"
            "lw         %[s7],      24(%[src1])         \n\t"
            "sw         %[s0],      16(%[dst])          \n\t"
            "lw         %[s8],      24(%[src2])         \n\t"
            "mulq_rs.w  %[s6],      %[s6],      %[s7]   \n\t"
            "addu       %[s3],      %[s3],      %[s5]   \n\t"
            "lw         %[s9],      28(%[src0])         \n\t"
            "lw         %[s10],     28(%[src1])         \n\t"
            "sw         %[s3],      20(%[dst])          \n\t"
            "lw         %[s11],     28(%[src2])         \n\t"
            "mulq_rs.w  %[s9],      %[s9],      %[s10]  \n\t"
            "addu       %[s6],      %[s6],      %[s8]   \n\t"
            "lw         %[s0],      32(%[src0])         \n\t"
            "lw         %[s1],      32(%[src1])         \n\t"
            "sw         %[s6],      24(%[dst])          \n\t"
            "lw         %[s2],      32(%[src2])         \n\t"
            "lw         %[s3],      36(%[src0])         \n\t"
            "mulq_rs.w  %[s0],      %[s0],      %[s1]   \n\t"
            "addu       %[s9],      %[s9],      %[s11]  \n\t"
            "lw         %[s4],      36(%[src1])         \n\t"
            "lw         %[s5],      36(%[src2])         \n\t"
            "sw         %[s9],      28(%[dst])          \n\t"
            "mulq_rs.w  %[s3],      %[s3],      %[s4]   \n\t"
            "lw         %[s6],      40(%[src0])         \n\t"
            "lw         %[s7],      40(%[src1])         \n\t"
            "addu       %[s0],      %[s0],      %[s2]   \n\t"
            "lw         %[s8],      40(%[src2])         \n\t"
            "sw         %[s0],      32(%[dst])          \n\t"
            "mulq_rs.w  %[s6],      %[s6],      %[s7]   \n\t"
            "addu       %[s3],      %[s3],      %[s5]   \n\t"
            "lw         %[s9],      44(%[src0])         \n\t"
            "lw         %[s10],     44(%[src1])         \n\t"
            "sw         %[s3],      36(%[dst])          \n\t"
            "lw         %[s11],     44(%[src2])         \n\t"
            "mulq_rs.w  %[s9],      %[s9],      %[s10]  \n\t"
            "addu       %[s6],      %[s6],      %[s8]   \n\t"
            "lw         %[s0],      48(%[src0])         \n\t"
            "lw         %[s1],      48(%[src1])         \n\t"
            "sw         %[s6],      40(%[dst])          \n\t"
            "lw         %[s2],      48(%[src2])         \n\t"
            "mulq_rs.w  %[s0],      %[s0],      %[s1]   \n\t"
            "addu       %[s9],      %[s9],      %[s11]  \n\t"
            "lw         %[s3],      52(%[src0])         \n\t"
            "lw         %[s4],      52(%[src1])         \n\t"
            "sw         %[s9],      44(%[dst])          \n\t"
            "lw         %[s5],      52(%[src2])         \n\t"
            "mulq_rs.w  %[s3],      %[s3],      %[s4]   \n\t"
            "addu       %[s0],      %[s0],      %[s2]   \n\t"
            "lw         %[s6],      56(%[src0])         \n\t"
            "lw         %[s7],      56(%[src1])         \n\t"
            "sw         %[s0],      48(%[dst])          \n\t"
            "lw         %[s8],      56(%[src2])         \n\t"
            "mulq_rs.w  %[s6],      %[s6],      %[s7]   \n\t"
            "addu       %[s3],      %[s3],      %[s5]   \n\t"
            "lw         %[s9],      60(%[src0])         \n\t"
            "lw         %[s10],     60(%[src1])         \n\t"
            "sw         %[s3],      52(%[dst])          \n\t"
            "lw         %[s11],     60(%[src2])         \n\t"
            "mulq_rs.w  %[s9],      %[s9],      %[s10]  \n\t"
            "addu       %[s6],      %[s6],      %[s8]   \n\t"
            "addiu      %[src0],    %[src0],    64      \n\t"
            "sw         %[s6],      56(%[dst])          \n\t"
            "addiu      %[src1],    %[src1],    64      \n\t"
            "addiu      %[src2],    %[src2],    64      \n\t"
            "addu       %[s9],      %[s9],      %[s11]  \n\t"
            "sw         %[s9],      60(%[dst])          \n\t"
            "addiu      %[dst],     %[dst],     64      \n\t"

            : [src0] "+r" (src0), [src1] "+r" (src1),
              [src2] "+r" (src2), [dst] "+r" (dst),
              [s0] "=&r" (s0), [s1] "=&r" (s1), [s2] "=&r" (s2),
              [s3] "=&r" (s3), [s4] "=&r" (s4), [s5] "=&r" (s5),
              [s6] "=&r" (s6), [s7] "=&r" (s7), [s8] "=&r" (s8),
              [s9] "=&r" (s9), [s10] "=&r" (s10), [s11] "=&r" (s11)
            :
            : "memory", "hi", "lo"
        );
    }
}

static void vector_q31mul_reverse_mips(int *dst, const int *src0, const int *src1, int len)
{
    int *src0_end;
    int s0, s1, s2, s3, s4, s5, s6, s7;
    int s8, s9, s10, s11;

    src1 += len-1;
    src0_end = (int *)src0 + len;

    __asm__ volatile(
        ".set       push                                    \n\t"
        ".set       noreorder                               \n\t"

            /*loop unrolled eight times */
        "1:                                                 \n\t"
        "lw         %[s0],      0(%[src0])                  \n\t"
        "lw         %[s1],      0(%[src1])                  \n\t"
        "lw         %[s2],      4(%[src0])                  \n\t"
        "lw         %[s3],      -4(%[src1])                 \n\t"
        "lw         %[s4],      8(%[src0])                  \n\t"
        "lw         %[s5],      -8(%[src1])                 \n\t"
        "lw         %[s6],      12(%[src0])                 \n\t"
        "lw         %[s7],      -12(%[src1])                \n\t"
        "mulq_rs.w  %[s0],      %[s0],          %[s1]       \n\t"
        "mulq_rs.w  %[s2],      %[s2],          %[s3]       \n\t"
        "mulq_rs.w  %[s4],      %[s4],          %[s5]       \n\t"
        "mulq_rs.w  %[s6],      %[s6],          %[s7]       \n\t"
        "lw         %[s8],      16(%[src0])                 \n\t"
        "lw         %[s9],      -16(%[src1])                \n\t"
        "lw         %[s10],     20(%[src0])                 \n\t"
        "lw         %[s11],     -20(%[src1])                \n\t"
        "lw         %[s1],      24(%[src0])                 \n\t"
        "lw         %[s3],      -24(%[src1])                \n\t"
        "lw         %[s5],      28(%[src0])                 \n\t"
        "lw         %[s7],      -28(%[src1])                \n\t"
        "mulq_rs.w  %[s8],      %[s8],          %[s9]       \n\t"
        "mulq_rs.w  %[s10],     %[s10],         %[s11]      \n\t"
        "mulq_rs.w  %[s9],     %[s1],           %[s3]       \n\t"
        "mulq_rs.w  %[s11],     %[s5],          %[s7]       \n\t"
        "addiu      %[src0],    32                          \n\t"
        "addiu      %[src1],    -32                         \n\t"
        "sw         %[s0],      0(%[dst])                   \n\t"
        "sw         %[s2],      4(%[dst])                   \n\t"
        "sw         %[s4],      8(%[dst])                   \n\t"
        "sw         %[s6],      12(%[dst])                  \n\t"
        "sw         %[s8],      16(%[dst])                  \n\t"
        "sw         %[s10],     20(%[dst])                  \n\t"
        "sw         %[s9],      24(%[dst])                  \n\t"
        "sw         %[s11],     28(%[dst])                  \n\t"
        "bne        %[src0],    %[src0_end],    1b          \n\t"
        " addiu     %[dst],     32                          \n\t"

        ".set       pop"
        : [src0]"+r"(src0), [src1]"+r"(src1), [dst]"+r"(dst),
          [s0]"=&r"(s0), [s1]"=&r"(s1), [s2]"=&r"(s2), [s3]"=&r"(s3),
          [s4]"=&r"(s4), [s5]"=&r"(s5), [s6]"=&r"(s6), [s7]"=&r"(s7),
          [s8]"=&r"(s8), [s9]"=&r"(s9), [s10]"=&r"(s10), [s11]"=&r"(s11)
        : [src0_end]"r"(src0_end)
        : "memory", "hi", "lo"
    );
}
#elif   HAVE_MIPSDSPR1
static void vector_q31mul_reverse_mips(int *dst, const int *src0, const int *src1, int len)
{
    int *src0_end;
    int s0, s1, s2, s3, s4, s5, s6, s7;
    int s8, s9, s10, s11, s12, s13, s14, s15;

    src1 += len-1;
    src0_end = (int *)src0 + len;

    __asm__ volatile(
        ".set       push                                    \n\t"
        ".set       noreorder                               \n\t"

            /*loop unrolled eight times */
    "1:                                                     \n\t"
        "lw         %[s0],      0(%[src0])                  \n\t"
        "lw         %[s1],      0(%[src1])                  \n\t"
        "lw         %[s2],      4(%[src0])                  \n\t"
        "lw         %[s3],      -4(%[src1])                 \n\t"
        "lw         %[s4],      8(%[src0])                  \n\t"
        "lw         %[s5],      -8(%[src1])                 \n\t"
        "lw         %[s6],      12(%[src0])                 \n\t"
        "lw         %[s7],      -12(%[src1])                \n\t"
        "mult       $ac0,       %[s0],          %[s1]       \n\t"
        "mult       $ac1,       %[s2],          %[s3]       \n\t"
        "mult       $ac2,       %[s4],          %[s5]       \n\t"
        "mult       $ac3,       %[s6],          %[s7]       \n\t"
        "addiu      %[src0],    32                          \n\t"
        "extr_r.w   %[s0],      $ac0,           31          \n\t"
        "extr_r.w   %[s2],      $ac1,           31          \n\t"
        "extr_r.w   %[s4],      $ac2,           31          \n\t"
        "extr_r.w   %[s6],      $ac3,           31          \n\t"
        "lw         %[s8],      -16(%[src0])                \n\t"
        "lw         %[s9],      -16(%[src1])                \n\t"
        "lw         %[s10],     -12(%[src0])                \n\t"
        "lw         %[s11],     -20(%[src1])                \n\t"
        "lw         %[s1],      -8(%[src0])                 \n\t"
        "lw         %[s3],      -24(%[src1])                \n\t"
        "lw         %[s5],      -4(%[src0])                 \n\t"
        "lw         %[s7],      -28(%[src1])                \n\t"
        "mult       $ac0,       %[s8],          %[s9]       \n\t"
        "mult       $ac1,       %[s10],         %[s11]      \n\t"
        "mult       $ac2,       %[s1],          %[s3]       \n\t"
        "mult       $ac3,       %[s5],          %[s7]       \n\t"
        "addiu      %[src1],    -32                         \n\t"
        "extr_r.w   %[s8],      $ac0,           31          \n\t"
        "extr_r.w   %[s9],      $ac1,           31          \n\t"
        "extr_r.w   %[s10],     $ac2,           31          \n\t"
        "extr_r.w   %[s11],     $ac3,           31          \n\t"
        "sw         %[s0],      0(%[dst])                   \n\t"
        "sw         %[s2],      4(%[dst])                   \n\t"
        "sw         %[s4],      8(%[dst])                   \n\t"
        "sw         %[s6],      12(%[dst])                  \n\t"
        "sw         %[s8],      16(%[dst])                  \n\t"
        "sw         %[s9],      20(%[dst])                  \n\t"
        "sw         %[s10],     24(%[dst])                  \n\t"
        "sw         %[s11],     28(%[dst])                  \n\t"
        "bne        %[src0],    %[src0_end],    1b          \n\t"
        " addiu     %[dst],     32                          \n\t"

        ".set       pop                                     \n\t"
        : [src0]"+r"(src0), [src1]"+r"(src1), [dst]"+r"(dst),
          [s0]"=&r"(s0), [s1]"=&r"(s1), [s2]"=&r"(s2), [s3]"=&r"(s3),
          [s4]"=&r"(s4), [s5]"=&r"(s5), [s6]"=&r"(s6), [s7]"=&r"(s7),
          [s8]"=&r"(s8), [s9]"=&r"(s9), [s10]"=&r"(s10), [s11]"=&r"(s11)
        : [src0_end]"r"(src0_end)
        : "memory", "hi", "lo", "$ac1hi", "$ac1lo",
          "$ac2hi", "$ac2lo", "$ac3hi", "$ac3lo"
    );
}

static void vector_imul_add_mips(int *dst, const int *src0, const int *src1, const int *src2, int len)
{
    int i;
    int s0, s1, s2;
    int s3, s4, s5;
    int s6, s7, s8;
    int s9, s10, s11;

    for(i=0; i<len; i+=16) {
        __asm__ volatile (
            "lw         %[s0],      0(%[src0])          \n\t"
            "lw         %[s1],      0(%[src1])          \n\t"
            "lw         %[s3],      4(%[src0])          \n\t"
            "lw         %[s4],      4(%[src1])          \n\t"
            "mult       $ac0,       %[s0],      %[s1]   \n\t"
            "mult       $ac1,       %[s3],      %[s4]   \n\t"
            "lw         %[s2],      0(%[src2])          \n\t"
            "lw         %[s6],      8(%[src0])          \n\t"
            "lw         %[s7],      8(%[src1])          \n\t"
            "extr_rs.w  %[s0],      $ac0,       31      \n\t"
            "extr_rs.w  %[s3],      $ac1,       31      \n\t"
            "lw         %[s5],      4(%[src2])          \n\t"
            "mult       $ac2,       %[s6],      %[s7]   \n\t"
            "lw         %[s9],      12(%[src0])         \n\t"
            "lw         %[s10],     12(%[src1])         \n\t"
            "addu       %[s0],      %[s0],      %[s2]   \n\t"
            "addu       %[s3],      %[s3],      %[s5]   \n\t"
            "extr_rs.w  %[s6],      $ac2,       31      \n\t"
            "mult       $ac3,       %[s9],      %[s10]  \n\t"
            "lw         %[s1],      16(%[src1])         \n\t"
            "lw         %[s8],      8(%[src2])          \n\t"
            "sw         %[s0],      0(%[dst])           \n\t"
            "sw         %[s3],      4(%[dst])           \n\t"
            "lw         %[s0],      16(%[src0])         \n\t"
            "extr_rs.w  %[s9],      $ac3,       31      \n\t"
            "lw         %[s11],     12(%[src2])         \n\t"
            "mult       $ac0,       %[s0],      %[s1]   \n\t"
            "addu       %[s6],      %[s6],      %[s8]   \n\t"
            "lw         %[s3],      20(%[src0])         \n\t"
            "lw         %[s4],      20(%[src1])         \n\t"
            "addu       %[s9],      %[s9],      %[s11]  \n\t"
            "extr_rs.w  %[s0],      $ac0,       31      \n\t"
            "sw         %[s6],      8(%[dst])           \n\t"
            "mult       $ac1,       %[s3],      %[s4]   \n\t"
            "sw         %[s9],      12(%[dst])          \n\t"
            "lw         %[s2],      16(%[src2])         \n\t"
            "lw         %[s6],      24(%[src0])         \n\t"
            "lw         %[s7],      24(%[src1])         \n\t"
            "extr_rs.w  %[s3],      $ac1,       31      \n\t"
            "addu       %[s0],      %[s0],      %[s2]   \n\t"
            "lw         %[s5],      20(%[src2])         \n\t"
            "mult       $ac2,       %[s6],      %[s7]   \n\t"
            "sw         %[s0],      16(%[dst])          \n\t"
            "addu       %[s3],      %[s3],      %[s5]   \n\t"
            "lw         %[s9],      28(%[src0])         \n\t"
            "lw         %[s10],     28(%[src1])         \n\t"
            "extr_rs.w  %[s6],      $ac2,       31      \n\t"
            "lw         %[s8],      24(%[src2])         \n\t"
            "sw         %[s3],      20(%[dst])          \n\t"
            "mult       $ac3,       %[s9],      %[s10]  \n\t"
            "lw         %[s11],     28(%[src2])         \n\t"
            "lw         %[s0],      32(%[src0])         \n\t"
            "lw         %[s1],      32(%[src1])         \n\t"
            "addu       %[s6],      %[s6],      %[s8]   \n\t"
            "extr_rs.w  %[s9],      $ac3,       31      \n\t"
            "lw         %[s3],      36(%[src0])         \n\t"
            "mult       $ac0,       %[s0],      %[s1]   \n\t"
            "lw         %[s4],      36(%[src1])         \n\t"
            "lw         %[s2],      32(%[src2])         \n\t"
            "sw         %[s6],      24(%[dst])          \n\t"
            "addu       %[s9],      %[s9],      %[s11]  \n\t"
            "extr_rs.w  %[s0],      $ac0,       31      \n\t"
            "sw         %[s9],      28(%[dst])          \n\t"
            "mult       $ac1,       %[s3],      %[s4]   \n\t"
            "lw         %[s6],      40(%[src0])         \n\t"
            "lw         %[s7],      40(%[src1])         \n\t"
            "lw         %[s9],      44(%[src0])         \n\t"
            "lw         %[s10],     44(%[src1])         \n\t"
            "extr_rs.w  %[s3],      $ac1,       31      \n\t"
            "addu       %[s0],      %[s0],      %[s2]   \n\t"
            "mult       $ac2,       %[s6],      %[s7]   \n\t"
            "lw         %[s5],      36(%[src2])         \n\t"
            "mult       $ac3,       %[s9],      %[s10]  \n\t"
            "lw         %[s8],      40(%[src2])         \n\t"
            "sw         %[s0],      32(%[dst])          \n\t"
            "extr_rs.w  %[s6],      $ac2,       31      \n\t"
            "addu       %[s3],      %[s3],      %[s5]   \n\t"
            "extr_rs.w  %[s9],      $ac3,       31      \n\t"
            "lw         %[s0],      48(%[src0])         \n\t"
            "lw         %[s1],      48(%[src1])         \n\t"
            "sw         %[s3],      36(%[dst])          \n\t"
            "lw         %[s11],     44(%[src2])         \n\t"
            "addu       %[s6],      %[s6],      %[s8]   \n\t"
            "mult       $ac0,       %[s0],      %[s1]   \n\t"
            "addu       %[s9],      %[s9],      %[s11]  \n\t"
            "lw         %[s3],      52(%[src0])         \n\t"
            "lw         %[s4],      52(%[src1])         \n\t"
            "sw         %[s9],      44(%[dst])          \n\t"
            "extr_rs.w  %[s0],      $ac0,       31      \n\t"
            "lw         %[s5],      52(%[src2])         \n\t"
            "mult       $ac1,       %[s3],      %[s4]   \n\t"
            "lw         %[s2],      48(%[src2])         \n\t"
            "sw         %[s6],      40(%[dst])          \n\t"
            "lw         %[s6],      56(%[src0])         \n\t"
            "lw         %[s7],      56(%[src1])         \n\t"
            "extr_rs.w  %[s3],      $ac1,       31      \n\t"
            "addu       %[s0],      %[s0],      %[s2]   \n\t"
            "lw         %[s9],      60(%[src0])         \n\t"
            "mult       $ac2,       %[s6],      %[s7]   \n\t"
            "lw         %[s10],     60(%[src1])         \n\t"
            "lw         %[s8],      56(%[src2])         \n\t"
            "sw         %[s0],      48(%[dst])          \n\t"
            "mult       $ac3,       %[s9],      %[s10]  \n\t"
            "extr_rs.w  %[s6],      $ac2,       31      \n\t"
            "addu       %[s3],      %[s3],      %[s5]   \n\t"
            "lw         %[s11],     60(%[src2])         \n\t"
            "sw         %[s3],      52(%[dst])          \n\t"
            "extr_rs.w  %[s9],      $ac3,       31      \n\t"
            "addiu      %[src0],    %[src0],    64      \n\t"
            "addu       %[s6],      %[s6],      %[s8]   \n\t"
            "addiu      %[src1],    %[src1],    64      \n\t"
            "sw         %[s6],      56(%[dst])          \n\t"
            "addu       %[s9],      %[s9],      %[s11]  \n\t"
            "addiu      %[src2],    %[src2],    64      \n\t"
            "sw         %[s9],      60(%[dst])          \n\t"
            "addiu      %[dst],     %[dst],     64      \n\t"

            : [src0] "+r" (src0), [src1] "+r" (src1),
              [src2] "+r" (src2), [dst] "+r" (dst),
              [s0] "=&r" (s0), [s1] "=&r" (s1), [s2] "=&r" (s2),
              [s3] "=&r" (s3), [s4] "=&r" (s4), [s5] "=&r" (s5),
              [s6] "=&r" (s6), [s7] "=&r" (s7), [s8] "=&r" (s8),
              [s9] "=&r" (s9), [s10] "=&r" (s10), [s11] "=&r" (s11)
            :
            : "memory", "hi", "lo", "$ac1hi", "$ac1lo",
              "$ac2hi", "$ac2lo", "$ac3hi", "$ac3lo"
        );
    }
}
#else
#if HAVE_MIPS32R2
static void vector_q31mul_reverse_mips(int *dst, const int *src0, const int *src1, int len)
{
    int *src0_end;
    int round, one;
    int s0, s1, s2, s3, s4, s5, s6, s7;
    int s8, s9, s10, s11, s12, s13, s14, s15;

    src1 += len-1;
    src0_end = (int *)src0 + len;

    __asm__ volatile(
        ".set       push                                    \n\t"
        ".set       noreorder                               \n\t"

            /* loop unrolled eight times */
        "lui        %[round],   0x4000                      \n\t"
        "li         %[one],     1                           \n\t"
    "1:                                                     \n\t"
        "lw         %[s0],      0(%[src0])                  \n\t"
        "lw         %[s1],      0(%[src1])                  \n\t"
        "mult       %[round],   %[one]                      \n\t"
        "madd       %[s0],      %[s1]                       \n\t"
        "mflo       %[s1]                                   \n\t"
        "mfhi       %[s0]                                   \n\t"
        "lw         %[s2],      4(%[src0])                  \n\t"
        "lw         %[s3],      -4(%[src1])                 \n\t"
        "mult       %[round],   %[one]                      \n\t"
        "srl        %[s1],      31                          \n\t"
        "sll        %[s0],      1                           \n\t"
        "or         %[s0],      %[s1]                       \n\t"
        "madd       %[s2],      %[s3]                       \n\t"
        "sw         %[s0],      0(%[dst])                   \n\t"
        "mflo       %[s3]                                   \n\t"
        "mfhi       %[s2]                                   \n\t"
        "lw         %[s4],      8(%[src0])                  \n\t"
        "lw         %[s5],      -8(%[src1])                 \n\t"
        "mult       %[round],   %[one]                      \n\t"
        "srl        %[s3],      31                          \n\t"
        "sll        %[s2],      1                           \n\t"
        "or         %[s2],      %[s3]                       \n\t"
        "madd       %[s4],      %[s5]                       \n\t"
        "sw         %[s2],      4(%[dst])                   \n\t"
        "mflo       %[s5]                                   \n\t"
        "mfhi       %[s4]                                   \n\t"
        "lw         %[s6],      12(%[src0])                 \n\t"
        "lw         %[s7],      -12(%[src1])                \n\t"
        "mult       %[round],   %[one]                      \n\t"
        "srl        %[s5],      31                          \n\t"
        "sll        %[s4],      1                           \n\t"
        "or         %[s4],      %[s5]                       \n\t"
        "madd       %[s6],      %[s7]                       \n\t"
        "sw         %[s4],      8(%[dst])                   \n\t"
        "mflo       %[s7]                                   \n\t"
        "mfhi       %[s6]                                   \n\t"
        "lw         %[s8],      16(%[src0])                 \n\t"
        "lw         %[s9],      -16(%[src1])                \n\t"
        "mult       %[round],   %[one]                      \n\t"
        "srl        %[s7],      31                          \n\t"
        "sll        %[s6],      1                           \n\t"
        "or         %[s6],      %[s7]                       \n\t"
        "madd       %[s8],      %[s9]                       \n\t"
        "sw         %[s6],      12(%[dst])                  \n\t"
        "mflo       %[s9]                                   \n\t"
        "mfhi       %[s8]                                   \n\t"
        "lw         %[s10],     20(%[src0])                 \n\t"
        "lw         %[s11],     -20(%[src1])                \n\t"
        "mult       %[round],   %[one]                      \n\t"
        "srl        %[s9],      31                          \n\t"
        "sll        %[s8],      1                           \n\t"
        "or         %[s8],      %[s9]                       \n\t"
        "madd       %[s10],     %[s11]                      \n\t"
        "sw         %[s8],      16(%[dst])                  \n\t"
        "mflo       %[s11]                                  \n\t"
        "mfhi       %[s10]                                  \n\t"
        "lw         %[s12],     24(%[src0])                 \n\t"
        "lw         %[s13],     -24(%[src1])                \n\t"
        "mult       %[round],   %[one]                      \n\t"
        "srl        %[s11],      31                         \n\t"
        "sll        %[s10],      1                          \n\t"
        "or         %[s10],      %[s11]                     \n\t"
        "madd       %[s12],      %[s13]                     \n\t"
        "sw         %[s10],     20(%[dst])                  \n\t"
        "mflo       %[s13]                                  \n\t"
        "mfhi       %[s12]                                  \n\t"
        "lw         %[s14],     28(%[src0])                 \n\t"
        "lw         %[s15],     -28(%[src1])                \n\t"
        "mult       %[round],   %[one]                      \n\t"
        "srl        %[s13],     31                          \n\t"
        "sll        %[s12],     1                           \n\t"
        "or         %[s12],      %[s13]                     \n\t"
        "madd       %[s14],      %[s15]                     \n\t"
        "sw         %[s12],     24(%[dst])                  \n\t"
        "mflo       %[s15]                                  \n\t"
        "mfhi       %[s14]                                  \n\t"
        "nop                                                \n\t"
        "addiu      %[dst],     32                          \n\t"
        "addiu      %[src0],    32                          \n\t"
        "addiu      %[src1],    -32                         \n\t"
        "srl        %[s15],     31                          \n\t"
        "sll        %[s14],     1                           \n\t"
        "or         %[s14],     %[s15]                      \n\t"
        "bne        %[src0],    %[src0_end],    1b          \n\t"
        " sw        %[s14],     -4(%[dst])                  \n\t"

        ".set       pop                                     \n\t"
        :[src0]"+r"(src0), [src1]"+r"(src1), [dst]"+r"(dst),
         [s0]"=&r"(s0), [s1]"=&r"(s1), [s2]"=&r"(s2), [s3]"=&r"(s3),
         [s4]"=&r"(s4), [s5]"=&r"(s5), [s6]"=&r"(s6), [s7]"=&r"(s7),
         [s8]"=&r"(s8), [s9]"=&r"(s9), [s10]"=&r"(s10), [s11]"=&r"(s11),
         [s12]"=&r"(s12), [s13]"=&r"(s13), [s14]"=&r"(s14), [s15]"=&r"(s15),
         [round]"=&r"(round), [one]"=&r"(one)
        :[src0_end]"r"(src0_end)
        :"memory",  "hi", "lo"
    );
}

static void vector_imul_add_mips(int *dst, const int *src0, const int *src1, const int *src2, int len)
{
    int i;
    int s0, s1, s2;
    int s3, s4, s5;
    int s6, s7, s8;
    int s9, s10, s11;
    int c1 = 0x80000000;

    for(i=0; i<len; i+=16) {
        __asm__ volatile (
            "lw         %[s0],      0(%[src0])          \n\t"
            "lw         %[s1],      0(%[src1])          \n\t"
            "lw         %[s2],      0(%[src2])          \n\t"
            "mthi       $0                              \n\t"
            "sll        %[s0],      %[s0],      1       \n\t"
            "mtlo       %[c1]                           \n\t"
            "madd       %[s0],      %[s1]               \n\t"
            "lw         %[s3],      4(%[src0])          \n\t"
            "lw         %[s4],      4(%[src1])          \n\t"
            "lw         %[s5],      4(%[src2])          \n\t"
            "mfhi       %[s0]                           \n\t"
            "sll        %[s3],      %[s3],      1       \n\t"
            "mthi       $0                              \n\t"
            "mtlo       %[c1]                           \n\t"
            "madd       %[s3],      %[s4]               \n\t"
            "lw         %[s6],      8(%[src0])          \n\t"
            "lw         %[s7],      8(%[src1])          \n\t"
            "addu       %[s0],      %[s0],      %[s2]   \n\t"
            "mfhi       %[s3]                           \n\t"
            "lw         %[s8],      8(%[src2])          \n\t"
            "sw         %[s0],      0(%[dst])           \n\t"
            "mthi       $0                              \n\t"
            "sll        %[s6],      %[s6],      1       \n\t"
            "mtlo       %[c1]                           \n\t"
            "madd       %[s6],      %[s7]               \n\t"
            "addu       %[s3],      %[s3],      %[s5]   \n\t"
            "lw         %[s9],      12(%[src0])         \n\t"
            "sw         %[s3],      4(%[dst])           \n\t"
            "mfhi       %[s6]                           \n\t"
            "lw         %[s10],     12(%[src1])         \n\t"
            "lw         %[s11],     12(%[src2])         \n\t"
            "mthi       $0                              \n\t"
            "sll        %[s9],      %[s9],      1       \n\t"
            "mtlo       %[c1]                           \n\t"
            "madd       %[s9],      %[s10]              \n\t"
            "addu       %[s6],      %[s6],      %[s8]   \n\t"
            "lw         %[s0],      16(%[src0])         \n\t"
            "sw         %[s6],      8(%[dst])           \n\t"
            "mfhi       %[s9]                           \n\t"
            "lw         %[s1],      16(%[src1])         \n\t"
            "lw         %[s2],      16(%[src2])         \n\t"
            "mthi       $0                              \n\t"
            "sll        %[s0],      %[s0],      1       \n\t"
            "mtlo       %[c1]                           \n\t"
            "madd       %[s0],      %[s1]               \n\t"
            "addu       %[s9],      %[s9],      %[s11]  \n\t"
            "lw         %[s3],      20(%[src0])         \n\t"
            "sw         %[s9],      12(%[dst])          \n\t"
            "mfhi       %[s0]                           \n\t"
            "lw         %[s4],      20(%[src1])         \n\t"
            "lw         %[s5],      20(%[src2])         \n\t"
            "mthi       $0                              \n\t"
            "sll        %[s3],      %[s3],      1       \n\t"
            "mtlo       %[c1]                           \n\t"
            "madd       %[s3],      %[s4]               \n\t"
            "addu       %[s0],      %[s0],      %[s2]   \n\t"
            "lw         %[s6],      24(%[src0])         \n\t"
            "sw         %[s0],      16(%[dst])          \n\t"
            "mfhi       %[s3]                           \n\t"
            "lw         %[s7],      24(%[src1])         \n\t"
            "lw         %[s8],      24(%[src2])         \n\t"
            "mthi       $0                              \n\t"
            "sll        %[s6],      %[s6],      1       \n\t"
            "mtlo       %[c1]                           \n\t"
            "madd       %[s6],      %[s7]               \n\t"
            "addu       %[s3],      %[s3],      %[s5]   \n\t"
            "lw         %[s9],      28(%[src0])         \n\t"
            "sw         %[s3],      20(%[dst])          \n\t"
            "mfhi       %[s6]                           \n\t"
            "lw         %[s10],     28(%[src1])         \n\t"
            "lw         %[s11],     28(%[src2])         \n\t"
            "mthi       $0                              \n\t"
            "sll        %[s9],      %[s9],      1       \n\t"
            "mtlo       %[c1]                           \n\t"
            "madd       %[s9],      %[s10]              \n\t"
            "addu       %[s6],      %[s6],      %[s8]   \n\t"
            "lw         %[s0],      32(%[src0])         \n\t"
            "sw         %[s6],      24(%[dst])          \n\t"
            "mfhi       %[s9]                           \n\t"
            "lw         %[s1],      32(%[src1])         \n\t"
            "lw         %[s2],      32(%[src2])         \n\t"
            "lw         %[s3],      36(%[src0])         \n\t"
            "mthi       $0                              \n\t"
            "sll        %[s0],      %[s0],      1       \n\t"
            "mtlo       %[c1]                           \n\t"
            "madd       %[s0],      %[s1]               \n\t"
            "addu       %[s9],      %[s9],      %[s11]  \n\t"
            "lw         %[s4],      36(%[src1])         \n\t"
            "lw         %[s5],      36(%[src2])         \n\t"
            "mfhi       %[s0]                           \n\t"
            "sw         %[s9],      28(%[dst])          \n\t"
            "mthi       $0                              \n\t"
            "sll        %[s3],      %[s3],      1       \n\t"
            "mtlo       %[c1]                           \n\t"
            "madd       %[s3],      %[s4]               \n\t"
            "lw         %[s6],      40(%[src0])         \n\t"
            "lw         %[s7],      40(%[src1])         \n\t"
            "addu       %[s0],      %[s0],      %[s2]   \n\t"
            "mfhi       %[s3]                           \n\t"
            "lw         %[s8],      40(%[src2])         \n\t"
            "sw         %[s0],      32(%[dst])          \n\t"
            "mthi       $0                              \n\t"
            "sll        %[s6],      %[s6],      1       \n\t"
            "mtlo       %[c1]                           \n\t"
            "madd       %[s6],      %[s7]               \n\t"
            "addu       %[s3],      %[s3],      %[s5]   \n\t"
            "lw         %[s9],      44(%[src0])         \n\t"
            "sw         %[s3],      36(%[dst])          \n\t"
            "mfhi       %[s6]                           \n\t"
            "lw         %[s10],     44(%[src1])         \n\t"
            "lw         %[s11],     44(%[src2])         \n\t"
            "mthi       $0                              \n\t"
            "sll        %[s9],      %[s9],      1       \n\t"
            "mtlo       %[c1]                           \n\t"
            "madd       %[s9],      %[s10]              \n\t"
            "addu       %[s6],      %[s6],      %[s8]   \n\t"
            "lw         %[s0],      48(%[src0])         \n\t"
            "sw         %[s6],      40(%[dst])          \n\t"
            "mfhi       %[s9]                           \n\t"
            "lw         %[s1],      48(%[src1])         \n\t"
            "lw         %[s2],      48(%[src2])         \n\t"
            "mthi       $0                              \n\t"
            "sll        %[s0],      %[s0],      1       \n\t"
            "mtlo       %[c1]                           \n\t"
            "madd       %[s0],      %[s1]               \n\t"
            "addu       %[s9],      %[s9],      %[s11]  \n\t"
            "lw         %[s3],      52(%[src0])         \n\t"
            "sw         %[s9],      44(%[dst])          \n\t"
            "mfhi       %[s0]                           \n\t"
            "lw         %[s4],      52(%[src1])         \n\t"
            "lw         %[s5],      52(%[src2])         \n\t"
            "mthi       $0                              \n\t"
            "sll        %[s3],      %[s3],      1       \n\t"
            "mtlo       %[c1]                           \n\t"
            "madd       %[s3],      %[s4]               \n\t"
            "addu       %[s0],      %[s0],      %[s2]   \n\t"
            "lw         %[s6],      56(%[src0])         \n\t"
            "sw         %[s0],      48(%[dst])          \n\t"
            "mfhi       %[s3]                           \n\t"
            "lw         %[s7],      56(%[src1])         \n\t"
            "lw         %[s8],      56(%[src2])         \n\t"
            "mthi       $0                              \n\t"
            "sll        %[s6],      %[s6],      1       \n\t"
            "mtlo       %[c1]                           \n\t"
            "madd       %[s6],      %[s7]               \n\t"
            "addu       %[s3],      %[s3],      %[s5]   \n\t"
            "lw         %[s9],      60(%[src0])         \n\t"
            "sw         %[s3],      52(%[dst])          \n\t"
            "mfhi       %[s6]                           \n\t"
            "lw         %[s10],     60(%[src1])         \n\t"
            "lw         %[s11],     60(%[src2])         \n\t"
            "mthi       $0                              \n\t"
            "sll        %[s9],      %[s9],      1       \n\t"
            "mtlo       %[c1]                           \n\t"
            "madd       %[s9],      %[s10]              \n\t"
            "addu       %[s6],      %[s6],      %[s8]   \n\t"
            "addiu      %[src0],    %[src0],    64      \n\t"
            "sw         %[s6],      56(%[dst])          \n\t"
            "mfhi       %[s9]                           \n\t"
            "addiu      %[src1],    %[src1],    64      \n\t"
            "addiu      %[src2],    %[src2],    64      \n\t"
            "addu       %[s9],      %[s9],      %[s11]  \n\t"
            "sw         %[s9],      60(%[dst])          \n\t"
            "addiu      %[dst],     %[dst],     64      \n\t"

            : [src0] "+r" (src0), [src1] "+r" (src1),
              [src2] "+r" (src2), [dst] "+r" (dst),
              [s0] "=&r" (s0), [s1] "=&r" (s1), [s2] "=&r" (s2),
              [s3] "=&r" (s3), [s4] "=&r" (s4), [s5] "=&r" (s5),
              [s6] "=&r" (s6), [s7] "=&r" (s7), [s8] "=&r" (s8),
              [s9] "=&r" (s9), [s10] "=&r" (s10), [s11] "=&r" (s11)
            : [c1] "r" (c1)
            : "memory", "hi", "lo"
        );
    }
}
#endif /* HAVE_MIPS32R2 */
#endif /* HAVE_MIPSDSPR2 */
#endif /* HAVE_INLINE_ASM */

void ff_fixed_dsp_init_mips(AVFixedDSPContext *fdsp) {
#if HAVE_MIPSDSPR1 || HAVE_MIPSDSPR2 || HAVE_MIPS32R2
    fdsp->vector_fmul_add_fixed     = vector_imul_add_mips;
    fdsp->vector_fmul_reverse_fixed = vector_q31mul_reverse_mips;
    fdsp->vector_fmul_window_fixed  = vector_q31mul_window_mips;
#endif /* HAVE_INLINE_ASM */
}
