/**
 * fixed_math.h - 16.16 fixed-point math library
 *
 * Extended math functions beyond the inline basics in sm_types.h.
 * Provides: abs, min, max, clamp, lerp, sin/cos (LUT), sqrt, SNES conversion.
 *
 * Implemented in: source/fixed_math.c (M1)
 */

#ifndef FIXED_MATH_H
#define FIXED_MATH_H

#include "sm_types.h"

/* Absolute value */
fx32 fx_abs(fx32 a);

/* Min / Max */
fx32 fx_min(fx32 a, fx32 b);
fx32 fx_max(fx32 a, fx32 b);

/* Clamp value to [lo, hi] */
fx32 fx_clamp(fx32 val, fx32 lo, fx32 hi);

/* Linear interpolation: a + t*(b-a), t in [0, FX_ONE] */
fx32 fx_lerp(fx32 a, fx32 b, fx32 t);

/* Sine / Cosine using 256-entry lookup table.
 * angle: 0 = 0 deg, 64 = 90 deg, 128 = 180 deg, 192 = 270 deg
 * Returns fx32 in [-FX_ONE, FX_ONE] */
fx32 fx_sin(int angle);
fx32 fx_cos(int angle);

/* Square root (Newton's method, integer) */
fx32 fx_sqrt(fx32 a);

/* Convert SNES subpixel value (16-bit) to fx32.
 * SNES stores position as (pixel:16, subpixel:16).
 * This just reinterprets the combined 32-bit value as fx32. */
fx32 fx_from_snes(int16_t pixel, uint16_t subpixel);

#endif /* FIXED_MATH_H */
