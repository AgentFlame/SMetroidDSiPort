/**
 * enemy.h - Enemy pool management and AI
 *
 * Fixed-size pool (MAX_ENEMIES) with swap-remove.
 * Each enemy type has its own AI update function.
 *
 * Implemented in: source/enemy.c, source/enemy_ai_*.c (M11)
 */

#ifndef ENEMY_H
#define ENEMY_H

#include "sm_types.h"
#include "physics.h"

/* Enemy type IDs */
typedef enum {
    ENEMY_NONE = 0,
    ENEMY_ZOOMER,
    ENEMY_GEEMER,
    ENEMY_WAVER,
    ENEMY_RINKA,
    ENEMY_SIDEHOPPER,
    ENEMY_KI_HUNTER,
    ENEMY_ZEBESIAN,
    ENEMY_TYPE_COUNT
} EnemyTypeID;

/* Per-enemy instance */
typedef struct {
    EnemyTypeID type;
    PhysicsBody body;
    Direction   facing;
    int16_t     hp;
    int16_t     hp_max;
    uint16_t    damage_contact;  /* Damage dealt on contact */
    uint16_t    ai_timer;
    uint16_t    ai_state;
    uint16_t    anim_frame;
    uint16_t    anim_timer;
    bool        active;
} Enemy;

/* Pool management */
void enemy_pool_init(void);
int  enemy_spawn(EnemyTypeID type, fx32 x, fx32 y);  /* Returns index or -1 */
void enemy_remove(int index);
void enemy_clear_all(void);

/* Per-frame update/render for all active enemies */
void enemy_update_all(void);
void enemy_render_all(void);

/* Query */
int    enemy_get_count(void);
Enemy* enemy_get(int index);

/* Damage an enemy. Removes it if HP <= 0. */
void   enemy_damage(int index, int16_t damage);

#endif /* ENEMY_H */
