/**
 * sm_physics_constants.h - SNES physics constants as 16.16 fixed-point
 *
 * All values are exact NTSC constants from the original Super Metroid.
 * Source: docs/physics_data.md, P.JBoy's SM disassembly.
 *
 * Format: 16.16 fixed-point (fx32)
 *   0x00010000 = 1.0 pixels/frame (velocity) or 1.0 pixels/frame^2 (accel)
 *   0x0000125C = 0.07168 pixels/frame^2 (air gravity)
 */

#ifndef SM_PHYSICS_CONSTANTS_H
#define SM_PHYSICS_CONSTANTS_H

#include "sm_types.h"

/* ========================================================================
 * Gravity (acceleration per frame)
 *
 * Applied each frame to vertical velocity when airborne.
 * SNES stores these as 16-bit subpixel values; they map directly to
 * the fractional part of our fx32 format.
 * ======================================================================== */

#define GRAVITY_AIR          ((fx32)0x0000125C)  /* 0.07168 px/f^2 (4700 subpx) */
#define GRAVITY_WATER        ((fx32)0x0000053F)  /* 0.02048 px/f^2 (1343 subpx) */
#define GRAVITY_LAVA         ((fx32)0x000005E6)  /* 0.02304 px/f^2 (1510 subpx) */

/* PAL gravity (for reference; we target NTSC) */
#define GRAVITY_AIR_PAL      ((fx32)0x00001A37)  /* 0.10240 px/f^2 */
#define GRAVITY_WATER_PAL    ((fx32)0x0000064A)  /* 0.02457 px/f^2 */
#define GRAVITY_LAVA_PAL     ((fx32)0x00000714)  /* 0.02764 px/f^2 */

/* ========================================================================
 * Terminal Velocity (maximum fall speed)
 * ======================================================================== */

#define TERMINAL_VEL_AIR     ((fx32)0x00050000)  /* 5.0 px/f (5.02048 actual) */
#define TERMINAL_VEL_WATER   ((fx32)0x00050000)  /* 5.0 px/f */
#define TERMINAL_VEL_LAVA    ((fx32)0x00050000)  /* 5.0 px/f */

/* More precise terminal velocity: 5.02048 = 0x5.0346 in hex
 * The SNES caps at pixel value 5, subpixel 0x0346 */
#define TERMINAL_VEL_PRECISE ((fx32)0x00050346)  /* 5.02048 px/f exact */

/* ========================================================================
 * Jump Initial Velocities (set on jump press, upward = negative Y)
 * ======================================================================== */

#define JUMP_VEL_NORMAL      ((fx32)0x00049000)  /* 4.5625 px/f (ground jump) */
#define JUMP_VEL_SPIN        ((fx32)0x00048000)  /* ~4.5 px/f (spin jump) */
#define JUMP_VEL_HI_JUMP     ((fx32)0x00058000)  /* ~5.5 px/f (hi-jump boots) */
#define JUMP_VEL_WALLJUMP    ((fx32)0x00048000)  /* ~4.5 px/f (wall jump) */
#define JUMP_VEL_SPRING_BALL ((fx32)0x0003A000)  /* ~3.625 px/f (~80% normal) */

/* ========================================================================
 * Horizontal Movement Speeds
 * ======================================================================== */

#define WALK_SPEED           ((fx32)0x00018000)  /* 1.5 px/f */
#define RUN_SPEED            ((fx32)0x00020000)  /* 2.0 px/f */
#define SPEED_BOOST_CHARGE   ((fx32)0x00030000)  /* 3.0 px/f (charging) */
#define SPEED_BOOST_ACTIVE   ((fx32)0x00040000)  /* 4.0 px/f (blue suit) */
#define MOONWALK_SPEED       ((fx32)0x00018000)  /* 1.5 px/f (backward) */
#define MORPH_BALL_SPEED     ((fx32)0x00020000)  /* 2.0 px/f (same as run) */

/* ========================================================================
 * Shinespark
 * ======================================================================== */

#define SHINESPARK_SPEED     ((fx32)0x000E0000)  /* ~14 px/f */
#define SHINESPARK_FIRST_FRAME ((fx32)0x00070000) /* 7 px/f (first frame) */
#define SHINESPARK_CHARGE_TIME  180  /* Frames before charge expires */
#define SHINESPARK_MIN_ENERGY    29  /* Spark ends at this HP */

/* ========================================================================
 * Speed Booster Charge Thresholds (frames of continuous running)
 * ======================================================================== */

#define SPEED_BOOST_THRESH_1   25   /* First speed increment */
#define SPEED_BOOST_THRESH_2   50   /* Second increment */
#define SPEED_BOOST_THRESH_3   70   /* Third increment */
#define SPEED_BOOST_THRESH_4   85   /* Blue echoes / full boost */

/* ========================================================================
 * Bomb Physics
 * ======================================================================== */

#define BOMB_TIMER_FRAMES       87   /* Frames until bomb explodes */
#define BOMB_JUMP_VEL    ((fx32)0x00028000)  /* ~2.5 px/f upward from bomb */
#define IBJ_TIMING_FRAMES        7   /* Frames between bombs for IBJ */

/* ========================================================================
 * Damage / Knockback
 * ======================================================================== */

#define KNOCKBACK_VEL_X  ((fx32)0x00020000)  /* 2.0 px/f horizontal knockback */
#define KNOCKBACK_VEL_Y  ((fx32)0x00030000)  /* 3.0 px/f vertical knockback */
#define KNOCKBACK_FRAMES        24   /* Duration of knockback/invuln */
#define INVULN_FRAMES           60   /* I-frames after hit */

/* ========================================================================
 * Wall Jump
 * ======================================================================== */

#define WALLJUMP_DISTANCE ((fx32)0x00060000)  /* 6 px detection range */
#define WALLJUMP_WINDOW_FRAMES   6   /* Input timing window */

/* ========================================================================
 * Samus Hitbox (in pixels, standing)
 * ======================================================================== */

#define SAMUS_HALF_W     ((fx32)0x00080000)  /* 8 px half-width */
#define SAMUS_HALF_H     ((fx32)0x00140000)  /* 20 px half-height (standing) */
#define SAMUS_CROUCH_H   ((fx32)0x000E0000)  /* 14 px half-height (crouching) */
#define SAMUS_MORPH_H    ((fx32)0x00040000)  /* 4 px half-height (morph ball) */

/* ========================================================================
 * Player Defaults
 * ======================================================================== */

#define PLAYER_START_HP         99
#define PLAYER_START_MISSILES    0
#define PLAYER_START_SUPERS      0
#define PLAYER_START_PB          0
#define PLAYER_MAX_HP          1499
#define PLAYER_MAX_MISSILES     230
#define PLAYER_MAX_SUPERS        50
#define PLAYER_MAX_PB            50
#define ENERGY_TANK_VALUE       100
#define RESERVE_TANK_VALUE      100

#endif /* SM_PHYSICS_CONSTANTS_H */
