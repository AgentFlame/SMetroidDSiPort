/*
 * Super Metroid DSi Port - Pause State
 *
 * Equipment/inventory menu with item selection.
 */

#include "game_state.h"
#include "input.h"
#include "player.h"
#include "graphics.h"

typedef enum {
    PAUSE_TAB_EQUIPMENT,
    PAUSE_TAB_MAP,
    PAUSE_TAB_COUNT
} PauseTab;

static PauseTab current_tab = PAUSE_TAB_EQUIPMENT;

/*
 * Enter pause state
 */
void state_pause_enter(void) {
    current_tab = PAUSE_TAB_EQUIPMENT;

    // TODO: Pause audio
    // TODO: Play pause sound effect
    // TODO: Load pause menu graphics
}

/*
 * Update pause state
 */
void state_pause_update(void) {
    // Unpause
    if (input_check(KEY_START, INPUT_PRESSED)) {
        game_state_pop();  // Return to gameplay
        return;
    }

    // Tab switching
    if (input_check(KEY_L, INPUT_PRESSED)) {
        current_tab = (current_tab + PAUSE_TAB_COUNT - 1) % PAUSE_TAB_COUNT;
    }
    if (input_check(KEY_R, INPUT_PRESSED)) {
        current_tab = (current_tab + 1) % PAUSE_TAB_COUNT;
    }

    // Switch to full map view
    if (input_check(KEY_SELECT, INPUT_PRESSED)) {
        game_state_set(STATE_MAP);
    }

    // TODO: Handle equipment selection
    // TODO: Handle missile type switching
}

/*
 * Render pause state
 */
void state_pause_render(void) {
    // TODO: Render paused gameplay in background (darkened)

    // TODO: Render pause menu overlay

    switch (current_tab) {
        case PAUSE_TAB_EQUIPMENT:
            // TODO: Render equipment screen
            // TODO: Show acquired items
            // TODO: Show energy tanks, missile counts, etc.
            // TODO: Highlight currently selected missile type
            break;

        case PAUSE_TAB_MAP:
            // TODO: Render minimap
            // TODO: Show current area map
            // TODO: Highlight current room
            break;

        default:
            break;
    }
}

/*
 * Exit pause state
 */
void state_pause_exit(void) {
    // TODO: Resume audio
    // TODO: Play unpause sound
}
