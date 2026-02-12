/**
 * player.c - Player state machine, equipment, animation
 *
 * Single static g_player instance. Input-driven state transitions with
 * physics body integration. States: standing, running, jumping,
 * spin jumping, falling, crouching, morphball (core); others stubbed.
 *
 * Update order per frame:
 *   1. State handler (reads input, sets velocity, triggers transitions)
 *   2. Physics (gravity, integration, collision resolution)
 *   3. Post-physics (landing detection, falling-off-edge detection)
 *   4. Timer decrements
 */

#include "player.h"
#include "camera.h"
#include "input.h"
#include "graphics.h"
#include <string.h>
#include <stdio.h>

/* ========================================================================
 * Global Player Instance
 * ======================================================================== */

Player g_player;

/* ========================================================================
 * State Handler Table
 * ======================================================================== */

typedef void (*StateUpdateFn)(void);
static StateUpdateFn state_fns[PSTATE_COUNT];

/* Forward declarations */
static void state_standing(void);
static void state_running(void);
static void state_jumping(void);
static void state_spin_jumping(void);
static void state_falling(void);
static void state_crouching(void);
static void state_morphball(void);
static void state_stub(void);
static void post_physics_check(void);

/* ========================================================================
 * State Transition Helper
 * ======================================================================== */

static void change_state(PlayerStateID new_state) {
    g_player.state = new_state;
    g_player.anim.frame_index = 0;
    g_player.anim.frame_timer = 0;

    /* Adjust hitbox for state */
    switch (new_state) {
        case PSTATE_CROUCHING:
            g_player.body.hitbox.half_h = SAMUS_CROUCH_H;
            break;
        case PSTATE_MORPHBALL:
        case PSTATE_SPRING_BALL:
            g_player.body.hitbox.half_h = SAMUS_MORPH_H;
            break;
        default:
            g_player.body.hitbox.half_h = SAMUS_HALF_H;
            break;
    }
}

/* ========================================================================
 * State Handlers
 * ======================================================================== */

static void state_standing(void) {
    g_player.body.vel.x = 0;

    if (input_held(KEY_LEFT) || input_held(KEY_RIGHT)) {
        change_state(PSTATE_RUNNING);
        return;
    }
    if (input_pressed(KEY_B)) {
        g_player.body.vel.y = -JUMP_VEL_NORMAL;
        change_state(PSTATE_JUMPING);
        return;
    }
    if (input_held(KEY_DOWN)) {
        change_state(PSTATE_CROUCHING);
        return;
    }
}

static void state_running(void) {
    fx32 speed = input_held(KEY_Y) ? RUN_SPEED : WALK_SPEED;

    if (input_held(KEY_LEFT)) {
        g_player.facing = DIR_LEFT;
        g_player.body.vel.x = -speed;
    } else if (input_held(KEY_RIGHT)) {
        g_player.facing = DIR_RIGHT;
        g_player.body.vel.x = speed;
    } else {
        change_state(PSTATE_STANDING);
        return;
    }

    if (input_pressed(KEY_B)) {
        g_player.body.vel.y = -JUMP_VEL_SPIN;
        change_state(PSTATE_SPIN_JUMPING);
        return;
    }
}

static void state_jumping(void) {
    /* Air control */
    fx32 speed = input_held(KEY_Y) ? RUN_SPEED : WALK_SPEED;
    if (input_held(KEY_LEFT)) {
        g_player.facing = DIR_LEFT;
        g_player.body.vel.x = -speed;
    } else if (input_held(KEY_RIGHT)) {
        g_player.facing = DIR_RIGHT;
        g_player.body.vel.x = speed;
    }

    /* Variable jump height: release B early = shorter jump */
    if (!input_held(KEY_B) && g_player.body.vel.y < -INT_TO_FX(1)) {
        g_player.body.vel.y = -INT_TO_FX(1);
    }
}

static void state_spin_jumping(void) {
    /* Same as jumping (spin animation differs, wall jump possible later) */
    fx32 speed = input_held(KEY_Y) ? RUN_SPEED : WALK_SPEED;
    if (input_held(KEY_LEFT)) {
        g_player.facing = DIR_LEFT;
        g_player.body.vel.x = -speed;
    } else if (input_held(KEY_RIGHT)) {
        g_player.facing = DIR_RIGHT;
        g_player.body.vel.x = speed;
    }

    if (!input_held(KEY_B) && g_player.body.vel.y < -INT_TO_FX(1)) {
        g_player.body.vel.y = -INT_TO_FX(1);
    }
}

static void state_falling(void) {
    fx32 speed = input_held(KEY_Y) ? RUN_SPEED : WALK_SPEED;
    if (input_held(KEY_LEFT)) {
        g_player.facing = DIR_LEFT;
        g_player.body.vel.x = -speed;
    } else if (input_held(KEY_RIGHT)) {
        g_player.facing = DIR_RIGHT;
        g_player.body.vel.x = speed;
    }
}

static void state_crouching(void) {
    g_player.body.vel.x = 0;

    if (!input_held(KEY_DOWN)) {
        change_state(PSTATE_STANDING);
        return;
    }
    if (input_pressed(KEY_B)) {
        g_player.body.vel.y = -JUMP_VEL_NORMAL;
        change_state(PSTATE_JUMPING);
        return;
    }
    if (input_pressed(KEY_A) && (g_player.equipment & EQUIP_MORPH_BALL)) {
        change_state(PSTATE_MORPHBALL);
        return;
    }
}

static void state_morphball(void) {
    fx32 speed = MORPH_BALL_SPEED;
    if (input_held(KEY_LEFT)) {
        g_player.facing = DIR_LEFT;
        g_player.body.vel.x = -speed;
    } else if (input_held(KEY_RIGHT)) {
        g_player.facing = DIR_RIGHT;
        g_player.body.vel.x = speed;
    } else {
        g_player.body.vel.x = 0;
    }

    if (input_pressed(KEY_A)) {
        change_state(PSTATE_CROUCHING);
        return;
    }
}

static void state_stub(void) {
    /* Placeholder for unimplemented states */
}

/* ========================================================================
 * Post-Physics State Corrections
 * ======================================================================== */

static void post_physics_check(void) {
    /* Landing detection */
    if (g_player.body.contact.on_ground) {
        switch (g_player.state) {
            case PSTATE_JUMPING:
            case PSTATE_SPIN_JUMPING:
            case PSTATE_FALLING:
                if (input_held(KEY_LEFT) || input_held(KEY_RIGHT)) {
                    change_state(PSTATE_RUNNING);
                } else {
                    change_state(PSTATE_STANDING);
                }
                break;
            default:
                break;
        }
    }
    /* Falling off edge */
    else {
        switch (g_player.state) {
            case PSTATE_STANDING:
            case PSTATE_RUNNING:
            case PSTATE_CROUCHING:
                change_state(PSTATE_FALLING);
                break;
            default:
                break;
        }
    }

    /* Jumping -> falling at apex */
    if (g_player.state == PSTATE_JUMPING ||
        g_player.state == PSTATE_SPIN_JUMPING) {
        if (g_player.body.vel.y >= 0 && !g_player.body.contact.on_ground) {
            change_state(PSTATE_FALLING);
        }
    }
}

/* ========================================================================
 * Test Sprite Data
 * ======================================================================== */

static u8 player_sprite_data[128];  /* 16x16 @ 4bpp = 128 bytes */

static const u16 player_palette[16] = {
    RGB15(0, 0, 0),       /* 0: transparent */
    RGB15(31, 31, 0),     /* 1: yellow (visor) */
    RGB15(0, 24, 0),      /* 2: green (suit) */
    RGB15(31, 16, 0),     /* 3: orange (arm cannon) */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/* ========================================================================
 * Public API
 * ======================================================================== */

void player_init(void) {
    memset(&g_player, 0, sizeof(g_player));

    /* Spawn position (center of test room, above floor) */
    g_player.body.pos.x = INT_TO_FX(128);
    g_player.body.pos.y = INT_TO_FX(120);
    g_player.body.hitbox.half_w = SAMUS_HALF_W;     /* 8px */
    g_player.body.hitbox.half_h = SAMUS_HALF_H;     /* 20px */
    g_player.body.env = ENV_AIR;

    g_player.state = PSTATE_FALLING;  /* Will land on first physics update */
    g_player.facing = DIR_RIGHT;
    g_player.hp = PLAYER_START_HP;
    g_player.hp_max = PLAYER_START_HP;
    g_player.alive = true;

    /* Register state update handlers */
    state_fns[PSTATE_STANDING]        = state_standing;
    state_fns[PSTATE_RUNNING]         = state_running;
    state_fns[PSTATE_JUMPING]         = state_jumping;
    state_fns[PSTATE_SPIN_JUMPING]    = state_spin_jumping;
    state_fns[PSTATE_FALLING]         = state_falling;
    state_fns[PSTATE_CROUCHING]       = state_crouching;
    state_fns[PSTATE_MORPHBALL]       = state_morphball;
    state_fns[PSTATE_SPRING_BALL]     = state_stub;
    state_fns[PSTATE_WALLJUMP]        = state_stub;
    state_fns[PSTATE_DAMAGE]          = state_stub;
    state_fns[PSTATE_DEATH]           = state_stub;
    state_fns[PSTATE_SHINESPARK_CHARGE] = state_stub;
    state_fns[PSTATE_SHINESPARK]      = state_stub;
    state_fns[PSTATE_GRAPPLE]         = state_stub;

    /* Load placeholder sprite (green 16x16 square) */
    memset(player_sprite_data, 0x22, sizeof(player_sprite_data));
    graphics_load_sprite_tiles(player_sprite_data, sizeof(player_sprite_data), 0);
    graphics_load_sprite_palette(0, player_palette);
}

void player_update(void) {
    if (!g_player.alive) return;

    /* 1. State handler (input -> velocity -> state transitions) */
    if (state_fns[g_player.state]) {
        state_fns[g_player.state]();
    }

    /* 2. Physics (gravity, integration, collision) */
    physics_update_body(&g_player.body);

    /* 3. Post-physics state corrections (landing, edge falling, apex) */
    post_physics_check();

    /* 4. Decrement timers */
    if (g_player.invuln_timer > 0) g_player.invuln_timer--;
    if (g_player.shinespark_timer > 0) g_player.shinespark_timer--;
}

void player_render(void) {
    if (!g_player.alive) return;

    /* World to screen: subtract camera offset */
    int sx = FX_TO_INT(g_player.body.pos.x) - FX_TO_INT(g_camera.x) - 8;
    int sy = FX_TO_INT(g_player.body.pos.y) - FX_TO_INT(g_camera.y) - 8;

    graphics_set_sprite(OAM_PLAYER_START, sx, sy, 0, 0, 1,
                       g_player.facing == DIR_LEFT, false);
}

void player_damage(int16_t damage) {
    if (!g_player.alive) return;
    if (g_player.invuln_timer > 0) return;

    g_player.hp -= damage;
    g_player.invuln_timer = INVULN_FRAMES;

    if (g_player.hp <= 0) {
        g_player.hp = 0;
        g_player.alive = false;
        change_state(PSTATE_DEATH);
    }
}
