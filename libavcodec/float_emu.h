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
 * 3. Neither the name of the MIPS Technologies, Inc., nor the names of is
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
 * Author:  Stanislav Ocovaj (stanislav.ocovaj imgtec com)
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

#ifndef AVUTIL_FLOAT_EMU_H
#define AVUTIL_FLOAT_EMU_H

#include "libavutil/common.h"
#include "libavutil/intmath.h"

extern const int divTable[128];
extern const int sqrtTab[513];
extern const int sqrExpMultTab[2];
extern const int aac_costbl_1[16];
extern const int aac_costbl_2[32];
extern const int aac_sintbl_2[32];
extern const int aac_costbl_3[32];
extern const int aac_sintbl_3[32];
extern const int aac_costbl_4[33];
extern const int aac_sintbl_4[33];

typedef struct aac_float_t {
    int mant;
    int expo;
} aac_float_t;

static const aac_float_t FLOAT_0          = {         0,   0};
static const aac_float_t FLOAT_05         = { 536870912,   0};
static const aac_float_t FLOAT_1          = { 536870912,   1};
static const aac_float_t FLOAT_EPSILON    = { 703687442, -16};
static const aac_float_t FLOAT_1584893192 = { 850883053,   1};
static const aac_float_t FLOAT_100000     = { 819200000,  17};
static const aac_float_t FLOAT_0999999    = {1073740750,   0};

static __inline aac_float_t int2float(const int x, const int exp)
{
    aac_float_t ret;
    int nz;

    if (x == 0)
    {
        ret.mant = 0;
        ret.expo = 0;
    }
    else
    {
        ret.expo = exp;
        ret.mant = x;
        nz = 29 - ff_log2(FFABS(ret.mant));
        ret.mant <<= nz;
        ret.expo -= nz;
    }

    return ret;
}

static __inline aac_float_t float_add(aac_float_t a, aac_float_t b)
{
    int diff, nz;
    int expa = a.expo;
    int expb = b.expo;
    int manta = a.mant;
    int mantb = b.mant;
    aac_float_t res;

    if (manta == 0)
        return b;

    if (mantb == 0)
        return a;

    diff = expa - expb;
    if (diff < 0)  // expa < expb
    {
        diff = -diff;
        if (diff >= 31)
        manta = 0;
        else if (diff != 0)
        manta >>= diff;
        expa = expb;
    }
    else  // expa >= expb
    {
        if (diff >= 31)
        mantb = 0;
        else if (diff != 0)
        mantb >>= diff;
    }

    manta = manta + mantb;
    if (manta == 0)
        expa = 0;
    else
    {
        nz = 30 - ff_log2(FFABS(manta));
        manta <<= nz;
        manta >>= 1;
        expa -= (nz-1);
    }

    res.mant = manta;
    res.expo = expa;

    return res;
}

static __inline aac_float_t float_sub(aac_float_t a, aac_float_t b)
{
    int diff, nz;
    int expa = a.expo;
    int expb = b.expo;
    int manta = a.mant;
    int mantb = b.mant;
    aac_float_t res;

    if (manta == 0)
    {
        res.mant = -mantb;
        res.expo = expb;
        return res;
    }

    if (mantb == 0)
        return a;

    diff = expa - expb;
    if (diff < 0)  // expa < expb
    {
        diff = -diff;
        if (diff >= 31)
        manta = 0;
        else if (diff != 0)
        manta >>= diff;
        expa = expb;
    }
    else  // expa >= expb
    {
        if (diff >= 31)
        mantb = 0;
        else if (diff != 0)
        mantb >>= diff;
    }

    manta = manta - mantb;
    if (manta == 0)
        expa = 0;
    else
    {
        nz = 30 - ff_log2(FFABS(manta));
        manta <<= nz;
        manta >>= 1;
        expa -= (nz-1);
    }

    res.mant = manta;
    res.expo = expa;

    return res;
}

static __inline aac_float_t float_mul(aac_float_t a, aac_float_t b)
{
    aac_float_t res;
    int mant;
    int expa = a.expo;
    int expb = b.expo;
    long long accu;

    expa = expa + expb;
    accu = (long long)a.mant * b.mant;
    mant = (int)((accu + 0x20000000) >> 30);
    if (mant == 0)
        expa = 0;
    else if (mant < 536870912 && mant > -536870912)
    {
        mant <<= 1;
        expa = expa - 1;
    }
    res.mant = mant;
    res.expo = expa;

    return res;
}

static __inline aac_float_t float_recip(const aac_float_t a)
{
    aac_float_t r;
    int s;
    int manta, expa;

    manta = a.mant;
    expa = a.expo;

    expa = 1 - expa;
    r.expo = expa;

    s = manta >> 31;
    manta = (manta ^ s) - s;

    manta = divTable[(manta - 0x20000000) >> 22];

    r.mant = (manta ^ s) - s;

    return r;
}

static __inline aac_float_t float_div(aac_float_t a, aac_float_t b)
{
    aac_float_t res;
    aac_float_t iB, tmp;
    int mantb;

    mantb = b.mant;
    if (mantb != 0)
    {
        iB = float_recip(b);
        // newton iteration to double precision
        tmp = float_sub(FLOAT_1, float_mul(b, iB));
        iB = float_add(iB, float_mul(iB, tmp));
        res = float_mul(a, iB);
    }
    else
    {
        res.mant = 1;
        res.expo = 2147483647;
    }

    return res;
}

static __inline int float_gt(aac_float_t a, aac_float_t b)
{
    int expa = a.expo;
    int expb = b.expo;
    int manta = a.mant;
    int mantb = b.mant;

    if (manta == 0)
        expa = 0x80000000;

    if (mantb == 0)
        expb = 0x80000000;

    if (expa > expb)
        return 1;
    else if (expa < expb)
        return 0;
    else // expa == expb
    {
        if (manta > mantb)
        return 1;
        else
        return 0;
    }
}

static __inline aac_float_t float_sqrt(aac_float_t val)
{
    int exp;
    int tabIndex, rem;
    int mant;
    long long accu;
    aac_float_t res;

    exp = val.expo;
    mant = val.mant;

    if (mant == 0)
    {
        res.mant = 0;
        res.expo = 0;
    }
    else
    {
        tabIndex = (mant - 536870912);
        tabIndex = tabIndex >> 20;

        rem = mant & 0xfffff;
        accu  = (long long)sqrtTab[tabIndex] * (0x100000-rem);
        accu += (long long)sqrtTab[tabIndex+1] * rem;
        mant = (int)((accu + 0x80000) >> 20);

        accu = (long long)sqrExpMultTab[exp&1] * mant;
        mant = (int)((accu + 0x10000000) >> 29);
        if (mant < 1073741824)
            exp -= 2;
        else
            mant >>= 1;

        res.mant = mant;
        res.expo = (exp>>1)+1;
    }

    return res;
}

static __inline void aac_fixed_sincos(int a, int *s, int *c)
{
    int idx, sign;
    int sv, cv;
    int st, ct;
    long long accu;

    idx = a >> 26;
    sign = (idx << 27) >> 31;
    cv = aac_costbl_1[idx & 0xf];
    cv = (cv ^ sign) - sign;

    idx -= 8;
    sign = (idx << 27) >> 31;
    sv = aac_costbl_1[idx & 0xf];
    sv = (sv ^ sign) - sign;

    idx = a >> 21;
    ct = aac_costbl_2[idx & 0x1f];
    st = aac_sintbl_2[idx & 0x1f];

    accu  = (long long)cv*ct;
    accu -= (long long)sv*st;
    idx = (int)((accu + 0x20000000) >> 30);

    accu  = (long long)cv*st;
    accu += (long long)sv*ct;
    sv = (int)((accu + 0x20000000) >> 30);
    cv = idx;

    idx = a >> 16;
    ct = aac_costbl_3[idx & 0x1f];
    st = aac_sintbl_3[idx & 0x1f];

    accu  = (long long)cv*ct;
    accu -= (long long)sv*st;
    idx = (int)((accu + 0x20000000) >> 30);

    accu  = (long long)cv*st;
    accu += (long long)sv*ct;
    sv = (int)((accu + 0x20000000) >> 30);
    cv = idx;

    idx = a >> 11;
    accu  = (long long)aac_costbl_4[idx & 0x1f]*(0x800 - (a&0x7ff));
    accu += (long long)aac_costbl_4[(idx & 0x1f)+1]*(a&0x7ff);
    ct = (int)((accu + 0x400) >> 11);
    accu  = (long long)aac_sintbl_4[idx & 0x1f]*(0x800 - (a&0x7ff));
    accu += (long long)aac_sintbl_4[(idx & 0x1f)+1]*(a&0x7ff);
    st = (int)((accu + 0x400) >> 11);

    accu  = (long long)cv*ct;
    accu -= (long long)sv*st;
    *c = (int)((accu + 0x20000000) >> 30);

    accu  = (long long)cv*st;
    accu += (long long)sv*ct;
    *s = (int)((accu + 0x20000000) >> 30);
}

#endif /* AVUTIL_FLOAT_EMU_H */
