/**
 * physics.c - Physics engine
 *
 * Gravity, velocity integration, tile-based collision resolution.
 * All values use 16.16 fixed-point matching exact SNES NTSC constants.
 *
 * Collision uses axis-separated movement:
 *   1. Move X, resolve X collisions
 *   2. Move Y, resolve Y collisions
 * This prevents diagonal corner-cutting bugs.
 *
 * Position convention: Y increases downward. Gravity is positive.
 * Jump velocity is set negative by the caller.
 */

#include "physics.h"
#include "room.h"
#include <string.h>

/* ========================================================================
 * Internal Helpers
 * ======================================================================== */

/* Convert fx32 position to tile coordinate.
 * Uses arithmetic right shift which floors for negative values on ARM/GCC. */
static inline int fx_to_tile(fx32 pos) {
    return FX_TO_INT(pos) >> TILE_SHIFT;
}

/* Check if any tile in a horizontal range at a given row is solid */
static bool row_has_solid(int tile_x_min, int tile_x_max, int tile_y) {
    for (int tx = tile_x_min; tx <= tile_x_max; tx++) {
        if (room_get_collision(tx, tile_y) == COLL_SOLID) {
            return true;
        }
    }
    return false;
}

/* Check if any tile in a vertical range at a given column is solid */
static bool col_has_solid(int tile_x, int tile_y_min, int tile_y_max) {
    for (int ty = tile_y_min; ty <= tile_y_max; ty++) {
        if (room_get_collision(tile_x, ty) == COLL_SOLID) {
            return true;
        }
    }
    return false;
}

/* ========================================================================
 * Horizontal Collision Resolution
 *
 * After moving pos.x by vel.x, check if the leading edge has entered
 * a solid tile. If so, snap body back to tile boundary and zero vel.x.
 *
 * Safe against tunneling as long as |vel.x| < TILE_SIZE per frame.
 * Max horizontal speed is 14 px/f (shinespark); TILE_SIZE = 16.
 * ======================================================================== */

static void resolve_horizontal(PhysicsBody* body) {
    fx32 top    = body->pos.y - body->hitbox.half_h;
    fx32 bottom = body->pos.y + body->hitbox.half_h;

    /* Vertical tile range of hitbox (inclusive) */
    int tile_t = fx_to_tile(top);
    int tile_b = fx_to_tile(bottom - 1);

    if (body->vel.x > 0) {
        /* Moving right: check right edge */
        fx32 right = body->pos.x + body->hitbox.half_w;
        int tile_x = fx_to_tile(right - 1);

        if (col_has_solid(tile_x, tile_t, tile_b)) {
            /* Snap: body right edge = solid tile's left edge */
            body->pos.x = INT_TO_FX(tile_x * TILE_SIZE) - body->hitbox.half_w;
            body->vel.x = 0;
            body->contact.on_wall_right = true;
        }
    } else if (body->vel.x < 0) {
        /* Moving left: check left edge */
        fx32 left = body->pos.x - body->hitbox.half_w;
        int tile_x = fx_to_tile(left);

        if (col_has_solid(tile_x, tile_t, tile_b)) {
            /* Snap: body left edge = solid tile's right edge */
            body->pos.x = INT_TO_FX((tile_x + 1) * TILE_SIZE) + body->hitbox.half_w;
            body->vel.x = 0;
            body->contact.on_wall_left = true;
        }
    }
}

/* ========================================================================
 * Vertical Collision Resolution
 *
 * After moving pos.y by vel.y, check leading edge against solid tiles.
 * Also provides ground sensor when vel.y == 0.
 * ======================================================================== */

static void resolve_vertical(PhysicsBody* body) {
    fx32 left  = body->pos.x - body->hitbox.half_w;
    fx32 right = body->pos.x + body->hitbox.half_w;

    /* Horizontal tile range of hitbox (inclusive) */
    int tile_l = fx_to_tile(left);
    int tile_r = fx_to_tile(right - 1);

    if (body->vel.y > 0) {
        /* Moving down: check bottom edge (landing) */
        fx32 bottom = body->pos.y + body->hitbox.half_h;
        int tile_y = fx_to_tile(bottom - 1);

        if (row_has_solid(tile_l, tile_r, tile_y)) {
            /* Land: body bottom = solid tile's top edge */
            body->pos.y = INT_TO_FX(tile_y * TILE_SIZE) - body->hitbox.half_h;
            body->vel.y = 0;
            body->contact.on_ground = true;
        }
    } else if (body->vel.y < 0) {
        /* Moving up: check top edge (ceiling) */
        fx32 top = body->pos.y - body->hitbox.half_h;
        int tile_y = fx_to_tile(top);

        if (row_has_solid(tile_l, tile_r, tile_y)) {
            /* Hit ceiling: body top = solid tile's bottom edge */
            body->pos.y = INT_TO_FX((tile_y + 1) * TILE_SIZE) + body->hitbox.half_h;
            body->vel.y = 0;
            body->contact.on_ceiling = true;
        }
    }
}

/* ========================================================================
 * Ground Sensor
 *
 * Checks the tile row directly below the feet (bottom edge of hitbox).
 * Sets on_ground if solid. Used when vel.y == 0 to confirm standing.
 * ======================================================================== */

static void check_ground_sensor(PhysicsBody* body) {
    if (body->contact.on_ground) return;  /* Already detected by landing */

    fx32 sensor_y = body->pos.y + body->hitbox.half_h;
    int tile_y = fx_to_tile(sensor_y);  /* Tile directly below feet */

    int tile_l = fx_to_tile(body->pos.x - body->hitbox.half_w);
    int tile_r = fx_to_tile(body->pos.x + body->hitbox.half_w - 1);

    if (row_has_solid(tile_l, tile_r, tile_y)) {
        body->contact.on_ground = true;
    }
}

/* ========================================================================
 * Public API
 * ======================================================================== */

void physics_apply_gravity(PhysicsBody* body) {
    fx32 gravity;
    fx32 terminal;

    switch (body->env) {
        case ENV_WATER:
            gravity = GRAVITY_WATER;
            terminal = TERMINAL_VEL_WATER;
            break;
        case ENV_LAVA:
            gravity = GRAVITY_LAVA;
            terminal = TERMINAL_VEL_LAVA;
            break;
        default:  /* ENV_AIR */
            gravity = GRAVITY_AIR;
            terminal = TERMINAL_VEL_AIR;
            break;
    }

    body->vel.y += gravity;

    /* Clamp to terminal velocity (downward only) */
    if (body->vel.y > terminal) {
        body->vel.y = terminal;
    }
}

void physics_integrate(PhysicsBody* body) {
    /* Simple position += velocity. No collision. For testing only. */
    body->pos.x += body->vel.x;
    body->pos.y += body->vel.y;
}

void physics_resolve_collisions(PhysicsBody* body) {
    /* Resolve current position against room tiles.
     * Call after physics_integrate for simple collision. */
    memset(&body->contact, 0, sizeof(body->contact));
    resolve_horizontal(body);
    resolve_vertical(body);
    check_ground_sensor(body);
}

void physics_update_body(PhysicsBody* body) {
    /* 1. Apply gravity (environment-dependent) */
    physics_apply_gravity(body);

    /* 2. Apply external acceleration */
    body->vel.x += body->accel.x;
    body->vel.y += body->accel.y;

    /* 3. Clear contact flags */
    memset(&body->contact, 0, sizeof(body->contact));

    /* 4. Move X, resolve X collisions */
    body->pos.x += body->vel.x;
    resolve_horizontal(body);

    /* 5. Move Y, resolve Y collisions */
    body->pos.y += body->vel.y;
    resolve_vertical(body);

    /* 6. Ground sensor (for standing detection when vel.y == 0) */
    check_ground_sensor(body);
}
