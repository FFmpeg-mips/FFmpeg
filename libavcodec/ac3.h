/*
 * Common code between the AC-3 encoder and decoder
 * Copyright (c) 2000, 2001, 2002 Fabrice Bellard
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
 * Common code between the AC-3 encoder and decoder.
 */

#ifndef AVCODEC_AC3_H
#define AVCODEC_AC3_H

#define AC3_MAX_CODED_FRAME_SIZE 3840 /* in bytes */
#define AC3_MAX_CHANNELS 7            /**< maximum number of channels, including coupling channel */
#define CPL_CH 0                      /**< coupling channel index */

#define AC3_MAX_COEFS   256
#define AC3_BLOCK_SIZE  256
#define AC3_MAX_BLOCKS    6
#define AC3_FRAME_SIZE (AC3_MAX_BLOCKS * 256)
#define AC3_WINDOW_SIZE (AC3_BLOCK_SIZE * 2)
#define AC3_CRITICAL_BANDS 50
#define AC3_MAX_CPL_BANDS  18

#include "libavutil/opt.h"
#include "avcodec.h"
#include "ac3tab.h"

/* exponent encoding strategy */
#define EXP_REUSE 0
#define EXP_NEW   1

#define EXP_D15   1
#define EXP_D25   2
#define EXP_D45   3

#ifndef CONFIG_AC3_FIXED
#define CONFIG_AC3_FIXED 0
#endif

#if CONFIG_AC3_FIXED

#define CONFIG_FFT_FLOAT 0

#define FIXR(a)                 ((int)((a) * 0 + 0.5))
#define FIXR12(a)               ((int)((a) * 4096 + 0.5))
#define FIXR15(a)               ((int)((a) * 32768 + 0.5))
#define FIXR16(a)               ((int)((a) * 65536 + 0.5))
#define ROUND12(x)              ((x) + 2048) >> 12
#define ROUND15(x)              ((x) + 16384) >> 15

#define AC3_RENAME(x)           x ## _fixed
#define AC3_NORM(norm)          (1<<12)/(norm)
#define AC3_MUL(a,b)            (((int64_t) (a)) * (b))
#define AC3_DYNAMIC_RANGE(x)    (x)
#define AC3_SPX_BLEND(x)        (x)
#define TYPE_PREFIX(x)          fixed_ ## x
#define AC3_DYNAMIC_RANGE1      0
#define INTFLOAT                int
#define SHORTFLOAT              int16_t

#else /* CONFIG_AC3_FIXED */

#define FIXR(x)                 ((float)(x))
#define FIXR12(x)               ((float)(x))
#define FIXR15(x)               ((float)(x))
#define FIXR16(x)               ((float)(x))
#define ROUND12(x)              (x)
#define ROUND15(x)              (x)

#define AC3_RENAME(x)           x
#define AC3_NORM(norm)          (1.0f/(norm))
#define AC3_MUL(a,b)            ((a) * (b))
#define AC3_DYNAMIC_RANGE(x)    ((dynamic_range_tab[x] - 1.0) * s->drc_scale) + 1.0
#define AC3_SPX_BLEND(x)        (x)* (1.0f/32)
#define TYPE_PREFIX(x)          float_ ## x

#define AC3_DYNAMIC_RANGE1      1.0f
#define INTFLOAT                float
#define SHORTFLOAT              float

#endif /* CONFIG_AC3_FIXED */

/* pre-defined gain values */
#define LEVEL_PLUS_3DB          FIXR12(1.4142135623730950)
#define LEVEL_PLUS_1POINT5DB    FIXR12(1.1892071150027209)
#define LEVEL_MINUS_1POINT5DB   FIXR12(0.8408964152537145)
#define LEVEL_MINUS_3DB         FIXR12(0.7071067811865476)
#define LEVEL_MINUS_4POINT5DB   FIXR12(0.5946035575013605)
#define LEVEL_MINUS_6DB         FIXR12(0.5000000000000000)
#define LEVEL_MINUS_9DB         FIXR12(0.3535533905932738)
#define LEVEL_ZERO              FIXR12(0.0000000000000000)
#define LEVEL_ONE               FIXR12(1.0000000000000000)

#define MUL_BIAS1               FIXR16(1.0f    )
#define MUL_BIAS2               FIXR16(32767.0f)

#define AC3_LEVEL(x)            ROUND15((x) * FIXR15(LEVEL_MINUS_3DB))

/** Delta bit allocation strategy */
typedef enum {
    DBA_REUSE = 0,
    DBA_NEW,
    DBA_NONE,
    DBA_RESERVED
} AC3DeltaStrategy;

/** Channel mode (audio coding mode) */
typedef enum {
    AC3_CHMODE_DUALMONO = 0,
    AC3_CHMODE_MONO,
    AC3_CHMODE_STEREO,
    AC3_CHMODE_3F,
    AC3_CHMODE_2F1R,
    AC3_CHMODE_3F1R,
    AC3_CHMODE_2F2R,
    AC3_CHMODE_3F2R
} AC3ChannelMode;

typedef struct AC3BitAllocParameters {
    int sr_code;
    int sr_shift;
    int slow_gain, slow_decay, fast_decay, db_per_bit, floor;
    int cpl_fast_leak, cpl_slow_leak;
} AC3BitAllocParameters;

/**
 * @struct AC3HeaderInfo
 * Coded AC-3 header values up to the lfeon element, plus derived values.
 */
typedef struct AC3HeaderInfo {
    /** @name Coded elements
     * @{
     */
    uint16_t sync_word;
    uint16_t crc1;
    uint8_t sr_code;
    uint8_t bitstream_id;
    uint8_t bitstream_mode;
    uint8_t channel_mode;
    uint8_t lfe_on;
    uint8_t frame_type;
    int substreamid;                        ///< substream identification
    int center_mix_level;                   ///< Center mix level index
    int surround_mix_level;                 ///< Surround mix level index
    uint16_t channel_map;
    int num_blocks;                         ///< number of audio blocks
    /** @} */

    /** @name Derived values
     * @{
     */
    uint8_t sr_shift;
    uint16_t sample_rate;
    uint32_t bit_rate;
    uint8_t channels;
    uint16_t frame_size;
    uint64_t channel_layout;
    /** @} */
} AC3HeaderInfo;

typedef enum {
    EAC3_FRAME_TYPE_INDEPENDENT = 0,
    EAC3_FRAME_TYPE_DEPENDENT,
    EAC3_FRAME_TYPE_AC3_CONVERT,
    EAC3_FRAME_TYPE_RESERVED
} EAC3FrameType;

void ff_ac3_common_init(void);

/**
 * Calculate the log power-spectral density of the input signal.
 * This gives a rough estimate of signal power in the frequency domain by using
 * the spectral envelope (exponents).  The psd is also separately grouped
 * into critical bands for use in the calculating the masking curve.
 * 128 units in psd = -6 dB.  The dbknee parameter in AC3BitAllocParameters
 * determines the reference level.
 *
 * @param[in]  exp        frequency coefficient exponents
 * @param[in]  start      starting bin location
 * @param[in]  end        ending bin location
 * @param[out] psd        signal power for each frequency bin
 * @param[out] band_psd   signal power for each critical band
 */
void ff_ac3_bit_alloc_calc_psd(int8_t *exp, int start, int end, int16_t *psd,
                               int16_t *band_psd);

/**
 * Calculate the masking curve.
 * First, the excitation is calculated using parameters in s and the signal
 * power in each critical band.  The excitation is compared with a predefined
 * hearing threshold table to produce the masking curve.  If delta bit
 * allocation information is provided, it is used for adjusting the masking
 * curve, usually to give a closer match to a better psychoacoustic model.
 *
 * @param[in]  s            adjustable bit allocation parameters
 * @param[in]  band_psd     signal power for each critical band
 * @param[in]  start        starting bin location
 * @param[in]  end          ending bin location
 * @param[in]  fast_gain    fast gain (estimated signal-to-mask ratio)
 * @param[in]  is_lfe       whether or not the channel being processed is the LFE
 * @param[in]  dba_mode     delta bit allocation mode (none, reuse, or new)
 * @param[in]  dba_nsegs    number of delta segments
 * @param[in]  dba_offsets  location offsets for each segment
 * @param[in]  dba_lengths  length of each segment
 * @param[in]  dba_values   delta bit allocation for each segment
 * @param[out] mask         calculated masking curve
 * @return returns 0 for success, non-zero for error
 */
int ff_ac3_bit_alloc_calc_mask(AC3BitAllocParameters *s, int16_t *band_psd,
                               int start, int end, int fast_gain, int is_lfe,
                               int dba_mode, int dba_nsegs, uint8_t *dba_offsets,
                               uint8_t *dba_lengths, uint8_t *dba_values,
                               int16_t *mask);

#endif /* AVCODEC_AC3_H */
