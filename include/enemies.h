#ifndef ENEMIES_H
#define ENEMIES_H

#include "physics.h"
#include <stdint.h>
#include <stdbool.h>

// Maximum enemies per room
#define MAX_ENEMIES 32

// Enemy type IDs (subset, full list from ROM analysis)
typedef enum {
    ENEMY_NONE = 0x0000,

    // Standard enemies
    ENEMY_ZOOMER = 0x0001,      // Crawls on surfaces
    ENEMY_GEEMER = 0x0002,      // Spiked crawler
    ENEMY_SIDEHOPPER = 0x0003,  // Jumping enemy
    ENEMY_WAVER = 0x0004,       // Sine wave flyer
    ENEMY_SPACE_PIRATE = 0x0005,
    ENEMY_METROID = 0x0006,

    // Mini-bosses
    ENEMY_BOMB_TORIZO = 0x0100,
    ENEMY_SPORE_SPAWN = 0x0101,
    ENEMY_CROCOMIRE = 0x0102,
    ENEMY_BOTWOON = 0x0103,
    ENEMY_GOLDEN_TORIZO = 0x0104,

    // Main bosses
    ENEMY_KRAID = 0x0200,
    ENEMY_PHANTOON = 0x0201,
    ENEMY_DRAYGON = 0x0202,
    ENEMY_RIDLEY = 0x0203,
    ENEMY_MOTHER_BRAIN_1 = 0x0204,  // Phase 1 (in tank)
    ENEMY_MOTHER_BRAIN_2 = 0x0205,  // Phase 2 (standing)
    ENEMY_MOTHER_BRAIN_3 = 0x0206   // Phase 3 (final)
} EnemyType;

// Enemy AI state (generic, boss-specific states defined per-boss)
typedef enum {
    ENEMY_AI_IDLE,
    ENEMY_AI_PATROL,
    ENEMY_AI_CHASE,
    ENEMY_AI_ATTACK,
    ENEMY_AI_RETREAT,
    ENEMY_AI_DAMAGE,
    ENEMY_AI_DEATH,
    ENEMY_AI_CUSTOM_1,  // Boss-specific
    ENEMY_AI_CUSTOM_2,
    ENEMY_AI_CUSTOM_3
} EnemyAIState;

// Enemy flags
typedef enum {
    ENEMY_FLAG_ACTIVE = 0x01,
    ENEMY_FLAG_VISIBLE = 0x02,
    ENEMY_FLAG_INVINCIBLE = 0x04,
    ENEMY_FLAG_GROUNDED = 0x08,
    ENEMY_FLAG_FLYING = 0x10,
    ENEMY_FLAG_BOSS = 0x20,
    ENEMY_FLAG_DAMAGED = 0x40,
    ENEMY_FLAG_DYING = 0x80
} EnemyFlags;

// Forward declaration for function pointers
struct Enemy;

// Enemy function pointers
typedef void (*EnemyUpdateFunc)(struct Enemy*);
typedef void (*EnemyRenderFunc)(struct Enemy*);
typedef void (*EnemyOnHitFunc)(struct Enemy*, int damage);
typedef void (*EnemyOnDeathFunc)(struct Enemy*);

// Enemy structure (40 bytes aligned with SNES RAM structure)
typedef struct Enemy {
    EnemyType enemy_id;
    int32_t x, y;           // 16.16 fixed-point position
    int32_t vx, vy;         // 16.16 fixed-point velocity
    int16_t hp;
    int16_t max_hp;
    uint16_t state;         // EnemyAIState
    uint16_t timer;
    uint16_t frame;
    uint8_t palette;
    uint8_t flags;          // EnemyFlags bitmask
    uint8_t layer;
    uint8_t damage_flash;

    // AI function pointers
    EnemyUpdateFunc update;
    EnemyRenderFunc render;
    EnemyOnHitFunc on_hit;
    EnemyOnDeathFunc on_death;

    // Enemy-specific data (16 bytes)
    uint8_t data[16];
} Enemy;

// Enemy spawn data (from room loader)
typedef struct {
    EnemyType enemy_id;
    int16_t x, y;
    uint16_t params[4];  // Enemy-specific parameters
} EnemySpawn;

// Boss HP constants (from ROM analysis)
#define BOSS_HP_BOMB_TORIZO 800
#define BOSS_HP_SPORE_SPAWN 960
#define BOSS_HP_KRAID 1000
#define BOSS_HP_PHANTOON 2500
#define BOSS_HP_BOTWOON 3000
#define BOSS_HP_DRAYGON 6000
#define BOSS_HP_GOLDEN_TORIZO 8000
#define BOSS_HP_RIDLEY 18000
#define BOSS_HP_MOTHER_BRAIN_1 3000
#define BOSS_HP_MOTHER_BRAIN_2 18000
#define BOSS_HP_MOTHER_BRAIN_3 36000

// Enemy system functions
void enemies_init(void);
void enemies_update_all(void);
void enemies_render_all(void);
void enemies_clear_all(void);
void enemies_shutdown(void);

// Enemy management
Enemy* enemies_spawn(EnemyType id, int x, int y);
void enemies_kill(Enemy* e);
void enemies_damage(Enemy* e, int damage);
Enemy* enemies_get(uint8_t index);
uint8_t enemies_get_count(void);

// Collision queries
Enemy* enemies_check_collision(int x, int y, int w, int h);
Enemy* enemies_check_collision_circle(int cx, int cy, int radius);
bool enemies_is_boss_active(void);

// AI utility functions
void enemy_set_ai_state(Enemy* e, EnemyAIState state);
bool enemy_can_see_player(Enemy* e, int max_distance);
void enemy_move_toward_player(Enemy* e, int32_t speed);
void enemy_shoot_at_player(Enemy* e);

// Drop item generation
void enemy_drop_item(Enemy* e);

// Boss-specific AI (implemented in separate files)
void boss_kraid_init(Enemy* e);
void boss_kraid_update(Enemy* e);
void boss_phantoon_init(Enemy* e);
void boss_phantoon_update(Enemy* e);
void boss_draygon_init(Enemy* e);
void boss_draygon_update(Enemy* e);
void boss_ridley_init(Enemy* e);
void boss_ridley_update(Enemy* e);
void boss_mother_brain_init(Enemy* e);
void boss_mother_brain_update(Enemy* e);

// Standard enemy AI (common behaviors)
void enemy_ai_patrol(Enemy* e);
void enemy_ai_chase_player(Enemy* e);
void enemy_ai_flee_player(Enemy* e);
void enemy_ai_circle_player(Enemy* e);

// Global enemy array
extern Enemy g_enemies[MAX_ENEMIES];
extern uint8_t g_enemy_count;

#endif // ENEMIES_H
