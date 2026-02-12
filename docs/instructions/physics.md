# Physics Engine Implementation Instructions (M8)

## PURPOSE

Implement the core physics engine for Super Metroid DS. This module handles gravity, velocity integration, and tile-based collision resolution using exact SNES NTSC 16.16 fixed-point values.

**CRITICALITY**: This is the MOST CRITICAL module in the project. Frame-perfect accuracy determines whether speedrun techniques (mockball, infinite bomb jump, short charge) work correctly. Even 1 subpixel error can break advanced techniques.

---

## DEPENDENCIES

### Required Headers
```c
#include <nds.h>
#include "sm_types.h"              // fx32, Vec2fx, AABBfx
#include "sm_physics_constants.h"  // GRAVITY_AIR, TERMINAL_VEL_PRECISE, etc.
#include "fixed_math.h"             // fx_mul, fx_div, fx_abs, fx_min, fx_max, fx_clamp
#include "physics.h"                // PhysicsBody, ContactFlags, EnvType
#include "room.h"                   // room_get_collision, room_get_bts
```

### Module Load Order
1. `sm_types.h` - base fixed-point types
2. `fixed_math.h` - fixed-point math operations
3. `sm_physics_constants.h` - physics constants
4. `room.h` - tile query functions
5. `physics.h` - physics body declarations (THIS MODULE implements these functions)

---

## SHARED TYPES (must match project-wide definitions)

```c
// Fixed-point type (16.16 format)
typedef int32_t fx32;
#define FX_SHIFT 16
#define FX_ONE (1 << FX_SHIFT)              // 0x10000
#define INT_TO_FX(i) ((fx32)((i) << FX_SHIFT))
#define FX_TO_INT(f) ((int)((f) >> FX_SHIFT))

// Tile constants
#define TILE_SIZE 16
#define TILE_SHIFT 4  // log2(16) for fast division

// 2D vector in 16.16 fixed-point
typedef struct {
    fx32 x;
    fx32 y;
} Vec2fx;

// Axis-aligned bounding box (stored as half-widths for efficient collision)
typedef struct {
    fx32 half_w;  // Half-width (center to edge)
    fx32 half_h;  // Half-height (center to edge)
} AABBfx;

// Environment type (determines gravity)
typedef enum {
    ENV_AIR = 0,
    ENV_WATER,
    ENV_LAVA
} EnvType;

// Contact flags (reset each frame, set by collision resolver)
typedef struct {
    bool on_ground;       // Touching solid tile below
    bool on_ceiling;      // Touching solid tile above
    bool on_wall_left;    // Touching solid tile on left
    bool on_wall_right;   // Touching solid tile on right
    bool in_water;        // Inside water tile
    bool on_slope;        // Standing on slope (future: slope resolution)
    int slope_angle;      // Slope angle in degrees (0-90, future use)
} ContactFlags;

// Physics body (represents any moving entity)
typedef struct {
    Vec2fx pos;           // Center position (16.16 fixed-point, world coordinates)
    Vec2fx vel;           // Velocity in pixels/frame (16.16 fixed-point)
    Vec2fx accel;         // Acceleration (user-controlled, not gravity)
    AABBfx hitbox;        // Collision box half-widths
    ContactFlags contact; // Collision state (reset each frame)
    EnvType env;          // Current environment (determines gravity)
} PhysicsBody;
```

---

## PHYSICS CONSTANTS (exact SNES NTSC values)

These values MUST be used exactly as specified. They are stored in `include/sm_physics_constants.h`.

### Gravity (acceleration per frame)
```c
#define GRAVITY_AIR          0x0000125C  // 4,700 subpixels = 0.07168 px/frame^2
#define GRAVITY_WATER        0x0000053F  // 1,343 subpixels = 0.02048 px/frame^2
#define GRAVITY_LAVA         0x000005E6  // 1,510 subpixels = 0.02304 px/frame^2
```

**Origin**: SNES RAM $7E09E4 (gravity constant applied each frame).

### Terminal Velocity (maximum fall speed)
```c
#define TERMINAL_VEL_PRECISE 0x00050346  // 329,542 subpixels = 5.02048 px/frame
#define TERMINAL_VEL_WATER   0x00050000  // 327,680 subpixels = 5.00000 px/frame
#define TERMINAL_VEL_LAVA    0x00050250  // 328,272 subpixels = 5.01792 px/frame
```

**Derivation**: 46 frames to reach terminal velocity in air (46 * 4700 = 216,200 subpixels, but SNES caps at 329,542).

### Jump Velocities (initial upward velocity)
```c
#define JUMP_VEL_NORMAL      0x00049000  // 299,008 subpixels = 4.57344 px/frame (upward)
#define JUMP_VEL_SPIN        0x00048000  // ~4.5 px/frame (spin jump, varies by state)
#define JUMP_VEL_HIJUMP      0x00058000  // ~5.5 px/frame (with Hi-Jump Boots)
#define JUMP_VEL_WALLJUMP    0x00048000  // ~4.5 px/frame (wall jump)
```

**Usage**: Set `body->vel.y = -JUMP_VEL_NORMAL` (negative = upward). Gravity decelerates each frame.

### Horizontal Speeds
```c
#define WALK_SPEED           0x00018000  // 98,304 subpixels = 1.5 px/frame
#define RUN_SPEED            0x00020000  // 131,072 subpixels = 2.0 px/frame
#define SPEEDBOOST_CHARGE    0x00030000  // 196,608 subpixels = 3.0 px/frame
#define SPEEDBOOST_ACTIVE    0x00040000  // 262,144 subpixels = 4.0 px/frame
```

---

## ALGORITHM: physics_update_body()

**Function signature**:
```c
void physics_update_body(PhysicsBody* body);
```

**Purpose**: Update a physics body for one frame (60 FPS). Called once per frame per entity.

**Steps** (in exact order):
```
1. physics_clear_contacts(body)         -- Zero all contact flags
2. physics_apply_gravity(body)          -- Add gravity to Y velocity
3. physics_integrate(body)              -- Apply velocity to position (pos += vel)
4. physics_resolve_collisions(body)     -- Tile collision detection and push-out
```

**Constraints**:
- No floating-point math allowed (ARM9 has no FPU)
- No dynamic memory allocation (called 60 times/second)
- No recursion (DTCM stack is ~16 KB total)
- Must complete in < 1 ms for typical entity count (~20 active entities)

**Implementation**:
```c
void physics_update_body(PhysicsBody* body) {
    // Step 1: Clear contact state from previous frame
    physics_clear_contacts(body);

    // Step 2: Apply gravity based on environment
    physics_apply_gravity(body);

    // Step 3: Integrate velocity into position
    physics_integrate(body);

    // Step 4: Resolve collisions with tiles
    physics_resolve_collisions(body);
}
```

---

## ALGORITHM: physics_clear_contacts()

**Function signature**:
```c
static inline void physics_clear_contacts(PhysicsBody* body);
```

**Purpose**: Reset all contact flags before collision detection.

**Implementation**:
```c
static inline void physics_clear_contacts(PhysicsBody* body) {
    body->contact.on_ground = false;
    body->contact.on_ceiling = false;
    body->contact.on_wall_left = false;
    body->contact.on_wall_right = false;
    body->contact.in_water = false;
    body->contact.on_slope = false;
    body->contact.slope_angle = 0;
}
```

**Why**: Contact state is computed fresh each frame. Stale flags cause bugs (e.g., "still on ground" when falling).

---

## ALGORITHM: physics_apply_gravity()

**Function signature**:
```c
static void physics_apply_gravity(PhysicsBody* body);
```

**Purpose**: Add gravity to Y velocity, clamped by terminal velocity.

**Steps**:
```
1. Select gravity constant based on body->env
2. Add gravity to body->vel.y (positive Y = downward)
3. Clamp vel.y to terminal velocity (if falling)
```

**IMPORTANT**: Positive Y is downward. Upward velocity is negative. Gravity always pulls downward (adds positive value).

**Implementation**:
```c
static void physics_apply_gravity(PhysicsBody* body) {
    // Select gravity based on environment
    fx32 gravity;
    fx32 terminal_vel;

    switch (body->env) {
        case ENV_AIR:
            gravity = GRAVITY_AIR;
            terminal_vel = TERMINAL_VEL_PRECISE;
            break;
        case ENV_WATER:
            gravity = GRAVITY_WATER;
            terminal_vel = TERMINAL_VEL_WATER;
            break;
        case ENV_LAVA:
            gravity = GRAVITY_LAVA;
            terminal_vel = TERMINAL_VEL_LAVA;
            break;
        default:
            gravity = GRAVITY_AIR;
            terminal_vel = TERMINAL_VEL_PRECISE;
            break;
    }

    // Apply gravity (add downward acceleration)
    body->vel.y += gravity;

    // Clamp to terminal velocity (only if falling, vel.y > 0)
    if (body->vel.y > terminal_vel) {
        body->vel.y = terminal_vel;
    }
}
```

**Edge case**: Do NOT clamp upward velocity (jumping). Terminal velocity only applies to falling.

---

## ALGORITHM: physics_integrate()

**Function signature**:
```c
static inline void physics_integrate(PhysicsBody* body);
```

**Purpose**: Apply velocity to position (Euler integration).

**Formula**: `pos(t+1) = pos(t) + vel(t) * dt`, where `dt = 1 frame`.

**Implementation**:
```c
static inline void physics_integrate(PhysicsBody* body) {
    body->pos.x += body->vel.x;
    body->pos.y += body->vel.y;
}
```

**Why inline**: Called once per entity per frame. Function call overhead is wasted cycles.

**Note**: Velocity is already in units of `pixels/frame` (16.16 fixed-point), so no dt scaling needed.

---

## ALGORITHM: physics_resolve_collisions()

**Function signature**:
```c
static void physics_resolve_collisions(PhysicsBody* body);
```

**Purpose**: Check tiles overlapping the body's AABB, push out if solid, set contact flags.

**Coordinate system**:
- World space: 16.16 fixed-point pixels (body->pos)
- Tile space: integer tile coordinates (each tile is 16x16 pixels)

**Steps** (detailed):

### Step 1: Compute AABB edges in world space
```c
fx32 left   = body->pos.x - body->hitbox.half_w;
fx32 right  = body->pos.x + body->hitbox.half_w;
fx32 top    = body->pos.y - body->hitbox.half_h;
fx32 bottom = body->pos.y + body->hitbox.half_h;
```

**Example**: Body at (100.5, 50.25) with hitbox (8, 12) -> left=92.5, right=108.5, top=38.25, bottom=62.25

### Step 2: Convert to tile coordinates
```c
// Convert fixed-point to integer pixels, then divide by tile size (16)
int tile_left   = FX_TO_INT(left)   >> TILE_SHIFT;
int tile_right  = FX_TO_INT(right)  >> TILE_SHIFT;
int tile_top    = FX_TO_INT(top)    >> TILE_SHIFT;
int tile_bottom = FX_TO_INT(bottom) >> TILE_SHIFT;
```

**Why `>> TILE_SHIFT` instead of `/16`**: Bit shift is faster and always rounds down (floor).

**Example continued**: left=92.5 -> 92 >> 4 = 5, right=108.5 -> 108 >> 4 = 6, top=38.25 -> 38 >> 4 = 2, bottom=62.25 -> 62 >> 4 = 3

### Step 3: Iterate over overlapping tiles
```c
for (int ty = tile_top; ty <= tile_bottom; ty++) {
    for (int tx = tile_left; tx <= tile_right; tx++) {
        // Query tile collision type
        uint8_t collision = room_get_collision(tx, ty);

        // Skip if not solid (0x00 = air)
        if (collision == 0x00) continue;

        // For now, treat any non-zero as solid
        // TODO: Handle slopes (0x10-0x1F) separately

        // Compute tile bounds in world space
        fx32 tile_x_min = INT_TO_FX(tx * TILE_SIZE);
        fx32 tile_x_max = INT_TO_FX((tx + 1) * TILE_SIZE);
        fx32 tile_y_min = INT_TO_FX(ty * TILE_SIZE);
        fx32 tile_y_max = INT_TO_FX((ty + 1) * TILE_SIZE);

        // Compute penetration depth on each axis
        fx32 pen_left   = right  - tile_x_min;  // Body's right edge into tile's left
        fx32 pen_right  = tile_x_max - left;    // Body's left edge into tile's right
        fx32 pen_top    = bottom - tile_y_min;  // Body's bottom edge into tile's top
        fx32 pen_bottom = tile_y_max - top;     // Body's top edge into tile's bottom

        // Find minimum penetration (shortest push-out distance)
        fx32 min_pen_x = fx_min(pen_left, pen_right);
        fx32 min_pen_y = fx_min(pen_top, pen_bottom);

        // Push out along axis with smallest penetration
        if (min_pen_x < min_pen_y) {
            // Horizontal push-out
            if (pen_left < pen_right) {
                // Pushing left (body hit tile's left edge)
                body->pos.x -= pen_left;
                body->vel.x = 0;  // Stop horizontal movement
                body->contact.on_wall_right = true;
            } else {
                // Pushing right (body hit tile's right edge)
                body->pos.x += pen_right;
                body->vel.x = 0;
                body->contact.on_wall_left = true;
            }
        } else {
            // Vertical push-out
            if (pen_top < pen_bottom) {
                // Pushing up (body hit tile's top edge - floor collision)
                body->pos.y -= pen_top;
                if (body->vel.y > 0) {
                    body->vel.y = 0;  // Stop falling
                }
                body->contact.on_ground = true;
            } else {
                // Pushing down (body hit tile's bottom edge - ceiling collision)
                body->pos.y += pen_bottom;
                if (body->vel.y < 0) {
                    body->vel.y = 0;  // Stop rising
                }
                body->contact.on_ceiling = true;
            }
        }
    }
}
```

**Critical details**:
- Penetration depth is distance from body edge to opposite tile edge
- Push-out moves body center, not edge
- Velocity zeroing prevents "sticky" collisions (body oscillating in/out of tile)
- Only zero velocity in collision direction (don't stop Y vel on wall hit)

### Step 4: Water detection (special tile type)
```c
// After solid collision loop, check center tile for water
int center_tile_x = FX_TO_INT(body->pos.x) >> TILE_SHIFT;
int center_tile_y = FX_TO_INT(body->pos.y) >> TILE_SHIFT;
uint8_t center_collision = room_get_collision(center_tile_x, center_tile_y);

// Check if water tile (SNES collision type 0x06, but verify in level_data.md)
if (center_collision == 0x06) {
    body->contact.in_water = true;
    body->env = ENV_WATER;
} else {
    body->env = ENV_AIR;
}
```

**Why center tile**: Water is region-based, not edge-based. Body is "in water" if center is submerged.

---

## SLOPE HANDLING (deferred to future milestone)

**Current implementation**: Treat slopes as solid blocks (full tile collision).

**Future implementation** (M9 or later):
- Check collision type `0x10-0x1F` (slopes)
- Compute slope angle from tile type
- Project body onto slope surface
- Set `body->contact.on_slope = true`
- Adjust Y position based on X position and angle

**Rationale for deferring**: Slope collision is complex (requires trigonometry or lookup tables) and not needed for basic movement. Flat surfaces validate physics accuracy first.

---

## TEST VECTORS (frame-perfect validation)

These test cases MUST pass exactly. Any deviation breaks speedrun techniques.

### Test 1: Freefall from rest (0-90 frames)

**Setup**:
```c
PhysicsBody body;
body.pos = (Vec2fx){INT_TO_FX(100), INT_TO_FX(50)};
body.vel = (Vec2fx){0, 0};
body.env = ENV_AIR;
body.hitbox = (AABBfx){INT_TO_FX(8), INT_TO_FX(12)};
```

**Expected velocities** (Y component only, X=0 throughout):

| Frame | vel.y (hex) | vel.y (decimal) | vel.y (pixels/frame) |
|-------|-------------|-----------------|----------------------|
| 0     | 0x00000000  | 0               | 0.00000              |
| 1     | 0x0000125C  | 4,700           | 0.07168              |
| 2     | 0x000024B8  | 9,400           | 0.14336              |
| 10    | 0x0000B798  | 47,000          | 0.71680              |
| 20    | 0x00016F30  | 94,000          | 1.43360              |
| 30    | 0x000224C8  | 141,000         | 2.15040              |
| 40    | 0x0002DA60  | 188,000         | 2.86719              |
| 43    | 0x00033F1C  | 212,764         | 3.24670              |
| 44    | 0x00035178  | 217,464         | 3.31837              |
| 45    | 0x000363D4  | 222,164         | 3.39005              |
| 46    | 0x00037630  | 226,864         | 3.46173              |
| 47    | 0x0003888C  | 231,564         | 3.53342              |
| 60    | 0x00050346  | 329,542         | 5.02048 (clamped)    |
| 90    | 0x00050346  | 329,542         | 5.02048 (clamped)    |

**Critical frame: 45**
- Calculated: 45 * 0x125C = 0x51F4C (335,692 subpixels)
- Terminal velocity: 0x50346 (329,542 subpixels)
- Result: Clamped to 0x50346

**Terminal velocity reached at frame 45** (not frame 46).

**Validation**:
```c
// Run simulation for 90 frames
for (int frame = 0; frame <= 90; frame++) {
    physics_update_body(&body);

    // Assert expected velocity (see table above)
    // Example for frame 10:
    if (frame == 10) {
        assert(body.vel.y == 0x0000B798);
    }

    // Example for frame 45 onward:
    if (frame >= 45) {
        assert(body.vel.y == 0x00050346);  // Terminal velocity
    }
}
```

**Why this matters**: Terminal velocity affects fall distance for damage boosts and precise platform landings.

---

### Test 2: Jump height (normal jump, no obstacles)

**Setup**:
```c
PhysicsBody body;
body.pos = (Vec2fx){INT_TO_FX(100), INT_TO_FX(200)};  // Start at Y=200
body.vel = (Vec2fx){0, -JUMP_VEL_NORMAL};              // Initial jump velocity (upward)
body.env = ENV_AIR;
body.hitbox = (AABBfx){INT_TO_FX(8), INT_TO_FX(12)};
```

**Expected behavior**:

| Frame | vel.y (approx) | Notes                          |
|-------|----------------|--------------------------------|
| 0     | -0x49000       | Initial jump velocity          |
| 1     | -0x47DA4       | -0x49000 + 0x125C (gravity)    |
| 2     | -0x46B48       | Slowing down                   |
| ...   | ...            | Velocity decreases each frame  |
| 62    | ~0x00000       | Crosses zero (peak height)     |
| 63    | +0x0000125C    | Falling (positive velocity)    |
| ...   | ...            | Accelerating downward          |

**Peak height calculation**:
- Initial velocity: V0 = 0x49000 = 299,008 subpixels = 4.57344 px/frame
- Gravity: g = 0x125C = 4,700 subpixels = 0.07168 px/frame^2
- Frames to peak: V0 / g = 299,008 / 4,700 ≈ 63.6 frames
- Peak height: Σ(V0 - g*i) for i=0 to 63 ≈ 146 pixels

**Exact peak height** (sum of velocities until zero):
```
H = (V0 * frames_to_peak) - (g * frames_to_peak * (frames_to_peak - 1) / 2)
H = (299,008 * 63.6) - (4,700 * 63.6 * 62.6 / 2)
H ≈ 9,527,500 subpixels ≈ 145.4 pixels
```

**Validation**:
```c
fx32 initial_y = body.pos.y;
fx32 max_height = initial_y;

for (int frame = 0; frame < 200; frame++) {
    physics_update_body(&body);

    // Track peak height
    if (body.pos.y < max_height) {
        max_height = body.pos.y;
    }

    // Break when falling back past initial height
    if (frame > 100 && body.pos.y >= initial_y) {
        break;
    }
}

// Peak height should be ~146 pixels above start
fx32 height_gained = initial_y - max_height;
int height_px = FX_TO_INT(height_gained);
assert(height_px >= 145 && height_px <= 147);  // Allow 1px tolerance for rounding
```

**Why this matters**: Jump height affects access to platforms. Mockball requires precise jump control.

---

### Test 3: Floor collision (falling into solid tile)

**Setup**:
```c
PhysicsBody body;
body.pos = (Vec2fx){INT_TO_FX(100), INT_TO_FX(50)};  // Start above floor at Y=80
body.vel = (Vec2fx){0, TERMINAL_VEL_PRECISE};        // Max fall speed
body.env = ENV_AIR;
body.hitbox = (AABBfx){INT_TO_FX(8), INT_TO_FX(12)};

// Assume floor tile at Y=80 (tile row 5: 80/16=5)
// Tile is solid (collision type 0x01)
```

**Expected behavior**:
1. Frame 0: Body at Y=50, falling at 5.02 px/frame
2. Frame 1: Body at Y=55.02 (50 + 5.02)
3. ...
4. Frame 5: Body at Y=75.1 (about to hit floor at Y=80-12=68, bottom edge)
5. Frame 6: Body pushed up to Y=68 (bottom of hitbox at Y=80), vel.y=0, on_ground=true

**Validation**:
```c
// Mock room with floor at Y=80
// (Assume room_get_collision returns 0x01 for tiles at Y>=80)

for (int frame = 0; frame < 20; frame++) {
    physics_update_body(&body);

    // Check if landed
    if (body.contact.on_ground) {
        // Bottom edge should be at Y=80 (floor level)
        fx32 bottom_edge = body.pos.y + body.hitbox.half_h;
        int bottom_px = FX_TO_INT(bottom_edge);
        assert(bottom_px == 80);  // Exactly on floor
        assert(body.vel.y == 0);  // Velocity zeroed
        break;
    }
}

assert(body.contact.on_ground);  // Must have landed
```

**Why this matters**: Collision resolution accuracy affects damage boost setup and elevator clip techniques.

---

### Test 4: Ceiling collision (jumping into solid tile)

**Setup**:
```c
PhysicsBody body;
body.pos = (Vec2fx){INT_TO_FX(100), INT_TO_FX(100)};  // Start below ceiling at Y=64
body.vel = (Vec2fx){0, -JUMP_VEL_NORMAL};             // Jump upward
body.env = ENV_AIR;
body.hitbox = (AABBfx){INT_TO_FX(8), INT_TO_FX(12)};

// Assume ceiling tile at Y=48 (tile row 3: 48/16=3)
```

**Expected behavior**:
1. Body jumps upward at -4.57 px/frame
2. After ~12 frames, top of hitbox reaches Y=48 (ceiling)
3. Collision resolver pushes body down, zeros upward velocity
4. Body begins falling (gravity applies next frame)

**Validation**:
```c
for (int frame = 0; frame < 50; frame++) {
    physics_update_body(&body);

    if (body.contact.on_ceiling) {
        // Top edge should be at Y=48 (ceiling level)
        fx32 top_edge = body.pos.y - body.hitbox.half_h;
        int top_px = FX_TO_INT(top_edge);
        assert(top_px == 48);
        assert(body.vel.y >= 0);  // Velocity zeroed or falling
        break;
    }
}

assert(body.contact.on_ceiling);  // Must have hit ceiling
```

**Why this matters**: Ceiling clips (e.g., Red Tower) require sub-pixel position adjustment.

---

### Test 5: Wall collision (horizontal movement into solid tile)

**Setup**:
```c
PhysicsBody body;
body.pos = (Vec2fx){INT_TO_FX(100), INT_TO_FX(100)};
body.vel = (Vec2fx){RUN_SPEED, 0};  // Moving right at 2.0 px/frame
body.env = ENV_AIR;
body.hitbox = (AABBfx){INT_TO_FX(8), INT_TO_FX(12)};

// Assume wall tile at X=128 (tile column 8: 128/16=8)
```

**Expected behavior**:
1. Body moves right at 2.0 px/frame
2. After ~14 frames, right edge reaches X=128 (wall)
3. Collision resolver pushes body left, zeros horizontal velocity
4. Body stops moving (vel.x=0, on_wall_right=true)

**Validation**:
```c
for (int frame = 0; frame < 50; frame++) {
    physics_update_body(&body);

    if (body.contact.on_wall_right) {
        // Right edge should be at X=128 (wall)
        fx32 right_edge = body.pos.x + body.hitbox.half_w;
        int right_px = FX_TO_INT(right_edge);
        assert(right_px == 128);
        assert(body.vel.x == 0);  // Stopped
        break;
    }
}

assert(body.contact.on_wall_right);  // Must have hit wall
```

**Why this matters**: Wall collision affects wall jump setup and horizontal IBJ (infinite bomb jump).

---

## CONSTRAINTS (hard requirements)

1. **No floating-point math**: ARM9 has no FPU. Use only `fx32` (16.16 fixed-point) and integer operations.

2. **No dynamic allocation**: No `malloc`, `free`, `new`, `delete`. Physics bodies allocated in static pools by entity manager.

3. **No recursion**: DTCM stack is ~16 KB total. Collision resolution is iterative, not recursive.

4. **O(1) per tile checked**: Collision loop iterates only over tiles overlapping AABB (~4-9 tiles typically). Do NOT iterate entire room.

5. **Pure C**: No C++ features (classes, templates, exceptions, RTTI). POD structs only.

6. **Frame budget**: Target < 1 ms for typical entity count (20 active physics bodies). On 67 MHz ARM9, this is ~67,000 cycles.

7. **No software framebuffer**: Physics does NOT directly modify pixel data. Rendering system reads `body->pos` and draws sprites/tiles.

8. **Fixed-size types**: Use `int32_t`, `uint8_t`, etc. from `<stdint.h>`. Never use `int`, `long` (platform-dependent size).

9. **Contact flags reset each frame**: Stale flags cause bugs (e.g., coyote time extending indefinitely).

10. **Exact SNES constants**: Do NOT "tune" physics values. Even 1 subpixel error breaks speedrun techniques.

---

## PERFORMANCE OPTIMIZATION NOTES

### Cycle counts (ARM946E-S, rough estimates)
- Fixed-point add/subtract: 1 cycle
- Fixed-point multiply (`fx_mul`): ~3-5 cycles (uses `smull`)
- Fixed-point divide (`fx_div`): ~20-30 cycles (software implementation)
- Tile coordinate conversion: 2 cycles (shift right)
- Collision query (`room_get_collision`): ~5-10 cycles (array lookup)

### Hot path analysis
- `physics_update_body()` called 20 times/frame (typical)
- `physics_resolve_collisions()` checks 4-9 tiles per body (typical)
- Total collision queries: 20 * 6 = 120 per frame
- Total cycles: ~120 * 200 = ~24,000 cycles (~0.36 ms at 67 MHz)

**Headroom**: ~43,000 cycles remaining for rendering, AI, input.

### Optimization opportunities
1. **Early-out for static bodies**: If `vel.x == 0 && vel.y == 0`, skip integration and collision.
2. **Cache tile queries**: If body didn't move between frames, reuse previous tile range.
3. **DTCM placement**: Keep `PhysicsBody` array in DTCM (fast 1-cycle access).
4. **Inline small functions**: `physics_clear_contacts()`, `physics_integrate()` are 1-2 instructions.

---

## SLOPE COLLISION (future work, not M8)

**Current implementation**: Treat slopes as solid blocks.

**Future milestone** (M9 or later):
- Detect slope tiles (collision type `0x10-0x1F`)
- Lookup slope angle from tile type (16 slope types: 45°, 30°, 15°, etc.)
- Project body position onto slope surface
- Adjust Y position based on X position within tile
- Set `on_slope` flag and `slope_angle`

**Rationale**: Slope collision is complex and not needed for initial movement validation. Flat surfaces prove physics accuracy first.

**Slope types** (from `level_data.md`):
- `0x10`: 45° slope (rising left-to-right)
- `0x11`: 45° slope (rising right-to-left)
- `0x12-0x13`: 30° slopes
- `0x14-0x17`: 15° slopes
- etc.

**Algorithm** (future):
```
1. if (collision_type >= 0x10 && collision_type <= 0x1F):
2.     angle = slope_lookup_table[collision_type - 0x10]
3.     x_in_tile = (body.pos.x >> FX_SHIFT) % TILE_SIZE
4.     slope_y = compute_slope_y(angle, x_in_tile)
5.     body.pos.y = tile_y_min + slope_y
6.     body.contact.on_slope = true
7.     body.contact.slope_angle = angle
```

---

## INTEGRATION WITH OTHER MODULES

### Entity Manager
- Allocates `PhysicsBody` array in static pool
- Calls `physics_update_body()` for each active entity
- Reads `body->contact` flags to drive animation state (walking, falling, etc.)

### Player Controller
- Sets `body->vel.x` based on input (left/right)
- Sets `body->vel.y = -JUMP_VEL_NORMAL` on jump press
- Checks `body->contact.on_ground` to allow jumping
- Checks `body->contact.on_wall_left/right` for wall jump

### Rendering System
- Reads `body->pos` to compute sprite screen position
- Converts world coordinates to screen coordinates (camera transform)
- Flips sprite based on `body->vel.x` sign (facing direction)

### Room Manager
- Provides `room_get_collision(tx, ty)` tile query function
- Unloads/loads rooms on transition (physics bodies destroyed/recreated)

---

## FILE OUTPUTS (M8 deliverables)

1. **`source/physics.c`**: Core physics implementation
   - `physics_update_body()`
   - `physics_apply_gravity()`
   - `physics_integrate()`
   - `physics_resolve_collisions()`
   - `physics_clear_contacts()`

2. **`include/physics.h`**: Public API (declarations only, types already defined in `sm_types.h`)
   ```c
   void physics_update_body(PhysicsBody* body);
   ```

3. **`source/physics_test.c`**: Test cases (run via debug build, not in release ROM)
   - Test 1: Freefall
   - Test 2: Jump height
   - Test 3: Floor collision
   - Test 4: Ceiling collision
   - Test 5: Wall collision

---

## VALIDATION CHECKLIST

Before marking M8 complete, verify:

- [ ] All 5 test cases pass exactly (no tolerance beyond ±1 subpixel for rounding)
- [ ] Gravity values match SNES constants exactly (0x125C, 0x053F, 0x05E6)
- [ ] Terminal velocity reached at frame 45 in freefall test
- [ ] Jump peak height is 145-147 pixels (tolerance for rounding)
- [ ] Floor collision zeroes Y velocity and sets `on_ground`
- [ ] Ceiling collision zeroes upward Y velocity and sets `on_ceiling`
- [ ] Wall collision zeroes X velocity and sets `on_wall_left/right`
- [ ] No floating-point math used (`grep -r "float\|double" source/physics.c` returns nothing)
- [ ] No dynamic allocation (`grep -r "malloc\|free\|new\|delete" source/physics.c` returns nothing)
- [ ] No recursion (visual inspection of call graph)
- [ ] Performance budget: < 1 ms for 20 bodies per frame (profile on hardware or emulator)

---

## REFERENCES

- `docs/physics_data.md` - SNES physics constants and formulas
- `docs/level_data.md` - Tile collision types
- `include/sm_types.h` - Fixed-point types and macros
- `include/fixed_math.h` - Fixed-point math operations
- [P.JBoy's SM Disassembly](https://patrickjohnston.org/bank/index.html) - SNES physics code
- [Kejardon's RAM Map](https://jathys.zophar.net/supermetroid/kejardon/RAMMap.txt) - SNES RAM addresses

---

## QUESTIONS TO RESOLVE BEFORE STARTING

1. **Tile query API**: Does `room_get_collision(tx, ty)` exist? If not, stub it for testing.
2. **Fixed-point math**: Does `fixed_math.h` provide `fx_min`, `fx_max`, `fx_clamp`? If not, implement inline.
3. **Test harness**: How to run test cases? Debug build with assertions, or separate test ROM?
4. **Coordinate origin**: Is (0,0) top-left or bottom-left? (Assume top-left, SNES convention.)
5. **Hitbox size**: What are typical hitbox dimensions? (Samus standing: 8x12 pixels, morph ball: 4x4 pixels.)

---

## EDGE CASES TO HANDLE

1. **Simultaneous floor and ceiling collision**: Impossible if hitbox height < tile height (16 pixels). Assert if detected.
2. **Multiple tile collisions per frame**: Loop over all overlapping tiles, push out of each in sequence.
3. **Subpixel precision loss**: Fixed-point math can accumulate rounding error. Clamp position after push-out to prevent drift.
4. **Zero-size hitbox**: Assert `half_w > 0 && half_h > 0` at body creation.
5. **Out-of-bounds tile query**: `room_get_collision()` should return 0x00 (air) for invalid coordinates.

---

## SUCCESS CRITERIA

M8 is complete when:
1. All 5 test vectors pass exactly.
2. Samus can walk, jump, and collide with floor/walls in a test room.
3. Physics values match SNES frame-perfect (validated with emulator TAS playback if possible).
4. Performance budget met (< 1 ms per frame on hardware).
5. Code reviewed for constraints (no float, no malloc, no recursion).

---

END OF INSTRUCTION FILE
