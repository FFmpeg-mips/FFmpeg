/*
 * MPEG-4 Parametric Stereo decoding functions
 * Copyright (c) 2010 Alex Converse <alex.converse@gmail.com>
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

#define CONFIG_AAC_FIXED 1

#include "aacps.c"

static void stereo_processing_fixed(PSContext *ps, int (*l)[32][2], int (*r)[32][2], int is34)
{
    int e, b, k;
    long long accu;
    int (*H11)[PS_MAX_NUM_ENV+1][PS_MAX_NR_IIDICC] = ps->H11;
    int (*H12)[PS_MAX_NUM_ENV+1][PS_MAX_NR_IIDICC] = ps->H12;
    int (*H21)[PS_MAX_NUM_ENV+1][PS_MAX_NR_IIDICC] = ps->H21;
    int (*H22)[PS_MAX_NUM_ENV+1][PS_MAX_NR_IIDICC] = ps->H22;
    int8_t *opd_hist = ps->opd_hist;
    int8_t *ipd_hist = ps->ipd_hist;
    int8_t iid_mapped_buf[PS_MAX_NUM_ENV][PS_MAX_NR_IIDICC];
    int8_t icc_mapped_buf[PS_MAX_NUM_ENV][PS_MAX_NR_IIDICC];
    int8_t ipd_mapped_buf[PS_MAX_NUM_ENV][PS_MAX_NR_IIDICC];
    int8_t opd_mapped_buf[PS_MAX_NUM_ENV][PS_MAX_NR_IIDICC];
    int8_t (*iid_mapped)[PS_MAX_NR_IIDICC] = iid_mapped_buf;
    int8_t (*icc_mapped)[PS_MAX_NR_IIDICC] = icc_mapped_buf;
    int8_t (*ipd_mapped)[PS_MAX_NR_IIDICC] = ipd_mapped_buf;
    int8_t (*opd_mapped)[PS_MAX_NR_IIDICC] = opd_mapped_buf;
    const int8_t *k_to_i = is34 ? k_to_i_34 : k_to_i_20;
    const int (*H_LUT)[8][4] = (PS_BASELINE || ps->icc_mode < 3) ? HA : HB;

    //Remapping
    if (ps->num_env_old) {
        memcpy(H11[0][0], H11[0][ps->num_env_old], PS_MAX_NR_IIDICC*sizeof(H11[0][0][0]));
        memcpy(H11[1][0], H11[1][ps->num_env_old], PS_MAX_NR_IIDICC*sizeof(H11[1][0][0]));
        memcpy(H12[0][0], H12[0][ps->num_env_old], PS_MAX_NR_IIDICC*sizeof(H12[0][0][0]));
        memcpy(H12[1][0], H12[1][ps->num_env_old], PS_MAX_NR_IIDICC*sizeof(H12[1][0][0]));
        memcpy(H21[0][0], H21[0][ps->num_env_old], PS_MAX_NR_IIDICC*sizeof(H21[0][0][0]));
        memcpy(H21[1][0], H21[1][ps->num_env_old], PS_MAX_NR_IIDICC*sizeof(H21[1][0][0]));
        memcpy(H22[0][0], H22[0][ps->num_env_old], PS_MAX_NR_IIDICC*sizeof(H22[0][0][0]));
        memcpy(H22[1][0], H22[1][ps->num_env_old], PS_MAX_NR_IIDICC*sizeof(H22[1][0][0]));
    }

    if (is34) {
        remap34(&iid_mapped, ps->iid_par, ps->nr_iid_par, ps->num_env, 1);
        remap34(&icc_mapped, ps->icc_par, ps->nr_icc_par, ps->num_env, 1);
        if (ps->enable_ipdopd) {
            remap34(&ipd_mapped, ps->ipd_par, ps->nr_ipdopd_par, ps->num_env, 0);
            remap34(&opd_mapped, ps->opd_par, ps->nr_ipdopd_par, ps->num_env, 0);
        }
        if (!ps->is34bands_old) {
            map_val_20_to_34(H11[0][0]);
            map_val_20_to_34(H11[1][0]);
            map_val_20_to_34(H12[0][0]);
            map_val_20_to_34(H12[1][0]);
            map_val_20_to_34(H21[0][0]);
            map_val_20_to_34(H21[1][0]);
            map_val_20_to_34(H22[0][0]);
            map_val_20_to_34(H22[1][0]);
            ipdopd_reset(ipd_hist, opd_hist);
        }
    } else {
        remap20(&iid_mapped, ps->iid_par, ps->nr_iid_par, ps->num_env, 1);
        remap20(&icc_mapped, ps->icc_par, ps->nr_icc_par, ps->num_env, 1);
        if (ps->enable_ipdopd) {
            remap20(&ipd_mapped, ps->ipd_par, ps->nr_ipdopd_par, ps->num_env, 0);
            remap20(&opd_mapped, ps->opd_par, ps->nr_ipdopd_par, ps->num_env, 0);
        }
        if (ps->is34bands_old) {
            map_val_34_to_20_fixed(H11[0][0]);
            map_val_34_to_20_fixed(H11[1][0]);
            map_val_34_to_20_fixed(H12[0][0]);
            map_val_34_to_20_fixed(H12[1][0]);
            map_val_34_to_20_fixed(H21[0][0]);
            map_val_34_to_20_fixed(H21[1][0]);
            map_val_34_to_20_fixed(H22[0][0]);
            map_val_34_to_20_fixed(H22[1][0]);
            ipdopd_reset(ipd_hist, opd_hist);
        }
    }

    //Mixing
    for (e = 0; e < ps->num_env; e++) {
        for (b = 0; b < NR_PAR_BANDS[is34]; b++) {
            int h11, h12, h21, h22;

            h11 = H_LUT[iid_mapped[e][b] + 7 + 23 * ps->iid_quant][icc_mapped[e][b]][0];
            h12 = H_LUT[iid_mapped[e][b] + 7 + 23 * ps->iid_quant][icc_mapped[e][b]][1];
            h21 = H_LUT[iid_mapped[e][b] + 7 + 23 * ps->iid_quant][icc_mapped[e][b]][2];
            h22 = H_LUT[iid_mapped[e][b] + 7 + 23 * ps->iid_quant][icc_mapped[e][b]][3];
            if (!PS_BASELINE && ps->enable_ipdopd && b < ps->nr_ipdopd_par) {
                //The spec say says to only run this smoother when enable_ipdopd
                //is set but the reference decoder appears to run it constantly
                int h11i, h12i, h21i, h22i;
                int ipd_adj_re, ipd_adj_im;
                int opd_idx = opd_hist[b] * 8 + opd_mapped[e][b];
                int ipd_idx = ipd_hist[b] * 8 + ipd_mapped[e][b];
                int opd_re = pd_re_smooth[opd_idx];
                int opd_im = pd_im_smooth[opd_idx];
                int ipd_re = pd_re_smooth[ipd_idx];
                int ipd_im = pd_im_smooth[ipd_idx];
                opd_hist[b] = opd_idx & 0x3F;
                ipd_hist[b] = ipd_idx & 0x3F;

                accu  = (long long)opd_re*ipd_re;
                accu += (long long)opd_im*ipd_im;
                ipd_adj_re = (int)((accu + 0x20000000) >> 30);
                accu  = (long long)opd_im*ipd_re;
                accu -= (long long)opd_re*ipd_im;
                ipd_adj_im = (int)((accu + 0x20000000) >> 30);
                accu  = (long long)h11 * opd_im;
                h11i = (int)((accu + 0x20000000) >> 30);
                accu  = (long long)h11 * opd_re;
                h11 = (int)((accu + 0x20000000) >> 30);
                accu  = (long long)h12 * ipd_adj_im;
                h12i = (int)((accu + 0x20000000) >> 30);
                accu  = (long long)h12 * ipd_adj_re;
                h12 = (int)((accu + 0x20000000) >> 30);
                accu  = (long long)h21 * opd_im;
                h21i = (int)((accu + 0x20000000) >> 30);
                accu  = (long long)h21 * opd_re;
                h21 = (int)((accu + 0x20000000) >> 30);
                accu  = (long long)h22 * ipd_adj_im;
                h22i = (int)((accu + 0x20000000) >> 30);
                accu  = (long long)h22 * ipd_adj_re;
                h22 = (int)((accu + 0x20000000) >> 30);
                H11[1][e+1][b] = h11i;
                H12[1][e+1][b] = h12i;
                H21[1][e+1][b] = h21i;
                H22[1][e+1][b] = h22i;
            }
            H11[0][e+1][b] = h11;
            H12[0][e+1][b] = h12;
            H21[0][e+1][b] = h21;
            H22[0][e+1][b] = h22;
        }
        for (k = 0; k < NR_BANDS[is34]; k++) {
            int h[2][4];
            int h_step[2][4];
            int start = ps->border_position[e];
            int stop  = ps->border_position[e+1];
            int width = 1073741824 / (stop - start);
            width <<= 1;
            b = k_to_i[k];
            h[0][0] = H11[0][e][b];
            h[0][1] = H12[0][e][b];
            h[0][2] = H21[0][e][b];
            h[0][3] = H22[0][e][b];
            if (!PS_BASELINE && ps->enable_ipdopd) {
            //Is this necessary? ps_04_new seems unchanged
                if ((is34 && k <= 13 && k >= 9) || (!is34 && k <= 1)) {
                    h[1][0] = -H11[1][e][b];
                    h[1][1] = -H12[1][e][b];
                    h[1][2] = -H21[1][e][b];
                    h[1][3] = -H22[1][e][b];
                } else {
                    h[1][0] = H11[1][e][b];
                    h[1][1] = H12[1][e][b];
                    h[1][2] = H21[1][e][b];
                    h[1][3] = H22[1][e][b];
                }
            }
            //Interpolation
            accu  = (long long)H11[0][e+1][b] * width;
            accu -= (long long)h[0][0] * width;
            h_step[0][0] = (int)((accu + 0x40000000) >> 31);
            accu  = (long long)H12[0][e+1][b] * width;
            accu -= (long long)h[0][1] * width;
            h_step[0][1] = (int)((accu + 0x40000000) >> 31);
            accu  = (long long)H21[0][e+1][b] * width;
            accu -= (long long)h[0][2] * width;
            h_step[0][2] = (int)((accu + 0x40000000) >> 31);
            accu  = (long long)H22[0][e+1][b] * width;
            accu -= (long long)h[0][3] * width;
            h_step[0][3] = (int)((accu + 0x40000000) >> 31);
            if (!PS_BASELINE && ps->enable_ipdopd) {
                accu  = (long long)H11[1][e+1][b] * width;
                accu -= (long long)h[1][0] * width;
                h_step[1][0] = (int)((accu + 0x40000000) >> 31);
                accu  = (long long)H12[1][e+1][b] * width;
                accu -= (long long)h[1][1] * width;
                h_step[1][1] = (int)((accu + 0x40000000) >> 31);
                accu  = (long long)H21[1][e+1][b] * width;
                accu -= (long long)h[1][2] * width;
                h_step[1][2] = (int)((accu + 0x40000000) >> 31);
                accu  = (long long)H22[1][e+1][b] * width;
                accu -= (long long)h[1][3] * width;
                h_step[1][3] = (int)((accu + 0x40000000) >> 31);
            }
            ps->dsp.stereo_interpolate_fixed[!PS_BASELINE && ps->enable_ipdopd](
                l[k] + start + 1, r[k] + start + 1,
                h, h_step, stop - start);
        }
    }
}
