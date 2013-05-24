/*
 * AAC Spectral Band Replication decoding functions
 * Copyright (c) 2008-2009 Robert Swain ( rob opendot cl )
 * Copyright (c) 2009-2010 Alex Converse <alex.converse@gmail.com>
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

#include "config.h"
#include "libavutil/attributes.h"
#include "libavutil/intfloat.h"
#include "sbrdsp.h"

static void sbr_sum64x5_c(float *z)
{
    int k;
    for (k = 0; k < 64; k++) {
        float f = z[k] + z[k + 64] + z[k + 128] + z[k + 192] + z[k + 256];
        z[k] = f;
    }
}

static void sbr_sum64x5_fixed_c(int *z)
{
    int k;
    for (k = 0; k < 64; k++) {
        int f = z[k] + z[k + 64] + z[k + 128] + z[k + 192] + z[k + 256];
        z[k] = f;
    }
}

static float sbr_sum_square_c(float (*x)[2], int n)
{
    float sum0 = 0.0f, sum1 = 0.0f;
    int i;

    for (i = 0; i < n; i += 2)
    {
        sum0 += x[i + 0][0] * x[i + 0][0];
        sum1 += x[i + 0][1] * x[i + 0][1];
        sum0 += x[i + 1][0] * x[i + 1][0];
        sum1 += x[i + 1][1] * x[i + 1][1];
    }

    return sum0 + sum1;
}

static aac_float_t sbr_sum_square_fixed_c(int (*x)[2], int n)
{
    aac_float_t ret;
    long long accu = 0;
    int i, nz, round;

    for (i = 0; i < n; i += 2) {
        accu += (long long)x[i + 0][0] * x[i + 0][0];
        accu += (long long)x[i + 0][1] * x[i + 0][1];
        accu += (long long)x[i + 1][0] * x[i + 1][0];
        accu += (long long)x[i + 1][1] * x[i + 1][1];
    }

    i = (int)(accu >> 32);
    if (i == 0) {
        nz = 31;
    } else {
        nz = 0;
        while (FFABS(i) < 0x40000000) {
            i <<= 1;
            nz++;
        }
    }

    nz = 32-nz;
    round = 1 << (nz-1);
    i = (int)((accu+round) >> nz);
    i >>= 1;
    ret = int2float(i, nz + 15);

    return ret;
}

static void sbr_neg_odd_64_c(float *x)
{
    union av_intfloat32 *xi = (union av_intfloat32*) x;
    int i;
    for (i = 1; i < 64; i += 4) {
        xi[i + 0].i ^= 1U << 31;
        xi[i + 2].i ^= 1U << 31;
    }
}

static void sbr_neg_odd_64_fixed_c(int *x)
{
    int i;
    for (i = 1; i < 64; i += 2)
        x[i] = -x[i];
}

static void sbr_qmf_pre_shuffle_c(float *z)
{
    union av_intfloat32 *zi = (union av_intfloat32*) z;
    int k;
    zi[64].i = zi[0].i;
    zi[65].i = zi[1].i;
    for (k = 1; k < 31; k += 2) {
        zi[64 + 2 * k + 0].i = zi[64 - k].i ^ (1U << 31);
        zi[64 + 2 * k + 1].i = zi[ k + 1].i;
        zi[64 + 2 * k + 2].i = zi[63 - k].i ^ (1U << 31);
        zi[64 + 2 * k + 3].i = zi[ k + 2].i;
    }

    zi[64 + 2 * 31 + 0].i = zi[64 - 31].i ^ (1U << 31);
    zi[64 + 2 * 31 + 1].i = zi[31 +  1].i;
}

static void sbr_qmf_pre_shuffle_fixed_c(int *z)
{
    int k;
    z[64] = z[0];
    z[65] = z[1];
    for (k = 1; k < 32; k++) {
        z[64+2*k  ] = -z[64 - k];
        z[64+2*k+1] =  z[ k + 1];
    }
}

static void sbr_qmf_post_shuffle_c(float W[32][2], const float *z)
{
    const union av_intfloat32 *zi = (const union av_intfloat32*) z;
    union av_intfloat32 *Wi       = (union av_intfloat32*) W;
    int k;
    for (k = 0; k < 32; k += 2) {
        Wi[2 * k + 0].i = zi[63 - k].i ^ (1U << 31);
        Wi[2 * k + 1].i = zi[ k + 0].i;
        Wi[2 * k + 2].i = zi[62 - k].i ^ (1U << 31);
        Wi[2 * k + 3].i = zi[ k + 1].i;
    }
}

static void sbr_qmf_post_shuffle_fixed_c(int W[32][2], const int *z)
{
    int k;
    for (k = 0; k < 32; k++) {
        W[k][0] = -z[63-k];
        W[k][1] = z[k];
    }
}

static void sbr_qmf_deint_neg_c(float *v, const float *src)
{
    const union av_intfloat32 *si = (const union av_intfloat32*)src;
    union av_intfloat32 *vi = (union av_intfloat32*)v;
    int i;
    for (i = 0; i < 32; i++) {
        vi[     i].i = si[63 - 2 * i    ].i;
        vi[63 - i].i = si[63 - 2 * i - 1].i ^ (1U << 31);
    }
}

static void sbr_qmf_deint_neg_fixed_c(int *v, const int *src)
{
    int i;
    for (i = 0; i < 32; i++) {
        v[     i] = ( src[63 - 2*i    ] + 16) >> 5;
        v[63 - i] = (-src[63 - 2*i - 1] + 16) >> 5;
    }
}

static void sbr_qmf_deint_bfly_c(float *v, const float *src0, const float *src1)
{
    int i;
    for (i = 0; i < 64; i++) {
        v[      i] = src0[i] - src1[63 - i];
        v[127 - i] = src0[i] + src1[63 - i];
    }
}

static void sbr_qmf_deint_bfly_fixed_c(int *v, const int *src0, const int *src1)
{
    int i;
    for (i = 0; i < 64; i++) {
        v[      i] = (src0[i] - src1[63 - i] + 16) >> 5;
        v[127 - i] = (src0[i] + src1[63 - i] + 16) >> 5;
    }
}

static av_always_inline void autocorrelate(const float x[40][2],
                                           float phi[3][2][2], int lag)
{
    int i;
    float real_sum = 0.0f;
    float imag_sum = 0.0f;
    if (lag) {
        for (i = 1; i < 38; i++) {
            real_sum += x[i][0] * x[i+lag][0] + x[i][1] * x[i+lag][1];
            imag_sum += x[i][0] * x[i+lag][1] - x[i][1] * x[i+lag][0];
        }
        phi[2-lag][1][0] = real_sum + x[ 0][0] * x[lag][0] + x[ 0][1] * x[lag][1];
        phi[2-lag][1][1] = imag_sum + x[ 0][0] * x[lag][1] - x[ 0][1] * x[lag][0];
        if (lag == 1) {
            phi[0][0][0] = real_sum + x[38][0] * x[39][0] + x[38][1] * x[39][1];
            phi[0][0][1] = imag_sum + x[38][0] * x[39][1] - x[38][1] * x[39][0];
        }
    } else {
        for (i = 1; i < 38; i++) {
            real_sum += x[i][0] * x[i][0] + x[i][1] * x[i][1];
        }
        phi[2][1][0] = real_sum + x[ 0][0] * x[ 0][0] + x[ 0][1] * x[ 0][1];
        phi[1][0][0] = real_sum + x[38][0] * x[38][0] + x[38][1] * x[38][1];
    }
}

static av_always_inline void autocorrelate_q31(const int x[40][2], aac_float_t phi[3][2][2], int lag)
{
    int i, nz, round, mant, expo;
    long long real_sum, imag_sum;
    long long accu_re = 0, accu_im = 0;

    if (lag) {
        for (i = 1; i < 38; i++) {
            accu_re += (long long)x[i][0] * x[i+lag][0];
            accu_re += (long long)x[i][1] * x[i+lag][1];
            accu_im += (long long)x[i][0] * x[i+lag][1];
            accu_im -= (long long)x[i][1] * x[i+lag][0];
        }

        real_sum = accu_re;
        imag_sum = accu_im;

        accu_re += (long long)x[ 0][0] * x[lag][0];
        accu_re += (long long)x[ 0][1] * x[lag][1];
        accu_im += (long long)x[ 0][0] * x[lag][1];
        accu_im -= (long long)x[ 0][1] * x[lag][0];

        i = (int)(accu_re >> 32);
        if (i == 0) {
            nz = 31;
        } else {
            nz = 0;
            while (FFABS(i) < 0x40000000) {
                i <<= 1;
                nz++;
            }
        }

        nz = 32-nz;
        round = 1 << (nz-1);
        mant = (int)((accu_re+round) >> nz);
        mant = (mant + 64)>>7;
        mant <<= 6;
        expo = nz + 15;
        phi[2-lag][1][0] = int2float(mant, expo);

        i = (int)(accu_im >> 32);
        if (i == 0) {
            nz = 31;
        } else {
            nz = 0;
            while (FFABS(i) < 0x40000000) {
                i <<= 1;
                nz++;
            }
        }

        nz = 32-nz;
        round = 1 << (nz-1);
        mant = (int)((accu_im+round) >> nz);
        mant = (mant + 64)>>7;
        mant <<= 6;
        expo = nz + 15;
        phi[2-lag][1][1] = int2float(mant, expo);

        if (lag == 1) {
            accu_re = real_sum;
            accu_im = imag_sum;
            accu_re += (long long)x[38][0] * x[39][0];
            accu_re += (long long)x[38][1] * x[39][1];
            accu_im += (long long)x[38][0] * x[39][1];
            accu_im -= (long long)x[38][1] * x[39][0];

            i = (int)(accu_re >> 32);
            if (i == 0) {
                nz = 31;
            } else {
                nz = 0;
                while (FFABS(i) < 0x40000000) {
                    i <<= 1;
                    nz++;
                }
            }

            nz = 32-nz;
            round = 1 << (nz-1);
            mant = (int)((accu_re+round) >> nz);
            mant = (mant + 64)>>7;
            mant <<= 6;
            expo = nz + 15;
            phi[0][0][0] = int2float(mant, expo);

            i = (int)(accu_im >> 32);
            if (i == 0) {
                nz = 31;
            } else {
                nz = 0;
                while (FFABS(i) < 0x40000000) {
                    i <<= 1;
                    nz++;
                }
            }

            nz = 32-nz;
            round = 1 << (nz-1);
            mant = (int)((accu_im+round) >> nz);
            mant = (mant + 64)>>7;
            mant <<= 6;
            expo = nz + 15;
            phi[0][0][1] = int2float(mant, expo);
        }
    } else {
        for (i = 1; i < 38; i++) {
            accu_re += (long long)x[i][0] * x[i][0];
            accu_re += (long long)x[i][1] * x[i][1];
        }
        real_sum = accu_re;
        accu_re += (long long)x[ 0][0] * x[ 0][0];
        accu_re += (long long)x[ 0][1] * x[ 0][1];

        i = (int)(accu_re >> 32);
        if (i == 0) {
            nz = 31;
        } else {
            nz = 0;
            while (FFABS(i) < 0x40000000) {
                i <<= 1;
                nz++;
            }
        }

        nz = 32-nz;
        round = 1 << (nz-1);
        mant = (int)((accu_re+round) >> nz);
        mant = (mant + 64)>>7;
        mant <<= 6;
        expo = nz + 15;
        phi[2][1][0] = int2float(mant, expo);

        accu_re = real_sum;
        accu_re += (long long)x[38][0] * x[38][0];
        accu_re += (long long)x[38][1] * x[38][1];

        i = (int)(accu_re >> 32);
        if (i == 0) {
            nz = 31;
        } else {
            nz = 0;
            while (FFABS(i) < 0x40000000) {
                i <<= 1;
                nz++;
            }
        }

        nz = 32-nz;
        round = 1 << (nz-1);
        mant = (int)((accu_re+round) >> nz);
        mant = (mant + 64)>>7;
        mant <<= 6;
        expo = nz + 15;
        phi[1][0][0] = int2float(mant, expo);
    }
}

static void sbr_autocorrelate_c(const float x[40][2], float phi[3][2][2])
{
#if 0
    /* This code is slower because it multiplies memory accesses.
     * It is left for educational purposes and because it may offer
     * a better reference for writing arch-specific DSP functions. */
    autocorrelate(x, phi, 0);
    autocorrelate(x, phi, 1);
    autocorrelate(x, phi, 2);
#else
    float real_sum2 = x[0][0] * x[2][0] + x[0][1] * x[2][1];
    float imag_sum2 = x[0][0] * x[2][1] - x[0][1] * x[2][0];
    float real_sum1 = 0.0f, imag_sum1 = 0.0f, real_sum0 = 0.0f;
    int   i;
    for (i = 1; i < 38; i++) {
        real_sum0 += x[i][0] * x[i    ][0] + x[i][1] * x[i    ][1];
        real_sum1 += x[i][0] * x[i + 1][0] + x[i][1] * x[i + 1][1];
        imag_sum1 += x[i][0] * x[i + 1][1] - x[i][1] * x[i + 1][0];
        real_sum2 += x[i][0] * x[i + 2][0] + x[i][1] * x[i + 2][1];
        imag_sum2 += x[i][0] * x[i + 2][1] - x[i][1] * x[i + 2][0];
    }
    phi[2 - 2][1][0] = real_sum2;
    phi[2 - 2][1][1] = imag_sum2;
    phi[2    ][1][0] = real_sum0 + x[ 0][0] * x[ 0][0] + x[ 0][1] * x[ 0][1];
    phi[1    ][0][0] = real_sum0 + x[38][0] * x[38][0] + x[38][1] * x[38][1];
    phi[2 - 1][1][0] = real_sum1 + x[ 0][0] * x[ 1][0] + x[ 0][1] * x[ 1][1];
    phi[2 - 1][1][1] = imag_sum1 + x[ 0][0] * x[ 1][1] - x[ 0][1] * x[ 1][0];
    phi[0    ][0][0] = real_sum1 + x[38][0] * x[39][0] + x[38][1] * x[39][1];
    phi[0    ][0][1] = imag_sum1 + x[38][0] * x[39][1] - x[38][1] * x[39][0];
#endif
}

static void sbr_autocorrelate_q31_c(const int x[40][2], aac_float_t phi[3][2][2])
{
    autocorrelate_q31(x, phi, 0);
    autocorrelate_q31(x, phi, 1);
    autocorrelate_q31(x, phi, 2);
}

static void sbr_hf_gen_c(float (*X_high)[2], const float (*X_low)[2],
                         const float alpha0[2], const float alpha1[2],
                         float bw, int start, int end)
{
    float alpha[4];
    int i;

    alpha[0] = alpha1[0] * bw * bw;
    alpha[1] = alpha1[1] * bw * bw;
    alpha[2] = alpha0[0] * bw;
    alpha[3] = alpha0[1] * bw;

    for (i = start; i < end; i++) {
        X_high[i][0] =
            X_low[i - 2][0] * alpha[0] -
            X_low[i - 2][1] * alpha[1] +
            X_low[i - 1][0] * alpha[2] -
            X_low[i - 1][1] * alpha[3] +
            X_low[i][0];
        X_high[i][1] =
            X_low[i - 2][1] * alpha[0] +
            X_low[i - 2][0] * alpha[1] +
            X_low[i - 1][1] * alpha[2] +
            X_low[i - 1][0] * alpha[3] +
            X_low[i][1];
    }
}

static void sbr_hf_gen_fixed_c(int (*X_high)[2], const int (*X_low)[2],
                             const int alpha0[2], const int alpha1[2],
                             int bw, int start, int end)
{
    int alpha[4];
    int i;
    long long accu;

    accu = (long long)alpha0[0] * bw;
    alpha[2] = (int)((accu + 0x40000000) >> 31);
    accu = (long long)alpha0[1] * bw;
    alpha[3] = (int)((accu + 0x40000000) >> 31);
    accu = (long long)bw * bw;
    bw = (int)((accu + 0x40000000) >> 31);
    accu = (long long)alpha1[0] * bw;
    alpha[0] = (int)((accu + 0x40000000) >> 31);
    accu = (long long)alpha1[1] * bw;
    alpha[1] = (int)((accu + 0x40000000) >> 31);

    for (i = start; i < end; i++) {
        accu  = (long long)X_low[i][0] * 0x20000000;
        accu += (long long)X_low[i - 2][0] * alpha[0];
        accu -= (long long)X_low[i - 2][1] * alpha[1];
        accu += (long long)X_low[i - 1][0] * alpha[2];
        accu -= (long long)X_low[i - 1][1] * alpha[3];
        X_high[i][0] = (int)((accu + 0x10000000) >> 29);

        accu  = (long long)X_low[i][1] * 0x20000000;
        accu += (long long)X_low[i - 2][1] * alpha[0];
        accu += (long long)X_low[i - 2][0] * alpha[1];
        accu += (long long)X_low[i - 1][1] * alpha[2];
        accu += (long long)X_low[i - 1][0] * alpha[3];
        X_high[i][1] = (int)((accu + 0x10000000) >> 29);
    }
}

static void sbr_hf_g_filt_c(float (*Y)[2], const float (*X_high)[40][2],
                            const float *g_filt, int m_max, intptr_t ixh)
{
    int m;

    for (m = 0; m < m_max; m++) {
        Y[m][0] = X_high[m][ixh][0] * g_filt[m];
        Y[m][1] = X_high[m][ixh][1] * g_filt[m];
    }
}

static void sbr_hf_g_filt_int_c(int (*Y)[2], const int (*X_high)[40][2],
                            const aac_float_t *g_filt, int m_max, intptr_t ixh)
{
    int m, r;
    long long accu;

    for (m = 0; m < m_max; m++) {
        r = 1 << (22-g_filt[m].expo);
        accu = (long long)X_high[m][ixh][0] * ((g_filt[m].mant + 64)>>7);
        Y[m][0] = (int)((accu + r) >> (23-g_filt[m].expo));

        accu = (long long)X_high[m][ixh][1] * ((g_filt[m].mant + 64)>>7);
        Y[m][1] = (int)((accu + r) >> (23-g_filt[m].expo));
    }
}

static av_always_inline void sbr_hf_apply_noise(float (*Y)[2],
                                                const float *s_m,
                                                const float *q_filt,
                                                int noise,
                                                float phi_sign0,
                                                float phi_sign1,
                                                int m_max)
{
    int m;

    for (m = 0; m < m_max; m++) {
        float y0 = Y[m][0];
        float y1 = Y[m][1];
        noise = (noise + 1) & 0x1ff;
        if (s_m[m]) {
            y0 += s_m[m] * phi_sign0;
            y1 += s_m[m] * phi_sign1;
        } else {
            y0 += q_filt[m] * ff_sbr_noise_table[noise][0];
            y1 += q_filt[m] * ff_sbr_noise_table[noise][1];
        }
        Y[m][0] = y0;
        Y[m][1] = y1;
        phi_sign1 = -phi_sign1;
    }
}

static av_always_inline void sbr_hf_apply_noise_int(int (*Y)[2],
                                                const aac_float_t *s_m,
                                                const aac_float_t *q_filt,
                                                int noise,
                                                int phi_sign0,
                                                int phi_sign1,
                                                int m_max)
{
    int m;

    for (m = 0; m < m_max; m++) {
        int y0 = Y[m][0];
        int y1 = Y[m][1];
        noise = (noise + 1) & 0x1ff;
        if (s_m[m].mant) {
            int shift, round;

            shift = 22 - s_m[m].expo;
            if (shift < 30) {
                round = 1 << (shift-1);
                y0 += (s_m[m].mant * phi_sign0 + round) >> shift;
                y1 += (s_m[m].mant * phi_sign1 + round) >> shift;
            }
        } else {
            int shift, round, tmp;
            long long accu;

            shift = 22 - q_filt[m].expo;
            if (shift < 30) {
                round = 1 << (shift-1);

                accu = (long long)q_filt[m].mant * ff_sbr_noise_table_fixed[noise][0];
                tmp = (int)((accu + 0x40000000) >> 31);
                y0 += (tmp + round) >> shift;

                accu = (long long)q_filt[m].mant * ff_sbr_noise_table_fixed[noise][1];
                tmp = (int)((accu + 0x40000000) >> 31);
                y1 += (tmp + round) >> shift;
            }
        }
        Y[m][0] = y0;
        Y[m][1] = y1;
        phi_sign1 = -phi_sign1;
    }
}

static void sbr_hf_apply_noise_0(float (*Y)[2], const float *s_m,
                                 const float *q_filt, int noise,
                                 int kx, int m_max)
{
    sbr_hf_apply_noise(Y, s_m, q_filt, noise, 1.0, 0.0, m_max);
}

static void sbr_hf_apply_noise_0_int(int (*Y)[2], const aac_float_t *s_m,
                                 const aac_float_t *q_filt, int noise,
                                 int kx, int m_max)
{
    sbr_hf_apply_noise_int(Y, s_m, q_filt, noise, 1, 0, m_max);
}

static void sbr_hf_apply_noise_1(float (*Y)[2], const float *s_m,
                                 const float *q_filt, int noise,
                                 int kx, int m_max)
{
    float phi_sign = 1 - 2 * (kx & 1);
    sbr_hf_apply_noise(Y, s_m, q_filt, noise, 0.0, phi_sign, m_max);
}

static void sbr_hf_apply_noise_1_int(int (*Y)[2], const aac_float_t *s_m,
                                 const aac_float_t *q_filt, int noise,
                                 int kx, int m_max)
{
    int phi_sign = 1 - 2 * (kx & 1);
    sbr_hf_apply_noise_int(Y, s_m, q_filt, noise, 0, phi_sign, m_max);
}

static void sbr_hf_apply_noise_2(float (*Y)[2], const float *s_m,
                                 const float *q_filt, int noise,
                                 int kx, int m_max)
{
    sbr_hf_apply_noise(Y, s_m, q_filt, noise, -1.0, 0.0, m_max);
}

static void sbr_hf_apply_noise_2_int(int (*Y)[2], const aac_float_t *s_m,
                                 const aac_float_t *q_filt, int noise,
                                 int kx, int m_max)
{
    sbr_hf_apply_noise_int(Y, s_m, q_filt, noise, -1, 0, m_max);
}

static void sbr_hf_apply_noise_3(float (*Y)[2], const float *s_m,
                                 const float *q_filt, int noise,
                                 int kx, int m_max)
{
    float phi_sign = 1 - 2 * (kx & 1);
    sbr_hf_apply_noise(Y, s_m, q_filt, noise, 0.0, -phi_sign, m_max);
}

static void sbr_hf_apply_noise_3_int(int (*Y)[2], const aac_float_t *s_m,
                                 const aac_float_t *q_filt, int noise,
                                 int kx, int m_max)
{
    int phi_sign = 1 - 2 * (kx & 1);
    sbr_hf_apply_noise_int(Y, s_m, q_filt, noise, 0, -phi_sign, m_max);
}

av_cold void ff_sbrdsp_init(SBRDSPContext *s)
{
    s->sum64x5 = sbr_sum64x5_c;
    s->sum64x5_fixed = sbr_sum64x5_fixed_c;
    s->sum_square = sbr_sum_square_c;
    s->sum_square_fixed = sbr_sum_square_fixed_c;
    s->neg_odd_64 = sbr_neg_odd_64_c;
    s->neg_odd_64_fixed = sbr_neg_odd_64_fixed_c;
    s->qmf_pre_shuffle = sbr_qmf_pre_shuffle_c;
    s->qmf_pre_shuffle_fixed = sbr_qmf_pre_shuffle_fixed_c;
    s->qmf_post_shuffle = sbr_qmf_post_shuffle_c;
    s->qmf_post_shuffle_fixed = sbr_qmf_post_shuffle_fixed_c;
    s->qmf_deint_neg = sbr_qmf_deint_neg_c;
    s->qmf_deint_neg_fixed = sbr_qmf_deint_neg_fixed_c;
    s->qmf_deint_bfly = sbr_qmf_deint_bfly_c;
    s->qmf_deint_bfly_fixed = sbr_qmf_deint_bfly_fixed_c;
    s->autocorrelate = sbr_autocorrelate_c;
    s->autocorrelate_q31 = sbr_autocorrelate_q31_c;
    s->hf_gen = sbr_hf_gen_c;
    s->hf_gen_fixed = sbr_hf_gen_fixed_c;
    s->hf_g_filt = sbr_hf_g_filt_c;
    s->hf_g_filt_int = sbr_hf_g_filt_int_c;

    s->hf_apply_noise[0] = sbr_hf_apply_noise_0;
    s->hf_apply_noise[1] = sbr_hf_apply_noise_1;
    s->hf_apply_noise[2] = sbr_hf_apply_noise_2;
    s->hf_apply_noise[3] = sbr_hf_apply_noise_3;

    s->hf_apply_noise_int[0] = sbr_hf_apply_noise_0_int;
    s->hf_apply_noise_int[1] = sbr_hf_apply_noise_1_int;
    s->hf_apply_noise_int[2] = sbr_hf_apply_noise_2_int;
    s->hf_apply_noise_int[3] = sbr_hf_apply_noise_3_int;

    if (ARCH_ARM)
        ff_sbrdsp_init_arm(s);
    if (ARCH_X86)
        ff_sbrdsp_init_x86(s);
    if (ARCH_MIPS)
        ff_sbrdsp_init_mips(s);
}
