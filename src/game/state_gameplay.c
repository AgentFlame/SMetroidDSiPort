/*
 * Super Metroid DSi Port - Gameplay State
 *
 * Main gameplay loop - player control, enemies, world updates.
 */

#include "game_state.h"
#include "input.h"
#include "player.h"
#include "room.h"
#include "enemies.h"
#include "physics.h"
#include "graphics.h"
#include "audio.h"
#include "save.h"

static uint32_t gameplay_frames = 0;

/*
 * Enter gameplay state
 */
void state_gameplay_enter(void) {
    gameplay_frames = 0;

    // TODO: Load current room
    // TODO: Initialize player at saved position
    // TODO: Start area music
    // TODO: Initialize HUD
}

/*
 * Update gameplay state
 */
void state_gameplay_update(void) {
    gameplay_frames++;

    // Check for pause
    if (input_check(KEY_START, INPUT_PRESSED)) {
        game_state_push(STATE_PAUSE);
        return;
    }

    // TODO: Update player
    // player_update(&g_player, &g_input);

    // TODO: Update enemies
    // enemies_update_all();

    // TODO: Update room (doors, items, hazards)
    // room_update();

    // TODO: Update camera
    // camera_update();

    // TODO: Check for door transitions

    // TODO: Check for death
    // if (!player_is_alive(&g_player)) {
    //     game_state_set(STATE_DEATH);
    // }

    // Update playtime in save data (every frame)
    // TODO: Call save_update_playtime when needed
}

/*
 * Render gameplay state
 */
void state_gameplay_render(void) {
    // TODO: Clear framebuffer
    // graphics_clear_framebuffer(0x0000);

    // TODO: Render backgrounds (with camera scroll)
    // bg_render_all();

    // TODO: Render room (tilemap, effects)
    // room_render();

    // TODO: Render enemies
    // enemies_render_all();

    // TODO: Render player
    // player_render(&g_player);

    // TODO: Render projectiles

    // TODO: Render HUD
    // hud_render(&g_player);

    // TODO: Apply screen effects (fade, flash, shake)
}

/*
 * Exit gameplay state
 */
void state_gameplay_exit(void) {
    // TODO: Nothing to cleanup (room stays loaded for pause/map)
}
