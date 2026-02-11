# Super Metroid Physics Data

## Overview

Super Metroid tracks positions and velocities with high precision (1/65536th of a pixel). The physics system runs at 60 FPS (NTSC) or 50 FPS (PAL) with different physics constants for each region.

**CRITICAL**: For frame-perfect accuracy, the DSi port must use these exact values.

---

## RAM Addresses

### Player Position

| Address | Size | Description |
|---------|------|-------------|
| $7E0AF6-$7E0AF7 | 2 bytes | Samus X position (pixels) |
| $7E0AF8-$7E0AF9 | 2 bytes | Samus X subpixel position |
| $7E0AFA-$7E0AFB | 2 bytes | Samus Y position (pixels) |
| $7E0AFC-$7E0AFD | 2 bytes | Samus Y subpixel position |
| $7E0AFE-$7E0AFF | 2 bytes | Samus X radius (hitbox) |
| $7E0B00-$7E0B01 | 2 bytes | Samus Y radius (hitbox) |

### Player Velocity

| Address | Size | Description |
|---------|------|-------------|
| $7E0B2C-$7E0B2D | 2 bytes | Vertical speed (subpixel/frame, 1/65536) |
| $7E0B2E-$7E0B2F | 2 bytes | Vertical speed (pixels/frame) |
| $7E0B42-$7E0B43 | 2 bytes | Horizontal speed forward (pixels/frame) |
| $7E0B44-$7E0B45 | 2 bytes | Horizontal speed forward (subpixel/frame) |
| $7E0B46-$7E0B47 | 2 bytes | Horizontal momentum (pixels/frame) |
| $7E0B48-$7E0B49 | 2 bytes | Horizontal momentum (subpixel/frame) |

### Speed Booster & Special States

| Address | Size | Description |
|---------|------|-------------|
| $7E0B3F | 1 byte | Speed boost level (0-4) |
| $7E0A68 | 1 byte | Shinespark charge timer |

---

## Gravity / Acceleration Constants

### Vertical Acceleration (per frame)

The gravity value is subtracted from rising speed, or added to falling speed, each frame.

| Region | Medium | Value (pixels/frame) | Subpixel Value |
|--------|--------|---------------------|----------------|
| **NTSC** | Air | 0.07168 | 4,700 ($125C) |
| **NTSC** | Water | 0.02048 | 1,343 ($053F) |
| **NTSC** | Lava/Acid | 0.02304 | 1,510 ($05E6) |
| **PAL** | Air | 0.10240 | 6,711 ($1A37) |
| **PAL** | Water | 0.02457 | 1,610 ($064A) |
| **PAL** | Lava/Acid | 0.02764 | 1,812 ($0714) |

### Subpixel Calculation

The game uses 16.16 fixed-point math:
- Upper 16 bits = pixel value
- Lower 16 bits = subpixel (fractional)
- Subpixel value 65536 = 1 pixel

**Conversion**: `pixels = subpixel_value / 65536`

---

## Terminal Velocity (Fall Speed Cap)

Maximum falling speed before gravity stops accelerating.

| Region | Medium | Cap (pixels/frame) | Frames to Reach |
|--------|--------|-------------------|-----------------|
| **NTSC** | Air | 5.02048 | 46 |
| **NTSC** | Water | 5.00000 | 160 |
| **NTSC** | Lava/Acid | 5.01792 | 143 |
| **PAL** | Air | 6.06144 | 39 |
| **PAL** | Water | 6.02361 | 161 |
| **PAL** | Lava/Acid | 6.02036 | 143 |

---

## Jump Physics

### Initial Jump Velocities

When jump is pressed, vertical velocity is set to these values (rising):

| Jump Type | Region | Initial Velocity (pixels/frame) |
|-----------|--------|-------------------------------|
| Normal (ground) | NTSC | 4.57344 |
| Normal (ground) | PAL | 5.55552 |
| Spin jump | NTSC | ~4.5 (varies) |
| Hi-Jump | NTSC | ~5.5 (varies) |
| Wall jump | NTSC | ~4.5 |

### Jump Mechanics

1. **Variable Height**: Holding jump maintains upward velocity longer
2. **Deceleration**: Rising speed decreases by gravity each frame (~7168 subpixels/frame NTSC)
3. **Direction Change**: When velocity crosses zero, Samus transitions from rising to falling
4. **Space Jump**: Allows infinite spin jumps while in spin state

### Jump Height Calculation

```
Initial velocity: V0 = 4.57344 pixels/frame (NTSC ground jump)
Gravity: g = 0.07168 pixels/frame²
Max height: H = V0² / (2 * g) ≈ 146 pixels
```

---

## Horizontal Movement

### Ground Movement Speeds

| Action | Speed (pixels/frame) |
|--------|---------------------|
| Walking | 1.5 |
| Running | 2.0 |
| Running (Speed Booster charging) | 3.0 |
| Running (Speed Booster active) | 4.0 |
| Moonwalk | 1.5 (backward) |

### Air Control

- Air horizontal acceleration is reduced compared to ground
- Momentum is preserved from ground speed
- Direction change in air is slower

### Speed Booster Mechanics

1. Begin running with Run button held
2. Speed booster charges over ~85 frames (NTSC)
3. Charge increments at frames: 25, 50, 70, 85
4. Blue echoes appear after increment 4
5. Minimum charge distance: 163.1875 pixels (NTSC), 157.668 pixels (PAL)

---

## Shinespark

### Shinespark Values

| Property | Value |
|----------|-------|
| Speed | ~14 pixels/frame (first frame: 7 pixels) |
| Energy drain | 1 energy per frame |
| Minimum energy | 29 (spark ends at 29 or below) |
| Maximum damage | 150 per frame to bosses |

### Shinespark Charge

- Store shinespark by pressing down while speed boosted
- Charge lasts ~180 frames before expiring
- Can be retriggered by landing during shinespark

---

## Collision Physics

### Tile Collision Types

| Type ID | Description |
|---------|-------------|
| $00 | Air (no collision) |
| $01-$0F | Solid (various) |
| $10-$1F | Slopes (various angles) |
| $20-$2F | Special blocks |
| $30+ | Hazards, water, etc. |

### Slope Physics

- Slopes push Samus up/down based on angle
- Speed is adjusted when running on slopes
- Collision detection uses 4 corner points

### Wall Jump

- Requires being within 6 pixels of wall
- Timing window: several frames
- Pushes away from wall with jump velocity

---

## Morph Ball Physics

### Morph Ball Properties

| Property | Value |
|----------|-------|
| Hitbox height | ~8 pixels (vs ~40 standing) |
| Hitbox width | ~8 pixels |
| Speed | Same as running |
| Momentum | Preserved when entering morph |

### Bomb Jump

- Bomb timer: 87 frames to explosion
- Bomb propels Samus upward
- IBJ (Infinite Bomb Jump): Lay bombs at intervals of ~7 frames
- Height gain: ~15-21 pixels per jump

### Spring Ball

- Allows jumping in morph ball form
- Jump height: ~80% of normal jump

---

## Mockball

Special technique: Morph while running, preserving run speed in morph ball.

**Execution**:
1. Run to gain speed
2. Jump
3. Morph while maintaining forward momentum
4. Land with running speed in morph ball

---

## Frame Data Quick Reference

| Action | Frames |
|--------|--------|
| Normal jump duration | ~46 frames rising, ~46 falling |
| Shinespark charge | ~85 frames |
| Speed booster activation | 85 frames |
| Bomb timer | 87 frames |
| IBJ timing | ~7 frames between bombs |
| Wall jump window | ~6 frames |

---

## DSi Port Implementation Notes

### Fixed-Point Math

The DSi ARM9 processor can use integer math efficiently:
```c
// 16.16 fixed point
typedef int32_t fixed_t;
#define FIXED_SHIFT 16
#define TO_FIXED(x) ((x) << FIXED_SHIFT)
#define FROM_FIXED(x) ((x) >> FIXED_SHIFT)

// Position update
position.x += velocity.x; // Both are fixed_t
```

### Physics Loop

```c
void update_physics(void) {
    // Apply gravity
    if (!on_ground) {
        velocity.y += GRAVITY_NTSC; // 0x125C in fixed point
        if (velocity.y > TERMINAL_VELOCITY) {
            velocity.y = TERMINAL_VELOCITY;
        }
    }

    // Apply horizontal movement
    position.x += velocity.x;
    position.y += velocity.y;

    // Collision detection and response
    handle_collisions();
}
```

### Critical Accuracy Points

1. Use exact NTSC gravity value: 4,700 subpixels ($125C)
2. Use exact terminal velocity: ~5.02 pixels/frame
3. Use exact jump initial velocity: ~4.57 pixels/frame
4. Implement subpixel precision (16.16 fixed point minimum)
5. Frame-by-frame physics must match original for speedrun techniques
