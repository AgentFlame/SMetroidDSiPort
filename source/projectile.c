/**
 * projectile.c - Weapon projectile pool
 *
 * Fixed-size pool (MAX_PROJECTILES=32) with swap-remove.
 * Handles beams, missiles, bombs, enemy bullets.
 * Collision: projectile vs enemy, projectile vs player, projectile vs tiles.
 *
 * Beams travel in facing direction, destroyed on wall hit (except Wave).
 * Bombs sit in place, explode on timer, apply bomb jump to nearby player.
 * Plasma beams pass through enemies (hit all in path).
 */

#include "projectile.h"
#include "camera.h"
#include "player.h"
#include "enemy.h"
#include "boss.h"
#include "room.h"
#include "graphics.h"
#include "fixed_math.h"
#include "sm_config.h"
#include <string.h>

/* ========================================================================
 * Projectile Pool
 * ======================================================================== */

static Projectile pool[MAX_PROJECTILES];
static int active_count;

/* ========================================================================
 * Placeholder Sprite Data
 * ======================================================================== */

static u8 proj_sprite_data[128];  /* 16x16 @ 4bpp = 128 bytes */
static bool sprites_loaded;

static const u16 proj_palette[16] = {
    RGB15(0, 0, 0),       /* 0: transparent */
    RGB15(16, 16, 31),    /* 1: light blue */
    RGB15(8, 8, 31),      /* 2: blue (beam) */
    RGB15(31, 31, 0),     /* 3: yellow (missile) */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static void load_proj_sprites(void) {
    if (sprites_loaded) return;
    /* Blue 16x16 square placeholder */
    memset(proj_sprite_data, 0x22, sizeof(proj_sprite_data));
    graphics_load_sprite_tiles(proj_sprite_data, sizeof(proj_sprite_data), 8);
    graphics_load_sprite_palette(2, proj_palette);
    sprites_loaded = true;
}

/* ========================================================================
 * Projectile Type Definitions
 * ======================================================================== */

typedef struct {
    int16_t  damage;
    fx32     speed;
    uint16_t lifetime;
    fx32     half_w;
    fx32     half_h;
    bool     wall_pass;    /* Wave beam passes through walls */
    bool     enemy_pass;   /* Plasma passes through enemies */
} ProjTypeDef;

static const ProjTypeDef proj_defs[PROJ_TYPE_COUNT] = {
    [PROJ_NONE]          = {   0, 0,               0, 0, 0, false, false },
    [PROJ_POWER_BEAM]    = {  20, INT_TO_FX(4),   30, INT_TO_FX(4), INT_TO_FX(2), false, false },
    [PROJ_ICE_BEAM]      = {  30, INT_TO_FX(3),   30, INT_TO_FX(4), INT_TO_FX(2), false, false },
    [PROJ_WAVE_BEAM]     = {  50, INT_TO_FX(4),   45, INT_TO_FX(4), INT_TO_FX(2), true,  false },
    [PROJ_SPAZER_BEAM]   = {  40, INT_TO_FX(4),   30, INT_TO_FX(6), INT_TO_FX(4), false, false },
    [PROJ_PLASMA_BEAM]   = { 150, INT_TO_FX(4),   30, INT_TO_FX(4), INT_TO_FX(2), false, true  },
    [PROJ_MISSILE]       = { 100, INT_TO_FX(5),   60, INT_TO_FX(4), INT_TO_FX(2), false, false },
    [PROJ_SUPER_MISSILE] = { 300, INT_TO_FX(5),   60, INT_TO_FX(4), INT_TO_FX(2), false, false },
    [PROJ_BOMB]          = {  30, 0,              BOMB_TIMER_FRAMES, INT_TO_FX(8), INT_TO_FX(8), false, false },
    [PROJ_POWER_BOMB]    = { 200, 0,              BOMB_TIMER_FRAMES, INT_TO_FX(32), INT_TO_FX(32), false, false },
    [PROJ_ENEMY_BULLET]  = {  10, INT_TO_FX(2),  120, INT_TO_FX(3), INT_TO_FX(3), false, false },
};

/* Solid tile test (matches physics.c is_collision_solid logic) */
static inline bool tile_is_solid(uint8_t coll) {
    if (coll == COLL_SOLID) return true;
    if ((coll & 0xF0) == COLL_SPECIAL_BASE) return true;
    return false;
}

/* ========================================================================
 * AABB Overlap Check
 * ======================================================================== */

static bool aabb_overlap(Vec2fx pos_a, AABBfx box_a, Vec2fx pos_b, AABBfx box_b) {
    fx32 dx = pos_a.x - pos_b.x;
    fx32 dy = pos_a.y - pos_b.y;
    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;
    return dx < (box_a.half_w + box_b.half_w) &&
           dy < (box_a.half_h + box_b.half_h);
}

/* ========================================================================
 * Bomb Explosion
 * ======================================================================== */

static void bomb_explode(Projectile* p) {
    const ProjTypeDef* def = &proj_defs[p->type];
    AABBfx blast = { def->half_w, def->half_h };

    /* Damage enemies in blast radius */
    for (int e = enemy_get_count() - 1; e >= 0; e--) {
        Enemy* en = enemy_get(e);
        if (!en) continue;
        if (aabb_overlap(p->pos, blast, en->body.pos, en->body.hitbox)) {
            enemy_damage(e, def->damage);
        }
    }

    /* Break bomb blocks in blast radius */
    {
        int center_tx = FX_TO_INT(p->pos.x) >> TILE_SHIFT;
        int center_ty = FX_TO_INT(p->pos.y) >> TILE_SHIFT;
        int radius_tiles = FX_TO_INT(def->half_w) >> TILE_SHIFT;
        if (radius_tiles < 1) radius_tiles = 1;
        for (int ty = center_ty - radius_tiles; ty <= center_ty + radius_tiles; ty++) {
            for (int tx = center_tx - radius_tiles; tx <= center_tx + radius_tiles; tx++) {
                if (room_get_collision(tx, ty) == COLL_SPECIAL_BOMB) {
                    room_set_collision(tx, ty, COLL_AIR);
                }
            }
        }
    }

    /* Bomb jump: push player upward if close enough */
    fx32 px = p->pos.x - g_player.body.pos.x;
    fx32 py = p->pos.y - g_player.body.pos.y;
    if (px < 0) px = -px;
    if (py < 0) py = -py;

    if (px < INT_TO_FX(16) && py < INT_TO_FX(16)) {
        g_player.body.vel.y = -BOMB_JUMP_VEL;
    }
}

/* ========================================================================
 * Collision: Player Projectile vs Enemies
 * ======================================================================== */

static void check_enemy_hits(int proj_idx) {
    Projectile* p = &pool[proj_idx];
    const ProjTypeDef* def = &proj_defs[p->type];
    AABBfx pbox = { p->hitbox.half_w, p->hitbox.half_h };

    for (int e = enemy_get_count() - 1; e >= 0; e--) {
        Enemy* en = enemy_get(e);
        if (!en) continue;

        if (aabb_overlap(p->pos, pbox, en->body.pos, en->body.hitbox)) {
            enemy_damage(e, p->damage);

            if (!def->enemy_pass) {
                p->active = false;
                return;
            }
        }
    }
}

/* ========================================================================
 * Collision: Player Projectile vs Boss
 * ======================================================================== */

static void check_boss_hit(int proj_idx) {
    if (!boss_is_active()) return;

    Projectile* p = &pool[proj_idx];
    AABBfx pbox = { p->hitbox.half_w, p->hitbox.half_h };

    if (aabb_overlap(p->pos, pbox, g_boss.body.pos, g_boss.body.hitbox)) {
        boss_damage(p->damage);
        /* Destroy projectile on hit (boss is always blocking) */
        p->active = false;
    }
}

/* ========================================================================
 * Collision: Enemy Projectile vs Player
 * ======================================================================== */

static void check_player_hit(int proj_idx) {
    Projectile* p = &pool[proj_idx];
    AABBfx pbox = { p->hitbox.half_w, p->hitbox.half_h };

    if (aabb_overlap(p->pos, pbox, g_player.body.pos, g_player.body.hitbox)) {
        player_damage(p->damage);
        p->active = false;
    }
}

/* ========================================================================
 * Public API: Pool Management
 * ======================================================================== */

void projectile_pool_init(void) {
    memset(pool, 0, sizeof(pool));
    active_count = 0;
    sprites_loaded = false;
}

int projectile_spawn(ProjectileTypeID type, ProjectileOwner owner,
                     fx32 x, fx32 y, fx32 vx, fx32 vy) {
    if (type == PROJ_NONE || type >= PROJ_TYPE_COUNT) return -1;
    if (active_count >= MAX_PROJECTILES) return -1;

    load_proj_sprites();

    const ProjTypeDef* def = &proj_defs[type];

    Projectile* p = &pool[active_count];
    memset(p, 0, sizeof(Projectile));
    p->type = type;
    p->owner = owner;
    p->active = true;
    p->damage = def->damage;
    p->lifetime = def->lifetime;
    p->hitbox.half_w = def->half_w;
    p->hitbox.half_h = def->half_h;

    p->pos.x = x;
    p->pos.y = y;
    p->vel.x = vx;
    p->vel.y = vy;

    /* Bombs use lifetime as countdown timer */
    if (type == PROJ_BOMB || type == PROJ_POWER_BOMB) {
        p->timer = def->lifetime;
    }

    int idx = active_count;
    active_count++;
    return idx;
}

void projectile_remove(int index) {
    if (index < 0 || index >= active_count) return;

    active_count--;
    if (index < active_count) {
        pool[index] = pool[active_count];
    }
    memset(&pool[active_count], 0, sizeof(Projectile));
}

void projectile_clear_all(void) {
    memset(pool, 0, sizeof(pool));
    active_count = 0;
}

/* ========================================================================
 * Public API: Per-Frame Update
 * ======================================================================== */

void projectile_update_all(void) {
    for (int i = active_count - 1; i >= 0; i--) {
        Projectile* p = &pool[i];
        if (!p->active) {
            projectile_remove(i);
            continue;
        }

        /* Bombs: count down timer, explode at 0 */
        if (p->type == PROJ_BOMB || p->type == PROJ_POWER_BOMB) {
            if (p->timer > 0) {
                p->timer--;
            }
            if (p->timer == 0) {
                bomb_explode(p);
                p->active = false;
                projectile_remove(i);
                continue;
            }
            /* Bombs don't move, skip movement/tile collision */
            continue;
        }

        /* Movement */
        p->pos.x += p->vel.x;
        p->pos.y += p->vel.y;

        /* Lifetime */
        if (p->lifetime > 0) {
            p->lifetime--;
            if (p->lifetime == 0) {
                p->active = false;
                projectile_remove(i);
                continue;
            }
        }

        /* Tile collision (skip for wave beam) */
        if (!proj_defs[p->type].wall_pass) {
            int tile_x = FX_TO_INT(p->pos.x) >> TILE_SHIFT;
            int tile_y = FX_TO_INT(p->pos.y) >> TILE_SHIFT;
            uint8_t coll = room_get_collision(tile_x, tile_y);
            if (tile_is_solid(coll)) {
                p->active = false;
                projectile_remove(i);
                continue;
            }
            /* Break shot blocks on beam/missile hit */
            if (coll == COLL_SPECIAL_SHOT && p->owner == PROJ_OWNER_PLAYER) {
                room_set_collision(tile_x, tile_y, COLL_AIR);
                /* Destroy non-plasma projectiles */
                if (!proj_defs[p->type].enemy_pass) {
                    p->active = false;
                    projectile_remove(i);
                    continue;
                }
            }
        }

        /* Entity collision */
        if (p->owner == PROJ_OWNER_PLAYER) {
            check_enemy_hits(i);
            if (!p->active) {
                projectile_remove(i);
                continue;
            }
            check_boss_hit(i);
            if (!p->active) {
                projectile_remove(i);
                continue;
            }
        } else {
            check_player_hit(i);
            if (!p->active) {
                projectile_remove(i);
                continue;
            }
        }
    }
}

/* ========================================================================
 * Public API: Render
 * ======================================================================== */

void projectile_render_all(void) {
    int cam_x = FX_TO_INT(g_camera.x);
    int cam_y = FX_TO_INT(g_camera.y);

    for (int i = 0; i < active_count; i++) {
        Projectile* p = &pool[i];

        int sx = FX_TO_INT(p->pos.x) - cam_x - 8;
        int sy = FX_TO_INT(p->pos.y) - cam_y - 8;

        int oam_idx = OAM_PROJ_START + i;

        /* Cull off-screen */
        if (sx < -16 || sx > SCREEN_WIDTH || sy < -16 || sy > SCREEN_HEIGHT) {
            graphics_hide_sprite(oam_idx);
            continue;
        }

        graphics_set_sprite(oam_idx, sx, sy, 8, 2, 0, false, false);
    }

    /* Hide unused projectile OAM slots */
    for (int i = active_count; i < OAM_PROJ_COUNT && i < MAX_PROJECTILES; i++) {
        graphics_hide_sprite(OAM_PROJ_START + i);
    }
}
