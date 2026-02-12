/**
 * sm_types.h - Core types for Super Metroid DS
 *
 * Master shared types header. Every module includes this.
 * Defines: fx32 (16.16 fixed-point), Vec2fx, AABBfx, enums, pool constants.
 */

#ifndef SM_TYPES_H
#define SM_TYPES_H

#include <nds.h>
#include <stdint.h>
#include <stdbool.h>

/* ========================================================================
 * 16.16 Fixed-Point Arithmetic
 * Upper 16 bits = integer, lower 16 bits = fraction
 * Range: -32768.0 to +32767.99998 (approx)
 * ======================================================================== */

typedef int32_t fx32;

#define FX_SHIFT        16
#define FX_ONE          (1 << FX_SHIFT)         /* 0x00010000 */
#define FX_HALF         (1 << (FX_SHIFT - 1))   /* 0x00008000 */
#define FX_FRAC_MASK    (FX_ONE - 1)            /* 0x0000FFFF */

#define INT_TO_FX(i)    ((fx32)(i) << FX_SHIFT)
#define FX_TO_INT(f)    ((f) >> FX_SHIFT)
#define FX_TO_INT_ROUND(f) (((f) + FX_HALF) >> FX_SHIFT)
#define FLOAT_TO_FX(f)  ((fx32)((f) * FX_ONE))  /* Build-time only! */

/* Multiply two fx32: uses 64-bit intermediate to avoid overflow */
static inline fx32 fx_mul(fx32 a, fx32 b) {
    return (fx32)(((int64_t)a * b) >> FX_SHIFT);
}

/* Divide two fx32: shifts numerator up first for precision */
static inline fx32 fx_div(fx32 a, fx32 b) {
    return (fx32)(((int64_t)a << FX_SHIFT) / b);
}

/* ========================================================================
 * 2D Vector (fixed-point)
 * ======================================================================== */

typedef struct {
    fx32 x;
    fx32 y;
} Vec2fx;

/* ========================================================================
 * Axis-Aligned Bounding Box (fixed-point)
 * Half-extents from center: actual box is center +/- half
 * ======================================================================== */

typedef struct {
    fx32 half_w;
    fx32 half_h;
} AABBfx;

/* ========================================================================
 * Direction / Facing
 * ======================================================================== */

typedef enum {
    DIR_LEFT  = 0,
    DIR_RIGHT = 1,
    DIR_UP    = 2,
    DIR_DOWN  = 3
} Direction;

/* ========================================================================
 * Game States
 * ======================================================================== */

typedef enum {
    STATE_TITLE = 0,
    STATE_FILE_SELECT,
    STATE_GAMEPLAY,
    STATE_PAUSE,
    STATE_MAP,
    STATE_DEATH,
    STATE_ENDING,
    STATE_COUNT
} GameStateID;

/* ========================================================================
 * Pool Size Constants
 * These define maximum active entities during gameplay.
 * All pools are statically allocated at startup.
 * ======================================================================== */

#define MAX_ENEMIES       16
#define MAX_PROJECTILES   32
#define MAX_PARTICLES     48
#define MAX_ITEMS         32
#define MAX_DOORS          8
#define MAX_PLMS          32   /* Post-Load Modifications (breakable blocks, etc.) */

/* ========================================================================
 * OAM Sprite Budget (128 per engine)
 * ======================================================================== */

#define OAM_PLAYER_START    0   /* Samus: sprites 0-7 (up to 8 tiles) */
#define OAM_PLAYER_COUNT    8
#define OAM_ENEMY_START     8   /* Enemies: sprites 8-55 (3 per enemy * 16) */
#define OAM_ENEMY_COUNT    48
#define OAM_PROJ_START     56   /* Projectiles: sprites 56-87 */
#define OAM_PROJ_COUNT     32
#define OAM_PARTICLE_START 88   /* Particles: sprites 88-111 */
#define OAM_PARTICLE_COUNT 24
#define OAM_ITEM_START    112   /* Items/effects: sprites 112-127 */
#define OAM_ITEM_COUNT     16

/* ========================================================================
 * Tile / Collision Constants
 * ======================================================================== */

#define TILE_SIZE          16   /* Metatile size in pixels (SNES uses 16x16) */
#define TILE_SHIFT          4   /* log2(TILE_SIZE) */
#define TILE_SIZE_FX       INT_TO_FX(TILE_SIZE)

/* Collision type categories (upper nibble) */
#define COLL_AIR           0x00
#define COLL_SOLID         0x01
#define COLL_SLOPE_BASE    0x10
#define COLL_SPECIAL_BASE  0x20
#define COLL_SPECIAL_SHOT  0x21   /* Breakable by beam */
#define COLL_SPECIAL_BOMB  0x22   /* Breakable by bomb */
#define COLL_SPECIAL_CRUMBLE 0x23 /* Crumbles when stepped on */
#define COLL_SPECIAL_SAVE  0x24   /* Save station */
#define COLL_HAZARD_BASE   0x30
#define COLL_HAZARD_SPIKE  0x31   /* Spike damage + upward bounce */
#define COLL_HAZARD_LAVA   0x32   /* Periodic damage, changed gravity */

/* Item type IDs */
typedef enum {
    ITEM_NONE = 0,
    ITEM_ENERGY_TANK,
    ITEM_MISSILE_TANK,
    ITEM_SUPER_TANK,
    ITEM_PB_TANK,
    ITEM_MORPH_BALL,
    ITEM_BOMBS,
    ITEM_HI_JUMP,
    ITEM_SPEED_BOOST,
    ITEM_VARIA_SUIT,
    ITEM_GRAVITY_SUIT,
    ITEM_SPACE_JUMP,
    ITEM_SCREW_ATTACK,
    ITEM_CHARGE_BEAM,
    ITEM_ICE_BEAM,
    ITEM_WAVE_BEAM,
    ITEM_SPAZER_BEAM,
    ITEM_PLASMA_BEAM,
    ITEM_GRAPPLE,
    ITEM_XRAY,
    ITEM_RESERVE_TANK,
    ITEM_TYPE_COUNT
} ItemTypeID;

/* Item instance data */
typedef struct {
    ItemTypeID type;
    fx32 x;
    fx32 y;
    bool collected;
} ItemData;

/* ========================================================================
 * Equipment Bitfield
 * ======================================================================== */

#define EQUIP_MORPH_BALL   (1 <<  0)
#define EQUIP_BOMBS        (1 <<  1)
#define EQUIP_SPRING_BALL  (1 <<  2)
#define EQUIP_HI_JUMP      (1 <<  3)
#define EQUIP_SPACE_JUMP   (1 <<  4)
#define EQUIP_SPEED_BOOST  (1 <<  5)
#define EQUIP_SCREW_ATTACK (1 <<  6)
#define EQUIP_VARIA_SUIT   (1 <<  7)
#define EQUIP_GRAVITY_SUIT (1 <<  8)
#define EQUIP_CHARGE_BEAM  (1 <<  9)
#define EQUIP_ICE_BEAM     (1 << 10)
#define EQUIP_WAVE_BEAM    (1 << 11)
#define EQUIP_SPAZER_BEAM  (1 << 12)
#define EQUIP_PLASMA_BEAM  (1 << 13)
#define EQUIP_GRAPPLE      (1 << 14)
#define EQUIP_XRAY         (1 << 15)

#endif /* SM_TYPES_H */
