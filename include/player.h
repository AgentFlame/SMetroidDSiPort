/**
 * player.h - Player state machine, equipment, animation
 *
 * Single static g_player instance. ~15 states with function pointer dispatch.
 * Samus rendered as 4-8 OAM sprites per pose.
 *
 * Implemented in: source/player.c (M9)
 */

#ifndef PLAYER_H
#define PLAYER_H

#include "sm_types.h"
#include "sm_physics_constants.h"
#include "physics.h"

/* Player states */
typedef enum {
    PSTATE_STANDING = 0,
    PSTATE_RUNNING,
    PSTATE_JUMPING,
    PSTATE_SPIN_JUMPING,
    PSTATE_FALLING,
    PSTATE_CROUCHING,
    PSTATE_MORPHBALL,
    PSTATE_SPRING_BALL,
    PSTATE_WALLJUMP,
    PSTATE_DAMAGE,
    PSTATE_DEATH,
    PSTATE_SHINESPARK_CHARGE,
    PSTATE_SHINESPARK,
    PSTATE_GRAPPLE,
    PSTATE_COUNT
} PlayerStateID;

/* Animation controller */
typedef struct {
    uint16_t anim_id;       /* Current animation */
    uint16_t frame_index;   /* Current frame within animation */
    uint16_t frame_timer;   /* Countdown to next frame */
} AnimController;

/* Player data (single global instance) */
typedef struct {
    PhysicsBody    body;
    PlayerStateID  state;
    Direction      facing;
    AnimController anim;

    /* Stats */
    int16_t  hp;
    int16_t  hp_max;
    uint16_t missiles;
    uint16_t missiles_max;
    uint16_t supers;
    uint16_t supers_max;
    uint16_t power_bombs;
    uint16_t power_bombs_max;
    uint16_t reserve_hp;
    uint16_t reserve_hp_max;

    /* Equipment bitfield (EQUIP_* flags from sm_types.h) */
    uint32_t equipment;

    /* Timers */
    uint16_t invuln_timer;      /* I-frames remaining */
    uint16_t shinespark_timer;  /* Charge remaining */
    uint16_t speed_boost_frames;/* Continuous running frame counter */

    /* Flags */
    bool     alive;
} Player;

extern Player g_player;

void player_init(void);
void player_update(void);
void player_render(void);
void player_damage(int16_t damage);
void player_damage_from(int16_t damage, fx32 source_x);

#endif /* PLAYER_H */
