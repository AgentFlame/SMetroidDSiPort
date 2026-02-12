/**
 * projectile.h - Weapon projectile pool
 *
 * Fixed-size pool (MAX_PROJECTILES) with swap-remove.
 * Beam types, missiles, bombs.
 *
 * Implemented in: source/projectile.c (M12)
 */

#ifndef PROJECTILE_H
#define PROJECTILE_H

#include "sm_types.h"
#include "physics.h"

/* Projectile types */
typedef enum {
    PROJ_NONE = 0,
    PROJ_POWER_BEAM,
    PROJ_ICE_BEAM,
    PROJ_WAVE_BEAM,
    PROJ_SPAZER_BEAM,
    PROJ_PLASMA_BEAM,
    PROJ_MISSILE,
    PROJ_SUPER_MISSILE,
    PROJ_BOMB,
    PROJ_POWER_BOMB,
    PROJ_ENEMY_BULLET,
    PROJ_TYPE_COUNT
} ProjectileTypeID;

/* Projectile owner (for collision filtering) */
typedef enum {
    PROJ_OWNER_PLAYER = 0,
    PROJ_OWNER_ENEMY
} ProjectileOwner;

/* Per-projectile instance */
typedef struct {
    ProjectileTypeID type;
    ProjectileOwner  owner;
    Vec2fx           pos;
    Vec2fx           vel;
    AABBfx           hitbox;
    int16_t          damage;
    uint16_t         lifetime;  /* Frames remaining (0 = expired) */
    uint16_t         timer;     /* For bombs: countdown to explosion */
    bool             active;
} Projectile;

/* Pool management */
void projectile_pool_init(void);
int  projectile_spawn(ProjectileTypeID type, ProjectileOwner owner,
                      fx32 x, fx32 y, fx32 vx, fx32 vy);
void projectile_remove(int index);
void projectile_clear_all(void);

/* Per-frame update/render */
void projectile_update_all(void);
void projectile_render_all(void);

#endif /* PROJECTILE_H */
