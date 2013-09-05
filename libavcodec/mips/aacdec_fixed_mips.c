/*
 * Copyright (c) 2010 Alex Converse <alex.converse@gmail.com>
 *
 * This file is part of Libav.
 *
 * Libav is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * Libav is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Libav; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */
#define CONFIG_FFT_FLOAT 0
#define CONFIG_FFT_FIXED_32 1
#define CONFIG_AAC_FIXED 1
#define CONFIG_FIXED 1

#include "libavcodec/aactab.h"
#include "libavcodec/sinewin.h"
#include "libavcodec/cbrt_fixed_tables.h"

#if HAVE_INLINE_ASM
static int32_t cbrt_tab_mips[2*8192] = {0};

static av_cold void aac_decode_init_mips(void)
{
    int i;
    for (i = 1; i < 8192; i++) {
        cbrt_tab_mips[8192+i] = cbrt_tab[i];
        cbrt_tab_mips[8192-i] = -cbrt_tab[i];
    }
}

#if HAVE_MIPSDSPR1 || HAVE_MIPSDSPR2
static void vector_pow43_mips(int *coefs, int len)
{
    int i;
    int coef, coef1, coef2, coef3;
    const int32_t *ptr_cbrt = &cbrt_tab_mips[8192];

    for (i=0; i<len; i+=4) {
        __asm__ volatile (
            "lw         %[coef],        0(%[coefs])                 \n\t"
            "lw         %[coef1],       4(%[coefs])                 \n\t"
            "lw         %[coef2],       8(%[coefs])                 \n\t"
            "lw         %[coef3],       12(%[coefs])                \n\t"
            "sll        %[coef],        %[coef],            2       \n\t"
            "sll        %[coef1],       %[coef1],           2       \n\t"
            "sll        %[coef2],       %[coef2],           2       \n\t"
            "sll        %[coef3],       %[coef3],           2       \n\t"
            "lwx        %[coef],        %[coef](%[ptr_cbrt])        \n\t"
            "lwx        %[coef1],       %[coef1](%[ptr_cbrt])       \n\t"
            "lwx        %[coef2],       %[coef2](%[ptr_cbrt])       \n\t"
            "lwx        %[coef3],       %[coef3](%[ptr_cbrt])       \n\t"
            "sw         %[coef],        0(%[coefs])                 \n\t"
            "sw         %[coef1],       4(%[coefs])                 \n\t"
            "sw         %[coef2],       8(%[coefs])                 \n\t"
            "sw         %[coef3],       12(%[coefs])                \n\t"
            "addiu      %[coefs],       %[coefs],           16      \n\t"

            : [coefs] "+r" (coefs),
              [coef] "=&r" (coef), [coef1] "=&r" (coef1),
              [coef2] "=&r" (coef2), [coef3] "=&r" (coef3)
            : [ptr_cbrt] "r" (ptr_cbrt)
            : "memory"
        );
    }
}

static void imdct_and_windowing_mips(AACContext *ac, SingleChannelElement *sce)
{
    IndividualChannelStream *ics = &sce->ics;
    int *in    = sce->coeffs;
    int *out   = sce->ret_buf;
    int *saved = sce->saved;
    const int *swindow      = ics->use_kb_window[0] ? ff_aac_kbd_short_128_fixed : ff_sine_128_fixed;
    const int *lwindow_prev = ics->use_kb_window[1] ? ff_aac_kbd_long_1024_fixed : ff_sine_1024_fixed;
    const int *swindow_prev = ics->use_kb_window[1] ? ff_aac_kbd_short_128_fixed : ff_sine_128_fixed;
    int *buf  = ac->buf_mdct;
    int *temp = ac->temp;
    int i;
    int m1, m2, m3, m4, m5, m6, m7, m8;
    int b1, b2, b3, b4, b5, b6, b7, b8;
    int *buf_ptr = buf;

    if (ics->window_sequence[0] == EIGHT_SHORT_SEQUENCE) {
        for (i = 0; i < 1024; i += 128)
            ac->mdct_small.imdct_half(&ac->mdct_small, buf + i, in + i);
    } else {
        ac->mdct.imdct_half(&ac->mdct, buf, in);
        for (i=0; i<1024; i+=8) {
            __asm__ volatile (
                "lw             %[b1],      0(%[buf_ptr])           \n\t"
                "lw             %[b2],      4(%[buf_ptr])           \n\t"
                "lw             %[b3],      8(%[buf_ptr])           \n\t"
                "lw             %[b4],      12(%[buf_ptr])          \n\t"
                "lw             %[b5],      16(%[buf_ptr])          \n\t"
                "lw             %[b6],      20(%[buf_ptr])          \n\t"
                "lw             %[b7],      24(%[buf_ptr])          \n\t"
                "lw             %[b8],      28(%[buf_ptr])          \n\t"
                "shra_r.w       %[m1],      %[b1],          3       \n\t"
                "shra_r.w       %[m2],      %[b2],          3       \n\t"
                "shra_r.w       %[m3],      %[b3],          3       \n\t"
                "shra_r.w       %[m4],      %[b4],          3       \n\t"
                "shra_r.w       %[m5],      %[b5],          3       \n\t"
                "shra_r.w       %[m6],      %[b6],          3       \n\t"
                "shra_r.w       %[m7],      %[b7],          3       \n\t"
                "shra_r.w       %[m8],      %[b8],          3       \n\t"
                "sw             %[m1],      0(%[buf_ptr])           \n\t"
                "sw             %[m2],      4(%[buf_ptr])           \n\t"
                "sw             %[m3],      8(%[buf_ptr])           \n\t"
                "sw             %[m4],      12(%[buf_ptr])          \n\t"
                "sw             %[m5],      16(%[buf_ptr])          \n\t"
                "sw             %[m6],      20(%[buf_ptr])          \n\t"
                "sw             %[m7],      24(%[buf_ptr])          \n\t"
                "sw             %[m8],      28(%[buf_ptr])          \n\t"
                "addiu          %[buf_ptr], %[buf_ptr],     32      \n\t"

                : [m1] "=&r" (m1), [b1] "=&r" (b1), [m2] "=&r" (m2),
                  [b2] "=&r" (b2), [m3] "=&r" (m3), [b3] "=&r" (b3),
                  [m4] "=&r" (m4), [b4] "=&r" (b4), [m5] "=&r" (m5),
                  [b5] "=&r" (b5), [m6] "=&r" (m6), [b6] "=&r" (b6),
                  [m7] "=&r" (m7), [b7] "=&r" (b7), [m8] "=&r" (m8),
                  [b8] "=&r" (b8), [buf_ptr] "+r" (buf_ptr)
                :
                :"memory"
            );
        }
    }

    /* window overlapping
     * NOTE: To simplify the overlapping code, all 'meaningless' short to long
     * and long to short transitions are considered to be short to short
     * transitions. This leaves just two cases (long to long and short to short)
     * with a little special sauce for EIGHT_SHORT_SEQUENCE.
     */
    if ((ics->window_sequence[1] == ONLY_LONG_SEQUENCE || ics->window_sequence[1] == LONG_STOP_SEQUENCE) &&
            (ics->window_sequence[0] == ONLY_LONG_SEQUENCE || ics->window_sequence[0] == LONG_START_SEQUENCE)) {
        ac->dsp.vector_q31mul_window(    out,               saved,            buf,         lwindow_prev, 512);
    } else {
        memcpy(                        out,               saved,            448 * sizeof(int));

        if (ics->window_sequence[0] == EIGHT_SHORT_SEQUENCE) {
            ac->dsp.vector_q31mul_window(out + 448 + 0*128, saved + 448,      buf + 0*128, swindow_prev, 64);
            ac->dsp.vector_q31mul_window(out + 448 + 1*128, buf + 0*128 + 64, buf + 1*128, swindow,      64);
            ac->dsp.vector_q31mul_window(out + 448 + 2*128, buf + 1*128 + 64, buf + 2*128, swindow,      64);
            ac->dsp.vector_q31mul_window(out + 448 + 3*128, buf + 2*128 + 64, buf + 3*128, swindow,      64);
            ac->dsp.vector_q31mul_window(temp,              buf + 3*128 + 64, buf + 4*128, swindow,      64);
            memcpy(                    out + 448 + 4*128, temp, 64 * sizeof(int));
        } else {
            ac->dsp.vector_q31mul_window(out + 448,         saved + 448,      buf,         swindow_prev, 64);
            memcpy(                    out + 576,         buf + 64,         448 * sizeof(int));
        }
    }

    // buffer update
    if (ics->window_sequence[0] == EIGHT_SHORT_SEQUENCE) {
        memcpy(                    saved,       temp + 64,         64 * sizeof(int));
        ac->dsp.vector_q31mul_window(saved + 64,  buf + 4*128 + 64, buf + 5*128, swindow, 64);
        ac->dsp.vector_q31mul_window(saved + 192, buf + 5*128 + 64, buf + 6*128, swindow, 64);
        ac->dsp.vector_q31mul_window(saved + 320, buf + 6*128 + 64, buf + 7*128, swindow, 64);
        memcpy(                    saved + 448, buf + 7*128 + 64,  64 * sizeof(int));
    } else if (ics->window_sequence[0] == LONG_START_SEQUENCE) {
        memcpy(                    saved,       buf + 512,        448 * sizeof(int));
        memcpy(                    saved + 448, buf + 7*128 + 64,  64 * sizeof(int));
    } else { // LONG_STOP or ONLY_LONG
        memcpy(                    saved,       buf + 512,        512 * sizeof(int));
    }
}

static void subband_scale_mips(int *dst, int *src, int scale, int offset, int len)
{
    int ssign;
    int s;
    int c;
    register int out1, out2, out3, out4;
    int * src_end;

    __asm__ volatile (
        ".set       push                            \n\t"
        ".set       noreorder                       \n\t"

        "absq_s.w   %[s],       %[scale]            \n\t"
        "li         %[out2],    1                   \n\t"
        "sra        %[ssign],   %[scale],   31      \n\t"
        "andi       %[out1],    %[s],       3       \n\t"
        "sll        %[out1],    2                   \n\t"
        "sra        %[s],       2                   \n\t"
        "movz       %[ssign],   %[out2],    %[ssign]\n\t"
        "sll        %[len],     2                   \n\t"
        "subu       %[s],       %[offset],  %[s]    \n\t"
        "lwx        %[c],       %[out1](%[exp2tab]) \n\t"
        "blez       %[s],       3f                  \n\t"
        " addu      %[src_end], %[src],     %[len]  \n\t"
    "1:                                             \n\t"
        "lw         %[out1],    0(%[src])           \n\t"
        "lw         %[out2],    4(%[src])           \n\t"
        "lw         %[out3],    8(%[src])           \n\t"
        "lw         %[out4],    12(%[src])          \n\t"
        "mult       $ac0,       %[out1],    %[c]    \n\t"
        "mult       $ac1,       %[out2],    %[c]    \n\t"
        "mult       $ac2,       %[out3],    %[c]    \n\t"
        "mult       $ac3,       %[out4],    %[c]    \n\t"
        "addiu      %[src],     16                  \n\t"
        "mfhi       %[out1],    $ac0                \n\t"
        "mfhi       %[out2],    $ac1                \n\t"
        "mfhi       %[out3],    $ac2                \n\t"
        "mfhi       %[out4],    $ac3                \n\t"
        "addiu      %[dst],     16                  \n\t"
        "shrav_r.w  %[out1],    %[out1],    %[s]    \n\t"
        "shrav_r.w  %[out2],    %[out2],    %[s]    \n\t"
        "shrav_r.w  %[out3],    %[out3],    %[s]    \n\t"
        "shrav_r.w  %[out4],    %[out4],    %[s]    \n\t"
        "mul        %[out1],    %[ssign]            \n\t"
        "mul        %[out2],    %[ssign]            \n\t"
        "mul        %[out3],    %[ssign]            \n\t"
        "mul        %[out4],    %[ssign]            \n\t"
        "sw         %[out1],    -16(%[dst])         \n\t"
        "sw         %[out2],    -12(%[dst])         \n\t"
        "sw         %[out3],    -8(%[dst])          \n\t"
        "bne        %[src],     %[src_end], 1b      \n\t"
        " sw        %[out4],    -4(%[dst])          \n\t"
        "b          4f                              \n\t"
        " nop                                       \n\t"
    "3:                                             \n\t"
        "addiu      %[s],       31                  \n\t"
    "2:                                             \n\t"
        "lw         %[out1],    0(%[src])           \n\t"
        "lw         %[out2],    4(%[src])           \n\t"
        "lw         %[out3],    8(%[src])           \n\t"
        "lw         %[out4],    12(%[src])          \n\t"
        "mult       $ac0,       %[out1],    %[c]    \n\t"
        "mult       $ac1,       %[out2],    %[c]    \n\t"
        "mult       $ac2,       %[out3],    %[c]    \n\t"
        "mult       $ac3,       %[out4],    %[c]    \n\t"
        "addiu      %[src],     16                  \n\t"
        "shilo      $ac0,       1                   \n\t"
        "shilo      $ac1,       1                   \n\t"
        "shilo      $ac2,       1                   \n\t"
        "shilo      $ac3,       1                   \n\t"
        "extrv_r.w  %[out1],    $ac0,       %[s]    \n\t"
        "extrv_r.w  %[out2],    $ac1,       %[s]    \n\t"
        "extrv_r.w  %[out3],    $ac2,       %[s]    \n\t"
        "extrv_r.w  %[out4],    $ac3,       %[s]    \n\t"
        "addiu      %[dst],     16                  \n\t"
        "mul        %[out1],    %[ssign]            \n\t"
        "mul        %[out2],    %[ssign]            \n\t"
        "mul        %[out3],    %[ssign]            \n\t"
        "mul        %[out4],    %[ssign]            \n\t"
        "sw         %[out1],    -16(%[dst])         \n\t"
        "sw         %[out2],    -12(%[dst])         \n\t"
        "sw         %[out3],    -8(%[dst])          \n\t"
        "bne        %[src],     %[src_end], 2b      \n\t"
        " sw        %[out4],    -4(%[dst])          \n\t"
        "4:                                         \n\t"

        ".set       pop                             \n\t"
        :[src]"+r"(src), [dst]"+r"(dst),
         [out1]"=&r"(out1), [out2]"=&r"(out2),
         [out3]"=&r"(out3), [out4]"=&r"(out4),
         [src_end]"=&r"(src_end), [s]"=&r"(s),
         [ssign]"=&r"(ssign), [c]"=&r"(c), [len]"+rr"(len)
        :[exp2tab]"r"(exp2tab),
         [offset]"r"(offset), [scale]"r"(scale)
        :"memory", "hi", "lo", "$ac1hi", "$ac1lo",
         "$ac2hi", "$ac2lo", "$ac3hi", "$ac3lo"
    );
}
#else
static void vector_pow43_mips(int *coefs, int len)
{
    int i;
    int coef, coef1, coef2, coef3;
    const int32_t *ptr_cbrt = &cbrt_tab_mips[8192];
    int32_t *ptr_cbrt1, *ptr_cbrt2, *ptr_cbrt3, *ptr_cbrt4;

    for (i=0; i<len; i+=4) {
        __asm__ volatile (
            "lw         %[coef],        0(%[coefs])                     \n\t"
            "lw         %[coef1],       4(%[coefs])                     \n\t"
            "lw         %[coef2],       8(%[coefs])                     \n\t"
            "lw         %[coef3],       12(%[coefs])                    \n\t"
            "sll        %[coef],        %[coef],            2           \n\t"
            "sll        %[coef1],       %[coef1],           2           \n\t"
            "sll        %[coef2],       %[coef2],           2           \n\t"
            "sll        %[coef3],       %[coef3],           2           \n\t"
            "addu       %[ptr_cbrt1],   %[ptr_cbrt],        %[coef]     \n\t"
            "addu       %[ptr_cbrt2],   %[ptr_cbrt],        %[coef1]    \n\t"
            "addu       %[ptr_cbrt3],   %[ptr_cbrt],        %[coef2]    \n\t"
            "addu       %[ptr_cbrt4],   %[ptr_cbrt],        %[coef3]    \n\t"
            "lw         %[coef],        0(%[ptr_cbrt1])                 \n\t"
            "lw         %[coef1],       0(%[ptr_cbrt2])                 \n\t"
            "lw         %[coef2],       0(%[ptr_cbrt3])                 \n\t"
            "lw         %[coef3],       0(%[ptr_cbrt4])                 \n\t"
            "sw         %[coef],        0(%[coefs])                     \n\t"
            "sw         %[coef1],       4(%[coefs])                     \n\t"
            "sw         %[coef2],       8(%[coefs])                     \n\t"
            "sw         %[coef3],       12(%[coefs])                    \n\t"
            "addiu      %[coefs],       %[coefs],           16          \n\t"

            : [coefs] "+r" (coefs), [coef] "=&r" (coef), [coef1] "=&r" (coef1),
              [coef2] "=&r" (coef2), [coef3] "=&r" (coef3),
              [ptr_cbrt1] "=&r" (ptr_cbrt1), [ptr_cbrt2] "=&r" (ptr_cbrt2),
              [ptr_cbrt3] "=&r" (ptr_cbrt3), [ptr_cbrt4] "=&r" (ptr_cbrt4)
            : [ptr_cbrt] "r" (ptr_cbrt)
            : "memory"
        );
    }
}

static void imdct_and_windowing_mips(AACContext *ac, SingleChannelElement *sce)
{
    IndividualChannelStream *ics = &sce->ics;
    int *in    = sce->coeffs;
    int *out   = sce->ret_buf;
    int *saved = sce->saved;
    const int *swindow      = ics->use_kb_window[0] ? ff_aac_kbd_short_128_fixed : ff_sine_128_fixed;
    const int *lwindow_prev = ics->use_kb_window[1] ? ff_aac_kbd_long_1024_fixed : ff_sine_1024_fixed;
    const int *swindow_prev = ics->use_kb_window[1] ? ff_aac_kbd_short_128_fixed : ff_sine_128_fixed;
    int *buf  = ac->buf_mdct;
    int *temp = ac->temp;
    int i;
    int m1, m2, m3, m4, m5, m6, m7, m8;
    int b1, b2, b3, b4, b5, b6, b7, b8;
    int *buf_ptr = buf;

    if (ics->window_sequence[0] == EIGHT_SHORT_SEQUENCE) {
        for (i = 0; i < 1024; i += 128)
            ac->mdct_small.imdct_half(&ac->mdct_small, buf + i, in + i);
    } else {
        ac->mdct.imdct_half(&ac->mdct, buf, in);

        for (i=0; i<1024; i+=8) {
            __asm__ volatile (
                "lw             %[b1],      0(%[buf_ptr])           \n\t"
                "lw             %[b2],      4(%[buf_ptr])           \n\t"
                "lw             %[b3],      8(%[buf_ptr])           \n\t"
                "lw             %[b4],      12(%[buf_ptr])          \n\t"
                "lw             %[b5],      16(%[buf_ptr])          \n\t"
                "lw             %[b6],      20(%[buf_ptr])          \n\t"
                "lw             %[b7],      24(%[buf_ptr])          \n\t"
                "lw             %[b8],      28(%[buf_ptr])          \n\t"
                "addiu          %[b1],      %[b1],          4       \n\t"
                "addiu          %[b2],      %[b2],          4       \n\t"
                "addiu          %[b3],      %[b3],          4       \n\t"
                "addiu          %[b4],      %[b4],          4       \n\t"
                "addiu          %[b5],      %[b5],          4       \n\t"
                "addiu          %[b6],      %[b6],          4       \n\t"
                "addiu          %[b7],      %[b7],          4       \n\t"
                "addiu          %[b8],      %[b8],          4       \n\t"
                "sra            %[m1],      %[b1],          3       \n\t"
                "sra            %[m2],      %[b2],          3       \n\t"
                "sra            %[m3],      %[b3],          3       \n\t"
                "sra            %[m4],      %[b4],          3       \n\t"
                "sra            %[m5],      %[b5],          3       \n\t"
                "sra            %[m6],      %[b6],          3       \n\t"
                "sra            %[m7],      %[b7],          3       \n\t"
                "sra            %[m8],      %[b8],          3       \n\t"
                "sw             %[m1],      0(%[buf_ptr])           \n\t"
                "sw             %[m2],      4(%[buf_ptr])           \n\t"
                "sw             %[m3],      8(%[buf_ptr])           \n\t"
                "sw             %[m4],      12(%[buf_ptr])          \n\t"
                "sw             %[m5],      16(%[buf_ptr])          \n\t"
                "sw             %[m6],      20(%[buf_ptr])          \n\t"
                "sw             %[m7],      24(%[buf_ptr])          \n\t"
                "sw             %[m8],      28(%[buf_ptr])          \n\t"
                "addiu          %[buf_ptr], %[buf_ptr],     32      \n\t"

                : [m1] "=&r" (m1), [b1] "=&r" (b1), [m2] "=&r" (m2),
                  [b2] "=&r" (b2), [m3] "=&r" (m3), [b3] "=&r" (b3),
                  [m4] "=&r" (m4), [b4] "=&r" (b4), [m5] "=&r" (m5),
                  [b5] "=&r" (b5), [m6] "=&r" (m6), [b6] "=&r" (b6),
                  [m7] "=&r" (m7), [b7] "=&r" (b7), [m8] "=&r" (m8),
                  [b8] "=&r" (b8), [buf_ptr] "+r" (buf_ptr)
                :
                : "memory"
            );
        }
    }

    /* window overlapping
     * NOTE: To simplify the overlapping code, all 'meaningless' short to long
     * and long to short transitions are considered to be short to short
     * transitions. This leaves just two cases (long to long and short to short)
     * with a little special sauce for EIGHT_SHORT_SEQUENCE.
     */
    if ((ics->window_sequence[1] == ONLY_LONG_SEQUENCE || ics->window_sequence[1] == LONG_STOP_SEQUENCE) &&
            (ics->window_sequence[0] == ONLY_LONG_SEQUENCE || ics->window_sequence[0] == LONG_START_SEQUENCE)) {
        ac->dsp.vector_q31mul_window(    out,               saved,            buf,         lwindow_prev, 512);
    } else {
        memcpy(                        out,               saved,            448 * sizeof(int));

        if (ics->window_sequence[0] == EIGHT_SHORT_SEQUENCE) {
            ac->dsp.vector_q31mul_window(out + 448 + 0*128, saved + 448,      buf + 0*128, swindow_prev, 64);
            ac->dsp.vector_q31mul_window(out + 448 + 1*128, buf + 0*128 + 64, buf + 1*128, swindow,      64);
            ac->dsp.vector_q31mul_window(out + 448 + 2*128, buf + 1*128 + 64, buf + 2*128, swindow,      64);
            ac->dsp.vector_q31mul_window(out + 448 + 3*128, buf + 2*128 + 64, buf + 3*128, swindow,      64);
            ac->dsp.vector_q31mul_window(temp,              buf + 3*128 + 64, buf + 4*128, swindow,      64);
            memcpy(                    out + 448 + 4*128, temp, 64 * sizeof(int));
        } else {
            ac->dsp.vector_q31mul_window(out + 448,         saved + 448,      buf,         swindow_prev, 64);
            memcpy(                    out + 576,         buf + 64,         448 * sizeof(int));
        }
    }

    // buffer update
    if (ics->window_sequence[0] == EIGHT_SHORT_SEQUENCE) {
        memcpy(                    saved,       temp + 64,         64 * sizeof(int));
        ac->dsp.vector_q31mul_window(saved + 64,  buf + 4*128 + 64, buf + 5*128, swindow, 64);
        ac->dsp.vector_q31mul_window(saved + 192, buf + 5*128 + 64, buf + 6*128, swindow, 64);
        ac->dsp.vector_q31mul_window(saved + 320, buf + 6*128 + 64, buf + 7*128, swindow, 64);
        memcpy(                    saved + 448, buf + 7*128 + 64,  64 * sizeof(int));
    } else if (ics->window_sequence[0] == LONG_START_SEQUENCE) {
        memcpy(                    saved,       buf + 512,        448 * sizeof(int));
        memcpy(                    saved + 448, buf + 7*128 + 64,  64 * sizeof(int));
    } else { // LONG_STOP or ONLY_LONG
        memcpy(                    saved,       buf + 512,        512 * sizeof(int));
    }
}

static void subband_scale_mips(int *dst, int *src, int scale, int offset, int len)
{
    int ssign;
    int s, round;
    int c, one;
    register int out1, out2, out3, out4;
    register int out5, out6, out7, out8;
    int * src_end;

    __asm__ volatile (
        ".set       push                            \n\t"
        ".set       noreorder                       \n\t"

        "slt        %[out3],    %[scale],   $zero   \n\t"
        "subu       %[out4],    $zero,      %[scale]\n\t"
        "move       %[s],       %[scale]            \n\t"
        "movn       %[s],       %[out4],    %[out3] \n\t"
        "li         %[one],     1                   \n\t"
        "sra        %[ssign],   %[scale],   31      \n\t"
        "andi       %[out1],    %[s],       3       \n\t"
        "sll        %[out1],    2                   \n\t"
        "addu       %[out1],    %[exp2tab]          \n\t"
        "sra        %[s],       2                   \n\t"
        "movz       %[ssign],   %[one],     %[ssign]\n\t"
        "sll        %[len],     2                   \n\t"
        "subu       %[s],       %[offset],  %[s]    \n\t"
        "lw         %[c],       0(%[out1])          \n\t"
        "blez       %[s],       3f                  \n\t"
        " addu      %[src_end], %[src],     %[len]  \n\t"
        "addiu      %[round],   %[s],       -1      \n\t"
        "sllv       %[round],   %[one],     %[round]\n\t"
    "1:                                             \n\t"
        "lw         %[out1],    0(%[src])           \n\t"
        "lw         %[out2],    4(%[src])           \n\t"
        "lw         %[out3],    8(%[src])           \n\t"
        "mult       %[out1],    %[c]                \n\t"
        "mfhi       %[out1]                         \n\t"
        "mult       %[out2],    %[c]                \n\t"
        "lw         %[out4],    12(%[src])          \n\t"
        "mfhi       %[out2]                         \n\t"
        "mult       %[out3],    %[c]                \n\t"
        "addiu      %[dst],     16                  \n\t"
        "mfhi       %[out3]                         \n\t"
        "mult       %[out4],    %[c]                \n\t"
        "addiu      %[src],     16                  \n\t"
        "addu       %[out1],    %[round]            \n\t"
        "mfhi       %[out4]                         \n\t"
        "srav       %[out1],    %[out1],    %[s]    \n\t"
        "addu       %[out2],    %[round]            \n\t"
        "mul        %[out1],    %[ssign]            \n\t"
        "srav       %[out2],    %[out2],    %[s]    \n\t"
        "addu       %[out3],    %[round]            \n\t"
        "mul        %[out2],    %[ssign]            \n\t"
        "srav       %[out3],    %[out3],    %[s]    \n\t"
        "addu       %[out4],    %[round]            \n\t"
        "srav       %[out4],    %[out4],    %[s]    \n\t"
        "mul        %[out3],    %[ssign]            \n\t"
        "mul        %[out4],    %[ssign]            \n\t"
        "sw         %[out1],    -16(%[dst])         \n\t"
        "sw         %[out2],    -12(%[dst])         \n\t"
        "sw         %[out3],    -8(%[dst])          \n\t"
        "bne        %[src],     %[src_end], 1b      \n\t"
        " sw        %[out4],    -4(%[dst])          \n\t"
        "b          5f                              \n\t"
        " nop                                       \n\t"
    "3:                                             \n\t"
        "beqz       %[s],       4f                  \n\t"
        " lui       %[round],   0x8000              \n\t"
        "addiu      %[s],       31                  \n\t"
        "sllv       %[round],   %[one],     %[s]    \n\t"
        "addiu      %[s],       1                   \n\t"
        "li         %[len],     32                  \n\t"
        "subu       %[len],     %[len],     %[s]    \n\t"
    "2:                                             \n\t"
        "lw         %[out1],    0(%[src])           \n\t"
        "lw         %[out2],    4(%[src])           \n\t"
        "multu      %[round],   %[one]              \n\t"
        "madd       %[out1],    %[c]                \n\t"
        "lw         %[out3],    8(%[src])           \n\t"
        "addiu      %[dst],     16                  \n\t"
        "mflo       %[out5]                         \n\t"
        "mfhi       %[out1]                         \n\t"
        "multu      %[round],   %[one]              \n\t"
        "madd       %[out2],    %[c]                \n\t"
        "lw         %[out4],    12(%[src])          \n\t"
        "mflo       %[out6]                         \n\t"
        "mfhi       %[out2]                         \n\t"
        "srlv       %[out5],    %[out5],    %[s]    \n\t"
        "sllv       %[out1],    %[out1],    %[len]  \n\t"
        "or         %[out1],    %[out5]             \n\t"
        "srlv       %[out6],    %[out6],    %[s]    \n\t"
        "sllv       %[out2],    %[out2],    %[len]  \n\t"
        "or         %[out2],    %[out6]             \n\t"
        "multu      %[round],   %[one]              \n\t"
        "madd       %[out3],    %[c]                \n\t"
        "mul        %[out1],    %[ssign]            \n\t"
        "mul        %[out2],    %[ssign]            \n\t"
        "mflo       %[out7]                         \n\t"
        "mfhi       %[out3]                         \n\t"
        "srlv       %[out7],    %[out7],    %[s]    \n\t"
        "sllv       %[out3],    %[out3],    %[len]  \n\t"
        "or         %[out3],    %[out7]             \n\t"
        "multu      %[round],   %[one]              \n\t"
        "madd       %[out4],    %[c]                \n\t"
        "mflo       %[out8]                         \n\t"
        "mfhi       %[out4]                         \n\t"
        "srlv       %[out8],    %[out8],    %[s]    \n\t"
        "sllv       %[out4],    %[out4],    %[len]  \n\t"
        "or         %[out4],    %[out8]             \n\t"
        "mul        %[out3],    %[ssign]            \n\t"
        "mul        %[out4],    %[ssign]            \n\t"
        "addiu      %[src],     16                  \n\t"
        "sw         %[out1],    -16(%[dst])         \n\t"
        "sw         %[out2],    -12(%[dst])         \n\t"
        "sw         %[out3],    -8(%[dst])          \n\t"
        "bne        %[src],     %[src_end], 2b      \n\t"
        " sw        %[out4],    -4(%[dst])          \n\t"
        "b          5f                              \n\t"
        " nop                                       \n\t"
    "4:                                             \n\t"
        "lw         %[out1],    0(%[src])           \n\t"
        "lw         %[out2],    4(%[src])           \n\t"
        "multu      %[round],   %[one]              \n\t"
        "madd       %[out1],    %[c]                \n\t"
        "mfhi       %[out1]                         \n\t"
        "multu      %[round],   %[one]              \n\t"
        "madd       %[out2],    %[c]                \n\t"
        "lw         %[out3],    8(%[src])           \n\t"
        "lw         %[out4],    12(%[src])          \n\t"
        "mfhi       %[out2]                         \n\t"
        "multu      %[round],   %[one]              \n\t"
        "madd       %[out3],    %[c]                \n\t"
        "mfhi       %[out3]                         \n\t"
        "multu      %[round],   %[one]              \n\t"
        "madd       %[out4],    %[c]                \n\t"
        "mfhi       %[out4]                         \n\t"
        "addiu      %[src],     16                  \n\t"
        "addiu      %[dst],     16                  \n\t"
        "mul        %[out1],    %[ssign]            \n\t"
        "mul        %[out2],    %[ssign]            \n\t"
        "mul        %[out3],    %[ssign]            \n\t"
        "mul        %[out4],    %[ssign]            \n\t"
        "sw         %[out1],    -16(%[dst])         \n\t"
        "sw         %[out2],    -12(%[dst])         \n\t"
        "sw         %[out3],    -8(%[dst])          \n\t"
        "bne        %[src],     %[src_end], 4b      \n\t"
        " sw        %[out4],    -4(%[dst])          \n\t"
    "5:                                             \n\t"

        ".set       pop                             \n\t"
        :[src]"+r"(src), [dst]"+r"(dst),
         [out1]"=&r"(out1), [out2]"=&r"(out2),
         [out3]"=&r"(out3), [out4]"=&r"(out4),
         [out5]"=&r"(out5), [out6]"=&r"(out6),
         [out7]"=&r"(out7), [out8]"=&r"(out8),
         [src_end]"=&r"(src_end), [s]"=&r"(s),
         [ssign]"=&r"(ssign), [c]"=&r"(c), [len]"+rr"(len),
         [round]"=&r"(round), [one]"=&r"(one)
        :[exp2tab]"r"(exp2tab),
         [offset]"r"(offset), [scale]"r"(scale)
        :"memory", "hi", "lo"
    );
}
#endif /*HAVE_MIPSDSPR1 || HAVE_MIPSDSPR2*/
#endif /*HAVE_INLINE_ASM*/

void ff_aacdec_fixed_init_mips(AACContext *c)
{
#if HAVE_INLINE_ASM
    aac_decode_init_mips();
    c->vector_pow43                 = vector_pow43_mips;
    c->imdct_and_windowing_fixed    = imdct_and_windowing_mips;
    c->subband_scale                = subband_scale_mips;
#endif /*HAVE_INLINE_ASM*/
}
