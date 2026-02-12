/**
 * physics.h - Physics engine
 *
 * Gravity, velocity integration, tile-based collision resolution.
 * All values use 16.16 fixed-point matching exact SNES NTSC constants.
 *
 * Implemented in: source/physics.c (M8)
 */

#ifndef PHYSICS_H
#define PHYSICS_H

#include "sm_types.h"
#include "sm_physics_constants.h"

/* Environment type (determines gravity constant) */
typedef enum {
    ENV_AIR = 0,
    ENV_WATER,
    ENV_LAVA
} EnvType;

/* Contact flags -- set by collision resolution each frame */
typedef struct {
    bool on_ground;
    bool on_ceiling;
    bool on_wall_left;
    bool on_wall_right;
    bool in_water;
    bool on_slope;
    int  slope_angle;   /* BTS slope variant if on_slope */
} ContactFlags;

/* Physics body -- attach to any entity that moves */
typedef struct {
    Vec2fx       pos;       /* Center position */
    Vec2fx       vel;       /* Velocity (px/frame in fx32) */
    Vec2fx       accel;     /* Acceleration (px/frame^2 in fx32) */
    AABBfx       hitbox;    /* Half-extents */
    ContactFlags contact;   /* Updated by collision resolution */
    EnvType      env;       /* Current environment */
} PhysicsBody;

/* Update a body: apply gravity, integrate velocity, resolve collisions */
void physics_update_body(PhysicsBody* body);

/* Sub-steps (called by physics_update_body, exposed for testing) */
void physics_apply_gravity(PhysicsBody* body);
void physics_integrate(PhysicsBody* body);
void physics_resolve_collisions(PhysicsBody* body);

#endif /* PHYSICS_H */
