/*
 * Super Metroid DSi Port - Map State
 *
 * Full-screen map view with area navigation.
 */

#include "game_state.h"
#include "input.h"
#include "save.h"
#include "graphics.h"

static uint8_t current_map_area = 0;
static int16_t map_cursor_x = 0;
static int16_t map_cursor_y = 0;

/*
 * Enter map state
 */
void state_map_enter(void) {
    // TODO: Get current area from room system
    current_map_area = 0;  // Default to Crateria

    // TODO: Center cursor on player's current room
    map_cursor_x = 0;
    map_cursor_y = 0;

    // TODO: Load map graphics
}

/*
 * Update map state
 */
void state_map_update(void) {
    // Exit map
    if (input_check(KEY_START, INPUT_PRESSED) ||
        input_check(KEY_SELECT, INPUT_PRESSED)) {
        game_state_pop();  // Return to previous state
        return;
    }

    // Cursor movement
    if (input_check(KEY_UP, INPUT_HELD)) {
        map_cursor_y--;
    }
    if (input_check(KEY_DOWN, INPUT_HELD)) {
        map_cursor_y++;
    }
    if (input_check(KEY_LEFT, INPUT_HELD)) {
        map_cursor_x--;
    }
    if (input_check(KEY_RIGHT, INPUT_HELD)) {
        map_cursor_x++;
    }

    // Area switching
    if (input_check(KEY_L, INPUT_PRESSED)) {
        if (current_map_area > 0) current_map_area--;
    }
    if (input_check(KEY_R, INPUT_PRESSED)) {
        if (current_map_area < 5) current_map_area++;  // 6 areas total
    }

    // TODO: Clamp cursor to map bounds
}

/*
 * Render map state
 */
void state_map_render(void) {
    // TODO: Render map background

    // TODO: Render revealed map tiles for current area
    // TODO: Use save data to check which tiles are revealed

    // TODO: Render room types (explored, item room, boss room, save room)

    // TODO: Render player position indicator

    // TODO: Render cursor

    // TODO: Render area name

    // TODO: Render item counts and completion percentage
}

/*
 * Exit map state
 */
void state_map_exit(void) {
    // TODO: Cleanup map graphics
}
