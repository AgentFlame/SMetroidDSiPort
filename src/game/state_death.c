/*
 * Super Metroid DSi Port - Death State
 *
 * Death animation and respawn handling.
 */

#include "game_state.h"
#include "input.h"
#include "player.h"
#include "save.h"
#include "graphics.h"
#include "audio.h"

static uint16_t death_timer = 0;
static const uint16_t DEATH_ANIMATION_DURATION = 180;  // 3 seconds @ 60fps

/*
 * Enter death state
 */
void state_death_enter(void) {
    death_timer = 0;

    // TODO: Stop gameplay music
    // TODO: Play death sound effect
    // TODO: Trigger death animation
}

/*
 * Update death state
 */
void state_death_update(void) {
    death_timer++;

    // TODO: Update death animation

    // After animation completes
    if (death_timer >= DEATH_ANIMATION_DURATION) {
        // Check if player has energy or reserve tanks
        if (g_player.reserve_energy > 0) {
            // Use reserve tanks to revive
            uint16_t restore = g_player.reserve_energy;
            if (restore > g_player.max_health) {
                restore = g_player.max_health;
            }
            g_player.health = restore;
            g_player.reserve_energy -= restore;

            // Return to gameplay
            game_state_set(STATE_GAMEPLAY);
        } else {
            // Game over - load from last save
            if (save_quick_load()) {
                game_state_set(STATE_GAMEPLAY);
            } else {
                // No save exists - back to title
                game_state_set(STATE_TITLE);
            }
        }
    }
}

/*
 * Render death state
 */
void state_death_render(void) {
    // TODO: Render gameplay background (frozen)

    // TODO: Render death animation (Samus exploding, fading out)

    // TODO: Apply screen flash effect
}

/*
 * Exit death state
 */
void state_death_exit(void) {
    death_timer = 0;

    // TODO: Cleanup death animation
}
