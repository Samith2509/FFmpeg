/*
 * Copyright (c) 2026 Samith <samith25092004@gmail.com>
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with FFmpeg; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <string.h>

#include "checkasm.h"
#include "libavcodec/vvc/ctu.h"
#include "libavcodec/vvc/dsp.h"

#include "libavutil/common.h"
#include "libavutil/intreadwrite.h"
#include "libavutil/mem_internal.h"

static const uint32_t pixel_mask[3] = { 0xffffffff, 0x03ff03ff, 0x0fff0fff };

#define SIZEOF_PIXEL ((bit_depth + 7) / 8)
#define BUF_STRIDE   (MAX_TB_SIZE * 2)
#define BUF_SIZE     (BUF_STRIDE * MAX_TB_SIZE)

#define randomize_buffers(buf0, buf1, size)                 \
    do {                                                    \
        uint32_t mask = pixel_mask[(bit_depth - 8) >> 1];   \
        for (int k = 0; k < size; k += 4) {                 \
            uint32_t r = rnd() & mask;                      \
            AV_WN32A(buf0 + k, r);                          \
            AV_WN32A(buf1 + k, r);                          \
        }                                                   \
    } while (0)

static void check_pred_planar(VVCDSPContext *c, const int bit_depth)
{
    LOCAL_ALIGNED_32(uint8_t, dst0, [BUF_SIZE]);
    LOCAL_ALIGNED_32(uint8_t, dst1, [BUF_SIZE]);
    LOCAL_ALIGNED_32(uint8_t, top,  [(MAX_TB_SIZE + 1) * 2]);
    LOCAL_ALIGNED_32(uint8_t, left, [(MAX_TB_SIZE + 1) * 2]);

    const ptrdiff_t stride = BUF_STRIDE / SIZEOF_PIXEL;

    declare_func(void, uint8_t *src, const uint8_t *top, const uint8_t *left,
                 int w, int h, ptrdiff_t stride);

    randomize_buffers(top,  top,  (MAX_TB_SIZE + 1) * 2);
    randomize_buffers(left, left, (MAX_TB_SIZE + 1) * 2);

    for (int h = 4; h <= 64; h <<= 1) {
        for (int w = 4; w <= 64; w <<= 1) {
            if (check_func(c->intra.pred_planar,
                           "vvc_pred_planar_%dx%d_%d", w, h, bit_depth)) {
                memset(dst0, 0, BUF_SIZE);
                memset(dst1, 0, BUF_SIZE);
                call_ref(dst0, top, left, w, h, stride);
                call_new(dst1, top, left, w, h, stride);
                if (memcmp(dst0, dst1, BUF_SIZE))
                    fail();
                bench_new(dst1, top, left, w, h, stride);
            }
        }
    }
}

void checkasm_check_vvc_intra(void)
{
    VVCDSPContext h;

    for (int bit_depth = 8; bit_depth <= 12; bit_depth += 2) {
        ff_vvc_dsp_init(&h, bit_depth);
        check_pred_planar(&h, bit_depth);
    }
    report("pred_planar");
}
