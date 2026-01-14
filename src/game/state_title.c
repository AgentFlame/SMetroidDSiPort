/*
 * Super Metroid DSi Port - Title State
 *
 * Title screen with demo mode and menu options.
 */

#include "game_state.h"
#include "input.h"
#include "graphics.h"

static uint16_t title_timer = 0;

/*
 * Enter title state
 */
void state_title_enter(void) {
    title_timer = 0;

    // TODO: Load title screen graphics
    // TODO: Start title music
    // TODO: Initialize demo mode
}

/*
 * Update title state
 */
void state_title_update(void) {
    title_timer++;

    // Check for start button to begin game
    if (input_check(KEY_START, INPUT_PRESSED)) {
        game_state_set(STATE_FILE_SELECT);
        return;
    }

    // TODO: After timeout, start demo mode
    // TODO: Handle menu navigation if present
}

/*
 * Render title state
 */
void state_title_render(void) {
    // TODO: Render title screen
    // TODO: Render "Press Start" text (blinking)
    // TODO: Render demo if active
}

/*
 * Exit title state
 */
void state_title_exit(void) {
    // TODO: Stop title music
    // TODO: Cleanup title graphics
}
