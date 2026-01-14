#ifndef PHYSICS_H
#define PHYSICS_H

#include <stdint.h>
#include <stdbool.h>

// Fixed-point math (16.16 format for subpixel precision)
#define FIXED_SHIFT 16
#define TO_FIXED(x)   ((x) << FIXED_SHIFT)
#define FROM_FIXED(x) ((x) >> FIXED_SHIFT)
#define FIXED_MUL(a, b) (((int64_t)(a) * (b)) >> FIXED_SHIFT)
#define FIXED_DIV(a, b) (((int64_t)(a) << FIXED_SHIFT) / (b))

// Physics constants (NTSC, from ROM analysis)
// All values in 16.16 fixed-point subpixels

// Gravity (pixels/frame²)
#define GRAVITY_AIR_NTSC    0x125C   // 0.07168 px/f²
#define GRAVITY_WATER_NTSC  0x053F   // 0.02048 px/f²
#define GRAVITY_LAVA_NTSC   0x125C   // Same as air
#define GRAVITY_SPACE_NTSC  0x0000   // Zero gravity

// Terminal velocities (pixels/frame)
#define TERMINAL_VELOCITY_AIR    0x50000  // 5.02 px/f
#define TERMINAL_VELOCITY_WATER  0x18000  // 1.5 px/f
#define TERMINAL_VELOCITY_LAVA   0x30000  // 3.0 px/f

// Movement speeds (pixels/frame)
#define WALK_SPEED       0x18000  // 1.5 px/f
#define RUN_SPEED        0x20000  // 2.0 px/f
#define DASH_SPEED       0x28000  // 2.5 px/f (speed booster)

// Jump velocities (pixels/frame)
#define JUMP_INITIAL_VEL      0x49000  // 4.57 px/f
#define JUMP_SPIN_VEL         0x4C000  // 4.75 px/f
#define JUMP_HI_JUMP_BONUS    0x10000  // +1.0 px/f with Hi-Jump Boots
#define WALL_JUMP_VEL         0x45000  // 4.31 px/f

// Acceleration/deceleration (pixels/frame²)
#define ACCEL_GROUND     0x0800   // Ground acceleration
#define ACCEL_AIR        0x0400   // Air acceleration
#define DECEL_GROUND     0x0C00   // Ground deceleration
#define DECEL_AIR        0x0200   // Air deceleration

// Environment types
typedef enum {
    ENV_AIR = 0,
    ENV_WATER,
    ENV_LAVA,
    ENV_ACID,
    ENV_SPACE,
    ENV_QUICKSAND
} EnvironmentType;

// Collision tile types (from BTS data)
typedef enum {
    TILE_AIR = 0x00,
    TILE_SOLID = 0x01,

    // Slopes (45 degree)
    TILE_SLOPE_45_UR = 0x10,  // Rising right
    TILE_SLOPE_45_UL = 0x11,  // Rising left
    TILE_SLOPE_45_DR = 0x12,  // Falling right
    TILE_SLOPE_45_DL = 0x13,  // Falling left

    // Gentle slopes (22.5 degree)
    TILE_SLOPE_GENTLE_START = 0x14,
    TILE_SLOPE_GENTLE_END = 0x1F,

    // Steep slopes (67.5 degree)
    TILE_SLOPE_STEEP_START = 0x20,
    TILE_SLOPE_STEEP_END = 0x2F,

    // Breakable blocks
    TILE_SHOT_BLOCK = 0x30,
    TILE_BOMB_BLOCK = 0x31,
    TILE_POWER_BOMB_BLOCK = 0x32,
    TILE_SUPER_MISSILE_BLOCK = 0x33,
    TILE_SPEED_BLOCK = 0x34,

    // Special blocks
    TILE_CRUMBLE = 0x40,

    // Hazards
    TILE_SPIKE = 0x50,
    TILE_WATER = 0x60,
    TILE_LAVA = 0x70,
    TILE_ACID = 0x78,

    // Interactive
    TILE_GRAPPLE_POINT = 0x80
} TileType;

// Physics flags
typedef enum {
    PHYS_FLAG_ON_GROUND = 0x01,
    PHYS_FLAG_IN_AIR = 0x02,
    PHYS_FLAG_IN_WATER = 0x04,
    PHYS_FLAG_IN_LAVA = 0x08,
    PHYS_FLAG_CROUCHING = 0x10,
    PHYS_FLAG_MORPHBALL = 0x20,
    PHYS_FLAG_WALL_JUMP_LEFT = 0x40,
    PHYS_FLAG_WALL_JUMP_RIGHT = 0x80
} PhysicsFlags;

// Physics body structure
typedef struct {
    int32_t x, y;           // 16.16 fixed-point position (pixels)
    int32_t vx, vy;         // 16.16 fixed-point velocity (pixels/frame)
    uint16_t width, height; // Collision box size (pixels)
    uint8_t flags;          // PhysicsFlags bitmask
    EnvironmentType env;    // Current environment
} PhysicsBody;

// Collision result structure
typedef struct {
    bool hit;
    bool hit_floor;
    bool hit_ceiling;
    bool hit_left_wall;
    bool hit_right_wall;
    TileType tile_type;
    int16_t penetration_x;
    int16_t penetration_y;
} CollisionResult;

// Physics system functions
void physics_init(void);
void physics_shutdown(void);

// Gravity and velocity
void physics_apply_gravity(PhysicsBody* body);
void physics_integrate_velocity(PhysicsBody* body);
void physics_apply_terminal_velocity(PhysicsBody* body);

// Movement
void physics_move_horizontal(PhysicsBody* body, int32_t acceleration);
void physics_apply_friction(PhysicsBody* body);
void physics_jump(PhysicsBody* body, bool spin_jump, bool hi_jump_equipped);
void physics_wall_jump(PhysicsBody* body, bool jump_left);

// Collision detection
CollisionResult physics_check_collision(PhysicsBody* body, int32_t dx, int32_t dy);
bool physics_check_tile_collision(int tile_x, int tile_y, uint16_t width, uint16_t height);
TileType physics_get_tile_type(int tile_x, int tile_y);
EnvironmentType physics_get_environment(int tile_x, int tile_y);

// Slope handling
bool physics_is_slope(TileType tile);
int16_t physics_get_slope_height(TileType tile, int16_t x_offset);

// Utility functions
void physics_set_position(PhysicsBody* body, int32_t x, int32_t y);
void physics_set_velocity(PhysicsBody* body, int32_t vx, int32_t vy);
void physics_get_tile_coords(int32_t x, int32_t y, int* tile_x, int* tile_y);

#endif // PHYSICS_H
