/*
 * Copyright (c) 2012 Mans Rullgard
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

#ifndef AVCODEC_SBRDSP_H
#define AVCODEC_SBRDSP_H

#include <stdint.h>
#include "float_emu.h"

typedef struct SBRDSPContext {
    void (*sum64x5)(float *z);
    void (*sum64x5_fixed)(int *z);
    float (*sum_square)(float (*x)[2], int n);
    aac_float_t (*sum_square_fixed)(int (*x)[2], int n);
    void (*neg_odd_64)(float *x);
    void (*neg_odd_64_fixed)(int *x);
    void (*qmf_pre_shuffle)(float *z);
    void (*qmf_pre_shuffle_fixed)(int *z);
    void (*qmf_post_shuffle)(float W[32][2], const float *z);
    void (*qmf_post_shuffle_fixed)(int W[32][2], const int *z);
    void (*qmf_deint_neg)(float *v, const float *src);
    void (*qmf_deint_neg_fixed)(int *v, const int *src);
    void (*qmf_deint_bfly)(float *v, const float *src0, const float *src1);
    void (*qmf_deint_bfly_fixed)(int *v, const int *src0, const int *src1);
    void (*autocorrelate)(const float x[40][2], float phi[3][2][2]);
    void (*autocorrelate_q31)(const int x[40][2], aac_float_t phi[3][2][2]);
    void (*hf_gen)(float (*X_high)[2], const float (*X_low)[2],
                   const float alpha0[2], const float alpha1[2],
                   float bw, int start, int end);
    void (*hf_gen_fixed)(int (*X_high)[2], const int (*X_low)[2],
                   const int alpha0[2], const int alpha1[2],
                   int bw, int start, int end);
    void (*hf_g_filt)(float (*Y)[2], const float (*X_high)[40][2],
                      const float *g_filt, int m_max, intptr_t ixh);
    void (*hf_g_filt_int)(int (*Y)[2], const int (*X_high)[40][2],
                      const aac_float_t *g_filt, int m_max, intptr_t ixh);
    void (*hf_gen_int)(int (*X_high)[2], const int (*X_low)[2],
                             const int alpha0[2], const int alpha1[2],
                             int bw, int start, int end);
    void (*qmf_deint_bfly_int)(int *v, const int *src0, const int *src1);
    void (*hf_apply_noise[4])(float (*Y)[2], const float *s_m,
                              const float *q_filt, int noise,
                              int kx, int m_max);
    void (*hf_apply_noise_int[4])(int (*Y)[2], const aac_float_t *s_m,
                              const aac_float_t *q_filt, int noise,
                              int kx, int m_max);
} SBRDSPContext;

extern const float ff_sbr_noise_table[][2];
extern const int   ff_sbr_noise_table_fixed[][2];

void ff_sbrdsp_init(SBRDSPContext *s);
void ff_sbrdsp_init_arm(SBRDSPContext *s);
void ff_sbrdsp_init_x86(SBRDSPContext *s);
void ff_sbrdsp_init_mips(SBRDSPContext *s);

#endif /* AVCODEC_SBRDSP_H */
