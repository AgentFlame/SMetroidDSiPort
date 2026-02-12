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
 *   Kraid        - 1000 HP, mouth-only vulnerability, fingernails, belly spikes
 *   Botwoon      - 3000 HP, emerge/spit from holes, serpentine snake phase
 *   Phantoon     - 2500 HP, visibility cycle, flame attacks, super missile rage
 *   Draygon      - 6000 HP, swim/swoop/grab, gunk spit
 *   Golden Torizo- 8000 HP, catches super missiles, energy balls, lunge
 *   Ridley       - 18000 HP, aggression scales with HP, 4 attack types
 *   Mother Brain - 3 phases (3000/18000/36000 HP), phase transitions on death
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
 * Kraid AI
 *
 * 1000 HP multi-part boss. Only vulnerable when mouth is open (roaring).
 * Attacks with fingernail projectiles and belly spikes.
 * Begins fight by rising from the floor.
 *
 * Behavior cycle:
 *   1. RISE:        Emerge from floor (one-time entrance)
 *   2. IDLE:        Standing, mouth closed, choosing attacks
 *   3. ROAR:        Mouth open — VULNERABLE
 *   4. FINGERNAILS: Shoot 3 fingernail projectiles
 *   5. BELLY_SPIKE: Shoot belly spikes upward (arc)
 *   6. FLINCH:      Hit reaction (mouth closes)
 *   7. DEATH:       Collapse animation
 *
 * Boss-specific param usage:
 *   param_a     = rise target Y
 *   param_b     = attacks this cycle (reset on roar)
 *   anchor_x    = (unused)
 *   anchor_y    = (unused)
 *   sub_timer   = idle duration
 *   attack_count = total attacks since last roar
 * ======================================================================== */

/* Kraid AI states */
enum {
    KRAID_RISE = 0,
    KRAID_IDLE,
    KRAID_ROAR,
    KRAID_FINGERNAILS,
    KRAID_BELLY_SPIKE,
    KRAID_FLINCH,
    KRAID_DEATH
};

/* Kraid constants */
#define KRAID_HP               1000
#define KRAID_CONTACT_DAMAGE   40
#define KRAID_HITBOX_HALF_W    INT_TO_FX(20)
#define KRAID_HITBOX_HALF_H    INT_TO_FX(24)
#define KRAID_RISE_SPEED       INT_TO_FX(1)  /* 1 px/f rising */
#define KRAID_RISE_OFFSET      INT_TO_FX(48) /* Start 48px below target */
#define KRAID_IDLE_MIN         60
#define KRAID_IDLE_RANGE       60
#define KRAID_ROAR_FRAMES      90            /* ~1.5 sec vulnerability window */
#define KRAID_FLINCH_FRAMES    15
#define KRAID_NAIL_FRAMES      30            /* Fingernail attack duration */
#define KRAID_SPIKE_FRAMES     30            /* Belly spike attack duration */
#define KRAID_NAIL_SPEED       INT_TO_FX(3)  /* Fingernail projectile speed */
#define KRAID_SPIKE_VX         INT_TO_FX(1)
#define KRAID_SPIKE_VY         (-INT_TO_FX(4)) /* Upward arc */
#define KRAID_ROAR_EVERY       3             /* Roar after every N attacks */
#define KRAID_DEATH_FRAMES     90

static void kraid_init(void) {
    g_boss.hp = KRAID_HP;
    g_boss.hp_max = KRAID_HP;
    g_boss.damage_contact = KRAID_CONTACT_DAMAGE;
    g_boss.body.hitbox.half_w = KRAID_HITBOX_HALF_W;
    g_boss.body.hitbox.half_h = KRAID_HITBOX_HALF_H;
    g_boss.vulnerable = false;
    g_boss.ai_state = KRAID_RISE;
    g_boss.ai_timer = 0;
    g_boss.ai_counter = 0;
    g_boss.sub_timer = 0;
    g_boss.attack_count = 0;

    /* Rise from below: target Y = spawn Y, start lower */
    g_boss.param_a = g_boss.body.pos.y;
    g_boss.body.pos.y += KRAID_RISE_OFFSET;
}

static void kraid_update(void) {
    Boss* b = &g_boss;

    if (b->invuln_timer > 0) b->invuln_timer--;

    switch (b->ai_state) {
        case KRAID_RISE: {
            /* Rise from floor to target position */
            b->body.pos.y -= KRAID_RISE_SPEED;
            if (b->body.pos.y <= b->param_a) {
                b->body.pos.y = b->param_a;
                b->ai_state = KRAID_IDLE;
                b->ai_timer = 0;
                b->sub_timer = KRAID_IDLE_MIN;
                camera_shake(20, 3);
            }
            break;
        }

        case KRAID_IDLE: {
            b->ai_timer++;
            if (b->ai_timer >= b->sub_timer) {
                /* After enough attacks, open mouth (roar) */
                if (b->attack_count >= KRAID_ROAR_EVERY) {
                    b->ai_state = KRAID_ROAR;
                    b->ai_timer = 0;
                    b->vulnerable = true;
                    b->attack_count = 0;
                } else {
                    /* Choose attack: fingernails (common) or belly spike */
                    if ((b->ai_counter & 1) == 0) {
                        b->ai_state = KRAID_FINGERNAILS;
                    } else {
                        b->ai_state = KRAID_BELLY_SPIKE;
                    }
                    b->ai_timer = 0;
                }
            }
            break;
        }

        case KRAID_ROAR: {
            b->ai_timer++;
            if (b->ai_timer >= KRAID_ROAR_FRAMES) {
                b->ai_state = KRAID_IDLE;
                b->ai_timer = 0;
                b->vulnerable = false;
                b->sub_timer = KRAID_IDLE_MIN +
                               (b->ai_counter % KRAID_IDLE_RANGE);
            }
            break;
        }

        case KRAID_FINGERNAILS: {
            /* Shoot 3 fingernails on first frame (spread pattern) */
            if (b->ai_timer == 0) {
                fx32 dx = g_player.body.pos.x - b->body.pos.x;
                fx32 base_vx = (dx > 0) ? KRAID_NAIL_SPEED :
                                           -KRAID_NAIL_SPEED;
                /* Top nail */
                projectile_spawn(PROJ_ENEMY_BULLET, PROJ_OWNER_ENEMY,
                                b->body.pos.x, b->body.pos.y - INT_TO_FX(16),
                                base_vx, -(KRAID_NAIL_SPEED >> 2));
                /* Middle nail */
                projectile_spawn(PROJ_ENEMY_BULLET, PROJ_OWNER_ENEMY,
                                b->body.pos.x, b->body.pos.y,
                                base_vx, 0);
                /* Bottom nail */
                projectile_spawn(PROJ_ENEMY_BULLET, PROJ_OWNER_ENEMY,
                                b->body.pos.x, b->body.pos.y + INT_TO_FX(16),
                                base_vx, KRAID_NAIL_SPEED >> 2);
                b->attack_count++;
                b->ai_counter++;
            }

            b->ai_timer++;
            if (b->ai_timer >= KRAID_NAIL_FRAMES) {
                b->ai_state = KRAID_IDLE;
                b->ai_timer = 0;
                b->sub_timer = KRAID_IDLE_MIN +
                               (b->ai_counter % KRAID_IDLE_RANGE);
            }
            break;
        }

        case KRAID_BELLY_SPIKE: {
            /* Shoot belly spikes in arc on first frame */
            if (b->ai_timer == 0) {
                fx32 dx = g_player.body.pos.x - b->body.pos.x;
                fx32 spike_vx = (dx > 0) ? KRAID_SPIKE_VX : -KRAID_SPIKE_VX;
                projectile_spawn(PROJ_ENEMY_BULLET, PROJ_OWNER_ENEMY,
                                b->body.pos.x, b->body.pos.y + INT_TO_FX(8),
                                spike_vx, KRAID_SPIKE_VY);
                /* Second spike offset */
                projectile_spawn(PROJ_ENEMY_BULLET, PROJ_OWNER_ENEMY,
                                b->body.pos.x, b->body.pos.y + INT_TO_FX(8),
                                spike_vx + (spike_vx >> 1), KRAID_SPIKE_VY);
                b->attack_count++;
                b->ai_counter++;
            }

            b->ai_timer++;
            if (b->ai_timer >= KRAID_SPIKE_FRAMES) {
                b->ai_state = KRAID_IDLE;
                b->ai_timer = 0;
                b->sub_timer = KRAID_IDLE_MIN +
                               (b->ai_counter % KRAID_IDLE_RANGE);
            }
            break;
        }

        case KRAID_FLINCH: {
            b->ai_timer++;
            if (b->ai_timer >= KRAID_FLINCH_FRAMES) {
                b->ai_state = KRAID_IDLE;
                b->ai_timer = 0;
                b->vulnerable = false;
                b->sub_timer = KRAID_IDLE_MIN;
            }
            break;
        }

        case KRAID_DEATH: {
            /* Sink back down */
            b->body.pos.y += KRAID_RISE_SPEED;
            b->ai_timer++;
            if (b->ai_timer >= KRAID_DEATH_FRAMES) {
                b->active = false;
            }
            break;
        }
    }

    /* Contact damage (not during rise or death) */
    if (b->active && b->ai_state != KRAID_RISE && b->ai_state != KRAID_DEATH) {
        if (boss_aabb_overlap(b->body.pos, b->body.hitbox,
                              g_player.body.pos, g_player.body.hitbox)) {
            player_damage(b->damage_contact);
        }
    }
}

/* ========================================================================
 * Botwoon AI
 *
 * 3000 HP snake boss. Moves in a sinusoidal pattern, poking head
 * out of holes to spit projectiles. Only head is vulnerable.
 *
 * Behavior:
 *   1. HIDDEN:  Head retracted, choosing next hole
 *   2. EMERGE:  Head pokes out of a wall hole
 *   3. SPIT:    Fires projectiles at player
 *   4. RETREAT: Head retracts
 *   5. SNAKE:   Rapid serpentine movement across room (phase 2)
 *   6. DEATH:   Collapse
 *
 * Boss-specific param usage:
 *   param_a     = sine angle for snake movement
 *   param_b     = hole index (0-3, which side/position head emerges from)
 *   anchor_x    = room center X (for snake patrol)
 *   anchor_y    = room center Y
 *   sub_timer   = emerge count per cycle
 *   attack_count = total emerges before snake phase
 * ======================================================================== */

/* Botwoon AI states */
enum {
    BOT_HIDDEN = 0,
    BOT_EMERGE,
    BOT_SPIT,
    BOT_RETREAT,
    BOT_SNAKE,
    BOT_DEATH
};

/* Botwoon constants */
#define BOT_HP                 3000
#define BOT_CONTACT_DAMAGE     30
#define BOT_HITBOX_HALF_W      INT_TO_FX(10)
#define BOT_HITBOX_HALF_H      INT_TO_FX(10)
#define BOT_HIDDEN_FRAMES      45
#define BOT_EMERGE_FRAMES      20
#define BOT_SPIT_FRAMES        30
#define BOT_RETREAT_FRAMES     15
#define BOT_SPIT_SPEED         INT_TO_FX(3)
#define BOT_SNAKE_SPEED        INT_TO_FX(2)
#define BOT_SNAKE_AMPLITUDE    INT_TO_FX(40)
#define BOT_SNAKE_FREQ         4          /* LUT units per frame */
#define BOT_SNAKE_FRAMES       180        /* ~3 sec snake phase */
#define BOT_EMERGE_PER_CYCLE   4          /* Emerges before snake phase */
#define BOT_DEATH_FRAMES       60

/* Hole positions (relative to room center) */
static const int16_t bot_hole_offsets[4][2] = {
    { -60, -30 },  /* Top-left */
    {  60, -30 },  /* Top-right */
    { -60,  30 },  /* Bottom-left */
    {  60,  30 },  /* Bottom-right */
};

static void botwoon_init(void) {
    g_boss.hp = BOT_HP;
    g_boss.hp_max = BOT_HP;
    g_boss.damage_contact = BOT_CONTACT_DAMAGE;
    g_boss.body.hitbox.half_w = BOT_HITBOX_HALF_W;
    g_boss.body.hitbox.half_h = BOT_HITBOX_HALF_H;
    g_boss.vulnerable = false;
    g_boss.ai_state = BOT_HIDDEN;
    g_boss.ai_timer = 0;
    g_boss.ai_counter = 0;
    g_boss.sub_timer = 0;
    g_boss.attack_count = 0;
    g_boss.param_a = 0;
    g_boss.param_b = 0;

    /* Remember room center */
    g_boss.anchor_x = g_boss.body.pos.x;
    g_boss.anchor_y = g_boss.body.pos.y;
}

static void botwoon_update(void) {
    Boss* b = &g_boss;

    if (b->invuln_timer > 0) b->invuln_timer--;

    switch (b->ai_state) {
        case BOT_HIDDEN: {
            b->ai_timer++;
            if (b->ai_timer >= BOT_HIDDEN_FRAMES) {
                /* Pick next hole based on counter */
                int hole = b->ai_counter & 3;
                b->param_b = INT_TO_FX(hole);
                b->body.pos.x = b->anchor_x +
                                INT_TO_FX(bot_hole_offsets[hole][0]);
                b->body.pos.y = b->anchor_y +
                                INT_TO_FX(bot_hole_offsets[hole][1]);
                b->ai_state = BOT_EMERGE;
                b->ai_timer = 0;
            }
            break;
        }

        case BOT_EMERGE: {
            b->ai_timer++;
            if (b->ai_timer >= BOT_EMERGE_FRAMES) {
                b->ai_state = BOT_SPIT;
                b->ai_timer = 0;
                b->vulnerable = true;
            }
            break;
        }

        case BOT_SPIT: {
            /* Fire on first frame */
            if (b->ai_timer == 0) {
                fx32 dx = g_player.body.pos.x - b->body.pos.x;
                fx32 dy = g_player.body.pos.y - b->body.pos.y;
                fx32 vx = (dx > 0) ? BOT_SPIT_SPEED : -BOT_SPIT_SPEED;
                fx32 vy = (dy > 0) ? (BOT_SPIT_SPEED >> 1) :
                                     -(BOT_SPIT_SPEED >> 1);
                projectile_spawn(PROJ_ENEMY_BULLET, PROJ_OWNER_ENEMY,
                                b->body.pos.x, b->body.pos.y, vx, vy);
            }

            b->ai_timer++;
            if (b->ai_timer >= BOT_SPIT_FRAMES) {
                b->ai_state = BOT_RETREAT;
                b->ai_timer = 0;
                b->vulnerable = false;
                b->sub_timer++;
                b->ai_counter++;
            }
            break;
        }

        case BOT_RETREAT: {
            b->ai_timer++;
            if (b->ai_timer >= BOT_RETREAT_FRAMES) {
                /* After enough emerges, do snake movement */
                if (b->sub_timer >= BOT_EMERGE_PER_CYCLE) {
                    b->ai_state = BOT_SNAKE;
                    b->ai_timer = 0;
                    b->param_a = 0;
                    b->sub_timer = 0;
                    b->vulnerable = true;
                } else {
                    b->ai_state = BOT_HIDDEN;
                    b->ai_timer = 0;
                }
            }
            break;
        }

        case BOT_SNAKE: {
            /* Serpentine movement across room */
            b->param_a += INT_TO_FX(BOT_SNAKE_FREQ);
            uint8_t angle = (uint8_t)FX_TO_INT(b->param_a);

            /* Horizontal sweep + sine vertical */
            fx32 progress = fx_div(INT_TO_FX(b->ai_timer),
                                   INT_TO_FX(BOT_SNAKE_FRAMES));
            /* Sweep left to right and back */
            fx32 sweep_x;
            if (b->ai_timer < BOT_SNAKE_FRAMES / 2) {
                sweep_x = b->anchor_x - INT_TO_FX(60) +
                          fx_mul(INT_TO_FX(120), progress << 1);
            } else {
                fx32 rev = FX_ONE - ((progress - FX_HALF) << 1);
                sweep_x = b->anchor_x - INT_TO_FX(60) +
                          fx_mul(INT_TO_FX(120), rev);
            }
            b->body.pos.x = sweep_x;
            b->body.pos.y = b->anchor_y +
                            fx_mul(fx_sin(angle), BOT_SNAKE_AMPLITUDE);

            b->ai_timer++;
            if (b->ai_timer >= BOT_SNAKE_FRAMES) {
                b->ai_state = BOT_HIDDEN;
                b->ai_timer = 0;
                b->vulnerable = false;
            }
            break;
        }

        case BOT_DEATH: {
            b->ai_timer++;
            if (b->ai_timer >= BOT_DEATH_FRAMES) {
                b->active = false;
            }
            break;
        }
    }

    /* Contact damage (only when visible) */
    if (b->active && b->ai_state != BOT_HIDDEN && b->ai_state != BOT_DEATH) {
        if (boss_aabb_overlap(b->body.pos, b->body.hitbox,
                              g_player.body.pos, g_player.body.hitbox)) {
            player_damage(b->damage_contact);
        }
    }
}

/* ========================================================================
 * Phantoon AI
 *
 * 2500 HP ghost boss. Cycles between invisible and visible. Only
 * vulnerable when visible (eye open). Enrages if hit with super missile
 * (faster, more flames).
 *
 * Behavior:
 *   1. INVISIBLE:  Floating off-screen, invulnerable
 *   2. FADE_IN:    Becoming visible (20 frames)
 *   3. VISIBLE:    Eye open — VULNERABLE, attacking with flame circles
 *   4. FADE_OUT:   Becoming invisible (20 frames)
 *   5. RAGE:       Enraged mode (faster attacks, more flames)
 *   6. DEATH:      Fade away
 *
 * Boss-specific param usage:
 *   param_a     = float angle (sine-based hover)
 *   param_b     = rage flag (0 = normal, 1 = enraged)
 *   anchor_x    = center X of patrol area
 *   anchor_y    = center Y of patrol area
 *   sub_timer   = flame attack cooldown
 *   attack_count = flames fired this cycle
 * ======================================================================== */

/* Phantoon AI states */
enum {
    PH_INVISIBLE = 0,
    PH_FADE_IN,
    PH_VISIBLE,
    PH_FADE_OUT,
    PH_RAGE,
    PH_DEATH
};

/* Phantoon constants */
#define PH_HP                  2500
#define PH_CONTACT_DAMAGE      30
#define PH_HITBOX_HALF_W       INT_TO_FX(16)
#define PH_HITBOX_HALF_H       INT_TO_FX(16)
#define PH_INVIS_FRAMES        90           /* Time spent invisible */
#define PH_FADE_FRAMES         20
#define PH_VISIBLE_FRAMES      120          /* ~2 sec visible */
#define PH_RAGE_FRAMES         180          /* ~3 sec rage */
#define PH_FLAME_INTERVAL      30           /* Normal flame rate */
#define PH_RAGE_FLAME_INTERVAL 15           /* Rage flame rate */
#define PH_FLAME_SPEED         INT_TO_FX(2)
#define PH_RAGE_FLAME_SPEED    INT_TO_FX(3)
#define PH_FLOAT_SPEED         3            /* LUT angle units per frame */
#define PH_FLOAT_AMPLITUDE     INT_TO_FX(20)
#define PH_HOVER_SPEED         0x8000       /* 0.5 px/f drift */
#define PH_DEATH_FRAMES        60
#define PH_FLAMES_PER_CYCLE    4
#define PH_RAGE_FLAMES         8

static void phantoon_init(void) {
    g_boss.hp = PH_HP;
    g_boss.hp_max = PH_HP;
    g_boss.damage_contact = PH_CONTACT_DAMAGE;
    g_boss.body.hitbox.half_w = PH_HITBOX_HALF_W;
    g_boss.body.hitbox.half_h = PH_HITBOX_HALF_H;
    g_boss.vulnerable = false;
    g_boss.ai_state = PH_INVISIBLE;
    g_boss.ai_timer = 0;
    g_boss.ai_counter = 0;
    g_boss.sub_timer = 0;
    g_boss.attack_count = 0;
    g_boss.param_a = 0;
    g_boss.param_b = 0;  /* Not enraged */

    g_boss.anchor_x = g_boss.body.pos.x;
    g_boss.anchor_y = g_boss.body.pos.y;
}

static void phantoon_update(void) {
    Boss* b = &g_boss;

    if (b->invuln_timer > 0) b->invuln_timer--;

    /* Hover float (applies during visible/rage states) */
    if (b->ai_state == PH_VISIBLE || b->ai_state == PH_RAGE) {
        b->param_a += INT_TO_FX(PH_FLOAT_SPEED);
        uint8_t angle = (uint8_t)FX_TO_INT(b->param_a);
        b->body.pos.y = b->anchor_y +
                        fx_mul(fx_sin(angle), PH_FLOAT_AMPLITUDE);

        /* Drift toward player X */
        fx32 dx = g_player.body.pos.x - b->body.pos.x;
        if (dx > 0) {
            b->body.pos.x += PH_HOVER_SPEED;
        } else if (dx < 0) {
            b->body.pos.x -= PH_HOVER_SPEED;
        }
    }

    switch (b->ai_state) {
        case PH_INVISIBLE: {
            b->ai_timer++;
            if (b->ai_timer >= PH_INVIS_FRAMES) {
                b->ai_state = PH_FADE_IN;
                b->ai_timer = 0;
                /* Reposition near player */
                b->body.pos.x = g_player.body.pos.x + INT_TO_FX(40);
                b->body.pos.y = g_player.body.pos.y - INT_TO_FX(32);
                b->anchor_y = b->body.pos.y;
            }
            break;
        }

        case PH_FADE_IN: {
            b->ai_timer++;
            if (b->ai_timer >= PH_FADE_FRAMES) {
                /* Enter rage or normal visible depending on flag */
                if (b->param_b != 0) {
                    b->ai_state = PH_RAGE;
                } else {
                    b->ai_state = PH_VISIBLE;
                }
                b->ai_timer = 0;
                b->vulnerable = true;
                b->sub_timer = 0;
                b->attack_count = 0;
                b->param_a = 0;
            }
            break;
        }

        case PH_VISIBLE: {
            b->ai_timer++;
            b->sub_timer++;

            /* Shoot flames periodically */
            if (b->sub_timer >= PH_FLAME_INTERVAL &&
                b->attack_count < PH_FLAMES_PER_CYCLE) {
                b->sub_timer = 0;
                b->attack_count++;
                /* Flame aimed at player */
                fx32 dx = g_player.body.pos.x - b->body.pos.x;
                fx32 dy = g_player.body.pos.y - b->body.pos.y;
                fx32 vx = (dx > 0) ? PH_FLAME_SPEED : -PH_FLAME_SPEED;
                fx32 vy = (dy > 0) ? (PH_FLAME_SPEED >> 1) :
                                     -(PH_FLAME_SPEED >> 1);
                projectile_spawn(PROJ_ENEMY_BULLET, PROJ_OWNER_ENEMY,
                                b->body.pos.x, b->body.pos.y, vx, vy);
            }

            if (b->ai_timer >= PH_VISIBLE_FRAMES) {
                b->ai_state = PH_FADE_OUT;
                b->ai_timer = 0;
                b->vulnerable = false;
            }
            break;
        }

        case PH_FADE_OUT: {
            b->ai_timer++;
            if (b->ai_timer >= PH_FADE_FRAMES) {
                b->ai_state = PH_INVISIBLE;
                b->ai_timer = 0;
            }
            break;
        }

        case PH_RAGE: {
            /* Same as visible but faster and more flames */
            b->ai_timer++;
            b->sub_timer++;

            if (b->sub_timer >= PH_RAGE_FLAME_INTERVAL &&
                b->attack_count < PH_RAGE_FLAMES) {
                b->sub_timer = 0;
                b->attack_count++;
                /* Spread flame pattern */
                fx32 dx = g_player.body.pos.x - b->body.pos.x;
                fx32 vx = (dx > 0) ? PH_RAGE_FLAME_SPEED :
                                     -PH_RAGE_FLAME_SPEED;
                projectile_spawn(PROJ_ENEMY_BULLET, PROJ_OWNER_ENEMY,
                                b->body.pos.x, b->body.pos.y,
                                vx, -(PH_RAGE_FLAME_SPEED >> 1));
                projectile_spawn(PROJ_ENEMY_BULLET, PROJ_OWNER_ENEMY,
                                b->body.pos.x, b->body.pos.y,
                                vx, PH_RAGE_FLAME_SPEED >> 1);
            }

            if (b->ai_timer >= PH_RAGE_FRAMES) {
                b->ai_state = PH_FADE_OUT;
                b->ai_timer = 0;
                b->vulnerable = false;
            }
            break;
        }

        case PH_DEATH: {
            b->ai_timer++;
            if (b->ai_timer >= PH_DEATH_FRAMES) {
                b->active = false;
            }
            break;
        }
    }

    /* Contact damage (only when visible/rage) */
    if (b->active &&
        (b->ai_state == PH_VISIBLE || b->ai_state == PH_RAGE)) {
        if (boss_aabb_overlap(b->body.pos, b->body.hitbox,
                              g_player.body.pos, g_player.body.hitbox)) {
            player_damage(b->damage_contact);
        }
    }
}

/* ========================================================================
 * Draygon AI
 *
 * 6000 HP aquatic boss. Swims back and forth, swoops to grab Samus,
 * spits gunk. Can be killed by grapple + turret (instant kill path).
 *
 * Behavior:
 *   1. SWIM:     Patrol left/right across room
 *   2. SWOOP:    Dive toward player
 *   3. GRAB:     Holding Samus (drains HP per frame)
 *   4. SPIT:     Spit gunk projectiles
 *   5. RETREAT:  Return to patrol altitude
 *   6. DEATH:    Collapse
 *
 * Boss-specific param usage:
 *   param_a     = swim direction (positive = right, negative = left)
 *   param_b     = grab timer (frames holding Samus)
 *   anchor_x    = patrol center X
 *   anchor_y    = patrol altitude Y
 *   sub_timer   = attacks this patrol
 *   attack_count = total swoops
 * ======================================================================== */

enum {
    DY_SWIM = 0,
    DY_SWOOP,
    DY_GRAB,
    DY_SPIT,
    DY_RETREAT,
    DY_DEATH
};

#define DY_HP                  6000
#define DY_CONTACT_DAMAGE      40
#define DY_HITBOX_HALF_W       INT_TO_FX(18)
#define DY_HITBOX_HALF_H       INT_TO_FX(14)
#define DY_SWIM_SPEED          INT_TO_FX(1)
#define DY_SWIM_RANGE          INT_TO_FX(80)   /* Distance from center */
#define DY_SWIM_ATTACK_EVERY   120              /* Frames between attacks */
#define DY_SWOOP_SPEED         INT_TO_FX(3)
#define DY_SWOOP_FRAMES        25
#define DY_GRAB_FRAMES         90               /* How long Draygon holds */
#define DY_GRAB_DAMAGE         2                /* HP drain per frame */
#define DY_SPIT_FRAMES         30
#define DY_SPIT_SPEED          INT_TO_FX(2)
#define DY_RETREAT_SPEED       INT_TO_FX(2)
#define DY_DEATH_FRAMES        90

static void draygon_init(void) {
    g_boss.hp = DY_HP;
    g_boss.hp_max = DY_HP;
    g_boss.damage_contact = DY_CONTACT_DAMAGE;
    g_boss.body.hitbox.half_w = DY_HITBOX_HALF_W;
    g_boss.body.hitbox.half_h = DY_HITBOX_HALF_H;
    g_boss.vulnerable = true;
    g_boss.ai_state = DY_SWIM;
    g_boss.ai_timer = 0;
    g_boss.param_a = FX_ONE; /* Start swimming right */
    g_boss.param_b = 0;
    g_boss.sub_timer = 0;
    g_boss.attack_count = 0;

    g_boss.anchor_x = g_boss.body.pos.x;
    g_boss.anchor_y = g_boss.body.pos.y;
}

static void draygon_update(void) {
    Boss* b = &g_boss;

    if (b->invuln_timer > 0) b->invuln_timer--;

    switch (b->ai_state) {
        case DY_SWIM: {
            /* Patrol left/right */
            if (b->param_a > 0) {
                b->body.pos.x += DY_SWIM_SPEED;
                if (b->body.pos.x > b->anchor_x + DY_SWIM_RANGE) {
                    b->param_a = -FX_ONE;
                }
            } else {
                b->body.pos.x -= DY_SWIM_SPEED;
                if (b->body.pos.x < b->anchor_x - DY_SWIM_RANGE) {
                    b->param_a = FX_ONE;
                }
            }

            b->ai_timer++;
            if (b->ai_timer >= DY_SWIM_ATTACK_EVERY) {
                b->ai_timer = 0;
                /* Alternate swoop and spit */
                if ((b->sub_timer & 1) == 0) {
                    b->ai_state = DY_SWOOP;
                } else {
                    b->ai_state = DY_SPIT;
                }
                b->sub_timer++;
            }
            break;
        }

        case DY_SWOOP: {
            /* Dive toward player */
            fx32 dx = g_player.body.pos.x - b->body.pos.x;
            fx32 dy = g_player.body.pos.y - b->body.pos.y;
            if (dx > 0) b->body.pos.x += DY_SWOOP_SPEED;
            else if (dx < 0) b->body.pos.x -= DY_SWOOP_SPEED;
            if (dy > 0) b->body.pos.y += DY_SWOOP_SPEED;
            else if (dy < 0) b->body.pos.y -= DY_SWOOP_SPEED;

            /* Check if close enough to grab */
            fx32 adx = dx < 0 ? -dx : dx;
            fx32 ady = dy < 0 ? -dy : dy;
            if (adx < INT_TO_FX(16) && ady < INT_TO_FX(16)) {
                b->ai_state = DY_GRAB;
                b->ai_timer = 0;
                b->param_b = 0;
                b->attack_count++;
                break;
            }

            b->ai_timer++;
            if (b->ai_timer >= DY_SWOOP_FRAMES) {
                b->ai_state = DY_RETREAT;
                b->ai_timer = 0;
            }
            break;
        }

        case DY_GRAB: {
            /* Hold Samus, drain HP */
            b->param_b += FX_ONE;
            if (FX_TO_INT(b->param_b) < DY_GRAB_FRAMES) {
                /* Drain player HP every 15 frames */
                if ((FX_TO_INT(b->param_b) % 15) == 0) {
                    player_damage(DY_GRAB_DAMAGE);
                }
            } else {
                /* Release */
                b->ai_state = DY_RETREAT;
                b->ai_timer = 0;
            }
            break;
        }

        case DY_SPIT: {
            if (b->ai_timer == 0) {
                fx32 dx = g_player.body.pos.x - b->body.pos.x;
                fx32 vx = (dx > 0) ? DY_SPIT_SPEED : -DY_SPIT_SPEED;
                projectile_spawn(PROJ_ENEMY_BULLET, PROJ_OWNER_ENEMY,
                                b->body.pos.x, b->body.pos.y,
                                vx, DY_SPIT_SPEED >> 1);
                projectile_spawn(PROJ_ENEMY_BULLET, PROJ_OWNER_ENEMY,
                                b->body.pos.x, b->body.pos.y,
                                vx, -(DY_SPIT_SPEED >> 1));
            }

            b->ai_timer++;
            if (b->ai_timer >= DY_SPIT_FRAMES) {
                b->ai_state = DY_SWIM;
                b->ai_timer = 0;
            }
            break;
        }

        case DY_RETREAT: {
            /* Return to patrol altitude */
            fx32 dy = b->anchor_y - b->body.pos.y;
            if (dy > INT_TO_FX(2)) {
                b->body.pos.y += DY_RETREAT_SPEED;
            } else if (dy < -INT_TO_FX(2)) {
                b->body.pos.y -= DY_RETREAT_SPEED;
            } else {
                b->body.pos.y = b->anchor_y;
                b->ai_state = DY_SWIM;
                b->ai_timer = 0;
            }
            break;
        }

        case DY_DEATH: {
            b->body.pos.y += FX_ONE; /* Sink */
            b->ai_timer++;
            if (b->ai_timer >= DY_DEATH_FRAMES) {
                b->active = false;
            }
            break;
        }
    }

    /* Contact damage (not during death) */
    if (b->active && b->ai_state != DY_DEATH) {
        if (boss_aabb_overlap(b->body.pos, b->body.hitbox,
                              g_player.body.pos, g_player.body.hitbox)) {
            player_damage(b->damage_contact);
        }
    }
}

/* ========================================================================
 * Golden Torizo AI
 *
 * 8000 HP enhanced Torizo. Same base pattern as Bomb Torizo but faster,
 * can catch super missiles and throw them back. Two attack types:
 * energy balls and lunges.
 *
 * Special: If hit by super missile, catches it and throws it back.
 * param_b tracks "catch" state.
 * ======================================================================== */

enum {
    GT_IDLE = 0,
    GT_ATTACK_ENERGY,
    GT_ATTACK_LUNGE,
    GT_CATCH,       /* Caught a super missile */
    GT_THROW_BACK,  /* Throw caught projectile */
    GT_FLINCH,
    GT_DEATH
};

#define GT_HP                  8000
#define GT_CONTACT_DAMAGE      50
#define GT_HITBOX_HALF_W       INT_TO_FX(14)
#define GT_HITBOX_HALF_H       INT_TO_FX(22)
#define GT_IDLE_MIN            20
#define GT_IDLE_RANGE          40
#define GT_ENERGY_VX           INT_TO_FX(3)
#define GT_ENERGY_VY           (-INT_TO_FX(2))
#define GT_ENERGY_FRAMES       25
#define GT_LUNGE_SPEED         INT_TO_FX(3)
#define GT_LUNGE_FRAMES        18
#define GT_CATCH_FRAMES        20
#define GT_THROW_SPEED         INT_TO_FX(4)
#define GT_THROW_FRAMES        15
#define GT_FLINCH_FRAMES       8
#define GT_DEATH_FRAMES        60
#define GT_LUNGE_EVERY         2

static void golden_torizo_init(void) {
    g_boss.hp = GT_HP;
    g_boss.hp_max = GT_HP;
    g_boss.damage_contact = GT_CONTACT_DAMAGE;
    g_boss.body.hitbox.half_w = GT_HITBOX_HALF_W;
    g_boss.body.hitbox.half_h = GT_HITBOX_HALF_H;
    g_boss.vulnerable = true;
    g_boss.ai_state = GT_IDLE;
    g_boss.ai_timer = 0;
    g_boss.ai_counter = 0;
    g_boss.sub_timer = GT_IDLE_MIN;
    g_boss.attack_count = 0;
    g_boss.param_a = g_boss.body.pos.x;
    g_boss.param_b = 0;
}

static void golden_torizo_update(void) {
    Boss* b = &g_boss;

    if (b->invuln_timer > 0) b->invuln_timer--;

    switch (b->ai_state) {
        case GT_IDLE: {
            b->ai_timer++;
            if (b->ai_timer >= b->sub_timer) {
                if (b->attack_count >= GT_LUNGE_EVERY) {
                    b->ai_state = GT_ATTACK_LUNGE;
                    b->attack_count = 0;
                } else {
                    b->ai_state = GT_ATTACK_ENERGY;
                }
                b->ai_timer = 0;
            }
            break;
        }

        case GT_ATTACK_ENERGY: {
            if (b->ai_timer == 0) {
                fx32 dx = g_player.body.pos.x - b->body.pos.x;
                fx32 vx = (dx > 0) ? GT_ENERGY_VX : -GT_ENERGY_VX;
                projectile_spawn(PROJ_ENEMY_BULLET, PROJ_OWNER_ENEMY,
                                b->body.pos.x, b->body.pos.y - INT_TO_FX(8),
                                vx, GT_ENERGY_VY);
                b->attack_count++;
                b->ai_counter++;
            }

            b->ai_timer++;
            if (b->ai_timer >= GT_ENERGY_FRAMES) {
                b->ai_state = GT_IDLE;
                b->ai_timer = 0;
                b->sub_timer = GT_IDLE_MIN + (b->ai_counter % GT_IDLE_RANGE);
            }
            break;
        }

        case GT_ATTACK_LUNGE: {
            fx32 dx = g_player.body.pos.x - b->body.pos.x;
            if (dx < 0) b->body.pos.x -= GT_LUNGE_SPEED;
            else b->body.pos.x += GT_LUNGE_SPEED;

            b->ai_timer++;
            if (b->ai_timer >= GT_LUNGE_FRAMES) {
                b->ai_state = GT_IDLE;
                b->ai_timer = 0;
                b->ai_counter++;
                b->sub_timer = GT_IDLE_MIN + (b->ai_counter % GT_IDLE_RANGE);
            }
            break;
        }

        case GT_CATCH: {
            /* Brief pause after catching */
            b->ai_timer++;
            if (b->ai_timer >= GT_CATCH_FRAMES) {
                b->ai_state = GT_THROW_BACK;
                b->ai_timer = 0;
            }
            break;
        }

        case GT_THROW_BACK: {
            /* Throw projectile back at player */
            if (b->ai_timer == 0) {
                fx32 dx = g_player.body.pos.x - b->body.pos.x;
                fx32 vx = (dx > 0) ? GT_THROW_SPEED : -GT_THROW_SPEED;
                projectile_spawn(PROJ_ENEMY_BULLET, PROJ_OWNER_ENEMY,
                                b->body.pos.x, b->body.pos.y, vx, 0);
            }

            b->ai_timer++;
            if (b->ai_timer >= GT_THROW_FRAMES) {
                b->ai_state = GT_IDLE;
                b->ai_timer = 0;
                b->param_b = 0;
                b->sub_timer = GT_IDLE_MIN;
            }
            break;
        }

        case GT_FLINCH: {
            b->ai_timer++;
            if (b->ai_timer >= GT_FLINCH_FRAMES) {
                b->ai_state = GT_IDLE;
                b->ai_timer = 0;
                b->sub_timer = GT_IDLE_MIN + (b->ai_counter % GT_IDLE_RANGE);
            }
            break;
        }

        case GT_DEATH: {
            b->ai_timer++;
            if (b->ai_timer >= GT_DEATH_FRAMES) {
                b->active = false;
            }
            break;
        }
    }

    if (b->active && b->ai_state != GT_DEATH) {
        if (boss_aabb_overlap(b->body.pos, b->body.hitbox,
                              g_player.body.pos, g_player.body.hitbox)) {
            player_damage(b->damage_contact);
        }
    }
}

/* ========================================================================
 * Ridley AI
 *
 * 18000 HP. Most aggressive boss. Flies around room, attacks with
 * tail swipe, fireballs, grab, and pogo. Aggression scales with HP
 * (attacks faster at lower HP).
 *
 * Behavior:
 *   1. FLY:       Patrol around room
 *   2. TAIL:      Tail swipe (contact damage)
 *   3. FIREBALL:  Fire 1-3 fireballs at player
 *   4. GRAB:      Grab and carry Samus
 *   5. POGO:      Bouncing attack from above
 *   6. DEATH:     Explosion sequence
 * ======================================================================== */

enum {
    RI_FLY = 0,
    RI_TAIL,
    RI_FIREBALL,
    RI_GRAB,
    RI_POGO,
    RI_DEATH
};

#define RI_HP                  18000
#define RI_CONTACT_DAMAGE      60
#define RI_HITBOX_HALF_W       INT_TO_FX(16)
#define RI_HITBOX_HALF_H       INT_TO_FX(18)
#define RI_FLY_SPEED           INT_TO_FX(2)
#define RI_FLY_RANGE           INT_TO_FX(70)
#define RI_ATTACK_INTERVAL     90    /* Base interval (reduced at low HP) */
#define RI_TAIL_FRAMES         20
#define RI_FIREBALL_SPEED      INT_TO_FX(3)
#define RI_FIREBALL_FRAMES     25
#define RI_GRAB_FRAMES         60
#define RI_GRAB_DAMAGE         3
#define RI_POGO_SPEED          INT_TO_FX(4)
#define RI_POGO_FRAMES         30
#define RI_DEATH_FRAMES        120

static void ridley_init(void) {
    g_boss.hp = RI_HP;
    g_boss.hp_max = RI_HP;
    g_boss.damage_contact = RI_CONTACT_DAMAGE;
    g_boss.body.hitbox.half_w = RI_HITBOX_HALF_W;
    g_boss.body.hitbox.half_h = RI_HITBOX_HALF_H;
    g_boss.vulnerable = true;
    g_boss.ai_state = RI_FLY;
    g_boss.ai_timer = 0;
    g_boss.ai_counter = 0;
    g_boss.sub_timer = 0;
    g_boss.attack_count = 0;
    g_boss.param_a = FX_ONE; /* fly direction */

    g_boss.anchor_x = g_boss.body.pos.x;
    g_boss.anchor_y = g_boss.body.pos.y;
}

/* Aggression factor: attack interval decreases as HP drops */
static uint16_t ridley_attack_interval(void) {
    /* At full HP: base interval. At 50% HP: 60% interval. At 25%: 40% */
    int32_t ratio = (int32_t)g_boss.hp * 100 / g_boss.hp_max;
    if (ratio > 75) return RI_ATTACK_INTERVAL;
    if (ratio > 50) return (RI_ATTACK_INTERVAL * 3) / 4;
    if (ratio > 25) return (RI_ATTACK_INTERVAL * 3) / 5;
    return RI_ATTACK_INTERVAL / 3;
}

static void ridley_update(void) {
    Boss* b = &g_boss;

    if (b->invuln_timer > 0) b->invuln_timer--;

    switch (b->ai_state) {
        case RI_FLY: {
            /* Patrol with sine-wave altitude */
            if (b->param_a > 0) {
                b->body.pos.x += RI_FLY_SPEED;
                if (b->body.pos.x > b->anchor_x + RI_FLY_RANGE) {
                    b->param_a = -FX_ONE;
                }
            } else {
                b->body.pos.x -= RI_FLY_SPEED;
                if (b->body.pos.x < b->anchor_x - RI_FLY_RANGE) {
                    b->param_a = FX_ONE;
                }
            }
            /* Slight sine bob */
            b->sub_timer++;
            uint8_t bob = (uint8_t)(b->sub_timer * 2);
            b->body.pos.y = b->anchor_y +
                            fx_mul(fx_sin(bob), INT_TO_FX(12));

            b->ai_timer++;
            if (b->ai_timer >= ridley_attack_interval()) {
                b->ai_timer = 0;
                /* Pick attack based on counter */
                int attack = b->ai_counter % 4;
                switch (attack) {
                    case 0: b->ai_state = RI_TAIL; break;
                    case 1: b->ai_state = RI_FIREBALL; break;
                    case 2: b->ai_state = RI_GRAB; break;
                    case 3: b->ai_state = RI_POGO; break;
                }
                b->ai_counter++;
            }
            break;
        }

        case RI_TAIL: {
            /* Quick lunge (tail swipe is contact damage) */
            fx32 dx = g_player.body.pos.x - b->body.pos.x;
            if (dx < 0) b->body.pos.x -= RI_FLY_SPEED * 2;
            else b->body.pos.x += RI_FLY_SPEED * 2;

            b->ai_timer++;
            if (b->ai_timer >= RI_TAIL_FRAMES) {
                b->ai_state = RI_FLY;
                b->ai_timer = 0;
            }
            break;
        }

        case RI_FIREBALL: {
            if (b->ai_timer == 0) {
                /* Fire 1-3 based on aggression (lower HP = more) */
                int count = (g_boss.hp < g_boss.hp_max / 2) ? 3 : 1;
                fx32 dx = g_player.body.pos.x - b->body.pos.x;
                fx32 base_vx = (dx > 0) ? RI_FIREBALL_SPEED :
                                           -RI_FIREBALL_SPEED;
                for (int i = 0; i < count; i++) {
                    fx32 vy = INT_TO_FX(i - 1); /* -1, 0, +1 spread */
                    projectile_spawn(PROJ_ENEMY_BULLET, PROJ_OWNER_ENEMY,
                                    b->body.pos.x, b->body.pos.y,
                                    base_vx, vy);
                }
            }

            b->ai_timer++;
            if (b->ai_timer >= RI_FIREBALL_FRAMES) {
                b->ai_state = RI_FLY;
                b->ai_timer = 0;
            }
            break;
        }

        case RI_GRAB: {
            if (b->ai_timer < RI_GRAB_FRAMES / 2) {
                /* Chase player */
                fx32 dx = g_player.body.pos.x - b->body.pos.x;
                fx32 dy = g_player.body.pos.y - b->body.pos.y;
                if (dx > 0) b->body.pos.x += RI_FLY_SPEED * 2;
                else if (dx < 0) b->body.pos.x -= RI_FLY_SPEED * 2;
                if (dy > 0) b->body.pos.y += RI_FLY_SPEED;
                else if (dy < 0) b->body.pos.y -= RI_FLY_SPEED;

                /* If close, damage */
                fx32 adx = dx < 0 ? -dx : dx;
                fx32 ady = dy < 0 ? -dy : dy;
                if (adx < INT_TO_FX(12) && ady < INT_TO_FX(12)) {
                    player_damage(RI_GRAB_DAMAGE);
                }
            }

            b->ai_timer++;
            if (b->ai_timer >= RI_GRAB_FRAMES) {
                b->ai_state = RI_FLY;
                b->ai_timer = 0;
            }
            break;
        }

        case RI_POGO: {
            /* Bounce from above */
            b->body.pos.y += RI_POGO_SPEED;
            b->ai_timer++;

            /* Bounce back up at half */
            if (b->ai_timer == RI_POGO_FRAMES / 2) {
                camera_shake(5, 2);
            }
            if (b->ai_timer > RI_POGO_FRAMES / 2) {
                b->body.pos.y -= RI_POGO_SPEED;
            }

            if (b->ai_timer >= RI_POGO_FRAMES) {
                b->ai_state = RI_FLY;
                b->ai_timer = 0;
            }
            break;
        }

        case RI_DEATH: {
            /* Explosion sequence */
            if ((b->ai_timer % 10) == 0) {
                camera_shake(5, 3);
            }
            b->ai_timer++;
            if (b->ai_timer >= RI_DEATH_FRAMES) {
                b->active = false;
            }
            break;
        }
    }

    if (b->active && b->ai_state != RI_DEATH) {
        if (boss_aabb_overlap(b->body.pos, b->body.hitbox,
                              g_player.body.pos, g_player.body.hitbox)) {
            player_damage(b->damage_contact);
        }
    }
}

/* ========================================================================
 * Mother Brain AI
 *
 * 3-phase final boss:
 *   Phase 1: Brain in tank (3000 HP) - fires rinkas, turret projectiles
 *   Phase 2: Standing form (18000 HP) - beam attacks, bomb drops
 *   Phase 3: Final (36000 HP) - hyper beam sequence (scripted)
 *
 * phase field tracks current phase (0, 1, 2).
 * HP is reset on phase transition.
 * ======================================================================== */

enum {
    MB_TANK_IDLE = 0,     /* Phase 1: idle in tank */
    MB_TANK_ATTACK,       /* Phase 1: rinka/turret fire */
    MB_TANK_BREAK,        /* Phase 1→2 transition */
    MB_STAND_IDLE,        /* Phase 2: standing idle */
    MB_STAND_BEAM,        /* Phase 2: beam attack */
    MB_STAND_BOMB,        /* Phase 2: bomb drop */
    MB_HYPER_SETUP,       /* Phase 2→3 transition (baby metroid) */
    MB_HYPER_BEAM,        /* Phase 3: Samus has hyper beam */
    MB_DEATH              /* Final death */
};

#define MB_HP_PHASE1           3000
#define MB_HP_PHASE2           18000
#define MB_HP_PHASE3           36000
#define MB_CONTACT_DAMAGE      20
#define MB_HITBOX_HALF_W       INT_TO_FX(16)
#define MB_HITBOX_HALF_H       INT_TO_FX(16)
#define MB_RINKA_SPEED         INT_TO_FX(2)
#define MB_RINKA_INTERVAL      60
#define MB_BEAM_SPEED          INT_TO_FX(4)
#define MB_BEAM_FRAMES         30
#define MB_BOMB_VY             INT_TO_FX(2)
#define MB_BOMB_FRAMES         25
#define MB_BREAK_FRAMES        90
#define MB_HYPER_SETUP_FRAMES  120
#define MB_IDLE_FRAMES         60
#define MB_DEATH_FRAMES        180

static void mother_brain_init(void) {
    g_boss.hp = MB_HP_PHASE1;
    g_boss.hp_max = MB_HP_PHASE1;
    g_boss.damage_contact = MB_CONTACT_DAMAGE;
    g_boss.body.hitbox.half_w = MB_HITBOX_HALF_W;
    g_boss.body.hitbox.half_h = MB_HITBOX_HALF_H;
    g_boss.vulnerable = true;
    g_boss.phase = 0;
    g_boss.ai_state = MB_TANK_IDLE;
    g_boss.ai_timer = 0;
    g_boss.ai_counter = 0;
    g_boss.sub_timer = 0;
    g_boss.attack_count = 0;
}

static void mother_brain_update(void) {
    Boss* b = &g_boss;

    if (b->invuln_timer > 0) b->invuln_timer--;

    switch (b->ai_state) {
        /* === Phase 1: Brain in tank === */
        case MB_TANK_IDLE: {
            b->ai_timer++;
            if (b->ai_timer >= MB_IDLE_FRAMES) {
                b->ai_state = MB_TANK_ATTACK;
                b->ai_timer = 0;
            }
            break;
        }

        case MB_TANK_ATTACK: {
            b->sub_timer++;
            if (b->sub_timer >= MB_RINKA_INTERVAL) {
                b->sub_timer = 0;
                /* Spawn rinka aimed at player */
                fx32 dx = g_player.body.pos.x - b->body.pos.x;
                fx32 vx = (dx > 0) ? MB_RINKA_SPEED : -MB_RINKA_SPEED;
                projectile_spawn(PROJ_ENEMY_BULLET, PROJ_OWNER_ENEMY,
                                b->body.pos.x, b->body.pos.y, vx, 0);
                b->attack_count++;
            }

            b->ai_timer++;
            if (b->ai_timer >= MB_IDLE_FRAMES * 3) {
                b->ai_state = MB_TANK_IDLE;
                b->ai_timer = 0;
            }
            break;
        }

        case MB_TANK_BREAK: {
            /* Phase 1 → 2 transition */
            b->ai_timer++;
            if ((b->ai_timer % 15) == 0) camera_shake(10, 3);
            if (b->ai_timer >= MB_BREAK_FRAMES) {
                b->phase = 1;
                b->hp = MB_HP_PHASE2;
                b->hp_max = MB_HP_PHASE2;
                b->ai_state = MB_STAND_IDLE;
                b->ai_timer = 0;
                b->sub_timer = 0;
                b->attack_count = 0;
                b->vulnerable = true;
                camera_shake(30, 5);
            }
            break;
        }

        /* === Phase 2: Standing === */
        case MB_STAND_IDLE: {
            b->ai_timer++;
            if (b->ai_timer >= MB_IDLE_FRAMES) {
                /* Alternate beam and bomb */
                if ((b->ai_counter & 1) == 0) {
                    b->ai_state = MB_STAND_BEAM;
                } else {
                    b->ai_state = MB_STAND_BOMB;
                }
                b->ai_timer = 0;
                b->ai_counter++;
            }
            break;
        }

        case MB_STAND_BEAM: {
            if (b->ai_timer == 0) {
                fx32 dx = g_player.body.pos.x - b->body.pos.x;
                fx32 vx = (dx > 0) ? MB_BEAM_SPEED : -MB_BEAM_SPEED;
                /* Ring pattern: 3 beams */
                projectile_spawn(PROJ_ENEMY_BULLET, PROJ_OWNER_ENEMY,
                                b->body.pos.x, b->body.pos.y,
                                vx, -(MB_BEAM_SPEED >> 1));
                projectile_spawn(PROJ_ENEMY_BULLET, PROJ_OWNER_ENEMY,
                                b->body.pos.x, b->body.pos.y, vx, 0);
                projectile_spawn(PROJ_ENEMY_BULLET, PROJ_OWNER_ENEMY,
                                b->body.pos.x, b->body.pos.y,
                                vx, MB_BEAM_SPEED >> 1);
            }

            b->ai_timer++;
            if (b->ai_timer >= MB_BEAM_FRAMES) {
                b->ai_state = MB_STAND_IDLE;
                b->ai_timer = 0;
            }
            break;
        }

        case MB_STAND_BOMB: {
            if (b->ai_timer == 0) {
                /* Drop bomb downward */
                projectile_spawn(PROJ_ENEMY_BULLET, PROJ_OWNER_ENEMY,
                                b->body.pos.x - INT_TO_FX(16),
                                b->body.pos.y, 0, MB_BOMB_VY);
                projectile_spawn(PROJ_ENEMY_BULLET, PROJ_OWNER_ENEMY,
                                b->body.pos.x + INT_TO_FX(16),
                                b->body.pos.y, 0, MB_BOMB_VY);
            }

            b->ai_timer++;
            if (b->ai_timer >= MB_BOMB_FRAMES) {
                b->ai_state = MB_STAND_IDLE;
                b->ai_timer = 0;
            }
            break;
        }

        case MB_HYPER_SETUP: {
            /* Phase 2 → 3 transition (baby metroid sequence) */
            b->ai_timer++;
            if ((b->ai_timer % 20) == 0) camera_shake(5, 2);
            if (b->ai_timer >= MB_HYPER_SETUP_FRAMES) {
                b->phase = 2;
                b->hp = MB_HP_PHASE3;
                b->hp_max = MB_HP_PHASE3;
                b->ai_state = MB_HYPER_BEAM;
                b->ai_timer = 0;
                b->vulnerable = true;
            }
            break;
        }

        /* === Phase 3: Hyper Beam === */
        case MB_HYPER_BEAM: {
            /* Simplified: Mother Brain attacks, Samus has hyper beam */
            b->sub_timer++;
            if (b->sub_timer >= 30) {
                b->sub_timer = 0;
                fx32 dx = g_player.body.pos.x - b->body.pos.x;
                fx32 vx = (dx > 0) ? MB_BEAM_SPEED : -MB_BEAM_SPEED;
                projectile_spawn(PROJ_ENEMY_BULLET, PROJ_OWNER_ENEMY,
                                b->body.pos.x, b->body.pos.y, vx, 0);
            }
            break;
        }

        case MB_DEATH: {
            if ((b->ai_timer % 10) == 0) camera_shake(10, 4);
            b->ai_timer++;
            if (b->ai_timer >= MB_DEATH_FRAMES) {
                b->active = false;
            }
            break;
        }
    }

    /* Contact damage (phases 2+3 only, not during transitions) */
    if (b->active && b->phase > 0 &&
        b->ai_state != MB_TANK_BREAK && b->ai_state != MB_HYPER_SETUP &&
        b->ai_state != MB_DEATH) {
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
    [BOSS_NONE]           = NULL,
    [BOSS_SPORE_SPAWN]    = spore_spawn_init,
    [BOSS_CROCOMIRE]      = crocomire_init,
    [BOSS_BOMB_TORIZO]    = bomb_torizo_init,
    [BOSS_KRAID]          = kraid_init,
    [BOSS_BOTWOON]        = botwoon_init,
    [BOSS_PHANTOON]       = phantoon_init,
    [BOSS_DRAYGON]        = draygon_init,
    [BOSS_GOLDEN_TORIZO]  = golden_torizo_init,
    [BOSS_RIDLEY]         = ridley_init,
    [BOSS_MOTHER_BRAIN]   = mother_brain_init,
};

static const BossUpdateFn boss_update_fns[BOSS_TYPE_COUNT] = {
    [BOSS_NONE]           = NULL,
    [BOSS_SPORE_SPAWN]    = spore_spawn_update,
    [BOSS_CROCOMIRE]      = crocomire_update,
    [BOSS_BOMB_TORIZO]    = bomb_torizo_update,
    [BOSS_KRAID]          = kraid_update,
    [BOSS_BOTWOON]        = botwoon_update,
    [BOSS_PHANTOON]       = phantoon_update,
    [BOSS_DRAYGON]        = draygon_update,
    [BOSS_GOLDEN_TORIZO]  = golden_torizo_update,
    [BOSS_RIDLEY]         = ridley_update,
    [BOSS_MOTHER_BRAIN]   = mother_brain_update,
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

void boss_damage(int32_t damage) {
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

    /* Kraid: flinch on hit (mouth closes, vulnerability ends) */
    if (g_boss.type == BOSS_KRAID && g_boss.hp > 0) {
        g_boss.ai_state = KRAID_FLINCH;
        g_boss.ai_timer = 0;
        g_boss.vulnerable = false;
    }

    /* Phantoon: super missile (damage >= 200) triggers rage mode */
    if (g_boss.type == BOSS_PHANTOON && g_boss.hp > 0 &&
        damage >= 200 && g_boss.param_b == 0) {
        g_boss.param_b = FX_ONE;  /* Set rage flag */
    }

    /* Golden Torizo: catches super missiles (damage >= 200) */
    if (g_boss.type == BOSS_GOLDEN_TORIZO && g_boss.hp > 0 &&
        damage >= 200 && g_boss.ai_state != GT_CATCH &&
        g_boss.ai_state != GT_THROW_BACK) {
        g_boss.hp += damage; /* Undo the damage */
        g_boss.ai_state = GT_CATCH;
        g_boss.ai_timer = 0;
        g_boss.param_b = FX_ONE; /* Has caught projectile */
        return; /* Skip normal death check */
    }

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
            case BOSS_KRAID:
                g_boss.ai_state = KRAID_DEATH;
                break;
            case BOSS_BOTWOON:
                g_boss.ai_state = BOT_DEATH;
                break;
            case BOSS_PHANTOON:
                g_boss.ai_state = PH_DEATH;
                break;
            case BOSS_DRAYGON:
                g_boss.ai_state = DY_DEATH;
                break;
            case BOSS_GOLDEN_TORIZO:
                g_boss.ai_state = GT_DEATH;
                break;
            case BOSS_RIDLEY:
                g_boss.ai_state = RI_DEATH;
                break;
            case BOSS_MOTHER_BRAIN:
                /* Phase transitions instead of death for phases 1 and 2 */
                if (g_boss.phase == 0) {
                    g_boss.ai_state = MB_TANK_BREAK;
                    g_boss.ai_timer = 0;
                    g_boss.vulnerable = false;
                } else if (g_boss.phase == 1) {
                    g_boss.ai_state = MB_HYPER_SETUP;
                    g_boss.ai_timer = 0;
                    g_boss.vulnerable = false;
                } else {
                    g_boss.ai_state = MB_DEATH;
                }
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
