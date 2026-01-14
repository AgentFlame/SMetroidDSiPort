#ifndef PLAYER_H
#define PLAYER_H

#include "physics.h"
#include "input.h"
#include <stdint.h>
#include <stdbool.h>

// Player state enumeration
typedef enum {
    PLAYER_STATE_STANDING,
    PLAYER_STATE_WALKING,
    PLAYER_STATE_RUNNING,
    PLAYER_STATE_JUMPING,
    PLAYER_STATE_SPIN_JUMP,
    PLAYER_STATE_FALLING,
    PLAYER_STATE_MORPHBALL,
    PLAYER_STATE_MORPHBALL_JUMP,
    PLAYER_STATE_AIMING_UP,
    PLAYER_STATE_AIMING_DOWN,
    PLAYER_STATE_CROUCHING,
    PLAYER_STATE_DAMAGE,
    PLAYER_STATE_DEATH,
    PLAYER_STATE_DOOR_TRANSITION,
    PLAYER_STATE_WALL_JUMP,
    PLAYER_STATE_SHINESPARK_CHARGE,
    PLAYER_STATE_SHINESPARK_ACTIVE,
    PLAYER_STATE_GRAPPLE,
    PLAYER_STATE_XRAY
} PlayerState;

// Equipment flags (items)
typedef enum {
    ITEM_MORPH_BALL = 0x00000001,
    ITEM_BOMBS = 0x00000002,
    ITEM_CHARGE_BEAM = 0x00000004,
    ITEM_VARIA_SUIT = 0x00000008,
    ITEM_HI_JUMP_BOOTS = 0x00000010,
    ITEM_SPEED_BOOSTER = 0x00000020,
    ITEM_GRAPPLE_BEAM = 0x00000040,
    ITEM_XRAY_SCOPE = 0x00000080,
    ITEM_GRAVITY_SUIT = 0x00000100,
    ITEM_SPACE_JUMP = 0x00000200,
    ITEM_SPRING_BALL = 0x00000400,
    ITEM_SCREW_ATTACK = 0x00000800,
    ITEM_POWER_BOMBS = 0x00001000
} ItemFlags;

// Beam flags (combinable)
typedef enum {
    BEAM_WAVE = 0x0001,
    BEAM_ICE = 0x0002,
    BEAM_SPAZER = 0x0004,
    BEAM_PLASMA = 0x0008
} BeamFlags;

// Missile types
typedef enum {
    MISSILE_NONE,
    MISSILE_NORMAL,
    MISSILE_SUPER
} MissileType;

// Player structure
typedef struct {
    PhysicsBody physics;
    PlayerState state;
    PlayerState prev_state;
    uint16_t state_timer;

    // Stats
    uint16_t health;
    uint16_t max_health;
    uint16_t missiles;
    uint16_t max_missiles;
    uint16_t super_missiles;
    uint16_t max_super_missiles;
    uint16_t power_bombs;
    uint16_t max_power_bombs;
    uint16_t reserve_energy;
    uint16_t max_reserve_energy;

    // Equipment
    uint32_t equipped_items;
    uint16_t equipped_beams;
    MissileType selected_missile;

    // Animation
    uint16_t animation_frame;
    uint16_t animation_timer;
    bool facing_right;

    // Aiming
    int16_t aim_angle;
    bool is_shooting;
    uint16_t shot_cooldown;

    // Advanced movement
    uint16_t shinespark_charge_timer;
    uint16_t shinespark_direction;
    uint16_t wall_jump_timer;

    // Invincibility frames
    uint16_t invincibility_timer;
    bool is_invincible;

    // Damage flash
    uint16_t damage_flash_timer;
} Player;

// Player system functions
void player_init(void);
void player_update(Player* p, InputState* input);
void player_render(Player* p);
void player_shutdown(void);

// State management
void player_set_state(Player* p, PlayerState new_state);
PlayerState player_get_state(Player* p);

// Health and damage
void player_take_damage(Player* p, uint16_t damage);
void player_heal(Player* p, uint16_t amount);
bool player_is_alive(Player* p);

// Equipment queries
bool player_has_item(Player* p, ItemFlags item);
bool player_has_beam(Player* p, BeamFlags beam);
void player_grant_item(Player* p, ItemFlags item);
void player_grant_beam(Player* p, BeamFlags beam);

// Ammo management
bool player_has_missiles(Player* p, MissileType type);
void player_consume_missile(Player* p, MissileType type);
void player_add_missiles(Player* p, MissileType type, uint16_t count);

// Movement queries
bool player_is_grounded(Player* p);
bool player_is_in_morphball(Player* p);
bool player_can_jump(Player* p);
bool player_can_wall_jump(Player* p);

// Position and velocity
void player_get_position(Player* p, int32_t* x, int32_t* y);
void player_set_position(Player* p, int32_t x, int32_t y);
void player_get_velocity(Player* p, int32_t* vx, int32_t* vy);

// Weapon firing
void player_fire_beam(Player* p);
void player_fire_missile(Player* p, MissileType type);
void player_place_bomb(Player* p);
void player_place_power_bomb(Player* p);

// Animation control
void player_update_animation(Player* p);
uint16_t player_get_sprite_index(Player* p);

// Global player instance
extern Player g_player;

#endif // PLAYER_H
