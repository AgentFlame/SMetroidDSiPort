/*
 * Super Metroid DSi Port - File Select State
 *
 * Save file selection menu with metadata display.
 */

#include "game_state.h"
#include "input.h"
#include "save.h"
#include "graphics.h"

static uint8_t selected_slot = 0;
static SaveMetadata slot_metadata[SAVE_SLOT_COUNT];

/*
 * Enter file select state
 */
void state_file_select_enter(void) {
    selected_slot = 0;

    // Load metadata for all save slots
    for (uint8_t i = 0; i < SAVE_SLOT_COUNT; i++) {
        save_get_metadata(i, &slot_metadata[i]);
    }

    // TODO: Load file select graphics
    // TODO: Start file select music
}

/*
 * Update file select state
 */
void state_file_select_update(void) {
    // Navigation
    if (input_check(KEY_UP, INPUT_PRESSED)) {
        if (selected_slot > 0) selected_slot--;
    }
    if (input_check(KEY_DOWN, INPUT_PRESSED)) {
        if (selected_slot < SAVE_SLOT_COUNT - 1) selected_slot++;
    }

    // Selection
    if (input_check(KEY_A, INPUT_PRESSED)) {
        save_set_current_slot(selected_slot);

        if (slot_metadata[selected_slot].exists) {
            // Load existing save
            if (save_quick_load()) {
                game_state_set(STATE_GAMEPLAY);
            }
        } else {
            // New game
            SaveData new_save;
            save_data_init(&new_save);
            save_write(selected_slot, &new_save);
            save_quick_load();
            game_state_set(STATE_GAMEPLAY);
        }
    }

    // Back to title
    if (input_check(KEY_B, INPUT_PRESSED)) {
        game_state_set(STATE_TITLE);
    }

    // TODO: Handle copy/delete operations (L/R + A/B)
}

/*
 * Render file select state
 */
void state_file_select_render(void) {
    // TODO: Render file select background
    // TODO: Render save slot information
    // TODO: For each slot, display:
    //       - Energy/Max Energy
    //       - Current area
    //       - Playtime
    //       - Item percentage
    //       - "[EMPTY]" if no save exists
    // TODO: Render cursor/selection indicator
}

/*
 * Exit file select state
 */
void state_file_select_exit(void) {
    // TODO: Cleanup file select graphics
}
