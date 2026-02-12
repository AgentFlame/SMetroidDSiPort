/**
 * fixed_math.c - 16.16 fixed-point math library
 *
 * Extended math functions for Super Metroid DS.
 * All operations use integer arithmetic only (ARM9 has no FPU).
 */

#include "fixed_math.h"

/* ========================================================================
 * Basic Operations
 * ======================================================================== */

fx32 fx_abs(fx32 a) {
    return (a < 0) ? -a : a;
}

fx32 fx_min(fx32 a, fx32 b) {
    return (a < b) ? a : b;
}

fx32 fx_max(fx32 a, fx32 b) {
    return (a > b) ? a : b;
}

fx32 fx_clamp(fx32 val, fx32 lo, fx32 hi) {
    if (val < lo) return lo;
    if (val > hi) return hi;
    return val;
}

fx32 fx_lerp(fx32 a, fx32 b, fx32 t) {
    return a + fx_mul(t, b - a);
}

/* ========================================================================
 * Sine / Cosine Lookup Table
 *
 * 256 entries covering full circle (0-360 degrees).
 * Index: 0=0deg, 64=90deg, 128=180deg, 192=270deg
 * Values: fx32, range [-0x10000, 0x10000] = [-1.0, 1.0]
 *
 * Generated: sin_lut[i] = round(sin(i * 2*PI / 256) * 65536)
 * ======================================================================== */

static const fx32 sin_lut[256] = {
     0x00000, 0x00648, 0x00C90, 0x012D5, 0x01918, 0x01F56, 0x02590, 0x02BC4,
     0x031F1, 0x03817, 0x03E34, 0x04447, 0x04A50, 0x0504D, 0x0563E, 0x05C22,
     0x061F8, 0x067BE, 0x06D74, 0x0731A, 0x078AD, 0x07E2F, 0x0839C, 0x088F6,
     0x08E3A, 0x09368, 0x09880, 0x09D80, 0x0A268, 0x0A736, 0x0ABEB, 0x0B086,
     0x0B505, 0x0B968, 0x0BDAF, 0x0C1D8, 0x0C5E4, 0x0C9D1, 0x0CD9F, 0x0D14D,
     0x0D4DB, 0x0D848, 0x0DB94, 0x0DEBE, 0x0E1C6, 0x0E4AA, 0x0E76C, 0x0EA0A,
     0x0EC83, 0x0EED9, 0x0F109, 0x0F314, 0x0F4FA, 0x0F6BA, 0x0F854, 0x0F9C8,
     0x0FB15, 0x0FC3B, 0x0FD3B, 0x0FE13, 0x0FEC4, 0x0FF4E, 0x0FFB1, 0x0FFEC,
     0x10000, 0x0FFEC, 0x0FFB1, 0x0FF4E, 0x0FEC4, 0x0FE13, 0x0FD3B, 0x0FC3B,
     0x0FB15, 0x0F9C8, 0x0F854, 0x0F6BA, 0x0F4FA, 0x0F314, 0x0F109, 0x0EED9,
     0x0EC83, 0x0EA0A, 0x0E76C, 0x0E4AA, 0x0E1C6, 0x0DEBE, 0x0DB94, 0x0D848,
     0x0D4DB, 0x0D14D, 0x0CD9F, 0x0C9D1, 0x0C5E4, 0x0C1D8, 0x0BDAF, 0x0B968,
     0x0B505, 0x0B086, 0x0ABEB, 0x0A736, 0x0A268, 0x09D80, 0x09880, 0x09368,
     0x08E3A, 0x088F6, 0x0839C, 0x07E2F, 0x078AD, 0x0731A, 0x06D74, 0x067BE,
     0x061F8, 0x05C22, 0x0563E, 0x0504D, 0x04A50, 0x04447, 0x03E34, 0x03817,
     0x031F1, 0x02BC4, 0x02590, 0x01F56, 0x01918, 0x012D5, 0x00C90, 0x00648,
     0x00000,-0x00648,-0x00C90,-0x012D5,-0x01918,-0x01F56,-0x02590,-0x02BC4,
    -0x031F1,-0x03817,-0x03E34,-0x04447,-0x04A50,-0x0504D,-0x0563E,-0x05C22,
    -0x061F8,-0x067BE,-0x06D74,-0x0731A,-0x078AD,-0x07E2F,-0x0839C,-0x088F6,
    -0x08E3A,-0x09368,-0x09880,-0x09D80,-0x0A268,-0x0A736,-0x0ABEB,-0x0B086,
    -0x0B505,-0x0B968,-0x0BDAF,-0x0C1D8,-0x0C5E4,-0x0C9D1,-0x0CD9F,-0x0D14D,
    -0x0D4DB,-0x0D848,-0x0DB94,-0x0DEBE,-0x0E1C6,-0x0E4AA,-0x0E76C,-0x0EA0A,
    -0x0EC83,-0x0EED9,-0x0F109,-0x0F314,-0x0F4FA,-0x0F6BA,-0x0F854,-0x0F9C8,
    -0x0FB15,-0x0FC3B,-0x0FD3B,-0x0FE13,-0x0FEC4,-0x0FF4E,-0x0FFB1,-0x0FFEC,
    -0x10000,-0x0FFEC,-0x0FFB1,-0x0FF4E,-0x0FEC4,-0x0FE13,-0x0FD3B,-0x0FC3B,
    -0x0FB15,-0x0F9C8,-0x0F854,-0x0F6BA,-0x0F4FA,-0x0F314,-0x0F109,-0x0EED9,
    -0x0EC83,-0x0EA0A,-0x0E76C,-0x0E4AA,-0x0E1C6,-0x0DEBE,-0x0DB94,-0x0D848,
    -0x0D4DB,-0x0D14D,-0x0CD9F,-0x0C9D1,-0x0C5E4,-0x0C1D8,-0x0BDAF,-0x0B968,
    -0x0B505,-0x0B086,-0x0ABEB,-0x0A736,-0x0A268,-0x09D80,-0x09880,-0x09368,
    -0x08E3A,-0x088F6,-0x0839C,-0x07E2F,-0x078AD,-0x0731A,-0x06D74,-0x067BE,
    -0x061F8,-0x05C22,-0x0563E,-0x0504D,-0x04A50,-0x04447,-0x03E34,-0x03817,
    -0x031F1,-0x02BC4,-0x02590,-0x01F56,-0x01918,-0x012D5,-0x00C90,-0x00648,
};

fx32 fx_sin(int angle) {
    return sin_lut[angle & 0xFF];
}

fx32 fx_cos(int angle) {
    return sin_lut[(angle + 64) & 0xFF];
}

/* ========================================================================
 * Square Root (Newton's method)
 *
 * For fx32 input x, we want sqrt(x) in fx32.
 * Strategy: compute integer sqrt of (x << FX_SHIFT) to get result in fx32.
 * Uses 64-bit intermediate to handle the shifted value.
 * ======================================================================== */

fx32 fx_sqrt(fx32 a) {
    if (a <= 0) return 0;

    /* We need sqrt(a) where a is in 16.16 fixed-point.
     * Result should also be 16.16.
     * sqrt(a_fixed) = sqrt(a_real * 65536) = sqrt(a_real) * 256
     * But we want the result in 16.16: sqrt(a_real) * 65536
     * So: result = sqrt(a_fixed * 65536) = sqrt(a_fixed << 16) */
    uint64_t val = (uint64_t)a << FX_SHIFT;

    /* Newton's method for integer sqrt of val */
    uint64_t guess = val;

    /* Start with a reasonable initial guess */
    if (guess > (uint64_t)1 << 32) {
        guess = (uint64_t)1 << 24;
    } else {
        guess = (uint64_t)1 << 16;
    }

    for (int i = 0; i < 16; i++) {
        if (guess == 0) break;
        uint64_t next = (guess + val / guess) >> 1;
        if (next >= guess) break;  /* Converged */
        guess = next;
    }

    return (fx32)guess;
}

/* ========================================================================
 * SNES Subpixel Conversion
 *
 * SNES stores position as separate pixel (16-bit) and subpixel (16-bit).
 * Our fx32 format: upper 16 bits = pixel, lower 16 bits = subpixel.
 * Direct combination.
 * ======================================================================== */

fx32 fx_from_snes(int16_t pixel, uint16_t subpixel) {
    return ((fx32)pixel << FX_SHIFT) | (fx32)subpixel;
}
