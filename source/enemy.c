/**
 * enemy.c - Enemy pool management and AI
 *
 * Fixed-size pool (MAX_ENEMIES=16) with swap-remove pattern.
 * Each enemy type has an AI update function dispatched via table.
 * Enemies use the physics engine for movement and tile collision.
 *
 * Implemented AI:
 *   Zoomer/Geemer - floor crawlers, reverse at walls/edges
 *   Waver         - sine wave flyer
 *   Rinka         - homing projectile with timeout
 *   Sidehopper    - jump toward player, land, repeat
 *   Ki-Hunter     - stub
 *   Zebesian      - stub
 */

#include "enemy.h"
#include "camera.h"
#include "player.h"
#include "room.h"
#include "graphics.h"
#include "fixed_math.h"
#include "sm_config.h"
#include <string.h>

/* ========================================================================
 * Enemy Pool
 * ======================================================================== */

static Enemy pool[MAX_ENEMIES];
static int active_count;

/* ========================================================================
 * Placeholder Sprite Data
 * ======================================================================== */

static u8 enemy_sprite_data[128];  /* 16x16 @ 4bpp = 128 bytes */
static bool sprites_loaded;

static const u16 enemy_palette[16] = {
    RGB15(0, 0, 0),       /* 0: transparent */
    RGB15(24, 0, 0),      /* 1: dark red */
    RGB15(31, 8, 8),      /* 2: red (enemy body) */
    RGB15(31, 24, 0),     /* 3: yellow (eye) */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static void load_enemy_sprites(void) {
    if (sprites_loaded) return;
    /* Red 16x16 square placeholder */
    memset(enemy_sprite_data, 0x22, sizeof(enemy_sprite_data));
    graphics_load_sprite_tiles(enemy_sprite_data, sizeof(enemy_sprite_data), 4);
    graphics_load_sprite_palette(1, enemy_palette);
    sprites_loaded = true;
}

/* ========================================================================
 * Enemy Type Definitions
 * ======================================================================== */

typedef struct {
    int16_t  hp;
    uint16_t damage;
    fx32     speed;
    fx32     half_w;
    fx32     half_h;
} EnemyTypeDef;

static const EnemyTypeDef enemy_defs[ENEMY_TYPE_COUNT] = {
    [ENEMY_NONE]       = {   0,  0, 0,                        0,            0 },
    [ENEMY_ZOOMER]     = {  20,  8, 0x00008000, INT_TO_FX(6), INT_TO_FX(6) },
    [ENEMY_GEEMER]     = {  60, 20, 0x0000C000, INT_TO_FX(6), INT_TO_FX(6) },
    [ENEMY_WAVER]      = {  40, 16, 0x00010000, INT_TO_FX(6), INT_TO_FX(6) },
    [ENEMY_RINKA]      = {   1, 16, 0x00018000, INT_TO_FX(4), INT_TO_FX(4) },
    [ENEMY_SIDEHOPPER] = { 200, 40, 0x00018000, INT_TO_FX(8), INT_TO_FX(12) },
    [ENEMY_KI_HUNTER]  = { 600, 48, 0x00010000, INT_TO_FX(8), INT_TO_FX(8) },
    [ENEMY_ZEBESIAN]   = { 400, 32, 0x00010000, INT_TO_FX(6), INT_TO_FX(10) },
};

/* ========================================================================
 * AI: Crawler (Zoomer, Geemer)
 *
 * Walks along floor. Reverses at walls and floor edges.
 * Uses full physics (gravity, tile collision).
 * ======================================================================== */

static void ai_crawler(Enemy* e) {
    const EnemyTypeDef* def = &enemy_defs[e->type];

    /* Move in facing direction */
    e->body.vel.x = (e->facing == DIR_RIGHT) ? def->speed : -def->speed;

    /* Physics: gravity + collision */
    physics_update_body(&e->body);

    /* Reverse on wall hit */
    if (e->body.contact.on_wall_right) {
        e->facing = DIR_LEFT;
    } else if (e->body.contact.on_wall_left) {
        e->facing = DIR_RIGHT;
    }

    /* Reverse at floor edges: check tile below leading foot */
    if (e->body.contact.on_ground) {
        int look_x;
        if (e->facing == DIR_RIGHT) {
            look_x = FX_TO_INT(e->body.pos.x + e->body.hitbox.half_w +
                               INT_TO_FX(1)) >> TILE_SHIFT;
        } else {
            look_x = FX_TO_INT(e->body.pos.x - e->body.hitbox.half_w -
                               INT_TO_FX(1)) >> TILE_SHIFT;
        }
        int foot_y = FX_TO_INT(e->body.pos.y + e->body.hitbox.half_h)
                     >> TILE_SHIFT;

        if (room_get_collision(look_x, foot_y) != COLL_SOLID) {
            e->facing = (e->facing == DIR_RIGHT) ? DIR_LEFT : DIR_RIGHT;
        }
    }
}

/* ========================================================================
 * AI: Waver (sine wave flyer)
 *
 * Horizontal movement + vertical sine oscillation.
 * Ignores gravity. Reverses at room horizontal edges.
 * ======================================================================== */

static void ai_waver(Enemy* e) {
    const EnemyTypeDef* def = &enemy_defs[e->type];

    /* Horizontal movement */
    e->body.vel.x = (e->facing == DIR_RIGHT) ? def->speed : -def->speed;

    /* Sine wave on Y using ai_timer as phase */
    e->ai_timer++;
    e->body.vel.y = fx_sin(e->ai_timer & 0xFF) >> 1;

    /* Direct position update (no gravity) */
    e->body.pos.x += e->body.vel.x;
    e->body.pos.y += e->body.vel.y;

    /* Reverse at room edges */
    int px = FX_TO_INT(e->body.pos.x);
    int room_w = g_current_room.width_tiles * TILE_SIZE;
    if (px <= TILE_SIZE || px >= room_w - TILE_SIZE) {
        e->facing = (e->facing == DIR_RIGHT) ? DIR_LEFT : DIR_RIGHT;
    }
}

/* ========================================================================
 * AI: Rinka (homing projectile)
 *
 * Flies toward player at constant speed. Despawns after 5 seconds.
 * Ignores gravity and tile collision.
 * ======================================================================== */

static void ai_rinka(Enemy* e) {
    const EnemyTypeDef* def = &enemy_defs[e->type];
    fx32 dx = g_player.body.pos.x - e->body.pos.x;
    fx32 dy = g_player.body.pos.y - e->body.pos.y;

    /* Approximate normalized direction */
    e->body.vel.x = (dx > 0) ? def->speed : (dx < 0) ? -def->speed : 0;
    e->body.vel.y = (dy > 0) ? def->speed : (dy < 0) ? -def->speed : 0;

    e->body.pos.x += e->body.vel.x;
    e->body.pos.y += e->body.vel.y;

    /* Despawn timer (5 seconds @ 60fps) */
    e->ai_timer++;
    if (e->ai_timer > 300) {
        e->active = false;
    }
}

/* ========================================================================
 * AI: Sidehopper (jump-and-wait)
 *
 * State 0: idle on ground, wait 1 second, then jump toward player.
 * State 1: airborne, wait for landing.
 * Uses full physics.
 * ======================================================================== */

static void ai_sidehopper(Enemy* e) {
    const EnemyTypeDef* def = &enemy_defs[e->type];

    switch (e->ai_state) {
        case 0: /* Idle on ground */
            e->body.vel.x = 0;
            physics_update_body(&e->body);
            e->ai_timer++;
            if (e->ai_timer > 60) {
                e->ai_state = 1;
                e->ai_timer = 0;
                /* Jump toward player */
                e->body.vel.y = -(JUMP_VEL_NORMAL >> 1);
                if (g_player.body.pos.x > e->body.pos.x) {
                    e->facing = DIR_RIGHT;
                    e->body.vel.x = def->speed;
                } else {
                    e->facing = DIR_LEFT;
                    e->body.vel.x = -def->speed;
                }
            }
            break;

        case 1: /* Airborne */
            physics_update_body(&e->body);
            if (e->body.contact.on_ground) {
                e->ai_state = 0;
                e->ai_timer = 0;
                e->body.vel.x = 0;
            }
            break;
    }
}

/* Stub for unimplemented AI */
static void ai_stub(Enemy* e) {
    /* Just apply physics so it doesn't float */
    physics_update_body(&e->body);
}

/* AI dispatch table */
typedef void (*AIUpdateFn)(Enemy*);

static const AIUpdateFn ai_fns[ENEMY_TYPE_COUNT] = {
    [ENEMY_NONE]       = ai_stub,
    [ENEMY_ZOOMER]     = ai_crawler,
    [ENEMY_GEEMER]     = ai_crawler,
    [ENEMY_WAVER]      = ai_waver,
    [ENEMY_RINKA]      = ai_rinka,
    [ENEMY_SIDEHOPPER] = ai_sidehopper,
    [ENEMY_KI_HUNTER]  = ai_stub,
    [ENEMY_ZEBESIAN]   = ai_stub,
};

/* ========================================================================
 * Public API: Pool Management
 * ======================================================================== */

void enemy_pool_init(void) {
    memset(pool, 0, sizeof(pool));
    active_count = 0;
    sprites_loaded = false;
}

int enemy_spawn(EnemyTypeID type, fx32 x, fx32 y) {
    if (type == ENEMY_NONE || type >= ENEMY_TYPE_COUNT) return -1;
    if (active_count >= MAX_ENEMIES) return -1;

    load_enemy_sprites();

    const EnemyTypeDef* def = &enemy_defs[type];

    Enemy* e = &pool[active_count];
    memset(e, 0, sizeof(Enemy));
    e->type = type;
    e->active = true;
    e->hp = def->hp;
    e->hp_max = def->hp;
    e->damage_contact = def->damage;
    e->facing = DIR_LEFT;

    e->body.pos.x = x;
    e->body.pos.y = y;
    e->body.hitbox.half_w = def->half_w;
    e->body.hitbox.half_h = def->half_h;
    e->body.env = ENV_AIR;

    int idx = active_count;
    active_count++;
    return idx;
}

void enemy_remove(int index) {
    if (index < 0 || index >= active_count) return;

    /* Swap with last active entry */
    active_count--;
    if (index < active_count) {
        pool[index] = pool[active_count];
    }
    memset(&pool[active_count], 0, sizeof(Enemy));
}

void enemy_clear_all(void) {
    memset(pool, 0, sizeof(pool));
    active_count = 0;
}

/* ========================================================================
 * Public API: Per-Frame Update & Render
 * ======================================================================== */

/* AABB overlap check for contact damage */
static bool enemy_aabb_overlap(const Enemy* e, const PhysicsBody* target) {
    fx32 dx = e->body.pos.x - target->pos.x;
    fx32 dy = e->body.pos.y - target->pos.y;
    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;
    return dx < (e->body.hitbox.half_w + target->hitbox.half_w) &&
           dy < (e->body.hitbox.half_h + target->hitbox.half_h);
}

void enemy_update_all(void) {
    /* Iterate backward so swap-remove doesn't skip entries */
    for (int i = active_count - 1; i >= 0; i--) {
        Enemy* e = &pool[i];
        if (!e->active) {
            enemy_remove(i);
            continue;
        }

        /* Run type-specific AI */
        if (ai_fns[e->type]) {
            ai_fns[e->type](e);
        }

        /* Contact damage vs player */
        if (e->active && e->damage_contact > 0 &&
            g_player.alive && g_player.invuln_timer == 0) {
            if (enemy_aabb_overlap(e, &g_player.body)) {
                player_damage_from(e->damage_contact, e->body.pos.x);
            }
        }

        /* Animation timer (simple 4-frame cycle) */
        e->anim_timer++;
        if (e->anim_timer >= 8) {
            e->anim_timer = 0;
            e->anim_frame = (e->anim_frame + 1) & 3;
        }
    }
}

void enemy_render_all(void) {
    int cam_x = FX_TO_INT(g_camera.x);
    int cam_y = FX_TO_INT(g_camera.y);

    for (int i = 0; i < active_count; i++) {
        Enemy* e = &pool[i];

        /* World to screen (subtract camera) */
        int sx = FX_TO_INT(e->body.pos.x) - cam_x - 8;
        int sy = FX_TO_INT(e->body.pos.y) - cam_y - 8;

        int oam_idx = OAM_ENEMY_START + i;

        /* Cull off-screen sprites */
        if (sx < -16 || sx > SCREEN_WIDTH || sy < -16 || sy > SCREEN_HEIGHT) {
            graphics_hide_sprite(oam_idx);
            continue;
        }

        graphics_set_sprite(oam_idx, sx, sy, 4, 1, 2,
                           e->facing == DIR_LEFT, false);
    }

    /* Hide unused enemy OAM slots */
    for (int i = active_count; i < MAX_ENEMIES; i++) {
        graphics_hide_sprite(OAM_ENEMY_START + i);
    }
}

int enemy_get_count(void) {
    return active_count;
}

Enemy* enemy_get(int index) {
    if (index < 0 || index >= active_count) return NULL;
    return &pool[index];
}

void enemy_damage(int index, int16_t damage) {
    if (index < 0 || index >= active_count) return;
    Enemy* e = &pool[index];
    e->hp -= damage;
    if (e->hp <= 0) {
        e->active = false;
    }
}
