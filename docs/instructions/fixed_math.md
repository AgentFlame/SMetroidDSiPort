# Fixed-Point Math Library Implementation (M1)

## Purpose

Implement fixed-point math functions for 16.16 fx32 arithmetic on Nintendo DS ARM9 (no FPU). This library provides the core mathematical operations needed for Super Metroid physics, collision, and animation on hardware without floating-point support.

## Context

The Nintendo DS ARM9 CPU (ARM946E-S) has no hardware FPU. Floating-point math uses soft-float emulation which is 10-100x slower than integer operations. All game physics must use 16.16 fixed-point arithmetic where the upper 16 bits represent the integer part and the lower 16 bits represent the fractional part.

## Dependencies

This implementation requires:
- `include/sm_types.h` - Provides fx32 type and basic inline operations
- `include/fixed_math.h` - Contains function declarations to implement

You must `#include "sm_types.h"` and `#include "fixed_math.h"` at the top of `source/fixed_math.c`.

## Shared Type Definitions

These types and macros are already defined in `include/sm_types.h`. Copy them here for reference:

```c
typedef int32_t fx32;

#define FX_SHIFT 16
#define FX_ONE (1 << FX_SHIFT)           // 0x10000 = 1.0
#define FX_HALF (1 << (FX_SHIFT - 1))    // 0x8000 = 0.5

#define INT_TO_FX(i) ((fx32)(i) << FX_SHIFT)
#define FX_TO_INT(f) ((f) >> FX_SHIFT)

// Already provided as inline functions in sm_types.h
static inline fx32 fx_mul(fx32 a, fx32 b) {
    return (fx32)(((int64_t)a * b) >> FX_SHIFT);
}

static inline fx32 fx_div(fx32 a, fx32 b) {
    return (fx32)(((int64_t)a << FX_SHIFT) / b);
}
```

## Function Implementations Required

Implement the following functions in `source/fixed_math.c`. Match the declarations in `fixed_math.h` exactly.

### 1. fx_abs - Absolute Value

```c
fx32 fx_abs(fx32 a);
```

Return the absolute value of `a`. Handle the minimum int32_t value correctly (avoid overflow).

**Implementation hint:**
```c
return (a < 0) ? -a : a;
```

### 2. fx_min - Minimum of Two Values

```c
fx32 fx_min(fx32 a, fx32 b);
```

Return the smaller of `a` and `b`.

**Implementation hint:**
```c
return (a < b) ? a : b;
```

### 3. fx_max - Maximum of Two Values

```c
fx32 fx_max(fx32 a, fx32 b);
```

Return the larger of `a` and `b`.

**Implementation hint:**
```c
return (a > b) ? a : b;
```

### 4. fx_clamp - Clamp Value to Range

```c
fx32 fx_clamp(fx32 val, fx32 lo, fx32 hi);
```

Clamp `val` to the range `[lo, hi]`. If `val < lo`, return `lo`. If `val > hi`, return `hi`. Otherwise return `val`.

**Implementation hint:**
```c
if (val < lo) return lo;
if (val > hi) return hi;
return val;
```

### 5. fx_lerp - Linear Interpolation

```c
fx32 fx_lerp(fx32 a, fx32 b, fx32 t);
```

Linearly interpolate between `a` and `b` using parameter `t` in range `[0, FX_ONE]`.
- When `t = 0`, return `a`
- When `t = FX_ONE`, return `b`
- When `t = FX_HALF`, return midpoint between `a` and `b`

**Formula:** `a + t * (b - a)`

**Implementation hint:**
```c
fx32 diff = b - a;
return a + fx_mul(t, diff);
```

**Warning:** Be careful with overflow if `a` and `b` have large absolute values. The subtraction `b - a` is safe because both are fx32.

### 6. fx_sin - Sine Function (Lookup Table)

```c
fx32 fx_sin(int angle);
```

Compute sine using a 256-entry lookup table. Angle is in the range `[0, 255]` which maps to `[0°, 360°)`.

**Return value:** `[-FX_ONE, FX_ONE]` representing `[-1.0, 1.0]`

**Angle mapping:**
- 0 → 0°
- 64 → 90°
- 128 → 180°
- 192 → 270°
- 255 → ~358.6°

**Implementation approach:**

1. Create a static lookup table with 256 precomputed values:
```c
static const fx32 sin_lut[256] = {
    // Precompute all 256 values at compile time
    // sin(0° * π/128) = 0.0 → 0x0000
    // sin(90° * π/128) = 1.0 → 0x10000
    // sin(180° * π/128) = 0.0 → 0x0000
    // sin(270° * π/128) = -1.0 → 0xFFFF0000
    // ...
};
```

2. Use symmetry to compute values:
   - **First quadrant (0-63):** Direct computation: `sin(angle * 2π / 256)`
   - **Second quadrant (64-127):** `sin(128 - angle)` = mirror of first quadrant
   - **Third quadrant (128-191):** `-sin(angle - 128)` = negative of first quadrant
   - **Fourth quadrant (192-255):** `-sin(256 - angle)` = negative of second quadrant

3. LUT generation: Compute first 64 entries (0-63), derive others by symmetry, or precompute all 256.

**Precomputed values (first few entries):**
- `sin_lut[0]` = 0 (0.0)
- `sin_lut[1]` ≈ 0x0648 (sin(1.40625°) ≈ 0.02454)
- `sin_lut[16]` ≈ 0x18F8 (sin(22.5°) ≈ 0.38268)
- `sin_lut[32]` ≈ 0x2D41 (sin(45°) ≈ 0.70711)
- `sin_lut[64]` = 0x10000 (sin(90°) = 1.0)
- `sin_lut[128]` = 0 (sin(180°) = 0.0)
- `sin_lut[192]` = -0x10000 (sin(270°) = -1.0)

**Formula for generating LUT entries:**
```c
// For entry i in range [0, 255]:
// angle_radians = i * 2 * π / 256
// sin_lut[i] = (int32_t)(sin(angle_radians) * 65536.0)
```

You must compute all 256 values and embed them as a static const array. Use this C code to generate:

```c
#include <math.h>
#include <stdint.h>

// Generate all 256 entries
for (int i = 0; i < 256; i++) {
    double angle_rad = i * 2.0 * M_PI / 256.0;
    int32_t value = (int32_t)(sin(angle_rad) * 65536.0);
    printf("0x%08X, ", (uint32_t)value);
    if ((i + 1) % 8 == 0) printf("\n");
}
```

**Runtime implementation:**
```c
fx32 fx_sin(int angle) {
    // Wrap angle to [0, 255]
    angle = angle & 0xFF;
    return sin_lut[angle];
}
```

### 7. fx_cos - Cosine Function

```c
fx32 fx_cos(int angle);
```

Cosine is sine shifted by 90°. Since 90° = 64 in our angle system:

**Implementation:**
```c
fx32 fx_cos(int angle) {
    return fx_sin(angle + 64);
}
```

The addition and wrapping are handled by `fx_sin`.

### 8. fx_sqrt - Square Root (Newton's Method)

```c
fx32 fx_sqrt(fx32 a);
```

Compute the square root of a fixed-point value using Newton's method (integer-only).

**Return value:** Square root in fx32 format

**Special cases:**
- If `a <= 0`, return 0
- If `a == FX_ONE`, return FX_ONE

**Newton's Method Formula:**
```
x_next = (x + a/x) / 2
```

For fixed-point, we work with the raw integer representation:

**Implementation approach:**

1. Handle edge cases (a <= 0, a == 0)
2. Start with initial guess: `x = a >> 1` (half of input as first guess)
3. Iterate Newton's method 8 times (sufficient for convergence on 32-bit values):
   ```c
   x = (x + a / x) >> 1;
   ```
4. Return `x`

**Important:** The input `a` is already in fx32 format. To get the square root in fx32 format, we need to adjust.

**Correct algorithm:**
```c
fx32 fx_sqrt(fx32 a) {
    if (a <= 0) return 0;
    if (a == FX_ONE) return FX_ONE;

    // Work with the value shifted for proper fixed-point sqrt
    // sqrt(x * 2^16) = sqrt(x) * 2^8
    // So we need to adjust by another shift
    uint32_t val = (uint32_t)a;
    uint32_t x = val;

    // Initial guess
    if (x >= FX_ONE) {
        x = x >> 1;
    }

    // Newton iterations (8 iterations)
    for (int i = 0; i < 8; i++) {
        if (x == 0) break;
        uint32_t x_next = (x + val / x) >> 1;
        if (x_next >= x) break;  // Converged
        x = x_next;
    }

    // Result needs to be shifted back to fx32 format
    // Since input was a*2^16, output is sqrt(a)*2^8
    // We need sqrt(a)*2^16, so shift left by 8
    return (fx32)(x << (FX_SHIFT / 2));
}
```

**Test cases:**
- `fx_sqrt(INT_TO_FX(4))` should return approximately `INT_TO_FX(2)` (0x20000)
- `fx_sqrt(INT_TO_FX(9))` should return approximately `INT_TO_FX(3)` (0x30000)
- `fx_sqrt(FX_ONE)` should return `FX_ONE` (0x10000)

### 9. fx_from_snes - Combine SNES Position Values

```c
fx32 fx_from_snes(int16_t pixel, uint16_t subpixel);
```

Super Metroid stores positions as two separate 16-bit values:
- `pixel`: The integer pixel coordinate (signed 16-bit)
- `subpixel`: The fractional component (unsigned 16-bit)

This function combines them into a single fx32 value.

**Formula:**
```
result = (pixel << 16) | subpixel
```

**Implementation:**
```c
fx32 fx_from_snes(int16_t pixel, uint16_t subpixel) {
    return ((fx32)pixel << 16) | (uint32_t)subpixel;
}
```

**Example:**
- `pixel = 5`, `subpixel = 0x8000` (0.5) → result = `0x00058000` (5.5 in fx32)
- `pixel = -3`, `subpixel = 0x4000` (0.25) → result = `0xFFFD4000` (-2.75 in fx32)

## Implementation Constraints

**CRITICAL RULES:**

1. **Pure C** - No C++ features, no classes, no templates
2. **No floating-point** - Do not use `float` or `double` anywhere, even in comments
3. **No malloc** - All data must be static or stack-allocated
4. **No recursion** - DTCM stack is only ~16KB total
5. **No standard math library** - Do not `#include <math.h>` in the final code (you may use it to generate the LUT offline)
6. **No undefined behavior** - Handle all edge cases (division by zero, overflow, etc.)
7. **Thread-safe** - All functions are pure (no mutable global state)

**Performance considerations:**
- Inline candidates: abs, min, max, clamp (but implement as regular functions for now)
- LUT lookup is O(1)
- Newton's method sqrt is O(1) with fixed iteration count
- All operations should complete in <100 cycles on ARM9

## File Structure

Create `source/fixed_math.c` with this structure:

```c
#include "sm_types.h"
#include "fixed_math.h"

// ============================================================================
// Sine Lookup Table (256 entries, precomputed at compile time)
// ============================================================================

static const fx32 sin_lut[256] = {
    // ... 256 precomputed values ...
};

// ============================================================================
// Basic Operations
// ============================================================================

fx32 fx_abs(fx32 a) {
    // Implementation
}

fx32 fx_min(fx32 a, fx32 b) {
    // Implementation
}

fx32 fx_max(fx32 a, fx32 b) {
    // Implementation
}

fx32 fx_clamp(fx32 val, fx32 lo, fx32 hi) {
    // Implementation
}

fx32 fx_lerp(fx32 a, fx32 b, fx32 t) {
    // Implementation
}

// ============================================================================
// Trigonometric Functions
// ============================================================================

fx32 fx_sin(int angle) {
    // Implementation
}

fx32 fx_cos(int angle) {
    // Implementation
}

// ============================================================================
// Advanced Math
// ============================================================================

fx32 fx_sqrt(fx32 a) {
    // Implementation
}

// ============================================================================
// SNES Compatibility
// ============================================================================

fx32 fx_from_snes(int16_t pixel, uint16_t subpixel) {
    // Implementation
}
```

## Test Criteria

The implementation must pass these tests (to be verified in `main.c`):

### Basic Arithmetic
```c
// Multiplication test
assert(fx_mul(INT_TO_FX(3), INT_TO_FX(4)) == INT_TO_FX(12));
assert(fx_mul(0x18000, 0x18000) == 0x24000);  // 1.5 * 1.5 = 2.25

// Absolute value
assert(fx_abs(-INT_TO_FX(5)) == INT_TO_FX(5));
assert(fx_abs(INT_TO_FX(7)) == INT_TO_FX(7));

// Min/max
assert(fx_min(INT_TO_FX(3), INT_TO_FX(5)) == INT_TO_FX(3));
assert(fx_max(INT_TO_FX(3), INT_TO_FX(5)) == INT_TO_FX(5));

// Clamp
assert(fx_clamp(INT_TO_FX(10), INT_TO_FX(0), INT_TO_FX(5)) == INT_TO_FX(5));
assert(fx_clamp(INT_TO_FX(-5), INT_TO_FX(0), INT_TO_FX(5)) == INT_TO_FX(0));
assert(fx_clamp(INT_TO_FX(3), INT_TO_FX(0), INT_TO_FX(5)) == INT_TO_FX(3));

// Lerp
assert(fx_lerp(INT_TO_FX(0), INT_TO_FX(10), FX_HALF) == INT_TO_FX(5));
assert(fx_lerp(INT_TO_FX(0), INT_TO_FX(10), FX_ONE) == INT_TO_FX(10));
```

### Trigonometric Functions
```c
// Sine tests (approximate within 1 LSB)
assert(fx_abs(fx_sin(0)) < 0x100);                    // sin(0°) ≈ 0
assert(fx_abs(fx_sin(64) - FX_ONE) < 0x100);          // sin(90°) ≈ 1.0
assert(fx_abs(fx_sin(128)) < 0x100);                  // sin(180°) ≈ 0
assert(fx_abs(fx_sin(192) + FX_ONE) < 0x100);         // sin(270°) ≈ -1.0

// Cosine tests
assert(fx_abs(fx_cos(0) - FX_ONE) < 0x100);           // cos(0°) ≈ 1.0
assert(fx_abs(fx_cos(64)) < 0x100);                   // cos(90°) ≈ 0
assert(fx_abs(fx_cos(128) + FX_ONE) < 0x100);         // cos(180°) ≈ -1.0
```

### Square Root
```c
// Square root tests (within 0.1% error)
fx32 sqrt4 = fx_sqrt(INT_TO_FX(4));
assert(fx_abs(sqrt4 - INT_TO_FX(2)) < 0x0666);        // sqrt(4) ≈ 2.0

fx32 sqrt9 = fx_sqrt(INT_TO_FX(9));
assert(fx_abs(sqrt9 - INT_TO_FX(3)) < 0x0666);        // sqrt(9) ≈ 3.0

fx32 sqrt1 = fx_sqrt(FX_ONE);
assert(fx_abs(sqrt1 - FX_ONE) < 0x0666);              // sqrt(1) ≈ 1.0
```

### Physics Simulation Test
```c
// Simulate gravity for 46 frames (from docs/physics_data.md)
#define GRAVITY_AIR 0x125C          // 0.07168 px/frame^2
#define TERMINAL_VEL_PRECISE 0x50346  // 5.02026 px/frame

fx32 vel = 0;
for (int frame = 0; frame < 46; frame++) {
    vel += GRAVITY_AIR;
}

// After 46 frames of gravity, velocity should be near terminal velocity
assert(fx_abs(vel - TERMINAL_VEL_PRECISE) < 0x1000);
```

### SNES Position Conversion
```c
// SNES position conversion
fx32 pos1 = fx_from_snes(5, 0x8000);      // 5.5
assert(pos1 == 0x00058000);

fx32 pos2 = fx_from_snes(0, 0x4000);      // 0.25
assert(pos2 == 0x00004000);

fx32 pos3 = fx_from_snes(-3, 0xC000);     // -2.25 (0xC000 is 0.75)
assert(pos3 == 0xFFFDC000);
```

## Reference Data

From `docs/physics_data.md`, these are the SNES physics constants in 16.16 format:

```c
// Gravity (acceleration)
#define GRAVITY_AIR     0x125C      // 0.07168 px/frame^2
#define GRAVITY_WATER   0x053F      // 0.02048 px/frame^2

// Terminal velocity
#define TERMINAL_VEL    0x50000     // 5.0 px/frame
#define TERMINAL_VEL_PRECISE 0x50346  // 5.02026 px/frame

// Movement speeds
#define WALK_SPEED      0x18000     // 1.5 px/frame
#define RUN_SPEED       0x20000     // 2.0 px/frame
#define JUMP_INITIAL    0x49000     // 4.5625 px/frame
```

## Sine Lookup Table Generator

Use this code **offline** (not in the final source) to generate the 256-entry LUT:

```c
#include <stdio.h>
#include <math.h>
#include <stdint.h>

int main(void) {
    printf("static const fx32 sin_lut[256] = {\n");
    for (int i = 0; i < 256; i++) {
        double angle_rad = i * 2.0 * M_PI / 256.0;
        int32_t value = (int32_t)(sin(angle_rad) * 65536.0);
        printf("    0x%08X,", (uint32_t)value);
        if ((i + 1) % 4 == 0) {
            printf("\n");
        } else {
            printf(" ");
        }
    }
    printf("};\n");
    return 0;
}
```

Compile and run this on your development machine, then copy the output into `source/fixed_math.c`.

## Validation Checklist

Before considering this milestone complete:

- [ ] All functions compile without warnings (`-Wall -Wextra`)
- [ ] No floating-point operations in final code
- [ ] No malloc/free
- [ ] No recursion
- [ ] All test cases pass in `main.c`
- [ ] Sine/cosine values accurate to within 1 LSB
- [ ] Square root accurate to within 0.1%
- [ ] Physics simulation reaches terminal velocity correctly
- [ ] SNES position conversion handles negative values correctly
- [ ] Code follows project style (pure C, clear comments)

## Common Pitfalls

1. **Sine LUT indexing** - Remember to mask angle with `& 0xFF` to keep in [0, 255]
2. **Square root precision** - Need to shift by FX_SHIFT/2 (8 bits) for correct fx32 output
3. **Negative values** - All functions must handle negative fx32 values correctly
4. **Overflow in lerp** - The subtraction `b - a` is safe, but be aware of range
5. **Division by zero** - Handle in fx_sqrt when x becomes 0
6. **Sign extension** - When combining SNES pixel/subpixel, cast pixel to fx32 before shifting

## Success Criteria

This milestone is complete when:
1. `source/fixed_math.c` compiles cleanly
2. All test cases pass
3. No floating-point operations are present
4. Code adheres to all constraints
5. Sine LUT has all 256 entries precomputed
6. Newton's method sqrt converges within 8 iterations

The implementation should be production-ready for use in the Super Metroid physics engine.
