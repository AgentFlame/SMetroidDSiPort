/**
 * boss.h - Boss AI framework
 *
 * Single active boss instance with type-specific AI dispatch.
 * Bosses are separate from the enemy pool -- they have unique
 * state machines, vulnerability windows, and multi-phase logic.
 *
 * OAM: borrows particle slots (88-111) since particles aren't
 * implemented yet. Each boss can use up to 16 OAM sprites.
 *
 * Implemented in: source/boss.c (M13)
 */

#ifndef BOSS_H
#define BOSS_H

#include "sm_types.h"
#include "physics.h"

/* Boss type IDs */
typedef enum {
    BOSS_NONE = 0,
    BOSS_SPORE_SPAWN,
    BOSS_CROCOMIRE,
    BOSS_BOMB_TORIZO,
    BOSS_KRAID,
    BOSS_BOTWOON,
    BOSS_PHANTOON,
    BOSS_DRAYGON,
    BOSS_GOLDEN_TORIZO,
    BOSS_RIDLEY,
    BOSS_MOTHER_BRAIN,
    BOSS_TYPE_COUNT
} BossTypeID;

/* Boss OAM allocation (borrow from unimplemented particles) */
#define BOSS_OAM_START   OAM_PARTICLE_START  /* 88 */
#define BOSS_OAM_COUNT   16

/* Boss instance (only one active at a time) */
typedef struct {
    BossTypeID  type;
    PhysicsBody body;
    int16_t     hp;
    int16_t     hp_max;
    uint16_t    phase;          /* Multi-phase bosses (0-indexed) */
    uint16_t    ai_state;       /* Current AI state (boss-specific enum) */
    uint16_t    ai_timer;       /* General-purpose frame timer */
    uint16_t    ai_counter;     /* General-purpose counter */
    uint16_t    damage_contact; /* Contact damage to player */
    bool        active;
    bool        vulnerable;     /* Can take damage right now? */
    uint16_t    invuln_timer;   /* Brief i-frames after hit */

    /* Anchor point (e.g., ceiling attachment for Spore Spawn) */
    fx32        anchor_x;
    fx32        anchor_y;

    /* Generic parameters (boss-specific usage documented per boss) */
    fx32        param_a;
    fx32        param_b;
    uint16_t    sub_timer;      /* Secondary timer */
    uint16_t    attack_count;   /* Track attacks per cycle */
} Boss;

extern Boss g_boss;

/* Initialize boss system (zeros state) */
void boss_init(void);

/* Spawn a boss at world position (x, y) */
void boss_spawn(BossTypeID type, fx32 x, fx32 y);

/* Per-frame update (AI + contact damage) */
void boss_update(void);

/* Per-frame render */
void boss_render(void);

/* Apply damage to boss (respects vulnerability and invuln timer) */
void boss_damage(int16_t damage);

/* Query */
bool boss_is_active(void);

#endif /* BOSS_H */
