/**
 * boss.c - Boss AI framework + implemented bosses
 *
 * Single static boss instance. Only one boss active at a time.
 * Each boss type has init/update functions dispatched via table.
 *
 * Implemented bosses:
 *   Spore Spawn  - 960 HP, pendulum swing, vulnerability window, spore attack
 *   Crocomire    - Push mechanic (no HP kill), advances/retreats, spit/lunge
 *   Bomb Torizo  - 800 HP, statue wake, bomb throw, lunge attack
 *
 * Boss-specific param usage:
 *   Spore Spawn:
 *     param_a  = swing angle (0-255 LUT units, stored as fx32)
 *     param_b  = (unused)
 *     anchor_x = ceiling attachment X
 *     anchor_y = ceiling attachment Y
 *     sub_timer   = spore attack cooldown
 *     attack_count = (unused)
 *
 *   Crocomire:
 *     param_a  = (unused)
 *     param_b  = (unused)
 *     anchor_x = lava pit X threshold (spawn_x + CROC_PUSH_THRESHOLD)
 *     anchor_y = spawn Y (ground level)
 *     sub_timer   = attack cooldown
 *     attack_count = spit count since last lunge
 *
 *   Bomb Torizo:
 *     param_a  = spawn X (for lunge return position)
 *     param_b  = (unused)
 *     anchor_x = (unused)
 *     anchor_y = (unused)
 *     sub_timer   = idle duration for current cycle
 *     attack_count = attacks since wake
 */

#include "boss.h"
#include "camera.h"
#include "player.h"
#include "projectile.h"
#include "graphics.h"
#include "fixed_math.h"
#include "sm_config.h"
#include <string.h>

/* ========================================================================
 * Global Boss Instance
 * ======================================================================== */

Boss g_boss;

/* ========================================================================
 * Placeholder Sprite Data
 * ======================================================================== */

static u8 boss_sprite_data[128];  /* 16x16 @ 4bpp = 128 bytes */
static bool sprites_loaded;

static const u16 boss_palette[16] = {
    RGB15(0, 0, 0),       /* 0: transparent */
    RGB15(31, 24, 0),     /* 1: yellow (shell) */
    RGB15(31, 16, 0),     /* 2: orange (body) */
    RGB15(31, 0, 0),      /* 3: red (core/vulnerable) */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static void load_boss_sprites(void) {
    if (sprites_loaded) return;
    /* Orange 16x16 square placeholder */
    memset(boss_sprite_data, 0x22, sizeof(boss_sprite_data));
    graphics_load_sprite_tiles(boss_sprite_data, sizeof(boss_sprite_data), 12);
    graphics_load_sprite_palette(3, boss_palette);
    sprites_loaded = true;
}

/* ========================================================================
 * AABB Overlap (local copy to avoid cross-module dependency)
 * ======================================================================== */

static bool boss_aabb_overlap(Vec2fx pos_a, AABBfx box_a,
                              Vec2fx pos_b, AABBfx box_b) {
    fx32 dx = pos_a.x - pos_b.x;
    fx32 dy = pos_a.y - pos_b.y;
    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;
    return dx < (box_a.half_w + box_b.half_w) &&
           dy < (box_a.half_h + box_b.half_h);
}

/* ========================================================================
 * Common Constants
 * ======================================================================== */

#define BOSS_HIT_INVULN   10   /* I-frames after a hit (all bosses) */

/* ========================================================================
 * Spore Spawn AI
 *
 * Behavior cycle:
 *   1. SWING:  Pendulum motion around ceiling anchor (~5 sec)
 *   2. DESCEND: Drop down toward room center
 *   3. OPEN:   Core opens (30 frames animation)
 *   4. VULNERABLE: Takes damage, shoots spores (~2 sec)
 *   5. CLOSE:  Core closes (30 frames animation)
 *   6. ASCEND: Rise back to anchor
 *   7. Repeat from SWING
 *
 * Death: 1-second animation then deactivate.
 * ======================================================================== */

/* Spore Spawn AI states */
enum {
    SS_SWING = 0,
    SS_DESCEND,
    SS_OPEN,
    SS_VULNERABLE,
    SS_CLOSE,
    SS_ASCEND,
    SS_DEATH
};

/* Spore Spawn constants */
#define SS_HP                960
#define SS_CONTACT_DAMAGE    40
#define SS_HITBOX_HALF_W     INT_TO_FX(12)
#define SS_HITBOX_HALF_H     INT_TO_FX(16)
#define SS_SWING_RADIUS      INT_TO_FX(48)   /* Horizontal swing extent */
#define SS_SWING_SPEED       3               /* LUT angle units per frame */
#define SS_SWING_FRAMES      300             /* ~5 sec of swinging */
#define SS_DESCEND_SPEED     INT_TO_FX(1)    /* px/frame downward */
#define SS_DESCEND_DIST      INT_TO_FX(64)   /* How far below anchor */
#define SS_ASCEND_SPEED      INT_TO_FX(1)    /* px/frame upward */
#define SS_OPEN_FRAMES       30
#define SS_VULN_FRAMES       120             /* ~2 sec vulnerability window */
#define SS_CLOSE_FRAMES      30
#define SS_SPORE_INTERVAL    45              /* Frames between spore shots */
#define SS_SPORE_SPEED       INT_TO_FX(2)    /* Spore projectile speed */
#define SS_DEATH_FRAMES      60              /* 1-second death animation */

static void spore_spawn_init(void) {
    g_boss.hp = SS_HP;
    g_boss.hp_max = SS_HP;
    g_boss.damage_contact = SS_CONTACT_DAMAGE;
    g_boss.body.hitbox.half_w = SS_HITBOX_HALF_W;
    g_boss.body.hitbox.half_h = SS_HITBOX_HALF_H;
    g_boss.vulnerable = false;
    g_boss.ai_state = SS_SWING;
    g_boss.ai_timer = 0;
    g_boss.ai_counter = 0;
    g_boss.param_a = 0;  /* swing angle */
    g_boss.sub_timer = 0;

    /* Anchor = spawn position (ceiling attachment) */
    g_boss.anchor_x = g_boss.body.pos.x;
    g_boss.anchor_y = g_boss.body.pos.y;
}

static void spore_spawn_update(void) {
    Boss* b = &g_boss;

    /* Decrement invuln timer */
    if (b->invuln_timer > 0) b->invuln_timer--;

    switch (b->ai_state) {
        case SS_SWING: {
            /* Pendulum motion: X = anchor + sin(angle) * radius */
            b->param_a += INT_TO_FX(SS_SWING_SPEED);
            uint8_t angle = (uint8_t)FX_TO_INT(b->param_a);
            fx32 offset_x = fx_mul(fx_sin(angle), SS_SWING_RADIUS);
            b->body.pos.x = b->anchor_x + offset_x;
            b->body.pos.y = b->anchor_y;

            /* Swing for fixed duration then descend */
            b->ai_timer++;
            if (b->ai_timer >= SS_SWING_FRAMES) {
                b->ai_state = SS_DESCEND;
                b->ai_timer = 0;
                /* Center X before descending */
                b->body.pos.x = b->anchor_x;
            }
            break;
        }

        case SS_DESCEND: {
            b->body.pos.y += SS_DESCEND_SPEED;
            fx32 target_y = b->anchor_y + SS_DESCEND_DIST;
            if (b->body.pos.y >= target_y) {
                b->body.pos.y = target_y;
                b->ai_state = SS_OPEN;
                b->ai_timer = 0;
            }
            break;
        }

        case SS_OPEN: {
            b->ai_timer++;
            if (b->ai_timer >= SS_OPEN_FRAMES) {
                b->ai_state = SS_VULNERABLE;
                b->ai_timer = 0;
                b->vulnerable = true;
                b->sub_timer = 0;
            }
            break;
        }

        case SS_VULNERABLE: {
            b->ai_timer++;
            b->sub_timer++;

            /* Shoot spores at player periodically */
            if (b->sub_timer >= SS_SPORE_INTERVAL) {
                b->sub_timer = 0;
                fx32 dx = g_player.body.pos.x - b->body.pos.x;
                fx32 dy = g_player.body.pos.y - b->body.pos.y;
                fx32 vx = (dx > 0) ? SS_SPORE_SPEED : -SS_SPORE_SPEED;
                fx32 vy = (dy > 0) ? (SS_SPORE_SPEED >> 1) :
                                     -(SS_SPORE_SPEED >> 1);
                projectile_spawn(PROJ_ENEMY_BULLET, PROJ_OWNER_ENEMY,
                                b->body.pos.x, b->body.pos.y, vx, vy);
            }

            /* Close after vulnerability window */
            if (b->ai_timer >= SS_VULN_FRAMES) {
                b->ai_state = SS_CLOSE;
                b->ai_timer = 0;
                b->vulnerable = false;
            }
            break;
        }

        case SS_CLOSE: {
            b->ai_timer++;
            if (b->ai_timer >= SS_CLOSE_FRAMES) {
                b->ai_state = SS_ASCEND;
                b->ai_timer = 0;
            }
            break;
        }

        case SS_ASCEND: {
            b->body.pos.y -= SS_ASCEND_SPEED;
            if (b->body.pos.y <= b->anchor_y) {
                b->body.pos.y = b->anchor_y;
                b->ai_state = SS_SWING;
                b->ai_timer = 0;
                b->param_a = 0;  /* Reset swing angle */
            }
            break;
        }

        case SS_DEATH: {
            b->ai_timer++;
            if (b->ai_timer >= SS_DEATH_FRAMES) {
                b->active = false;
            }
            break;
        }
    }

    /* Contact damage to player */
    if (b->active && b->ai_state != SS_DEATH) {
        if (boss_aabb_overlap(b->body.pos, b->body.hitbox,
                              g_player.body.pos, g_player.body.hitbox)) {
            player_damage(b->damage_contact);
        }
    }
}

/* ========================================================================
 * Crocomire AI
 *
 * Unique push mechanic: hits push Crocomire toward lava pit instead
 * of depleting HP. When pushed past the lava threshold, falls in.
 *
 * Behavior:
 *   1. ADVANCE:  Walk toward player (slow, menacing)
 *   2. SPIT:     Stop and fire projectiles at player
 *   3. FLINCH:   Brief stun when hit, pushed backward
 *   4. LUNGE:    Quick forward charge
 *   5. FALLING:  Toppling into lava pit
 *   6. DEATH:    Dissolving in lava
 * ======================================================================== */

/* Crocomire AI states */
enum {
    CROC_ADVANCE = 0,
    CROC_SPIT,
    CROC_FLINCH,
    CROC_LUNGE,
    CROC_FALLING,
    CROC_DEATH
};

/* Crocomire constants */
#define CROC_HP_DUMMY           9999  /* Effectively infinite */
#define CROC_CONTACT_DAMAGE     30
#define CROC_HITBOX_HALF_W      INT_TO_FX(16)
#define CROC_HITBOX_HALF_H      INT_TO_FX(20)
#define CROC_ADVANCE_SPEED      0x4000       /* 0.25 px/f toward player */
#define CROC_PUSH_PER_HIT       INT_TO_FX(8) /* 8 px pushback per hit */
#define CROC_PUSH_THRESHOLD     INT_TO_FX(160) /* 160 px to reach lava */
#define CROC_FLINCH_FRAMES      20
#define CROC_SPIT_FRAMES        40           /* Duration of spit attack */
#define CROC_SPIT_SPEED         INT_TO_FX(3)
#define CROC_LUNGE_SPEED        INT_TO_FX(3) /* Fast forward lunge */
#define CROC_LUNGE_FRAMES       15
#define CROC_ADVANCE_DURATION   180          /* Frames before spit attack */
#define CROC_LUNGE_EVERY        3            /* Lunge after every N spits */
#define CROC_DEATH_FRAMES       90
#define CROC_FALL_SPEED         INT_TO_FX(2)
#define CROC_FALL_FRAMES        45

static void crocomire_init(void) {
    g_boss.hp = CROC_HP_DUMMY;
    g_boss.hp_max = CROC_HP_DUMMY;
    g_boss.damage_contact = CROC_CONTACT_DAMAGE;
    g_boss.body.hitbox.half_w = CROC_HITBOX_HALF_W;
    g_boss.body.hitbox.half_h = CROC_HITBOX_HALF_H;
    g_boss.vulnerable = true;   /* Always vulnerable to receive push hits */
    g_boss.ai_state = CROC_ADVANCE;
    g_boss.ai_timer = 0;
    g_boss.ai_counter = 0;
    g_boss.sub_timer = 0;
    g_boss.attack_count = 0;

    /* Lava threshold = spawn position + push distance */
    g_boss.anchor_x = g_boss.body.pos.x + CROC_PUSH_THRESHOLD;
    g_boss.anchor_y = g_boss.body.pos.y;
}

static void crocomire_update(void) {
    Boss* b = &g_boss;

    if (b->invuln_timer > 0) b->invuln_timer--;

    switch (b->ai_state) {
        case CROC_ADVANCE: {
            /* Walk toward player (leftward assuming player is left) */
            fx32 dx = g_player.body.pos.x - b->body.pos.x;
            if (dx < 0) {
                b->body.pos.x -= CROC_ADVANCE_SPEED;
            } else {
                b->body.pos.x += CROC_ADVANCE_SPEED;
            }

            b->ai_timer++;
            if (b->ai_timer >= CROC_ADVANCE_DURATION) {
                /* Decide: lunge or spit */
                if (b->attack_count >= CROC_LUNGE_EVERY) {
                    b->ai_state = CROC_LUNGE;
                    b->attack_count = 0;
                } else {
                    b->ai_state = CROC_SPIT;
                }
                b->ai_timer = 0;
            }
            break;
        }

        case CROC_SPIT: {
            /* Fire projectile at start of spit state */
            if (b->ai_timer == 0) {
                fx32 dx = g_player.body.pos.x - b->body.pos.x;
                fx32 dy = g_player.body.pos.y - b->body.pos.y;
                fx32 vx = (dx > 0) ? CROC_SPIT_SPEED : -CROC_SPIT_SPEED;
                fx32 vy = (dy > 0) ? (CROC_SPIT_SPEED >> 2) :
                                     -(CROC_SPIT_SPEED >> 2);
                projectile_spawn(PROJ_ENEMY_BULLET, PROJ_OWNER_ENEMY,
                                b->body.pos.x, b->body.pos.y, vx, vy);
                b->attack_count++;
            }

            b->ai_timer++;
            if (b->ai_timer >= CROC_SPIT_FRAMES) {
                b->ai_state = CROC_ADVANCE;
                b->ai_timer = 0;
            }
            break;
        }

        case CROC_FLINCH: {
            /* Stunned briefly after being hit */
            b->ai_timer++;
            if (b->ai_timer >= CROC_FLINCH_FRAMES) {
                b->ai_state = CROC_ADVANCE;
                b->ai_timer = 0;
            }
            break;
        }

        case CROC_LUNGE: {
            /* Quick charge toward player */
            fx32 dx = g_player.body.pos.x - b->body.pos.x;
            if (dx < 0) {
                b->body.pos.x -= CROC_LUNGE_SPEED;
            } else {
                b->body.pos.x += CROC_LUNGE_SPEED;
            }

            b->ai_timer++;
            if (b->ai_timer >= CROC_LUNGE_FRAMES) {
                b->ai_state = CROC_ADVANCE;
                b->ai_timer = 0;
            }
            break;
        }

        case CROC_FALLING: {
            /* Falling into lava */
            b->body.pos.y += CROC_FALL_SPEED;
            b->ai_timer++;
            if (b->ai_timer >= CROC_FALL_FRAMES) {
                b->ai_state = CROC_DEATH;
                b->ai_timer = 0;
            }
            break;
        }

        case CROC_DEATH: {
            b->ai_timer++;
            if (b->ai_timer >= CROC_DEATH_FRAMES) {
                b->active = false;
            }
            break;
        }
    }

    /* Contact damage to player (not during death) */
    if (b->active && b->ai_state != CROC_FALLING && b->ai_state != CROC_DEATH) {
        if (boss_aabb_overlap(b->body.pos, b->body.hitbox,
                              g_player.body.pos, g_player.body.hitbox)) {
            player_damage(b->damage_contact);
        }
    }
}

/* ========================================================================
 * Bomb Torizo AI
 *
 * 800 HP, starts as a statue, wakes up, throws bomb projectiles
 * in arcs and lunges at the player.
 *
 * Behavior:
 *   1. STATUE:  Dormant until player proximity triggers wake
 *   2. WAKE:    Statue-crumbling animation
 *   3. IDLE:    Facing player, choosing next attack
 *   4. BOMB:    Throw bomb projectile in arc
 *   5. LUNGE:   Charge toward player
 *   6. FLINCH:  Brief hit reaction
 *   7. DEATH:   Collapse animation
 * ======================================================================== */

/* Bomb Torizo AI states */
enum {
    BT_STATUE = 0,
    BT_WAKE,
    BT_IDLE,
    BT_BOMB,
    BT_LUNGE,
    BT_FLINCH,
    BT_DEATH
};

/* Bomb Torizo constants */
#define BT_HP                  800
#define BT_CONTACT_DAMAGE      20
#define BT_HITBOX_HALF_W       INT_TO_FX(12)
#define BT_HITBOX_HALF_H       INT_TO_FX(20)
#define BT_WAKE_DIST           INT_TO_FX(80) /* Player proximity to trigger wake */
#define BT_WAKE_FRAMES         60
#define BT_IDLE_MIN            30
#define BT_IDLE_RANGE          60  /* Idle duration = MIN + (counter % RANGE) */
#define BT_BOMB_VX             INT_TO_FX(2)
#define BT_BOMB_VY             (-INT_TO_FX(3))  /* Arc upward */
#define BT_BOMB_FRAMES         30  /* Attack animation duration */
#define BT_LUNGE_SPEED         INT_TO_FX(2)
#define BT_LUNGE_FRAMES        20
#define BT_FLINCH_FRAMES       10
#define BT_DEATH_FRAMES        60
#define BT_LUNGE_EVERY         2   /* Lunge after every N bomb throws */

static void bomb_torizo_init(void) {
    g_boss.hp = BT_HP;
    g_boss.hp_max = BT_HP;
    g_boss.damage_contact = BT_CONTACT_DAMAGE;
    g_boss.body.hitbox.half_w = BT_HITBOX_HALF_W;
    g_boss.body.hitbox.half_h = BT_HITBOX_HALF_H;
    g_boss.vulnerable = false;  /* Not vulnerable until awake */
    g_boss.ai_state = BT_STATUE;
    g_boss.ai_timer = 0;
    g_boss.ai_counter = 0;
    g_boss.sub_timer = 0;
    g_boss.attack_count = 0;

    /* Remember spawn position for lunge return */
    g_boss.param_a = g_boss.body.pos.x;
}

static void bomb_torizo_update(void) {
    Boss* b = &g_boss;

    if (b->invuln_timer > 0) b->invuln_timer--;

    switch (b->ai_state) {
        case BT_STATUE: {
            /* Wake when player gets close enough */
            fx32 dx = g_player.body.pos.x - b->body.pos.x;
            fx32 dy = g_player.body.pos.y - b->body.pos.y;
            if (dx < 0) dx = -dx;
            if (dy < 0) dy = -dy;
            if (dx < BT_WAKE_DIST && dy < BT_WAKE_DIST) {
                b->ai_state = BT_WAKE;
                b->ai_timer = 0;
                camera_shake(15, 2);
            }
            break;
        }

        case BT_WAKE: {
            b->ai_timer++;
            if (b->ai_timer >= BT_WAKE_FRAMES) {
                b->ai_state = BT_IDLE;
                b->ai_timer = 0;
                b->vulnerable = true;
                /* Set first idle duration */
                b->sub_timer = BT_IDLE_MIN + (b->ai_counter % BT_IDLE_RANGE);
            }
            break;
        }

        case BT_IDLE: {
            b->ai_timer++;
            if (b->ai_timer >= b->sub_timer) {
                /* Choose attack: lunge or bomb */
                if (b->attack_count >= BT_LUNGE_EVERY) {
                    b->ai_state = BT_LUNGE;
                    b->attack_count = 0;
                } else {
                    b->ai_state = BT_BOMB;
                }
                b->ai_timer = 0;
            }
            break;
        }

        case BT_BOMB: {
            /* Throw bomb on first frame */
            if (b->ai_timer == 0) {
                fx32 dx = g_player.body.pos.x - b->body.pos.x;
                fx32 vx = (dx > 0) ? BT_BOMB_VX : -BT_BOMB_VX;
                projectile_spawn(PROJ_ENEMY_BULLET, PROJ_OWNER_ENEMY,
                                b->body.pos.x, b->body.pos.y - INT_TO_FX(8),
                                vx, BT_BOMB_VY);
                b->attack_count++;
                b->ai_counter++;
            }

            b->ai_timer++;
            if (b->ai_timer >= BT_BOMB_FRAMES) {
                b->ai_state = BT_IDLE;
                b->ai_timer = 0;
                b->sub_timer = BT_IDLE_MIN + (b->ai_counter % BT_IDLE_RANGE);
            }
            break;
        }

        case BT_LUNGE: {
            /* Charge toward player */
            fx32 dx = g_player.body.pos.x - b->body.pos.x;
            if (dx < 0) {
                b->body.pos.x -= BT_LUNGE_SPEED;
            } else {
                b->body.pos.x += BT_LUNGE_SPEED;
            }

            b->ai_timer++;
            if (b->ai_timer >= BT_LUNGE_FRAMES) {
                b->ai_state = BT_IDLE;
                b->ai_timer = 0;
                b->sub_timer = BT_IDLE_MIN + (b->ai_counter % BT_IDLE_RANGE);
            }
            break;
        }

        case BT_FLINCH: {
            b->ai_timer++;
            if (b->ai_timer >= BT_FLINCH_FRAMES) {
                b->ai_state = BT_IDLE;
                b->ai_timer = 0;
                b->sub_timer = BT_IDLE_MIN + (b->ai_counter % BT_IDLE_RANGE);
            }
            break;
        }

        case BT_DEATH: {
            b->ai_timer++;
            if (b->ai_timer >= BT_DEATH_FRAMES) {
                b->active = false;
            }
            break;
        }
    }

    /* Contact damage (not during statue or death) */
    if (b->active && b->ai_state != BT_STATUE && b->ai_state != BT_DEATH) {
        if (boss_aabb_overlap(b->body.pos, b->body.hitbox,
                              g_player.body.pos, g_player.body.hitbox)) {
            player_damage(b->damage_contact);
        }
    }
}

/* ========================================================================
 * AI Dispatch Tables
 * ======================================================================== */

typedef void (*BossInitFn)(void);
typedef void (*BossUpdateFn)(void);

static const BossInitFn boss_init_fns[BOSS_TYPE_COUNT] = {
    [BOSS_SPORE_SPAWN]  = spore_spawn_init,
    [BOSS_CROCOMIRE]    = crocomire_init,
    [BOSS_BOMB_TORIZO]  = bomb_torizo_init,
};

static const BossUpdateFn boss_update_fns[BOSS_TYPE_COUNT] = {
    [BOSS_SPORE_SPAWN]  = spore_spawn_update,
    [BOSS_CROCOMIRE]    = crocomire_update,
    [BOSS_BOMB_TORIZO]  = bomb_torizo_update,
};

/* ========================================================================
 * Public API
 * ======================================================================== */

void boss_init(void) {
    memset(&g_boss, 0, sizeof(Boss));
    sprites_loaded = false;
}

void boss_spawn(BossTypeID type, fx32 x, fx32 y) {
    if (type == BOSS_NONE || type >= BOSS_TYPE_COUNT) return;

    load_boss_sprites();

    memset(&g_boss, 0, sizeof(Boss));
    g_boss.type = type;
    g_boss.active = true;
    g_boss.body.pos.x = x;
    g_boss.body.pos.y = y;
    g_boss.body.env = ENV_AIR;

    /* Type-specific initialization */
    if (boss_init_fns[type]) {
        boss_init_fns[type]();
    }
}

void boss_update(void) {
    if (!g_boss.active) return;

    if (boss_update_fns[g_boss.type]) {
        boss_update_fns[g_boss.type]();
    }
}

void boss_render(void) {
    if (!g_boss.active) {
        graphics_hide_sprite(BOSS_OAM_START);
        return;
    }

    int cam_x = FX_TO_INT(g_camera.x);
    int cam_y = FX_TO_INT(g_camera.y);

    int sx = FX_TO_INT(g_boss.body.pos.x) - cam_x - 8;
    int sy = FX_TO_INT(g_boss.body.pos.y) - cam_y - 8;

    /* Cull off-screen */
    if (sx < -16 || sx > SCREEN_WIDTH || sy < -16 || sy > SCREEN_HEIGHT) {
        graphics_hide_sprite(BOSS_OAM_START);
        return;
    }

    /* Blink when invulnerable (hide on odd frames) */
    if (g_boss.invuln_timer > 0 && (g_boss.invuln_timer & 1)) {
        graphics_hide_sprite(BOSS_OAM_START);
        return;
    }

    graphics_set_sprite(BOSS_OAM_START, sx, sy, 12, 3, 1, false, false);
}

void boss_damage(int16_t damage) {
    if (!g_boss.active) return;
    if (!g_boss.vulnerable) return;
    if (g_boss.invuln_timer > 0) return;

    /* Crocomire: push mechanic instead of HP damage */
    if (g_boss.type == BOSS_CROCOMIRE) {
        g_boss.invuln_timer = BOSS_HIT_INVULN;
        camera_shake(5, 2);

        /* Push toward lava (increase X position) */
        g_boss.body.pos.x += CROC_PUSH_PER_HIT;
        g_boss.ai_state = CROC_FLINCH;
        g_boss.ai_timer = 0;

        /* Check lava threshold */
        if (g_boss.body.pos.x >= g_boss.anchor_x) {
            g_boss.body.pos.x = g_boss.anchor_x;
            g_boss.vulnerable = false;
            g_boss.ai_state = CROC_FALLING;
            g_boss.ai_timer = 0;
            camera_shake(30, 4);
        }
        return;
    }

    /* Standard HP-based damage for all other bosses */
    g_boss.hp -= damage;
    g_boss.invuln_timer = BOSS_HIT_INVULN;
    camera_shake(5, 2);

    if (g_boss.hp <= 0) {
        g_boss.hp = 0;
        g_boss.vulnerable = false;
        g_boss.ai_timer = 0;
        camera_shake(30, 4);

        /* Transition to boss-specific death state */
        switch (g_boss.type) {
            case BOSS_SPORE_SPAWN:
                g_boss.ai_state = SS_DEATH;
                break;
            case BOSS_BOMB_TORIZO:
                g_boss.ai_state = BT_DEATH;
                break;
            default:
                g_boss.active = false;
                break;
        }
    }
}

bool boss_is_active(void) {
    return g_boss.active;
}
