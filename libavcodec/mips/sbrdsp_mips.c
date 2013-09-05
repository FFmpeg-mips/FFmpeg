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
 * Authors:  Darko Laus      (darko.laus imgtec com)
 *           Djordje Pesut   (djordje.pesut imgtec com)
 *           Mirjana Vulin   (mirjana.vulin imgtec com)
 *
 * AAC Spectral Band Replication decoding functions optimized for MIPS
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
 * Reference: libavcodec/sbrdsp.c
 */

#include "config.h"
#include "libavcodec/sbrdsp.h"

#if HAVE_INLINE_ASM
static void sbr_neg_odd_64_mips(float *x)
{
    int Temp1, Temp2, Temp3, Temp4, Temp5;
    float *x1    = &x[1];
    float *x_end = x1 + 64;

    /* loop unrolled 4 times */
    __asm__ volatile (
        "lui    %[Temp5],   0x8000                  \n\t"
    "1:                                             \n\t"
        "lw     %[Temp1],   0(%[x1])                \n\t"
        "lw     %[Temp2],   8(%[x1])                \n\t"
        "lw     %[Temp3],   16(%[x1])               \n\t"
        "lw     %[Temp4],   24(%[x1])               \n\t"
        "xor    %[Temp1],   %[Temp1],   %[Temp5]    \n\t"
        "xor    %[Temp2],   %[Temp2],   %[Temp5]    \n\t"
        "xor    %[Temp3],   %[Temp3],   %[Temp5]    \n\t"
        "xor    %[Temp4],   %[Temp4],   %[Temp5]    \n\t"
        "sw     %[Temp1],   0(%[x1])                \n\t"
        "sw     %[Temp2],   8(%[x1])                \n\t"
        "sw     %[Temp3],   16(%[x1])               \n\t"
        "sw     %[Temp4],   24(%[x1])               \n\t"
        "addiu  %[x1],      %[x1],      32          \n\t"
        "bne    %[x1],      %[x_end],   1b          \n\t"

        : [Temp1]"=&r"(Temp1), [Temp2]"=&r"(Temp2),
          [Temp3]"=&r"(Temp3), [Temp4]"=&r"(Temp4),
          [Temp5]"=&r"(Temp5), [x1]"+r"(x1)
        : [x_end]"r"(x_end)
        : "memory"
    );
}

static void sbr_qmf_pre_shuffle_mips(float *z)
{
    int Temp1, Temp2, Temp3, Temp4, Temp5, Temp6;
    float *z1 = &z[66];
    float *z2 = &z[59];
    float *z3 = &z[2];
    float *z4 = z1 + 60;

    /* loop unrolled 5 times */
    __asm__ volatile (
        "lui    %[Temp6],   0x8000                  \n\t"
    "1:                                             \n\t"
        "lw     %[Temp1],   0(%[z2])                \n\t"
        "lw     %[Temp2],   4(%[z2])                \n\t"
        "lw     %[Temp3],   8(%[z2])                \n\t"
        "lw     %[Temp4],   12(%[z2])               \n\t"
        "lw     %[Temp5],   16(%[z2])               \n\t"
        "xor    %[Temp1],   %[Temp1],   %[Temp6]    \n\t"
        "xor    %[Temp2],   %[Temp2],   %[Temp6]    \n\t"
        "xor    %[Temp3],   %[Temp3],   %[Temp6]    \n\t"
        "xor    %[Temp4],   %[Temp4],   %[Temp6]    \n\t"
        "xor    %[Temp5],   %[Temp5],   %[Temp6]    \n\t"
        "addiu  %[z2],      %[z2],      -20         \n\t"
        "sw     %[Temp1],   32(%[z1])               \n\t"
        "sw     %[Temp2],   24(%[z1])               \n\t"
        "sw     %[Temp3],   16(%[z1])               \n\t"
        "sw     %[Temp4],   8(%[z1])                \n\t"
        "sw     %[Temp5],   0(%[z1])                \n\t"
        "lw     %[Temp1],   0(%[z3])                \n\t"
        "lw     %[Temp2],   4(%[z3])                \n\t"
        "lw     %[Temp3],   8(%[z3])                \n\t"
        "lw     %[Temp4],   12(%[z3])               \n\t"
        "lw     %[Temp5],   16(%[z3])               \n\t"
        "sw     %[Temp1],   4(%[z1])                \n\t"
        "sw     %[Temp2],   12(%[z1])               \n\t"
        "sw     %[Temp3],   20(%[z1])               \n\t"
        "sw     %[Temp4],   28(%[z1])               \n\t"
        "sw     %[Temp5],   36(%[z1])               \n\t"
        "addiu  %[z3],      %[z3],      20          \n\t"
        "addiu  %[z1],      %[z1],      40          \n\t"
        "bne    %[z1],      %[z4],      1b          \n\t"
        "lw     %[Temp1],   132(%[z])               \n\t"
        "lw     %[Temp2],   128(%[z])               \n\t"
        "lw     %[Temp3],   0(%[z])                 \n\t"
        "lw     %[Temp4],   4(%[z])                 \n\t"
        "xor    %[Temp1],   %[Temp1],   %[Temp6]    \n\t"
        "sw     %[Temp1],   504(%[z])               \n\t"
        "sw     %[Temp2],   508(%[z])               \n\t"
        "sw     %[Temp3],   256(%[z])               \n\t"
        "sw     %[Temp4],   260(%[z])               \n\t"

        : [Temp1]"=&r"(Temp1), [Temp2]"=&r"(Temp2),
          [Temp3]"=&r"(Temp3), [Temp4]"=&r"(Temp4),
          [Temp5]"=&r"(Temp5), [Temp6]"=&r"(Temp6),
          [z1]"+r"(z1), [z2]"+r"(z2), [z3]"+r"(z3)
        : [z4]"r"(z4), [z]"r"(z)
        : "memory"
    );
}

static void sbr_qmf_post_shuffle_mips(float W[32][2], const float *z)
{
    int Temp1, Temp2, Temp3, Temp4, Temp5;
    float *W_ptr = (float *)W;
    float *z1    = (float *)z;
    float *z2    = (float *)&z[60];
    float *z_end = z1 + 32;

     /* loop unrolled 4 times */
    __asm__ volatile (
        "lui    %[Temp5],   0x8000                  \n\t"
    "1:                                             \n\t"
        "lw     %[Temp1],   0(%[z2])                \n\t"
        "lw     %[Temp2],   4(%[z2])                \n\t"
        "lw     %[Temp3],   8(%[z2])                \n\t"
        "lw     %[Temp4],   12(%[z2])               \n\t"
        "xor    %[Temp1],   %[Temp1],   %[Temp5]    \n\t"
        "xor    %[Temp2],   %[Temp2],   %[Temp5]    \n\t"
        "xor    %[Temp3],   %[Temp3],   %[Temp5]    \n\t"
        "xor    %[Temp4],   %[Temp4],   %[Temp5]    \n\t"
        "addiu  %[z2],      %[z2],      -16         \n\t"
        "sw     %[Temp1],   24(%[W_ptr])            \n\t"
        "sw     %[Temp2],   16(%[W_ptr])            \n\t"
        "sw     %[Temp3],   8(%[W_ptr])             \n\t"
        "sw     %[Temp4],   0(%[W_ptr])             \n\t"
        "lw     %[Temp1],   0(%[z1])                \n\t"
        "lw     %[Temp2],   4(%[z1])                \n\t"
        "lw     %[Temp3],   8(%[z1])                \n\t"
        "lw     %[Temp4],   12(%[z1])               \n\t"
        "sw     %[Temp1],   4(%[W_ptr])             \n\t"
        "sw     %[Temp2],   12(%[W_ptr])            \n\t"
        "sw     %[Temp3],   20(%[W_ptr])            \n\t"
        "sw     %[Temp4],   28(%[W_ptr])            \n\t"
        "addiu  %[z1],      %[z1],      16          \n\t"
        "addiu  %[W_ptr],   %[W_ptr],   32          \n\t"
        "bne    %[z1],      %[z_end],   1b          \n\t"

        : [Temp1]"=&r"(Temp1), [Temp2]"=&r"(Temp2),
          [Temp3]"=&r"(Temp3), [Temp4]"=&r"(Temp4),
          [Temp5]"=&r"(Temp5), [z1]"+r"(z1),
          [z2]"+r"(z2), [W_ptr]"+r"(W_ptr)
        : [z_end]"r"(z_end)
        : "memory"
    );
}

#if HAVE_MIPSFPU
static void sbr_sum64x5_mips(float *z)
{
    int k;
    float *z1;
    float f1, f2, f3, f4, f5, f6, f7, f8;
    for (k = 0; k < 64; k += 8) {

        z1 = &z[k];

         /* loop unrolled 8 times */
        __asm__ volatile (
            "lwc1   $f0,    0(%[z1])        \n\t"
            "lwc1   $f1,    256(%[z1])      \n\t"
            "lwc1   $f2,    4(%[z1])        \n\t"
            "lwc1   $f3,    260(%[z1])      \n\t"
            "lwc1   $f4,    8(%[z1])        \n\t"
            "add.s  %[f1],  $f0,    $f1     \n\t"
            "lwc1   $f5,    264(%[z1])      \n\t"
            "add.s  %[f2],  $f2,    $f3     \n\t"
            "lwc1   $f6,    12(%[z1])       \n\t"
            "lwc1   $f7,    268(%[z1])      \n\t"
            "add.s  %[f3],  $f4,    $f5     \n\t"
            "lwc1   $f8,    16(%[z1])       \n\t"
            "lwc1   $f9,    272(%[z1])      \n\t"
            "add.s  %[f4],  $f6,    $f7     \n\t"
            "lwc1   $f10,   20(%[z1])       \n\t"
            "lwc1   $f11,   276(%[z1])      \n\t"
            "add.s  %[f5],  $f8,    $f9     \n\t"
            "lwc1   $f12,   24(%[z1])       \n\t"
            "lwc1   $f13,   280(%[z1])      \n\t"
            "add.s  %[f6],  $f10,   $f11    \n\t"
            "lwc1   $f14,   28(%[z1])       \n\t"
            "lwc1   $f15,   284(%[z1])      \n\t"
            "add.s  %[f7],  $f12,   $f13    \n\t"
            "lwc1   $f0,    512(%[z1])      \n\t"
            "lwc1   $f1,    516(%[z1])      \n\t"
            "add.s  %[f8],  $f14,   $f15    \n\t"
            "lwc1   $f2,    520(%[z1])      \n\t"
            "add.s  %[f1],  %[f1],  $f0     \n\t"
            "add.s  %[f2],  %[f2],  $f1     \n\t"
            "lwc1   $f3,    524(%[z1])      \n\t"
            "add.s  %[f3],  %[f3],  $f2     \n\t"
            "lwc1   $f4,    528(%[z1])      \n\t"
            "lwc1   $f5,    532(%[z1])      \n\t"
            "add.s  %[f4],  %[f4],  $f3     \n\t"
            "lwc1   $f6,    536(%[z1])      \n\t"
            "add.s  %[f5],  %[f5],  $f4     \n\t"
            "add.s  %[f6],  %[f6],  $f5     \n\t"
            "lwc1   $f7,    540(%[z1])      \n\t"
            "add.s  %[f7],  %[f7],  $f6     \n\t"
            "lwc1   $f0,    768(%[z1])      \n\t"
            "lwc1   $f1,    772(%[z1])      \n\t"
            "add.s  %[f8],  %[f8],  $f7     \n\t"
            "lwc1   $f2,    776(%[z1])      \n\t"
            "add.s  %[f1],  %[f1],  $f0     \n\t"
            "add.s  %[f2],  %[f2],  $f1     \n\t"
            "lwc1   $f3,    780(%[z1])      \n\t"
            "add.s  %[f3],  %[f3],  $f2     \n\t"
            "lwc1   $f4,    784(%[z1])      \n\t"
            "lwc1   $f5,    788(%[z1])      \n\t"
            "add.s  %[f4],  %[f4],  $f3     \n\t"
            "lwc1   $f6,    792(%[z1])      \n\t"
            "add.s  %[f5],  %[f5],  $f4     \n\t"
            "add.s  %[f6],  %[f6],  $f5     \n\t"
            "lwc1   $f7,    796(%[z1])      \n\t"
            "add.s  %[f7],  %[f7],  $f6     \n\t"
            "lwc1   $f0,    1024(%[z1])     \n\t"
            "lwc1   $f1,    1028(%[z1])     \n\t"
            "add.s  %[f8],  %[f8],  $f7     \n\t"
            "lwc1   $f2,    1032(%[z1])     \n\t"
            "add.s  %[f1],  %[f1],  $f0     \n\t"
            "add.s  %[f2],  %[f2],  $f1     \n\t"
            "lwc1   $f3,    1036(%[z1])     \n\t"
            "add.s  %[f3],  %[f3],  $f2     \n\t"
            "lwc1   $f4,    1040(%[z1])     \n\t"
            "lwc1   $f5,    1044(%[z1])     \n\t"
            "add.s  %[f4],  %[f4],  $f3     \n\t"
            "lwc1   $f6,    1048(%[z1])     \n\t"
            "add.s  %[f5],  %[f5],  $f4     \n\t"
            "add.s  %[f6],  %[f6],  $f5     \n\t"
            "lwc1   $f7,    1052(%[z1])     \n\t"
            "add.s  %[f7],  %[f7],  $f6     \n\t"
            "swc1   %[f1],  0(%[z1])        \n\t"
            "swc1   %[f2],  4(%[z1])        \n\t"
            "add.s  %[f8],  %[f8],  $f7     \n\t"
            "swc1   %[f3],  8(%[z1])        \n\t"
            "swc1   %[f4],  12(%[z1])       \n\t"
            "swc1   %[f5],  16(%[z1])       \n\t"
            "swc1   %[f6],  20(%[z1])       \n\t"
            "swc1   %[f7],  24(%[z1])       \n\t"
            "swc1   %[f8],  28(%[z1])       \n\t"

            : [f1]"=&f"(f1), [f2]"=&f"(f2), [f3]"=&f"(f3),
              [f4]"=&f"(f4), [f5]"=&f"(f5), [f6]"=&f"(f6),
              [f7]"=&f"(f7), [f8]"=&f"(f8)
            : [z1]"r"(z1)
            : "$f0", "$f1", "$f2", "$f3", "$f4", "$f5",
              "$f6", "$f7", "$f8", "$f9", "$f10", "$f11",
              "$f12", "$f13", "$f14", "$f15",
              "memory"
        );
    }
}

static float sbr_sum_square_mips(float (*x)[2], int n)
{
    float sum0 = 0.0f, sum1 = 0.0f;
    float *p_x;
    float temp0, temp1, temp2, temp3;
    float *loop_end;
    p_x = &x[0][0];
    loop_end = p_x + (n >> 1)*4 - 4;

    __asm__ volatile (
        ".set      push                                             \n\t"
        ".set      noreorder                                        \n\t"
        "lwc1      %[temp0],   0(%[p_x])                            \n\t"
        "lwc1      %[temp1],   4(%[p_x])                            \n\t"
        "lwc1      %[temp2],   8(%[p_x])                            \n\t"
        "lwc1      %[temp3],   12(%[p_x])                           \n\t"
    "1:                                                             \n\t"
        "addiu     %[p_x],     %[p_x],       16                     \n\t"
        "madd.s    %[sum0],    %[sum0],      %[temp0],   %[temp0]   \n\t"
        "lwc1      %[temp0],   0(%[p_x])                            \n\t"
        "madd.s    %[sum1],    %[sum1],      %[temp1],   %[temp1]   \n\t"
        "lwc1      %[temp1],   4(%[p_x])                            \n\t"
        "madd.s    %[sum0],    %[sum0],      %[temp2],   %[temp2]   \n\t"
        "lwc1      %[temp2],   8(%[p_x])                            \n\t"
        "madd.s    %[sum1],    %[sum1],      %[temp3],   %[temp3]   \n\t"
        "bne       %[p_x],     %[loop_end],  1b                     \n\t"
        " lwc1     %[temp3],   12(%[p_x])                           \n\t"
        "madd.s    %[sum0],    %[sum0],      %[temp0],   %[temp0]   \n\t"
        "madd.s    %[sum1],    %[sum1],      %[temp1],   %[temp1]   \n\t"
        "madd.s    %[sum0],    %[sum0],      %[temp2],   %[temp2]   \n\t"
        "madd.s    %[sum1],    %[sum1],      %[temp3],   %[temp3]   \n\t"
        ".set      pop                                              \n\t"

        : [temp0]"=&f"(temp0), [temp1]"=&f"(temp1), [temp2]"=&f"(temp2),
          [temp3]"=&f"(temp3), [sum0]"+f"(sum0), [sum1]"+f"(sum1),
          [p_x]"+r"(p_x)
        : [loop_end]"r"(loop_end)
        : "memory"
    );
    return sum0 + sum1;
}

static void sbr_qmf_deint_bfly_mips(float *v, const float *src0, const float *src1)
{
    int i;
    float temp0, temp1, temp2, temp3, temp4, temp5;
    float temp6, temp7, temp8, temp9, temp10, temp11;
    float *v0 = v;
    float *v1 = &v[127];
    float *psrc0 = (float*)src0;
    float *psrc1 = (float*)&src1[63];

    for (i = 0; i < 4; i++) {

         /* loop unrolled 16 times */
        __asm__ volatile(
            "lwc1       %[temp0],   0(%[src0])             \n\t"
            "lwc1       %[temp1],   0(%[src1])             \n\t"
            "lwc1       %[temp3],   4(%[src0])             \n\t"
            "lwc1       %[temp4],   -4(%[src1])            \n\t"
            "lwc1       %[temp6],   8(%[src0])             \n\t"
            "lwc1       %[temp7],   -8(%[src1])            \n\t"
            "lwc1       %[temp9],   12(%[src0])            \n\t"
            "lwc1       %[temp10],  -12(%[src1])           \n\t"
            "add.s      %[temp2],   %[temp0],   %[temp1]   \n\t"
            "add.s      %[temp5],   %[temp3],   %[temp4]   \n\t"
            "add.s      %[temp8],   %[temp6],   %[temp7]   \n\t"
            "add.s      %[temp11],  %[temp9],   %[temp10]  \n\t"
            "sub.s      %[temp0],   %[temp0],   %[temp1]   \n\t"
            "sub.s      %[temp3],   %[temp3],   %[temp4]   \n\t"
            "sub.s      %[temp6],   %[temp6],   %[temp7]   \n\t"
            "sub.s      %[temp9],   %[temp9],   %[temp10]  \n\t"
            "swc1       %[temp2],   0(%[v1])               \n\t"
            "swc1       %[temp0],   0(%[v0])               \n\t"
            "swc1       %[temp5],   -4(%[v1])              \n\t"
            "swc1       %[temp3],   4(%[v0])               \n\t"
            "swc1       %[temp8],   -8(%[v1])              \n\t"
            "swc1       %[temp6],   8(%[v0])               \n\t"
            "swc1       %[temp11],  -12(%[v1])             \n\t"
            "swc1       %[temp9],   12(%[v0])              \n\t"
            "lwc1       %[temp0],   16(%[src0])            \n\t"
            "lwc1       %[temp1],   -16(%[src1])           \n\t"
            "lwc1       %[temp3],   20(%[src0])            \n\t"
            "lwc1       %[temp4],   -20(%[src1])           \n\t"
            "lwc1       %[temp6],   24(%[src0])            \n\t"
            "lwc1       %[temp7],   -24(%[src1])           \n\t"
            "lwc1       %[temp9],   28(%[src0])            \n\t"
            "lwc1       %[temp10],  -28(%[src1])           \n\t"
            "add.s      %[temp2],   %[temp0],   %[temp1]   \n\t"
            "add.s      %[temp5],   %[temp3],   %[temp4]   \n\t"
            "add.s      %[temp8],   %[temp6],   %[temp7]   \n\t"
            "add.s      %[temp11],  %[temp9],   %[temp10]  \n\t"
            "sub.s      %[temp0],   %[temp0],   %[temp1]   \n\t"
            "sub.s      %[temp3],   %[temp3],   %[temp4]   \n\t"
            "sub.s      %[temp6],   %[temp6],   %[temp7]   \n\t"
            "sub.s      %[temp9],   %[temp9],   %[temp10]  \n\t"
            "swc1       %[temp2],   -16(%[v1])             \n\t"
            "swc1       %[temp0],   16(%[v0])              \n\t"
            "swc1       %[temp5],   -20(%[v1])             \n\t"
            "swc1       %[temp3],   20(%[v0])              \n\t"
            "swc1       %[temp8],   -24(%[v1])             \n\t"
            "swc1       %[temp6],   24(%[v0])              \n\t"
            "swc1       %[temp11],  -28(%[v1])             \n\t"
            "swc1       %[temp9],   28(%[v0])              \n\t"
            "lwc1       %[temp0],   32(%[src0])            \n\t"
            "lwc1       %[temp1],   -32(%[src1])           \n\t"
            "lwc1       %[temp3],   36(%[src0])            \n\t"
            "lwc1       %[temp4],   -36(%[src1])           \n\t"
            "lwc1       %[temp6],   40(%[src0])            \n\t"
            "lwc1       %[temp7],   -40(%[src1])           \n\t"
            "lwc1       %[temp9],   44(%[src0])            \n\t"
            "lwc1       %[temp10],  -44(%[src1])           \n\t"
            "add.s      %[temp2],   %[temp0],   %[temp1]   \n\t"
            "add.s      %[temp5],   %[temp3],   %[temp4]   \n\t"
            "add.s      %[temp8],   %[temp6],   %[temp7]   \n\t"
            "add.s      %[temp11],  %[temp9],   %[temp10]  \n\t"
            "sub.s      %[temp0],   %[temp0],   %[temp1]   \n\t"
            "sub.s      %[temp3],   %[temp3],   %[temp4]   \n\t"
            "sub.s      %[temp6],   %[temp6],   %[temp7]   \n\t"
            "sub.s      %[temp9],   %[temp9],   %[temp10]  \n\t"
            "swc1       %[temp2],   -32(%[v1])             \n\t"
            "swc1       %[temp0],   32(%[v0])              \n\t"
            "swc1       %[temp5],   -36(%[v1])             \n\t"
            "swc1       %[temp3],   36(%[v0])              \n\t"
            "swc1       %[temp8],   -40(%[v1])             \n\t"
            "swc1       %[temp6],   40(%[v0])              \n\t"
            "swc1       %[temp11],  -44(%[v1])             \n\t"
            "swc1       %[temp9],   44(%[v0])              \n\t"
            "lwc1       %[temp0],   48(%[src0])            \n\t"
            "lwc1       %[temp1],   -48(%[src1])           \n\t"
            "lwc1       %[temp3],   52(%[src0])            \n\t"
            "lwc1       %[temp4],   -52(%[src1])           \n\t"
            "lwc1       %[temp6],   56(%[src0])            \n\t"
            "lwc1       %[temp7],   -56(%[src1])           \n\t"
            "lwc1       %[temp9],   60(%[src0])            \n\t"
            "lwc1       %[temp10],  -60(%[src1])           \n\t"
            "add.s      %[temp2],   %[temp0],   %[temp1]   \n\t"
            "add.s      %[temp5],   %[temp3],   %[temp4]   \n\t"
            "add.s      %[temp8],   %[temp6],   %[temp7]   \n\t"
            "add.s      %[temp11],  %[temp9],   %[temp10]  \n\t"
            "sub.s      %[temp0],   %[temp0],   %[temp1]   \n\t"
            "sub.s      %[temp3],   %[temp3],   %[temp4]   \n\t"
            "sub.s      %[temp6],   %[temp6],   %[temp7]   \n\t"
            "sub.s      %[temp9],   %[temp9],   %[temp10]  \n\t"
            "swc1       %[temp2],   -48(%[v1])             \n\t"
            "swc1       %[temp0],   48(%[v0])              \n\t"
            "swc1       %[temp5],   -52(%[v1])             \n\t"
            "swc1       %[temp3],   52(%[v0])              \n\t"
            "swc1       %[temp8],   -56(%[v1])             \n\t"
            "swc1       %[temp6],   56(%[v0])              \n\t"
            "swc1       %[temp11],  -60(%[v1])             \n\t"
            "swc1       %[temp9],   60(%[v0])              \n\t"
            "addiu      %[src0],    %[src0],    64         \n\t"
            "addiu      %[src1],    %[src1],    -64        \n\t"
            "addiu      %[v0],      %[v0],      64         \n\t"
            "addiu      %[v1],      %[v1],      -64        \n\t"

            : [v0]"+r"(v0), [v1]"+r"(v1), [src0]"+r"(psrc0), [src1]"+r"(psrc1),
              [temp0]"=&f"(temp0), [temp1]"=&f"(temp1), [temp2]"=&f"(temp2),
              [temp3]"=&f"(temp3), [temp4]"=&f"(temp4), [temp5]"=&f"(temp5),
              [temp6]"=&f"(temp6), [temp7]"=&f"(temp7), [temp8]"=&f"(temp8),
              [temp9]"=&f"(temp9), [temp10]"=&f"(temp10), [temp11]"=&f"(temp11)
            :
            :"memory"
        );
    }
}

static void sbr_autocorrelate_mips(const float x[40][2], float phi[3][2][2])
{
    int i;
    float real_sum_0 = 0.0f;
    float real_sum_1 = 0.0f;
    float real_sum_2 = 0.0f;
    float imag_sum_1 = 0.0f;
    float imag_sum_2 = 0.0f;
    float *p_x, *p_phi;
    float temp0, temp1, temp2, temp3, temp4, temp5, temp6;
    float temp7, temp_r, temp_r1, temp_r2, temp_r3, temp_r4;
    p_x = (float*)&x[0][0];
    p_phi = &phi[0][0][0];

    __asm__ volatile (
        "lwc1    %[temp0],      8(%[p_x])                           \n\t"
        "lwc1    %[temp1],      12(%[p_x])                          \n\t"
        "lwc1    %[temp2],      16(%[p_x])                          \n\t"
        "lwc1    %[temp3],      20(%[p_x])                          \n\t"
        "lwc1    %[temp4],      24(%[p_x])                          \n\t"
        "lwc1    %[temp5],      28(%[p_x])                          \n\t"
        "mul.s   %[temp_r],     %[temp1],      %[temp1]             \n\t"
        "mul.s   %[temp_r1],    %[temp1],      %[temp3]             \n\t"
        "mul.s   %[temp_r2],    %[temp1],      %[temp2]             \n\t"
        "mul.s   %[temp_r3],    %[temp1],      %[temp5]             \n\t"
        "mul.s   %[temp_r4],    %[temp1],      %[temp4]             \n\t"
        "madd.s  %[temp_r],     %[temp_r],     %[temp0],  %[temp0]  \n\t"
        "madd.s  %[temp_r1],    %[temp_r1],    %[temp0],  %[temp2]  \n\t"
        "msub.s  %[temp_r2],    %[temp_r2],    %[temp0],  %[temp3]  \n\t"
        "madd.s  %[temp_r3],    %[temp_r3],    %[temp0],  %[temp4]  \n\t"
        "msub.s  %[temp_r4],    %[temp_r4],    %[temp0],  %[temp5]  \n\t"
        "add.s   %[real_sum_0], %[real_sum_0], %[temp_r]            \n\t"
        "add.s   %[real_sum_1], %[real_sum_1], %[temp_r1]           \n\t"
        "add.s   %[imag_sum_1], %[imag_sum_1], %[temp_r2]           \n\t"
        "add.s   %[real_sum_2], %[real_sum_2], %[temp_r3]           \n\t"
        "add.s   %[imag_sum_2], %[imag_sum_2], %[temp_r4]           \n\t"
        "addiu   %[p_x],        %[p_x],        8                    \n\t"

        : [temp0]"=&f"(temp0), [temp1]"=&f"(temp1), [temp2]"=&f"(temp2),
          [temp3]"=&f"(temp3), [temp4]"=&f"(temp4), [temp5]"=&f"(temp5),
          [real_sum_0]"+f"(real_sum_0), [real_sum_1]"+f"(real_sum_1),
          [imag_sum_1]"+f"(imag_sum_1), [real_sum_2]"+f"(real_sum_2),
          [temp_r]"=&f"(temp_r), [temp_r1]"=&f"(temp_r1), [temp_r2]"=&f"(temp_r2),
          [temp_r3]"=&f"(temp_r3), [temp_r4]"=&f"(temp_r4),
          [p_x]"+r"(p_x), [imag_sum_2]"+f"(imag_sum_2)
        :
        : "memory"
    );

    for (i = 0; i < 12; i++) {
        __asm__ volatile (
            "lwc1    %[temp0],      8(%[p_x])                           \n\t"
            "lwc1    %[temp1],      12(%[p_x])                          \n\t"
            "lwc1    %[temp2],      16(%[p_x])                          \n\t"
            "lwc1    %[temp3],      20(%[p_x])                          \n\t"
            "lwc1    %[temp4],      24(%[p_x])                          \n\t"
            "lwc1    %[temp5],      28(%[p_x])                          \n\t"
            "mul.s   %[temp_r],     %[temp1],      %[temp1]             \n\t"
            "mul.s   %[temp_r1],    %[temp1],      %[temp3]             \n\t"
            "mul.s   %[temp_r2],    %[temp1],      %[temp2]             \n\t"
            "mul.s   %[temp_r3],    %[temp1],      %[temp5]             \n\t"
            "mul.s   %[temp_r4],    %[temp1],      %[temp4]             \n\t"
            "madd.s  %[temp_r],     %[temp_r],     %[temp0],  %[temp0]  \n\t"
            "madd.s  %[temp_r1],    %[temp_r1],    %[temp0],  %[temp2]  \n\t"
            "msub.s  %[temp_r2],    %[temp_r2],    %[temp0],  %[temp3]  \n\t"
            "madd.s  %[temp_r3],    %[temp_r3],    %[temp0],  %[temp4]  \n\t"
            "msub.s  %[temp_r4],    %[temp_r4],    %[temp0],  %[temp5]  \n\t"
            "add.s   %[real_sum_0], %[real_sum_0], %[temp_r]            \n\t"
            "add.s   %[real_sum_1], %[real_sum_1], %[temp_r1]           \n\t"
            "add.s   %[imag_sum_1], %[imag_sum_1], %[temp_r2]           \n\t"
            "add.s   %[real_sum_2], %[real_sum_2], %[temp_r3]           \n\t"
            "add.s   %[imag_sum_2], %[imag_sum_2], %[temp_r4]           \n\t"
            "lwc1    %[temp0],      32(%[p_x])                          \n\t"
            "lwc1    %[temp1],      36(%[p_x])                          \n\t"
            "mul.s   %[temp_r],     %[temp3],      %[temp3]             \n\t"
            "mul.s   %[temp_r1],    %[temp3],      %[temp5]             \n\t"
            "mul.s   %[temp_r2],    %[temp3],      %[temp4]             \n\t"
            "mul.s   %[temp_r3],    %[temp3],      %[temp1]             \n\t"
            "mul.s   %[temp_r4],    %[temp3],      %[temp0]             \n\t"
            "madd.s  %[temp_r],     %[temp_r],     %[temp2],  %[temp2]  \n\t"
            "madd.s  %[temp_r1],    %[temp_r1],    %[temp2],  %[temp4]  \n\t"
            "msub.s  %[temp_r2],    %[temp_r2],    %[temp2],  %[temp5]  \n\t"
            "madd.s  %[temp_r3],    %[temp_r3],    %[temp2],  %[temp0]  \n\t"
            "msub.s  %[temp_r4],    %[temp_r4],    %[temp2],  %[temp1]  \n\t"
            "add.s   %[real_sum_0], %[real_sum_0], %[temp_r]            \n\t"
            "add.s   %[real_sum_1], %[real_sum_1], %[temp_r1]           \n\t"
            "add.s   %[imag_sum_1], %[imag_sum_1], %[temp_r2]           \n\t"
            "add.s   %[real_sum_2], %[real_sum_2], %[temp_r3]           \n\t"
            "add.s   %[imag_sum_2], %[imag_sum_2], %[temp_r4]           \n\t"
            "lwc1    %[temp2],      40(%[p_x])                          \n\t"
            "lwc1    %[temp3],      44(%[p_x])                          \n\t"
            "mul.s   %[temp_r],     %[temp5],      %[temp5]             \n\t"
            "mul.s   %[temp_r1],    %[temp5],      %[temp1]             \n\t"
            "mul.s   %[temp_r2],    %[temp5],      %[temp0]             \n\t"
            "mul.s   %[temp_r3],    %[temp5],      %[temp3]             \n\t"
            "mul.s   %[temp_r4],    %[temp5],      %[temp2]             \n\t"
            "madd.s  %[temp_r],     %[temp_r],     %[temp4],  %[temp4]  \n\t"
            "madd.s  %[temp_r1],    %[temp_r1],    %[temp4],  %[temp0]  \n\t"
            "msub.s  %[temp_r2],    %[temp_r2],    %[temp4],  %[temp1]  \n\t"
            "madd.s  %[temp_r3],    %[temp_r3],    %[temp4],  %[temp2]  \n\t"
            "msub.s  %[temp_r4],    %[temp_r4],    %[temp4],  %[temp3]  \n\t"
            "add.s   %[real_sum_0], %[real_sum_0], %[temp_r]            \n\t"
            "add.s   %[real_sum_1], %[real_sum_1], %[temp_r1]           \n\t"
            "add.s   %[imag_sum_1], %[imag_sum_1], %[temp_r2]           \n\t"
            "add.s   %[real_sum_2], %[real_sum_2], %[temp_r3]           \n\t"
            "add.s   %[imag_sum_2], %[imag_sum_2], %[temp_r4]           \n\t"
            "addiu   %[p_x],        %[p_x],        24                   \n\t"

            : [temp0]"=&f"(temp0), [temp1]"=&f"(temp1), [temp2]"=&f"(temp2),
              [temp3]"=&f"(temp3), [temp4]"=&f"(temp4), [temp5]"=&f"(temp5),
              [real_sum_0]"+f"(real_sum_0), [real_sum_1]"+f"(real_sum_1),
              [imag_sum_1]"+f"(imag_sum_1), [real_sum_2]"+f"(real_sum_2),
              [temp_r]"=&f"(temp_r), [temp_r1]"=&f"(temp_r1),
              [temp_r2]"=&f"(temp_r2), [temp_r3]"=&f"(temp_r3),
              [temp_r4]"=&f"(temp_r4), [p_x]"+r"(p_x),
              [imag_sum_2]"+f"(imag_sum_2)
            :
            : "memory"
        );
    }
    __asm__ volatile (
        "lwc1    %[temp0],    -296(%[p_x])                        \n\t"
        "lwc1    %[temp1],    -292(%[p_x])                        \n\t"
        "lwc1    %[temp2],    8(%[p_x])                           \n\t"
        "lwc1    %[temp3],    12(%[p_x])                          \n\t"
        "lwc1    %[temp4],    -288(%[p_x])                        \n\t"
        "lwc1    %[temp5],    -284(%[p_x])                        \n\t"
        "lwc1    %[temp6],    -280(%[p_x])                        \n\t"
        "lwc1    %[temp7],    -276(%[p_x])                        \n\t"
        "madd.s  %[temp_r],   %[real_sum_0], %[temp0],  %[temp0]  \n\t"
        "madd.s  %[temp_r1],  %[real_sum_0], %[temp2],  %[temp2]  \n\t"
        "madd.s  %[temp_r2],  %[real_sum_1], %[temp0],  %[temp4]  \n\t"
        "madd.s  %[temp_r3],  %[imag_sum_1], %[temp0],  %[temp5]  \n\t"
        "madd.s  %[temp_r],   %[temp_r],     %[temp1],  %[temp1]  \n\t"
        "madd.s  %[temp_r1],  %[temp_r1],    %[temp3],  %[temp3]  \n\t"
        "madd.s  %[temp_r2],  %[temp_r2],    %[temp1],  %[temp5]  \n\t"
        "nmsub.s  %[temp_r3], %[temp_r3],    %[temp1],  %[temp4]  \n\t"
        "lwc1    %[temp4],    16(%[p_x])                          \n\t"
        "lwc1    %[temp5],    20(%[p_x])                          \n\t"
        "swc1    %[temp_r],   40(%[p_phi])                        \n\t"
        "swc1    %[temp_r1],  16(%[p_phi])                        \n\t"
        "swc1    %[temp_r2],  24(%[p_phi])                        \n\t"
        "swc1    %[temp_r3],  28(%[p_phi])                        \n\t"
        "madd.s  %[temp_r],   %[real_sum_1], %[temp2],  %[temp4]  \n\t"
        "madd.s  %[temp_r1],  %[imag_sum_1], %[temp2],  %[temp5]  \n\t"
        "madd.s  %[temp_r2],  %[real_sum_2], %[temp0],  %[temp6]  \n\t"
        "madd.s  %[temp_r3],  %[imag_sum_2], %[temp0],  %[temp7]  \n\t"
        "madd.s  %[temp_r],   %[temp_r],     %[temp3],  %[temp5]  \n\t"
        "nmsub.s %[temp_r1],  %[temp_r1],    %[temp3],  %[temp4]  \n\t"
        "madd.s  %[temp_r2],  %[temp_r2],    %[temp1],  %[temp7]  \n\t"
        "nmsub.s %[temp_r3],  %[temp_r3],    %[temp1],  %[temp6]  \n\t"
        "swc1    %[temp_r],   0(%[p_phi])                         \n\t"
        "swc1    %[temp_r1],  4(%[p_phi])                         \n\t"
        "swc1    %[temp_r2],  8(%[p_phi])                         \n\t"
        "swc1    %[temp_r3],  12(%[p_phi])                        \n\t"

        : [temp0]"=&f"(temp0), [temp1]"=&f"(temp1), [temp2]"=&f"(temp2),
          [temp3]"=&f"(temp3), [temp4]"=&f"(temp4), [temp5]"=&f"(temp5),
          [temp6]"=&f"(temp6), [temp7]"=&f"(temp7), [temp_r]"=&f"(temp_r),
          [real_sum_0]"+f"(real_sum_0), [real_sum_1]"+f"(real_sum_1),
          [real_sum_2]"+f"(real_sum_2), [imag_sum_1]"+f"(imag_sum_1),
          [temp_r2]"=&f"(temp_r2), [temp_r3]"=&f"(temp_r3),
          [temp_r1]"=&f"(temp_r1), [p_phi]"+r"(p_phi),
          [imag_sum_2]"+f"(imag_sum_2)
        : [p_x]"r"(p_x)
        : "memory"
    );
}

static void sbr_hf_gen_mips(float (*X_high)[2], const float (*X_low)[2],
                         const float alpha0[2], const float alpha1[2],
                         float bw, int start, int end)
{
    float alpha[4];
    int i;
    float *p_x_low = (float*)&X_low[0][0] + 2*start;
    float *p_x_high = &X_high[0][0] + 2*start;
    float temp0, temp1, temp2, temp3, temp4, temp5, temp6;
    float temp7, temp8, temp9, temp10, temp11, temp12;

    alpha[0] = alpha1[0] * bw * bw;
    alpha[1] = alpha1[1] * bw * bw;
    alpha[2] = alpha0[0] * bw;
    alpha[3] = alpha0[1] * bw;

    for (i = start; i < end; i++) {
        __asm__ volatile (
            "lwc1    %[temp0],    -16(%[p_x_low])                        \n\t"
            "lwc1    %[temp1],    -12(%[p_x_low])                        \n\t"
            "lwc1    %[temp2],    -8(%[p_x_low])                         \n\t"
            "lwc1    %[temp3],    -4(%[p_x_low])                         \n\t"
            "lwc1    %[temp5],    0(%[p_x_low])                          \n\t"
            "lwc1    %[temp6],    4(%[p_x_low])                          \n\t"
            "lwc1    %[temp7],    0(%[alpha])                            \n\t"
            "lwc1    %[temp8],    4(%[alpha])                            \n\t"
            "lwc1    %[temp9],    8(%[alpha])                            \n\t"
            "lwc1    %[temp10],   12(%[alpha])                           \n\t"
            "addiu   %[p_x_high], %[p_x_high],     8                     \n\t"
            "addiu   %[p_x_low],  %[p_x_low],      8                     \n\t"
            "mul.s   %[temp11],   %[temp1],        %[temp8]              \n\t"
            "msub.s  %[temp11],   %[temp11],       %[temp0],  %[temp7]   \n\t"
            "madd.s  %[temp11],   %[temp11],       %[temp2],  %[temp9]   \n\t"
            "nmsub.s %[temp11],   %[temp11],       %[temp3],  %[temp10]  \n\t"
            "add.s   %[temp11],   %[temp11],       %[temp5]              \n\t"
            "swc1    %[temp11],   -8(%[p_x_high])                        \n\t"
            "mul.s   %[temp12],   %[temp1],        %[temp7]              \n\t"
            "madd.s  %[temp12],   %[temp12],       %[temp0],  %[temp8]   \n\t"
            "madd.s  %[temp12],   %[temp12],       %[temp3],  %[temp9]   \n\t"
            "madd.s  %[temp12],   %[temp12],       %[temp2],  %[temp10]  \n\t"
            "add.s   %[temp12],   %[temp12],       %[temp6]              \n\t"
            "swc1    %[temp12],   -4(%[p_x_high])                        \n\t"

            : [temp0]"=&f"(temp0), [temp1]"=&f"(temp1), [temp2]"=&f"(temp2),
              [temp3]"=&f"(temp3), [temp4]"=&f"(temp4), [temp5]"=&f"(temp5),
              [temp6]"=&f"(temp6), [temp7]"=&f"(temp7), [temp8]"=&f"(temp8),
              [temp9]"=&f"(temp9), [temp10]"=&f"(temp10), [temp11]"=&f"(temp11),
              [temp12]"=&f"(temp12), [p_x_high]"+r"(p_x_high),
              [p_x_low]"+r"(p_x_low)
            : [alpha]"r"(alpha)
            : "memory"
        );
    }
}

static void sbr_hf_g_filt_mips(float (*Y)[2], const float (*X_high)[40][2],
                            const float *g_filt, int m_max, intptr_t ixh)
{
    float *p_y, *p_x, *p_g;
    float temp0, temp1, temp2;
    int loop_end;

    p_g = (float*)&g_filt[0];
    p_y = &Y[0][0];
    p_x = (float*)&X_high[0][ixh][0];
    loop_end = (int)((int*)p_g + m_max);

    __asm__ volatile(
        ".set    push                                \n\t"
        ".set    noreorder                           \n\t"
    "1:                                              \n\t"
        "lwc1    %[temp0],   0(%[p_g])               \n\t"
        "lwc1    %[temp1],   0(%[p_x])               \n\t"
        "lwc1    %[temp2],   4(%[p_x])               \n\t"
        "mul.s   %[temp1],   %[temp1],     %[temp0]  \n\t"
        "mul.s   %[temp2],   %[temp2],     %[temp0]  \n\t"
        "addiu   %[p_g],     %[p_g],       4         \n\t"
        "addiu   %[p_x],     %[p_x],       320       \n\t"
        "swc1    %[temp1],   0(%[p_y])               \n\t"
        "swc1    %[temp2],   4(%[p_y])               \n\t"
        "bne     %[p_g],     %[loop_end],  1b        \n\t"
        " addiu  %[p_y],     %[p_y],       8         \n\t"
        ".set    pop                                 \n\t"

        : [temp0]"=&f"(temp0), [temp1]"=&f"(temp1),
          [temp2]"=&f"(temp2), [p_x]"+r"(p_x),
          [p_y]"+r"(p_y), [p_g]"+r"(p_g)
        : [loop_end]"r"(loop_end)
        : "memory"
    );
}

static void sbr_hf_apply_noise_0_mips(float (*Y)[2], const float *s_m,
                                 const float *q_filt, int noise,
                                 int kx, int m_max)
{
    int m;

    for (m = 0; m < m_max; m++){

        float *Y1=&Y[m][0];
        float *ff_table;
        float y0,y1, temp1, temp2, temp4, temp5;
        int temp0, temp3;
        const float *s_m1=&s_m[m];
        const float *q_filt1= &q_filt[m];

        __asm__ volatile(
            "lwc1    %[y0],       0(%[Y1])                                    \n\t"
            "lwc1    %[temp1],    0(%[s_m1])                                  \n\t"
            "addiu   %[noise],    %[noise],              1                    \n\t"
            "andi    %[noise],    %[noise],              0x1ff                \n\t"
            "sll     %[temp0],    %[noise], 3                                 \n\t"
            "addu    %[ff_table], %[ff_sbr_noise_table], %[temp0]             \n\t"
            "add.s   %[y0],       %[y0],                 %[temp1]             \n\t"
            "mfc1    %[temp3],    %[temp1]                                    \n\t"
            "bne     %[temp3],    $0,                    1f                   \n\t"
            "lwc1    %[y1],       4(%[Y1])                                    \n\t"
            "lwc1    %[temp2],    0(%[q_filt1])                               \n\t"
            "lwc1    %[temp4],    0(%[ff_table])                              \n\t"
            "lwc1    %[temp5],    4(%[ff_table])                              \n\t"
            "madd.s  %[y0],       %[y0],                 %[temp2],  %[temp4]  \n\t"
            "madd.s  %[y1],       %[y1],                 %[temp2],  %[temp5]  \n\t"
            "swc1    %[y1],       4(%[Y1])                                    \n\t"
        "1:                                                                   \n\t"
            "swc1    %[y0],       0(%[Y1])                                    \n\t"

            : [ff_table]"=&r"(ff_table), [y0]"=&f"(y0), [y1]"=&f"(y1),
              [temp0]"=&r"(temp0), [temp1]"=&f"(temp1), [temp2]"=&f"(temp2),
              [temp3]"=&r"(temp3), [temp4]"=&f"(temp4), [temp5]"=&f"(temp5)
            : [ff_sbr_noise_table]"r"(ff_sbr_noise_table), [noise]"r"(noise),
              [Y1]"r"(Y1), [s_m1]"r"(s_m1), [q_filt1]"r"(q_filt1)
            : "memory"
        );
    }
}

static void sbr_hf_apply_noise_1_mips(float (*Y)[2], const float *s_m,
                                 const float *q_filt, int noise,
                                 int kx, int m_max)
{
    float y0,y1,temp1, temp2, temp4, temp5;
    int temp0, temp3, m;
    float phi_sign = 1 - 2 * (kx & 1);

    for (m = 0; m < m_max; m++) {

        float *ff_table;
        float *Y1=&Y[m][0];
        const float *s_m1=&s_m[m];
        const float *q_filt1= &q_filt[m];

        __asm__ volatile(
            "lwc1   %[y1],       4(%[Y1])                                     \n\t"
            "lwc1   %[temp1],    0(%[s_m1])                                   \n\t"
            "lw     %[temp3],    0(%[s_m1])                                   \n\t"
            "addiu  %[noise],    %[noise],               1                    \n\t"
            "andi   %[noise],    %[noise],               0x1ff                \n\t"
            "sll    %[temp0],    %[noise],               3                    \n\t"
            "addu   %[ff_table], %[ff_sbr_noise_table], %[temp0]              \n\t"
            "madd.s %[y1],       %[y1],                 %[temp1], %[phi_sign] \n\t"
            "bne    %[temp3],    $0,                    1f                    \n\t"
            "lwc1   %[y0],       0(%[Y1])                                     \n\t"
            "lwc1   %[temp2],    0(%[q_filt1])                                \n\t"
            "lwc1   %[temp4],    0(%[ff_table])                               \n\t"
            "lwc1   %[temp5],    4(%[ff_table])                               \n\t"
            "madd.s %[y0],       %[y0],                 %[temp2], %[temp4]    \n\t"
            "madd.s %[y1],       %[y1],                 %[temp2], %[temp5]    \n\t"
            "swc1   %[y0],       0(%[Y1])                                     \n\t"
        "1:                                                                   \n\t"
            "swc1   %[y1],       4(%[Y1])                                     \n\t"

            : [ff_table] "=&r" (ff_table), [y0] "=&f" (y0), [y1] "=&f" (y1),
              [temp0] "=&r" (temp0), [temp1] "=&f" (temp1), [temp2] "=&f" (temp2),
              [temp3] "=&r" (temp3), [temp4] "=&f" (temp4), [temp5] "=&f" (temp5)
            : [ff_sbr_noise_table] "r" (ff_sbr_noise_table), [noise] "r" (noise),
              [Y1] "r" (Y1), [s_m1] "r" (s_m1), [q_filt1] "r" (q_filt1),
              [phi_sign] "f" (phi_sign)
            : "memory"
        );
        phi_sign = -phi_sign;
    }
}

static void sbr_hf_apply_noise_2_mips(float (*Y)[2], const float *s_m,
                                 const float *q_filt, int noise,
                                 int kx, int m_max)
{
    int m;
    float *ff_table;
    float y0,y1, temp0, temp1, temp2, temp3, temp4, temp5;

    for (m = 0; m < m_max; m++) {

        float *Y1=&Y[m][0];
        const float *s_m1=&s_m[m];
        const float *q_filt1= &q_filt[m];

        __asm__ volatile(
            "lwc1   %[y0],       0(%[Y1])                                  \n\t"
            "lwc1   %[temp1],    0(%[s_m1])                                \n\t"
            "addiu  %[noise],    %[noise],              1                  \n\t"
            "andi   %[noise],    %[noise],              0x1ff              \n\t"
            "sll    %[temp0],    %[noise],              3                  \n\t"
            "addu   %[ff_table], %[ff_sbr_noise_table], %[temp0]           \n\t"
            "sub.s  %[y0],       %[y0],                 %[temp1]           \n\t"
            "mfc1   %[temp3],    %[temp1]                                  \n\t"
            "bne    %[temp3],    $0,                    1f                 \n\t"
            "lwc1   %[y1],       4(%[Y1])                                  \n\t"
            "lwc1   %[temp2],    0(%[q_filt1])                             \n\t"
            "lwc1   %[temp4],    0(%[ff_table])                            \n\t"
            "lwc1   %[temp5],    4(%[ff_table])                            \n\t"
            "madd.s %[y0],       %[y0],                 %[temp2], %[temp4] \n\t"
            "madd.s %[y1],       %[y1],                 %[temp2], %[temp5] \n\t"
            "swc1   %[y1],       4(%[Y1])                                  \n\t"
        "1:                                                                \n\t"
            "swc1   %[y0],       0(%[Y1])                                  \n\t"

            : [temp0]"=&r"(temp0), [ff_table]"=&r"(ff_table), [y0]"=&f"(y0),
              [y1]"=&f"(y1), [temp1]"=&f"(temp1), [temp2]"=&f"(temp2),
              [temp3]"=&r"(temp3), [temp4]"=&f"(temp4), [temp5]"=&f"(temp5)
            : [ff_sbr_noise_table]"r"(ff_sbr_noise_table), [noise]"r"(noise),
              [Y1]"r"(Y1), [s_m1]"r"(s_m1), [q_filt1]"r"(q_filt1)
            : "memory"
        );
    }
}

static void sbr_hf_apply_noise_3_mips(float (*Y)[2], const float *s_m,
                                 const float *q_filt, int noise,
                                 int kx, int m_max)
{
    float phi_sign = 1 - 2 * (kx & 1);
    int m;

    for (m = 0; m < m_max; m++) {

        float *Y1=&Y[m][0];
        float *ff_table;
        float y0,y1, temp1, temp2, temp4, temp5;
        int temp0, temp3;
        const float *s_m1=&s_m[m];
        const float *q_filt1= &q_filt[m];

        __asm__ volatile(
            "lwc1    %[y1],       4(%[Y1])                                     \n\t"
            "lwc1    %[temp1],    0(%[s_m1])                                   \n\t"
            "addiu   %[noise],    %[noise],              1                     \n\t"
            "andi    %[noise],    %[noise],              0x1ff                 \n\t"
            "sll     %[temp0],    %[noise],              3                     \n\t"
            "addu    %[ff_table], %[ff_sbr_noise_table], %[temp0]              \n\t"
            "nmsub.s %[y1],       %[y1],                 %[temp1], %[phi_sign] \n\t"
            "mfc1    %[temp3],    %[temp1]                                     \n\t"
            "bne     %[temp3],    $0,                    1f                    \n\t"
            "lwc1    %[y0],       0(%[Y1])                                     \n\t"
            "lwc1    %[temp2],    0(%[q_filt1])                                \n\t"
            "lwc1    %[temp4],    0(%[ff_table])                               \n\t"
            "lwc1    %[temp5],    4(%[ff_table])                               \n\t"
            "madd.s  %[y0],       %[y0],                 %[temp2], %[temp4]    \n\t"
            "madd.s  %[y1],       %[y1],                 %[temp2], %[temp5]    \n\t"
            "swc1    %[y0],       0(%[Y1])                                     \n\t"
            "1:                                                                \n\t"
            "swc1    %[y1],       4(%[Y1])                                     \n\t"

            : [ff_table]"=&r"(ff_table), [y0]"=&f"(y0), [y1]"=&f"(y1),
              [temp0]"=&r"(temp0), [temp1]"=&f"(temp1), [temp2]"=&f"(temp2),
              [temp3]"=&r"(temp3), [temp4]"=&f"(temp4), [temp5]"=&f"(temp5)
            : [ff_sbr_noise_table]"r"(ff_sbr_noise_table), [noise]"r"(noise),
              [Y1]"r"(Y1), [s_m1]"r"(s_m1), [q_filt1]"r"(q_filt1),
              [phi_sign]"f"(phi_sign)
            : "memory"
        );
       phi_sign = -phi_sign;
    }
}
#endif /* HAVE_MIPSFPU */

#if HAVE_MIPSDSPR1 || HAVE_MIPSDSPR2
static av_always_inline void autocorrelate_q31_0(const int x[40][2], aac_float_t phi[3][2][2])
{
    int mant, expo, mant1, expo1;
    unsigned int real_sum_hi, real_sum_lo;

    int x1, x2, x3, x4;
    int *x_ptr, *x_end;

    __asm__ volatile (
        ".set       push                                    \n\t"
        ".set       noreorder                               \n\t"
        "addiu      %[x_ptr],       %[x],           16      \n\t"   // x_ptr = &x[2][0]
        "addiu      %[x_end],       %[x],           304     \n\t"   // x_end = &x[38][0]
        "lw         %[x1],          8(%[x])                 \n\t"
        "lw         %[x2],          12(%[x])                \n\t"
        "lw         %[x3],          16(%[x])                \n\t"
        "lw         %[x4],          20(%[x])                \n\t"
        "mult       %[x1],          %[x1]                   \n\t"
        "madd       %[x2],          %[x2]                   \n\t"
        /* loop unrolled 4 times */
        "1:                                                 \n\t"   // for (i = 2; i < 38; i++)
        "lw         %[x1],          8(%[x_ptr])             \n\t"
        "lw         %[x2],          12(%[x_ptr])            \n\t"
        "madd       %[x3],          %[x3]                   \n\t"
        "madd       %[x4],          %[x4]                   \n\t"
        "madd       %[x1],          %[x1]                   \n\t"
        "madd       %[x2],          %[x2]                   \n\t"
        "lw         %[x3],          16(%[x_ptr])            \n\t"
        "lw         %[x4],          20(%[x_ptr])            \n\t"
        "lw         %[x1],          24(%[x_ptr])            \n\t"
        "lw         %[x2],          28(%[x_ptr])            \n\t"
        "madd       %[x3],          %[x3]                   \n\t"
        "madd       %[x4],          %[x4]                   \n\t"
        "madd       %[x1],          %[x1]                   \n\t"
        "madd       %[x2],          %[x2]                   \n\t"
        "addiu      %[x_ptr],       32                      \n\t"
        "lw         %[x3],          0(%[x_ptr])             \n\t"
        "bne        %[x_ptr],       %[x_end],       1b      \n\t"
        " lw        %[x4],          4(%[x_ptr])             \n\t"
        "mfhi       %[real_sum_hi]                          \n\t"
        "mflo       %[real_sum_lo]                          \n\t"
        "madd       %[x3],          %[x3]                   \n\t"   // accu_re += (int64_t)x[38][0] * x[38][0];
        "madd       %[x4],          %[x4]                   \n\t"   // accu_re += (int64_t)x[38][1] * x[38][1];
        "lw         %[x1],          0(%[x])                 \n\t"
        "lw         %[x2],          4(%[x])                 \n\t"
        "mfhi       %[mant]                                 \n\t"   // accu_re >> 32
        "mthi       %[real_sum_hi], $ac1                    \n\t"
        "mtlo       %[real_sum_lo], $ac1                    \n\t"
        "madd       $ac1,           %[x1],          %[x1]   \n\t"   // accu_re += (int64_t)x[0][0] * x[0][0];
        "madd       $ac1,           %[x2],          %[x2]   \n\t"   // accu_re += (int64_t)x[0][1] * x[0][1];
        "absq_s.w   %[mant],        %[mant]                 \n\t"
        "mfhi       %[mant1],       $ac1                    \n\t"
        "li         %[x1],          33                      \n\t"
        "clz        %[expo],        %[mant]                 \n\t"
        "subu       %[expo],        %[x1],          %[expo] \n\t"
        "extrv_r.w  %[mant],        $ac0,           %[expo] \n\t"   // mant = (int)((accu_re+round) >> nz)
        "absq_s.w   %[mant1],       %[mant1]                \n\t"
        "clz        %[expo1],       %[mant1]                \n\t"
        "subu       %[expo1],       %[x1],          %[expo1]\n\t"
        "extrv_r.w  %[mant1],       $ac1,           %[expo1]\n\t"   // mant = (int)((accu_re+round) >> nz)
        "shra_r.w   %[mant],        %[mant],        7       \n\t"
        "sll        %[mant],        6                       \n\t"
        "addiu      %[expo],        15                      \n\t"
        "shra_r.w   %[mant1],       %[mant1],       7       \n\t"
        "sll        %[mant1],       6                       \n\t"
        "addiu      %[expo1],       15                      \n\t"
        ".set   pop                                         \n\t"
        :[x_ptr]"=&r"(x_ptr), [x_end]"=&r"(x_end),
         [x1]"=&r"(x1), [x2]"=&r"(x2), [x3]"=&r"(x3), [x4]"=&r"(x4),
         [real_sum_lo]"=&r"(real_sum_lo), [real_sum_hi]"=&r"(real_sum_hi),
         [mant]"=&r"(mant), [expo]"=&r"(expo), [mant1]"=&r"(mant1), [expo1]"=&r"(expo1)
        :[x]"r"(x), [phi]"r"(phi)
        :"memory", "hi", "lo", "$ac1hi", "$ac1lo"
    );

    phi[1][0][0] = int2float(mant, expo);
    phi[2][1][0] = int2float(mant1, expo1);

}

static av_always_inline void autocorrelate_q31_12(const int x[40][2], aac_float_t phi[3][2][2])
{
    int mant, expo, mant1, expo1;
    int x1, x2, x3, x4, x5, x6, x7, x8;
    int *x_ptr, *x_end;
    int real_sum_hi, real_sum_lo, imag_sum_hi, imag_sum_lo, real_sum_hi2, real_sum_lo2, imag_sum_hi2, imag_sum_lo2;

    __asm__ volatile (
        /* Instructions are scheduled to minimize pipeline stall.
           Symbolic register names reused for different values to
           minimized number of used registers. */
        ".set       push                                            \n\t"
        ".set       noreorder                                       \n\t"

        "addiu      %[x_ptr],       %[x],           16              \n\t"   // x_ptr = &x[2][0]
        "addiu      %[x_end],       %[x],           304             \n\t"   // x_end = &x[38][0]
        "lw         %[x7],          0(%[x])                         \n\t"   // x[0][0]
        "lw         %[x8],          4(%[x])                         \n\t"   // x[0][1]
        "lw         %[x5],          8(%[x])                         \n\t"
        "lw         %[x6],          12(%[x])                        \n\t"
        "lw         %[x1],          16(%[x])                        \n\t"
        "lw         %[x2],          20(%[x])                        \n\t"
        "lw         %[x3],          24(%[x])                        \n\t"
        "lw         %[x4],          28(%[x])                        \n\t"
        "mult       $ac0,           %[x5],          %[x1]           \n\t"   // $ac0 = accu_re1
        "madd       $ac0,           %[x6],          %[x2]           \n\t"
        "mult       $ac1,           %[x5],          %[x2]           \n\t"   // $ac1 = accu_im1
        "msub       $ac1,           %[x6],          %[x1]           \n\t"
        "mult       $ac2,           %[x5],          %[x3]           \n\t"   // $ac2 = accu_re2
        "madd       $ac2,           %[x6],          %[x4]           \n\t"
        "mult       $ac3,           %[x5],          %[x4]           \n\t"   // $ac3 = accu_im2
        "msub       $ac3,           %[x6],          %[x3]           \n\t"

        "madd       $ac2,           %[x7],          %[x1]           \n\t"   // accu_re2 += (int64_t)x[ 0][0] * x[2][0];
        "madd       $ac2,           %[x8],          %[x2]           \n\t"
        "madd       $ac3,           %[x7],          %[x2]           \n\t"   // accu_im2 += (int64_t)x[ 0][0] * x[2][1];
        "msub       $ac3,           %[x8],          %[x1]           \n\t"

        /* loop unrolled 4 times */
        "1:                                                         \n\t"   // for (i = 2; i < 38; i++)
        "lw         %[x5],          16(%[x_ptr])                    \n\t"
        "lw         %[x6],          20(%[x_ptr])                    \n\t"
        "madd       $ac0,           %[x1],          %[x3]           \n\t"
        "madd       $ac0,           %[x2],          %[x4]           \n\t"
        "madd       $ac1,           %[x1],          %[x4]           \n\t"
        "msub       $ac1,           %[x2],          %[x3]           \n\t"
        "madd       $ac2,           %[x1],          %[x5]           \n\t"
        "madd       $ac2,           %[x2],          %[x6]           \n\t"
        "madd       $ac3,           %[x1],          %[x6]           \n\t"
        "msub       $ac3,           %[x2],          %[x5]           \n\t"
        "lw         %[x7],          24(%[x_ptr])                    \n\t"
        "lw         %[x8],          28(%[x_ptr])                    \n\t"
        "madd       $ac0,           %[x3],          %[x5]           \n\t"
        "madd       $ac0,           %[x4],          %[x6]           \n\t"
        "madd       $ac1,           %[x3],          %[x6]           \n\t"
        "msub       $ac1,           %[x4],          %[x5]           \n\t"
        "madd       $ac2,           %[x3],          %[x7]           \n\t"
        "madd       $ac2,           %[x4],          %[x8]           \n\t"
        "madd       $ac3,           %[x3],          %[x8]           \n\t"
        "msub       $ac3,           %[x4],          %[x7]           \n\t"
        "lw         %[x1],          32(%[x_ptr])                    \n\t"
        "lw         %[x2],          36(%[x_ptr])                    \n\t"
        "madd       $ac0,           %[x5],          %[x7]           \n\t"
        "madd       $ac0,           %[x6],          %[x8]           \n\t"
        "madd       $ac1,           %[x5],          %[x8]           \n\t"
        "msub       $ac1,           %[x6],          %[x7]           \n\t"
        "madd       $ac2,           %[x5],          %[x1]           \n\t"
        "madd       $ac2,           %[x6],          %[x2]           \n\t"
        "madd       $ac3,           %[x5],          %[x2]           \n\t"
        "msub       $ac3,           %[x6],          %[x1]           \n\t"
        "lw         %[x3],          40(%[x_ptr])                    \n\t"
        "lw         %[x4],          44(%[x_ptr])                    \n\t"
        "addiu      %[x_ptr],       32                              \n\t"
        "madd       $ac0,           %[x7],          %[x1]           \n\t"
        "madd       $ac0,           %[x8],          %[x2]           \n\t"
        "madd       $ac1,           %[x7],          %[x2]           \n\t"
        "msub       $ac1,           %[x8],          %[x1]           \n\t"
        "madd       $ac2,           %[x7],          %[x3]           \n\t"
        "madd       $ac2,           %[x8],          %[x4]           \n\t"
        "madd       $ac3,           %[x7],          %[x4]           \n\t"
        "bne        %[x_ptr],       %[x_end],       1b              \n\t"
        " msub      $ac3,           %[x8],          %[x3]           \n\t"

        "mflo       %[real_sum_lo], $ac0                            \n\t"
        "mfhi       %[real_sum_hi], $ac0                            \n\t"
        "mflo       %[imag_sum_lo], $ac1                            \n\t"
        "mfhi       %[imag_sum_hi], $ac1                            \n\t"
        "madd       $ac0,           %[x1],          %[x3]           \n\t"   // accu_re1 += (int64_t)x[38][0] * x[39][0];
        "madd       $ac0,           %[x2],          %[x4]           \n\t"   // accu_re1 += (int64_t)x[38][1] * x[39][1];
        "madd       $ac1,           %[x1],          %[x4]           \n\t"   // accu_im1 += (int64_t)x[38][0] * x[39][1];
        "msub       $ac1,           %[x2],          %[x3]           \n\t"   // accu_im1 -= (int64_t)x[38][1] * x[39][0];
        "mfhi       %[real_sum_hi2],$ac2                            \n\t"
        "mfhi       %[imag_sum_hi2],$ac3                            \n\t"
        "lw         %[x2],          0(%[x])                         \n\t"   // x[0][0]
        "lw         %[x3],          4(%[x])                         \n\t"   // x[0][1]
        "mfhi       %[mant],        $ac0                            \n\t"
        "lw         %[x4],          8(%[x])                         \n\t"   // x[1][0]
        "mfhi       %[mant1],       $ac1                            \n\t"
        "lw         %[x5],          12(%[x])                        \n\t"   // x[1][1]
        "absq_s.w   %[mant],        %[mant]                         \n\t"
        "li         %[x1],          33                              \n\t"
        "absq_s.w   %[mant1],       %[mant1]                        \n\t"
        "clz        %[expo],        %[mant]                         \n\t"
        "subu       %[expo],        %[x1],          %[expo]         \n\t"
        "clz        %[expo1],       %[mant1]                        \n\t"
        "extrv_r.w  %[mant],        $ac0,           %[expo]         \n\t"   // mant = (int)((accu_re1+round) >> nz)
        "subu       %[expo1],       %[x1],          %[expo1]        \n\t"
        "mtlo       %[real_sum_lo], $ac0                            \n\t"
        "extrv_r.w  %[mant1],       $ac1,           %[expo1]        \n\t"   // mant = (int)((accu_im1+round) >> nz)
        "mthi       %[real_sum_hi], $ac0                            \n\t"
        "mtlo       %[imag_sum_lo], $ac1                            \n\t"
        "mthi       %[imag_sum_hi], $ac1                            \n\t"
        "shra_r.w   %[mant],        %[mant],        7               \n\t"
        "absq_s.w   %[real_sum_hi2],%[real_sum_hi2]                 \n\t"   // real_sum_hi is used as mant
        "sll        %[mant],        6                               \n\t"
        "addiu      %[expo],        15                              \n\t"
        "shra_r.w   %[mant1],       %[mant1],       7               \n\t"
        "absq_s.w   %[imag_sum_hi2],%[imag_sum_hi2]                 \n\t"   // imag_sum_hi is used as mant
        "sll        %[mant1],       6                               \n\t"
        "addiu      %[expo1],       15                              \n\t"
        "madd       $ac0,           %[x2],          %[x4]           \n\t"   // accu_re1 += (long long)x[ 0][0] * x[1][0];
        "madd       $ac0,           %[x3],          %[x5]           \n\t"
        "madd       $ac1,           %[x2],          %[x5]           \n\t"   // accu_im1 += (long long)x[ 0][0] * x[1][1];
        "msub       $ac1,           %[x3],          %[x4]           \n\t"
        "clz        %[real_sum_lo2],%[real_sum_hi2]                 \n\t"   // real_sum_lo is used as expo
        "clz        %[imag_sum_lo2],%[imag_sum_hi2]                 \n\t"   // imag_sum_lo is used as expo
        "mflo       %[real_sum_lo], $ac0                            \n\t"
        "mfhi       %[real_sum_hi], $ac0                            \n\t"
        "mflo       %[imag_sum_lo], $ac1                            \n\t"
        "mfhi       %[imag_sum_hi], $ac1                            \n\t"
        "subu       %[real_sum_lo2],%[x1],          %[real_sum_lo2] \n\t"
        "subu       %[imag_sum_lo2],%[x1],          %[imag_sum_lo2] \n\t"
        "extrv_r.w  %[real_sum_hi2],$ac2,           %[real_sum_lo2] \n\t"   // mant = (int)((accu_re1+round) >> nz)
        "absq_s.w   %[real_sum_hi], %[real_sum_hi]                  \n\t"   // real_sum_hi is used as mant
        "extrv_r.w  %[imag_sum_hi2],$ac3,           %[imag_sum_lo2] \n\t"   // mant = (int)((accu_im1+round) >> nz)
        "absq_s.w   %[imag_sum_hi], %[imag_sum_hi]                  \n\t"   // imag_sum_hi is used as mant
        "clz        %[real_sum_lo], %[real_sum_hi]                  \n\t"   // real_sum_lo is used as expo
        "subu       %[real_sum_lo], %[x1],          %[real_sum_lo]  \n\t"
        "clz        %[imag_sum_lo], %[imag_sum_hi]                  \n\t"   // imag_sum_lo is used as expo
        "extrv_r.w  %[real_sum_hi], $ac0,           %[real_sum_lo]  \n\t"   // mant = (int)((accu_re1+round) >> nz)
        "subu       %[imag_sum_lo], %[x1],          %[imag_sum_lo]  \n\t"
        "shra_r.w   %[real_sum_hi2],%[real_sum_hi2],7               \n\t"
        "extrv_r.w  %[imag_sum_hi], $ac1,           %[imag_sum_lo]  \n\t"   // mant = (int)((accu_im1+round) >> nz)
        "shra_r.w   %[imag_sum_hi2],%[imag_sum_hi2],7               \n\t"
        "sll        %[real_sum_hi2],6                               \n\t"
        "sll        %[imag_sum_hi2],6                               \n\t"
        "shra_r.w   %[real_sum_hi], %[real_sum_hi], 7               \n\t"
        "addiu      %[real_sum_lo2],15                              \n\t"
        "sll        %[real_sum_hi], 6                               \n\t"
        "addiu      %[real_sum_lo], 15                              \n\t"
        "shra_r.w   %[imag_sum_hi], %[imag_sum_hi], 7               \n\t"
        "addiu      %[imag_sum_lo2],15                              \n\t"
        "sll        %[imag_sum_hi], 6                               \n\t"
        "addiu      %[imag_sum_lo], 15                              \n\t"

        ".set       pop                                             \n\t"
        :[x_ptr]"=&r"(x_ptr), [x_end]"=&r"(x_end),
         [x1]"=&r"(x1), [x2]"=&r"(x2), [x3]"=&r"(x3), [x4]"=&r"(x4),
         [x5]"=&r"(x5), [x6]"=&r"(x6), [x7]"=&r"(x7), [x8]"=&r"(x8),
         [real_sum_lo]"=&r"(real_sum_lo), [real_sum_hi]"=&r"(real_sum_hi),
         [imag_sum_lo]"=&r"(imag_sum_lo), [imag_sum_hi]"=&r"(imag_sum_hi),
         [real_sum_lo2]"=&r"(real_sum_lo2), [real_sum_hi2]"=&r"(real_sum_hi2),
         [imag_sum_lo2]"=&r"(imag_sum_lo2), [imag_sum_hi2]"=&r"(imag_sum_hi2),
         [mant]"=&r"(mant), [expo]"=&r"(expo), [mant1]"=&r"(mant1), [expo1]"=&r"(expo1)
        :[x]"r"(x)
        :"memory", "hi", "lo", "$ac1hi", "$ac1lo", "$ac2hi", "$ac2lo", "$ac3hi", "$ac3lo"
    );

    phi[0][0][0] = int2float(mant, expo);
    phi[0][0][1] = int2float(mant1, expo1);
    phi[1][1][0] = int2float(real_sum_hi, real_sum_lo);
    phi[1][1][1] = int2float(imag_sum_hi, imag_sum_lo);
    phi[0][1][0] = int2float(real_sum_hi2, real_sum_lo2);
    phi[0][1][1] = int2float(imag_sum_hi2, imag_sum_lo2);
}

static void sbr_autocorrelate_q31_mips(const int x[40][2], aac_float_t phi[3][2][2])
{
    autocorrelate_q31_0(x, phi);
    autocorrelate_q31_12(x, phi);
}

static void sbr_hf_g_filt_int_mips(int (*Y)[2], const int (*X_high)[40][2],
                            const aac_float_t *g_filt, int m_max, intptr_t ixh)
{
    int m, r, c23 = 23, c1 = 1;
    long long accu, accu1;

    int *g_filt_ptr, *ixh_ptr, *Y_ptr;
    int b1, b2, m1, m2, tm, tm1, tm2;

    g_filt_ptr = (int *)g_filt;
    ixh_ptr = (int *)&X_high[0][ixh][0];
    Y_ptr = (int *)&Y[0][0];

    for (m = 0; m < m_max; m++) {
        __asm__ volatile (
            "lw             %[b1],      0(%[g_filt_ptr])        \n\t"
            "lw             %[tm],      4(%[g_filt_ptr])        \n\t"
            "lw             %[m2],      0(%[ixh_ptr])           \n\t"
            "lw             %[b2],      4(%[ixh_ptr])           \n\t"
            "shra_r.w       %[m1],      %[b1],          7       \n\t"
            "subu           %[tm],      %[c23],         %[tm]   \n\t"

            : [m1] "=&r" (m1), [b1] "=&r" (b1), [b2] "=&r" (b2),
              [m2] "=&r" (m2), [tm] "=&r" (tm)
            : [g_filt_ptr] "r" (g_filt_ptr), [ixh_ptr] "r" (ixh_ptr),
              [c23] "r" (c23)
            : "memory"
        );
        if (tm < 32) {
            __asm__ volatile (
                "mult           $ac0,           %[m1],          %[m2]   \n\t"
                "mult           $ac1,           %[m1],          %[b2]   \n\t"
                "extrv_r.w      %[tm1],         $ac0,           %[tm]   \n\t"
                "extrv_r.w      %[tm2],         $ac1,           %[tm]   \n\t"
                "addiu          %[g_filt_ptr],  %[g_filt_ptr],  8       \n\t"
                "addiu          %[ixh_ptr],     %[ixh_ptr],     320     \n\t"
                "sw             %[tm1],         0(%[Y_ptr])             \n\t"
                "sw             %[tm2],         4(%[Y_ptr])             \n\t"
                "addiu          %[Y_ptr],       %[Y_ptr],       8       \n\t"

                : [tm1] "=&r" (tm1), [tm2] "=&r" (tm2), [Y_ptr] "+r" (Y_ptr),
                  [g_filt_ptr] "+r" (g_filt_ptr), [ixh_ptr] "+r" (ixh_ptr)
                : [m1] "r" (m1), [b1] "r" (b1), [b2] "r" (b2),
                  [m2] "r" (m2), [tm] "r" (tm)
                : "memory", "hi", "lo", "$ac1hi", "$ac1lo"
            );
        }
        else {
            __asm__ volatile (
                "subu           %[r],      %[tm],           %[c1]   \n\t"
                "sllv           %[r],      %[c1],           %[r]    \n\t"

                : [r] "=&r" (r)
                : [tm] "r" (tm), [c1] "r" (c1)
            );
            accu = (long long)m2 * m1;
            accu1 = (long long)b2 * m1;
            g_filt_ptr+=2;
            ixh_ptr+=80;
            Y_ptr+=2;
            Y[m][0] = (int)((accu + r) >> tm);
            Y[m][1] = (int)((accu1 + r) >> tm);
        }
    }
}
static void sbr_qmf_deint_bfly_int_mips(int *v, const int *src0, const int *src1)
{
    int i, b1, b2, m1, m2, tm1, tm2;
    int b3, b4, m3, m4, tm3, tm4;
    int *v1 = v + 127;
    src1 = src1 + 63;

    for (i = 0; i < 64; i+=4) {
        __asm__ volatile (
            "lw             %[b1],      0(%[src0])              \n\t"
            "lw             %[b3],      4(%[src0])              \n\t"
            "lw             %[b2],      0(%[src1])              \n\t"
            "lw             %[b4],      -4(%[src1])             \n\t"
            "subu           %[m1],      %[b1],          %[b2]   \n\t"
            "addu           %[m2],      %[b1],          %[b2]   \n\t"
            "subu           %[m3],      %[b3],          %[b4]   \n\t"
            "addu           %[m4],      %[b3],          %[b4]   \n\t"
            "shra_r.w       %[tm1],     %[m1],          5       \n\t"
            "shra_r.w       %[tm2],     %[m2],          5       \n\t"
            "shra_r.w       %[tm3],     %[m3],          5       \n\t"
            "shra_r.w       %[tm4],     %[m4],          5       \n\t"
            "lw             %[b1],      8(%[src0])              \n\t"
            "lw             %[b3],      12(%[src0])             \n\t"
            "lw             %[b2],      -8(%[src1])             \n\t"
            "lw             %[b4],      -12(%[src1])            \n\t"
            "sw             %[tm1],     0(%[v])                 \n\t"
            "sw             %[tm2],     0(%[v1])                \n\t"
            "sw             %[tm3],     4(%[v])                 \n\t"
            "sw             %[tm4],     -4(%[v1])               \n\t"
            "subu           %[m1],      %[b1],          %[b2]   \n\t"
            "addu           %[m2],      %[b1],          %[b2]   \n\t"
            "subu           %[m3],      %[b3],          %[b4]   \n\t"
            "addu           %[m4],      %[b3],          %[b4]   \n\t"
            "shra_r.w       %[tm1],     %[m1],          5       \n\t"
            "shra_r.w       %[tm2],     %[m2],          5       \n\t"
            "shra_r.w       %[tm3],     %[m3],          5       \n\t"
            "shra_r.w       %[tm4],     %[m4],          5       \n\t"
            "addiu          %[src0],    %[src0],        16      \n\t"
            "addiu          %[src1],    %[src1],        -16     \n\t"
            "sw             %[tm1],     8(%[v])                 \n\t"
            "sw             %[tm2],     -8(%[v1])               \n\t"
            "sw             %[tm3],     12(%[v])                \n\t"
            "sw             %[tm4],     -12(%[v1])              \n\t"
            "addiu          %[v],       %[v],           16      \n\t"
            "addiu          %[v1],      %[v1],          -16     \n\t"

            : [m1] "=&r" (m1), [b1] "=&r" (b1), [src0] "+r" (src0),
              [m2] "=&r" (m2), [b2] "=&r" (b2), [src1] "+r" (src1),
              [m3] "=r" (m3), [b3] "=&r" (b3),
              [m4] "=r" (m4), [b4] "=&r" (b4),
              [tm1] "=&r" (tm1), [tm2] "=&r" (tm2), [v] "+r" (v), [v1] "+r" (v1),
              [tm3] "=&r" (tm3), [tm4] "=&r" (tm4)
            :
            : "memory"
        );
    }
}

static void sbr_hf_gen_int_mips(int (*X_high)[2], const int (*X_low)[2],
                             const int alpha0[2], const int alpha1[2],
                             int bw, int start, int end)
{
    int i;
    int a0, a1, a2, a3;
    int b1, b2, m1, m2, v1, v2;
    int bw1, bw2, bw3, bw4;
    int *Xh_ptr, *Xl_ptr;
    int c2 = 0x20000000;

    __asm__ volatile (
        "lw             %[m1],          0(%[alpha0])            \n\t"
        "mult           $ac2,           %[bw],          %[bw]   \n\t"
        "extr_r.w       %[bw1],         $ac2,           31      \n\t"
        "lw             %[m2],          4(%[alpha0])            \n\t"
        "lw             %[b1],          0(%[alpha1])            \n\t"
        "lw             %[b2],          4(%[alpha1])            \n\t"
        "mult           $ac0,           %[m1],          %[bw]   \n\t"
        "mult           $ac1,           %[m2],          %[bw]   \n\t"
        "mult           $ac2,           %[b1],          %[bw1]  \n\t"
        "mult           $ac3,           %[b2],          %[bw1]  \n\t"
        "extr_r.w       %[a2],          $ac0,           31      \n\t"
        "extr_r.w       %[a3],          $ac1,           31      \n\t"
        "extr_r.w       %[a0],          $ac2,           31      \n\t"
        "extr_r.w       %[a1],          $ac3,           31      \n\t"

        : [m1] "=&r" (m1), [m2] "=&r" (m2),
          [b1] "=&r" (b1), [b2] "=&r" (b2),
          [a0] "=r" (a0), [a1] "=r" (a1),
          [a2] "=r" (a2), [a3] "=r" (a3), [bw1] "=&r" (bw1)
        : [alpha0] "r" (alpha0), [alpha1] "r" (alpha1), [bw] "r" (bw)
        : "memory", "hi", "lo", "$ac1hi", "$ac1lo",
          "$ac2hi", "$ac2lo", "$ac3hi", "$ac3lo"
    );

    Xh_ptr = (int *)&X_high[start][0];
    Xl_ptr = (int *)&X_low[start][0];

    for (i = start; i < end; i+=2) {
        __asm__ volatile (
            "lw             %[v1],          0(%[Xl_ptr])            \n\t"
            "lw             %[m1],          -16(%[Xl_ptr])          \n\t"
            "lw             %[m2],          -12(%[Xl_ptr])          \n\t"
            "lw             %[b1],          -8(%[Xl_ptr])           \n\t"
            "lw             %[b2],          -4(%[Xl_ptr])           \n\t"
            "lw             %[v2],          4(%[Xl_ptr])            \n\t"
            "mult           $ac0,           %[v1],          %[c2]   \n\t"
            "madd           $ac0,           %[m1],          %[a0]   \n\t"
            "msub           $ac0,           %[m2],          %[a1]   \n\t"
            "madd           $ac0,           %[b1],          %[a2]   \n\t"
            "msub           $ac0,           %[b2],          %[a3]   \n\t"
            "mult           $ac1,           %[v2],          %[c2]   \n\t"
            "madd           $ac1,           %[m2],          %[a0]   \n\t"
            "madd           $ac1,           %[m1],          %[a1]   \n\t"
            "madd           $ac1,           %[b2],          %[a2]   \n\t"
            "madd           $ac1,           %[b1],          %[a3]   \n\t"
            "lw             %[m1],          8(%[Xl_ptr])            \n\t"
            "lw             %[m2],          12(%[Xl_ptr])           \n\t"
            "mult           $ac2,           %[m1],          %[c2]   \n\t"
            "madd           $ac2,           %[b1],          %[a0]   \n\t"
            "msub           $ac2,           %[b2],          %[a1]   \n\t"
            "madd           $ac2,           %[v1],          %[a2]   \n\t"
            "msub           $ac2,           %[v2],          %[a3]   \n\t"
            "mult           $ac3,           %[m2],          %[c2]   \n\t"
            "madd           $ac3,           %[b2],          %[a0]   \n\t"
            "madd           $ac3,           %[b1],          %[a1]   \n\t"
            "madd           $ac3,           %[v2],          %[a2]   \n\t"
            "madd           $ac3,           %[v1],          %[a3]   \n\t"
            "extr_r.w       %[bw1],         $ac0,           29      \n\t"
            "extr_r.w       %[bw2],         $ac1,           29      \n\t"
            "extr_r.w       %[bw3],         $ac2,           29      \n\t"
            "extr_r.w       %[bw4],         $ac3,           29      \n\t"
            "addiu          %[Xl_ptr],      %[Xl_ptr],      16      \n\t"
            "sw             %[bw1],         0(%[Xh_ptr])            \n\t"
            "sw             %[bw2],         4(%[Xh_ptr])            \n\t"
            "sw             %[bw3],         8(%[Xh_ptr])            \n\t"
            "sw             %[bw4],         12(%[Xh_ptr])           \n\t"
            "addiu          %[Xh_ptr],      %[Xh_ptr],      16      \n\t"

            : [m1] "=&r" (m1), [m2] "=&r" (m2), [v1] "=&r" (v1), [v2] "=&r" (v2),
              [b1] "=&r" (b1), [b2] "=&r" (b2), [bw1] "=&r" (bw1),
              [bw2] "=&r" (bw2), [bw3] "=&r" (bw3), [bw4] "=&r" (bw4)
            : [Xl_ptr] "r" (Xl_ptr), [Xh_ptr] "r" (Xh_ptr),
              [a0] "r" (a0), [a1] "r" (a1),
              [a2] "r" (a2), [a3] "r" (a3), [c2] "r" (c2)
            : "memory", "hi", "lo", "$ac1hi", "$ac1lo",
              "$ac2hi", "$ac2lo", "$ac3hi", "$ac3lo"
        );
    }
}
#else
#if HAVE_MIPS32R2
static void sbr_qmf_deint_bfly_int_mips(int *v, const int *src0, const int *src1)
{
    int i, b1, b2, m1, m2, tm1, tm2;
    int b3, b4, m3, m4, tm3, tm4;
    int *v1 = v + 127;
    src1 = src1 + 63;

    for (i = 0; i < 64; i+=4) {

        __asm__ volatile (
            "lw             %[b1],      0(%[src0])              \n\t"
            "lw             %[b3],      4(%[src0])              \n\t"
            "lw             %[b2],      0(%[src1])              \n\t"
            "lw             %[b4],      -4(%[src1])             \n\t"
            "subu           %[m1],      %[b1],          %[b2]   \n\t"
            "addu           %[m2],      %[b1],          %[b2]   \n\t"
            "subu           %[m3],      %[b3],          %[b4]   \n\t"
            "addu           %[m4],      %[b3],          %[b4]   \n\t"
            "addiu          %[m1],      %[m1],          16      \n\t"
            "addiu          %[m2],      %[m2],          16      \n\t"
            "addiu          %[m3],      %[m3],          16      \n\t"
            "addiu          %[m4],      %[m4],          16      \n\t"
            "sra            %[tm1],     %[m1],          5       \n\t"
            "sra            %[tm2],     %[m2],          5       \n\t"
            "sra            %[tm3],     %[m3],          5       \n\t"
            "sra            %[tm4],     %[m4],          5       \n\t"
            "lw             %[b1],      8(%[src0])              \n\t"
            "lw             %[b3],      12(%[src0])             \n\t"
            "lw             %[b2],      -8(%[src1])             \n\t"
            "lw             %[b4],      -12(%[src1])            \n\t"
            "sw             %[tm1],     0(%[v])                 \n\t"
            "sw             %[tm2],     0(%[v1])                \n\t"
            "sw             %[tm3],     4(%[v])                 \n\t"
            "sw             %[tm4],     -4(%[v1])               \n\t"
            "subu           %[m1],      %[b1],          %[b2]   \n\t"
            "addu           %[m2],      %[b1],          %[b2]   \n\t"
            "subu           %[m3],      %[b3],          %[b4]   \n\t"
            "addu           %[m4],      %[b3],          %[b4]   \n\t"
            "addiu          %[m1],      %[m1],          16      \n\t"
            "addiu          %[m2],      %[m2],          16      \n\t"
            "addiu          %[m3],      %[m3],          16      \n\t"
            "addiu          %[m4],      %[m4],          16      \n\t"
            "sra            %[tm1],     %[m1],          5       \n\t"
            "sra            %[tm2],     %[m2],          5       \n\t"
            "sra            %[tm3],     %[m3],          5       \n\t"
            "sra            %[tm4],     %[m4],          5       \n\t"
            "addiu          %[src0],    %[src0],        16      \n\t"
            "addiu          %[src1],    %[src1],        -16     \n\t"
            "sw             %[tm1],     8(%[v])                 \n\t"
            "sw             %[tm2],     -8(%[v1])               \n\t"
            "sw             %[tm3],     12(%[v])                \n\t"
            "sw             %[tm4],     -12(%[v1])              \n\t"
            "addiu          %[v],       %[v],           16      \n\t"
            "addiu          %[v1],      %[v1],          -16     \n\t"

            : [m1] "=&r" (m1), [b1] "=&r" (b1), [src0] "+r" (src0),
              [m2] "=&r" (m2), [b2] "=&r" (b2), [src1] "+r" (src1),
              [m3] "=&r" (m3), [b3] "=&r" (b3),
              [m4] "=&r" (m4), [b4] "=&r" (b4),
              [tm1] "=&r" (tm1), [tm2] "=&r" (tm2), [v] "+r" (v), [v1] "+r" (v1),
              [tm3] "=&r" (tm3), [tm4] "=&r" (tm4)
            :
            : "memory"
        );
    }
}
static void sbr_hf_gen_int_mips(int (*X_high)[2], const int (*X_low)[2],
                             const int alpha0[2], const int alpha1[2],
                             int bw, int start, int end)
{
    int i;
    int a0, a1, a2, a3;
    int b1, b2, m1, m2, v1, v2;
    int bw1, bw2, bw3, bw4;
    int *Xh_ptr, *Xl_ptr;
    int c1 = 0x40000000;
    int c2 = 0x20000000;
    int c3 = 0x10000000;

    __asm__ volatile (
        "lw             %[m1],          0(%[alpha0])            \n\t"
        "mthi           $0                                      \n\t"
        "mtlo           %[c1]                                   \n\t"
        "madd           %[bw],          %[bw]                   \n\t"
        "mfhi           %[bw3]                                  \n\t"
        "mflo           %[bw4]                                  \n\t"
        "sll            %[bw3],         %[bw3],         1       \n\t"
        "srl            %[bw4],         %[bw4],         31      \n\t"
        "or             %[bw1],         %[bw3],         %[bw4]  \n\t"
        "lw             %[m2],          4(%[alpha0])            \n\t"
        "lw             %[b1],          0(%[alpha1])            \n\t"
        "lw             %[b2],          4(%[alpha1])            \n\t"
        "mthi           $0                                      \n\t"
        "mtlo           %[c1]                                   \n\t"
        "madd           %[m1],          %[bw]                   \n\t"
        "mfhi           %[bw3]                                  \n\t"
        "mflo           %[bw4]                                  \n\t"
        "sll            %[bw3],         %[bw3],         1       \n\t"
        "srl            %[bw4],         %[bw4],         31      \n\t"
        "or             %[a2],          %[bw3],         %[bw4]  \n\t"
        "mthi           $0                                      \n\t"
        "mtlo           %[c1]                                   \n\t"
        "madd           %[m2],          %[bw]                   \n\t"
        "mfhi           %[bw3]                                  \n\t"
        "mflo           %[bw4]                                  \n\t"
        "sll            %[bw3],         %[bw3],         1       \n\t"
        "srl            %[bw4],         %[bw4],         31      \n\t"
        "or             %[a3],          %[bw3],         %[bw4]  \n\t"
        "mthi           $0                                      \n\t"
        "mtlo           %[c1]                                   \n\t"
        "madd           %[b1],          %[bw1]                  \n\t"
        "mfhi           %[bw3]                                  \n\t"
        "mflo           %[bw4]                                  \n\t"
        "sll            %[bw3],         %[bw3],         1       \n\t"
        "srl            %[bw4],         %[bw4],         31      \n\t"
        "or             %[a0],          %[bw3],         %[bw4]  \n\t"
        "mthi           $0                                      \n\t"
        "mtlo           %[c1]                                   \n\t"
        "madd           %[b2],          %[bw1]                  \n\t"
        "mfhi           %[bw3]                                  \n\t"
        "mflo           %[bw4]                                  \n\t"
        "sll            %[bw3],         %[bw3],         1       \n\t"
        "srl            %[bw4],         %[bw4],         31      \n\t"
        "or             %[a1],          %[bw3],         %[bw4]  \n\t"

        : [m1] "=&r" (m1), [m2] "=&r" (m2),
          [b1] "=&r" (b1), [b2] "=&r" (b2),
          [a0] "=&r" (a0), [a1] "=r" (a1),
          [a2] "=&r" (a2), [a3] "=&r" (a3),
          [bw1] "=&r" (bw1), [bw3] "=&r" (bw3), [bw4] "=&r" (bw4)
        : [alpha0] "r" (alpha0), [alpha1] "r" (alpha1),
          [bw] "r" (bw), [c1] "r" (c1)
        : "memory", "hi", "lo", "$ac1hi", "$ac1lo",
          "$ac2hi", "$ac2lo", "$ac3hi", "$ac3lo"
    );

    Xh_ptr = (int *)&X_high[start][0];
    Xl_ptr = (int *)&X_low[start][0];

    for (i = start; i < end; i+=2) {
        __asm__ volatile (
            "lw             %[v1],          0(%[Xl_ptr])            \n\t"
            "lw             %[m1],          -16(%[Xl_ptr])          \n\t"
            "lw             %[m2],          -12(%[Xl_ptr])          \n\t"
            "lw             %[b1],          -8(%[Xl_ptr])           \n\t"
            "lw             %[b2],          -4(%[Xl_ptr])           \n\t"
            "lw             %[v2],          4(%[Xl_ptr])            \n\t"
            "mthi           $0                                      \n\t"
            "mtlo           %[c3]                                   \n\t"
            "madd           %[v1],          %[c2]                   \n\t"
            "madd           %[m1],          %[a0]                   \n\t"
            "msub           %[m2],          %[a1]                   \n\t"
            "madd           %[b1],          %[a2]                   \n\t"
            "msub           %[b2],          %[a3]                   \n\t"
            "mfhi           %[bw3]                                  \n\t"
            "mflo           %[bw4]                                  \n\t"
            "sll            %[bw3],         %[bw3],         3       \n\t"
            "srl            %[bw4],         %[bw4],         29      \n\t"
            "or             %[bw1],         %[bw3],         %[bw4]  \n\t"
            "mthi           $0                                      \n\t"
            "mtlo           %[c3]                                   \n\t"
            "madd           %[v2],          %[c2]                   \n\t"
            "madd           %[m2],          %[a0]                   \n\t"
            "madd           %[m1],          %[a1]                   \n\t"
            "madd           %[b2],          %[a2]                   \n\t"
            "madd           %[b1],          %[a3]                   \n\t"
            "mfhi           %[bw3]                                  \n\t"
            "mflo           %[bw4]                                  \n\t"
            "sll            %[bw3],         %[bw3],         3       \n\t"
            "srl            %[bw4],         %[bw4],         29      \n\t"
            "or             %[bw2],         %[bw3],         %[bw4]  \n\t"
            "lw             %[m1],          8(%[Xl_ptr])            \n\t"
            "lw             %[m2],          12(%[Xl_ptr])           \n\t"
            "mthi           $0                                      \n\t"
            "mtlo           %[c3]                                   \n\t"
            "madd           %[m1],          %[c2]                   \n\t"
            "madd           %[b1],          %[a0]                   \n\t"
            "msub           %[b2],          %[a1]                   \n\t"
            "madd           %[v1],          %[a2]                   \n\t"
            "msub           %[v2],          %[a3]                   \n\t"
            "mfhi           %[bw3]                                  \n\t"
            "mflo           %[bw4]                                  \n\t"
            "sll            %[bw3],         %[bw3],         3       \n\t"
            "srl            %[bw4],         %[bw4],         29      \n\t"
            "or             %[bw3],         %[bw3],         %[bw4]  \n\t"
            "mthi           $0                                      \n\t"
            "mtlo           %[c3]                                   \n\t"
            "madd           %[m2],          %[c2]                   \n\t"
            "madd           %[b2],          %[a0]                   \n\t"
            "madd           %[b1],          %[a1]                   \n\t"
            "madd           %[v2],          %[a2]                   \n\t"
            "madd           %[v1],          %[a3]                   \n\t"
            "mfhi           %[m1]                                   \n\t"
            "mflo           %[m2]                                   \n\t"
            "sll            %[m1],          %[m1],          3       \n\t"
            "srl            %[m2],          %[m2],          29      \n\t"
            "or             %[bw4],         %[m1],          %[m2]   \n\t"
            "addiu          %[Xl_ptr],      %[Xl_ptr],      16      \n\t"
            "sw             %[bw1],         0(%[Xh_ptr])            \n\t"
            "sw             %[bw2],         4(%[Xh_ptr])            \n\t"
            "sw             %[bw3],         8(%[Xh_ptr])            \n\t"
            "sw             %[bw4],         12(%[Xh_ptr])           \n\t"
            "addiu          %[Xh_ptr],      %[Xh_ptr],      16      \n\t"

            : [m1] "=&r" (m1), [m2] "=&r" (m2), [v1] "=&r" (v1), [v2] "=&r" (v2),
              [b1] "=&r" (b1), [b2] "=&r" (b2), [bw1] "=&r" (bw1),
              [bw2] "=&r" (bw2), [bw3] "=&r" (bw3), [bw4] "=&r" (bw4)
            : [Xl_ptr] "r" (Xl_ptr), [Xh_ptr] "r" (Xh_ptr),
              [a0] "r" (a0), [a1] "r" (a1), [c3] "r" (c3),
              [a2] "r" (a2), [a3] "r" (a3), [c2] "r" (c2)
            : "memory", "hi", "lo"
        );
    }
}

static void sbr_hf_g_filt_int_mips(int (*Y)[2], const int (*X_high)[40][2],
                            const aac_float_t *g_filt, int m_max, intptr_t ixh)
{
    int m, r, r1, c23 = 23, c32 = 32, c1 = 1;
    long long accu, accu1, accu2, accu3;
    int *g_filt_ptr, *ixh_ptr, *Y_ptr;
    int b1, b2, m1, m2, tm, tm1, tm2;
    int b3, b4, m3, m4, tmi, tm3, tm4, sh1;

    g_filt_ptr = (int *)g_filt;
    ixh_ptr = (int *)&X_high[0][ixh][0];
    Y_ptr = (int *)&Y[0][0];

    for (m = 0; m < (m_max >> 1); m++) {
        __asm__ volatile (
            "lw             %[b1],      0(%[g_filt_ptr])        \n\t"
            "lw             %[tm],      4(%[g_filt_ptr])        \n\t"
            "lw             %[m2],      0(%[ixh_ptr])           \n\t"
            "lw             %[b2],      4(%[ixh_ptr])           \n\t"
            "lw             %[b3],      8(%[g_filt_ptr])        \n\t"
            "lw             %[tmi],     12(%[g_filt_ptr])       \n\t"
            "lw             %[m4],      320(%[ixh_ptr])         \n\t"
            "lw             %[b4],      324(%[ixh_ptr])         \n\t"
            "addiu          %[b1],      %[b1],          64      \n\t"
            "sra            %[m1],      %[b1],          7       \n\t"
            "subu           %[tm],      %[c23],         %[tm]   \n\t"
            "addiu          %[b3],      %[b3],          64      \n\t"
            "sra            %[m3],      %[b3],          7       \n\t"
            "subu           %[tmi],     %[c23],         %[tmi]  \n\t"

            : [m1] "=&r" (m1), [b1] "=&r" (b1), [b2] "=&r" (b2),
              [m2] "=&r" (m2), [tm] "=&r" (tm),
              [m3] "=&r" (m3), [b3] "=&r" (b3), [b4] "=&r" (b4),
              [m4] "=&r" (m4), [tmi] "=&r" (tmi)
            : [g_filt_ptr] "r" (g_filt_ptr), [ixh_ptr] "r" (ixh_ptr),
              [c23] "r" (c23)
            : "memory"
        );

        if ((tm < 32) && (tmi < 32)) {
            __asm__ volatile (
                "subu           %[sh1],         %[c32],         %[tm]   \n\t"
                "subu           %[r],           %[tm],          %[c1]   \n\t"
                "sllv           %[r],           %[c1],          %[r]    \n\t"
                "mthi           $0                                      \n\t"
                "mtlo           %[r]                                    \n\t"
                "madd           %[m1],          %[m2]                   \n\t"
                "mfhi           %[b1]                                   \n\t"
                "mflo           %[b3]                                   \n\t"
                "sllv           %[b1],          %[b1],          %[sh1]  \n\t"
                "srlv           %[b3],          %[b3],          %[tm]   \n\t"
                "or             %[tm1],         %[b1],          %[b3]   \n\t"
                "mthi           $0                                      \n\t"
                "mtlo           %[r]                                    \n\t"
                "madd           %[m1],          %[b2]                   \n\t"
                "mfhi           %[m1]                                   \n\t"
                "mflo           %[m2]                                   \n\t"
                "sllv           %[m1],          %[m1],          %[sh1]  \n\t"
                "srlv           %[m2],          %[m2],          %[tm]   \n\t"
                "or             %[tm2],         %[m1],          %[m2]   \n\t"
                "subu           %[sh1],         %[c32],         %[tmi]  \n\t"
                "subu           %[r],           %[tmi],         %[c1]   \n\t"
                "sllv           %[r],           %[c1],          %[r]    \n\t"
                "mthi           $0                                      \n\t"
                "mtlo           %[r]                                    \n\t"
                "madd           %[m3],          %[m4]                   \n\t"
                "mfhi           %[m1]                                   \n\t"
                "mflo           %[m2]                                   \n\t"
                "sllv           %[m1],          %[m1],          %[sh1]  \n\t"
                "srlv           %[m2],          %[m2],          %[tmi]  \n\t"
                "or             %[tm3],         %[m1],          %[m2]   \n\t"
                "mthi           $0                                      \n\t"
                "mtlo           %[r]                                    \n\t"
                "madd           %[m3],          %[b4]                   \n\t"
                "mfhi           %[m1]                                   \n\t"
                "mflo           %[m2]                                   \n\t"
                "sllv           %[m1],          %[m1],          %[sh1]  \n\t"
                "srlv           %[m2],          %[m2],          %[tmi]  \n\t"
                "or             %[tm4],         %[m1],          %[m2]   \n\t"
                "addiu          %[g_filt_ptr],  %[g_filt_ptr],  16      \n\t"
                "addiu          %[ixh_ptr],     %[ixh_ptr],     640     \n\t"
                "sw             %[tm1],         0(%[Y_ptr])             \n\t"
                "sw             %[tm2],         4(%[Y_ptr])             \n\t"
                "sw             %[tm3],         8(%[Y_ptr])             \n\t"
                "sw             %[tm4],         12(%[Y_ptr])            \n\t"
                "addiu          %[Y_ptr],       %[Y_ptr],       16      \n\t"

                : [tm1] "=&r" (tm1), [tm2] "=&r" (tm2), [Y_ptr] "+r" (Y_ptr),
                  [g_filt_ptr] "+r" (g_filt_ptr), [ixh_ptr] "+r" (ixh_ptr),
                  [tm3] "=&r" (tm3), [tm4] "=&r" (tm4), [sh1] "=&r" (sh1),
                  [r] "=&r" (r), [b1] "=&r" (b1), [b3] "=&r" (b3)
                : [m1] "r" (m1), [b2] "r" (b2), [c1] "r" (c1),
                  [m2] "r" (m2), [tm] "r" (tm), [c32] "r" (c32),
                  [m3] "r" (m3), [b4] "r" (b4),
                  [m4] "r" (m4), [tmi] "r" (tmi)
                : "memory", "hi", "lo"
            );
        } else {
            __asm__ volatile (
                "subu           %[r],      %[tm],           %[c1]   \n\t"
                "subu           %[r1],     %[tmi],          %[c1]   \n\t"
                "sllv           %[r],      %[c1],           %[r]    \n\t"
                "sllv           %[r1],     %[c1],           %[r1]   \n\t"

                : [r] "=&r" (r), [r1] "=&r" (r1)
                : [tmi] "r" (tmi), [tm] "r" (tm), [c1] "r" (c1)
                :
            );

            accu = (long long)m2 * m1;
            accu1 = (long long)b2 * m1;
            accu2 = (long long)m4 * m3;
            accu3 = (long long)b4 * m3;
            g_filt_ptr+=4;
            ixh_ptr+=160;
            Y_ptr+=4;
            Y[2*m][0] = (int)((accu + r) >> tm);
            Y[2*m][1] = (int)((accu1 + r) >> tm);
            Y[2*m+1][0] = (int)((accu2 + r1) >> tmi);
            Y[2*m+1][1] = (int)((accu3 + r1) >> tmi);
        }
    }

    if(m_max & 1){
        __asm__ volatile (
            "lw             %[b1],      0(%[g_filt_ptr])        \n\t"
            "lw             %[tm],      4(%[g_filt_ptr])        \n\t"
            "lw             %[m2],      0(%[ixh_ptr])           \n\t"
            "lw             %[b2],      4(%[ixh_ptr])           \n\t"
            "addiu          %[b1],      %[b1],          64      \n\t"
            "sra            %[m1],      %[b1],          7       \n\t"
            "subu           %[tm],      %[c23],         %[tm]   \n\t"

            : [m1] "=&r" (m1), [b1] "=&r" (b1), [b2] "=&r" (b2),
              [m2] "=&r" (m2), [tm] "=&r" (tm)
            : [g_filt_ptr] "r" (g_filt_ptr), [ixh_ptr] "r" (ixh_ptr),
              [c23] "r" (c23)
            : "memory"
        );

        if (tm < 32) {
            __asm__ volatile (
                "subu           %[sh1],         %[c32],         %[tm]   \n\t"
                "subu           %[r],           %[tm],          %[c1]   \n\t"
                "sllv           %[r],           %[c1],          %[r]    \n\t"
                "mthi           $0                                      \n\t"
                "mtlo           %[r]                                    \n\t"
                "madd           %[m1],          %[m2]                   \n\t"
                "mfhi           %[b1]                                   \n\t"
                "mflo           %[b3]                                   \n\t"
                "sllv           %[b1],          %[b1],          %[sh1]  \n\t"
                "srlv           %[b3],          %[b3],          %[tm]   \n\t"
                "or             %[tm1],         %[b1],          %[b3]   \n\t"
                "mthi           $0                                      \n\t"
                "mtlo           %[r]                                    \n\t"
                "madd           %[m1],          %[b2]                   \n\t"
                "mfhi           %[m1]                                   \n\t"
                "mflo           %[m2]                                   \n\t"
                "sllv           %[m1],          %[m1],          %[sh1]  \n\t"
                "srlv           %[m2],          %[m2],          %[tm]   \n\t"
                "or             %[tm2],         %[m1],          %[m2]   \n\t"
                "sw             %[tm1],         0(%[Y_ptr])             \n\t"
                "sw             %[tm2],         4(%[Y_ptr])             \n\t"

                : [tm1] "=&r" (tm1), [tm2] "=&r" (tm2), [Y_ptr] "+r" (Y_ptr),
                  [g_filt_ptr] "+r" (g_filt_ptr), [ixh_ptr] "+r" (ixh_ptr),
                  [sh1] "=&r" (sh1), [r] "=&r" (r), [b1] "=&r" (b1), [b3] "=&r" (b3)
                : [m1] "r" (m1), [b2] "r" (b2), [c1] "r" (c1),
                  [m2] "r" (m2), [tm] "r" (tm), [c32] "r" (c32)
                : "memory", "hi", "lo"
            );
        } else {
            __asm__ volatile (
                "subu           %[r],      %[tm],           %[c1]   \n\t"
                "sllv           %[r],      %[c1],           %[r]    \n\t"

                : [r] "=&r" (r)
                : [tm] "r" (tm), [c1] "r" (c1)
                :
            );

            accu = (long long)m2 * m1;
            accu1 = (long long)b2 * m1;
            Y[m_max-1][0] = (int)((accu + r) >> tm);
            Y[m_max-1][1] = (int)((accu1 + r) >> tm);
        }
    }
}
#endif /* HAVE_MIPS32R2 */
#endif /* HAVE_MIPSDSPR1 || HAVE_MIPSDSPR2 */
#endif /* HAVE_INLINE_ASM */

void ff_sbrdsp_init_mips(SBRDSPContext *s)
{
#if HAVE_INLINE_ASM
    s->neg_odd_64       = sbr_neg_odd_64_mips;
    s->qmf_pre_shuffle  = sbr_qmf_pre_shuffle_mips;
    s->qmf_post_shuffle = sbr_qmf_post_shuffle_mips;
#if HAVE_MIPSFPU
    s->sum64x5          = sbr_sum64x5_mips;
    s->sum_square       = sbr_sum_square_mips;
    s->qmf_deint_bfly   = sbr_qmf_deint_bfly_mips;
    s->autocorrelate    = sbr_autocorrelate_mips;
    s->hf_gen           = sbr_hf_gen_mips;
    s->hf_g_filt        = sbr_hf_g_filt_mips;

    s->hf_apply_noise[0] = sbr_hf_apply_noise_0_mips;
    s->hf_apply_noise[1] = sbr_hf_apply_noise_1_mips;
    s->hf_apply_noise[2] = sbr_hf_apply_noise_2_mips;
    s->hf_apply_noise[3] = sbr_hf_apply_noise_3_mips;
#endif /* HAVE_MIPSFPU */
#if HAVE_MIPSDSPR1 || HAVE_MIPSDSPR2
    s->autocorrelate_q31 = sbr_autocorrelate_q31_mips;
#endif
#if HAVE_MIPSDSPR1 || HAVE_MIPSDSPR2 || HAVE_MIPS32R2
    s->hf_g_filt_int        = sbr_hf_g_filt_int_mips;
    s->hf_gen_fixed         = sbr_hf_gen_int_mips;
    s->qmf_deint_bfly_fixed = sbr_qmf_deint_bfly_int_mips;
#endif
#endif /* HAVE_INLINE_ASM */
}
