/*
 * Super Metroid DSi Port - Game State Manager
 *
 * Manages game state transitions and dispatches update/render calls.
 * Supports state stack for push/pop operations (e.g., pause over gameplay).
 */

#include "game_state.h"
#include <string.h>

// State stack for push/pop operations
#define STATE_STACK_SIZE 4
static GameState state_stack[STATE_STACK_SIZE];
static uint8_t stack_top = 0;

// Current and previous states
static GameState current_state = STATE_BOOT;
static GameState previous_state = STATE_BOOT;

// State definitions table
static StateDefinition state_definitions[STATE_COUNT];

// Forward declarations for state functions
// These are implemented in separate files (state_*.c)
extern void state_title_enter(void);
extern void state_title_update(void);
extern void state_title_render(void);
extern void state_title_exit(void);

extern void state_file_select_enter(void);
extern void state_file_select_update(void);
extern void state_file_select_render(void);
extern void state_file_select_exit(void);

extern void state_gameplay_enter(void);
extern void state_gameplay_update(void);
extern void state_gameplay_render(void);
extern void state_gameplay_exit(void);

extern void state_pause_enter(void);
extern void state_pause_update(void);
extern void state_pause_render(void);
extern void state_pause_exit(void);

extern void state_map_enter(void);
extern void state_map_update(void);
extern void state_map_render(void);
extern void state_map_exit(void);

extern void state_death_enter(void);
extern void state_death_update(void);
extern void state_death_render(void);
extern void state_death_exit(void);

extern void state_ending_enter(void);
extern void state_ending_update(void);
extern void state_ending_render(void);
extern void state_ending_exit(void);

// Stub functions for boot and gameover states (minimal implementation)
static void state_boot_enter(void) {}
static void state_boot_update(void) { game_state_set(STATE_TITLE); }
static void state_boot_render(void) {}
static void state_boot_exit(void) {}

static void state_gameover_enter(void) {}
static void state_gameover_update(void) {}
static void state_gameover_render(void) {}
static void state_gameover_exit(void) {}

/*
 * Initialize the game state manager
 * Sets up state definitions and initializes to BOOT state
 */
void game_state_init(void) {
    // Clear state stack
    memset(state_stack, 0, sizeof(state_stack));
    stack_top = 0;

    // Initialize state definitions table
    // Boot state
    state_definitions[STATE_BOOT].state_id = STATE_BOOT;
    state_definitions[STATE_BOOT].on_enter = state_boot_enter;
    state_definitions[STATE_BOOT].on_update = state_boot_update;
    state_definitions[STATE_BOOT].on_render = state_boot_render;
    state_definitions[STATE_BOOT].on_exit = state_boot_exit;

    // Title state
    state_definitions[STATE_TITLE].state_id = STATE_TITLE;
    state_definitions[STATE_TITLE].on_enter = state_title_enter;
    state_definitions[STATE_TITLE].on_update = state_title_update;
    state_definitions[STATE_TITLE].on_render = state_title_render;
    state_definitions[STATE_TITLE].on_exit = state_title_exit;

    // File select state
    state_definitions[STATE_FILE_SELECT].state_id = STATE_FILE_SELECT;
    state_definitions[STATE_FILE_SELECT].on_enter = state_file_select_enter;
    state_definitions[STATE_FILE_SELECT].on_update = state_file_select_update;
    state_definitions[STATE_FILE_SELECT].on_render = state_file_select_render;
    state_definitions[STATE_FILE_SELECT].on_exit = state_file_select_exit;

    // Gameplay state
    state_definitions[STATE_GAMEPLAY].state_id = STATE_GAMEPLAY;
    state_definitions[STATE_GAMEPLAY].on_enter = state_gameplay_enter;
    state_definitions[STATE_GAMEPLAY].on_update = state_gameplay_update;
    state_definitions[STATE_GAMEPLAY].on_render = state_gameplay_render;
    state_definitions[STATE_GAMEPLAY].on_exit = state_gameplay_exit;

    // Pause state
    state_definitions[STATE_PAUSE].state_id = STATE_PAUSE;
    state_definitions[STATE_PAUSE].on_enter = state_pause_enter;
    state_definitions[STATE_PAUSE].on_update = state_pause_update;
    state_definitions[STATE_PAUSE].on_render = state_pause_render;
    state_definitions[STATE_PAUSE].on_exit = state_pause_exit;

    // Map state
    state_definitions[STATE_MAP].state_id = STATE_MAP;
    state_definitions[STATE_MAP].on_enter = state_map_enter;
    state_definitions[STATE_MAP].on_update = state_map_update;
    state_definitions[STATE_MAP].on_render = state_map_render;
    state_definitions[STATE_MAP].on_exit = state_map_exit;

    // Death state
    state_definitions[STATE_DEATH].state_id = STATE_DEATH;
    state_definitions[STATE_DEATH].on_enter = state_death_enter;
    state_definitions[STATE_DEATH].on_update = state_death_update;
    state_definitions[STATE_DEATH].on_render = state_death_render;
    state_definitions[STATE_DEATH].on_exit = state_death_exit;

    // Gameover state
    state_definitions[STATE_GAMEOVER].state_id = STATE_GAMEOVER;
    state_definitions[STATE_GAMEOVER].on_enter = state_gameover_enter;
    state_definitions[STATE_GAMEOVER].on_update = state_gameover_update;
    state_definitions[STATE_GAMEOVER].on_render = state_gameover_render;
    state_definitions[STATE_GAMEOVER].on_exit = state_gameover_exit;

    // Ending state
    state_definitions[STATE_ENDING].state_id = STATE_ENDING;
    state_definitions[STATE_ENDING].on_enter = state_ending_enter;
    state_definitions[STATE_ENDING].on_update = state_ending_update;
    state_definitions[STATE_ENDING].on_render = state_ending_render;
    state_definitions[STATE_ENDING].on_exit = state_ending_exit;

    // Set initial state
    current_state = STATE_BOOT;
    previous_state = STATE_BOOT;
}

/*
 * Update the current game state
 * Calls the current state's update function
 */
void game_state_update(void) {
    if (current_state >= STATE_COUNT) return;

    StateDefinition* state = &state_definitions[current_state];
    if (state->on_update) {
        state->on_update();
    }
}

/*
 * Render the current game state
 * Calls the current state's render function
 */
void game_state_render(void) {
    if (current_state >= STATE_COUNT) return;

    StateDefinition* state = &state_definitions[current_state];
    if (state->on_render) {
        state->on_render();
    }
}

/*
 * Shutdown the game state manager
 */
void game_state_shutdown(void) {
    // Exit current state
    if (current_state < STATE_COUNT) {
        StateDefinition* state = &state_definitions[current_state];
        if (state->on_exit) {
            state->on_exit();
        }
    }

    current_state = STATE_BOOT;
    previous_state = STATE_BOOT;
    stack_top = 0;
}

/*
 * Set the current game state (direct transition)
 * Exits current state and enters new state
 *
 * @param new_state State to transition to
 */
void game_state_set(GameState new_state) {
    if (new_state >= STATE_COUNT) return;
    if (new_state == current_state) return;

    // Exit current state
    if (current_state < STATE_COUNT) {
        StateDefinition* state = &state_definitions[current_state];
        if (state->on_exit) {
            state->on_exit();
        }
    }

    // Update state tracking
    previous_state = current_state;
    current_state = new_state;

    // Enter new state
    if (current_state < STATE_COUNT) {
        StateDefinition* state = &state_definitions[current_state];
        if (state->on_enter) {
            state->on_enter();
        }
    }
}

/*
 * Push current state and switch to new state
 * Used for temporary states like pause menu
 *
 * @param new_state State to push to
 */
void game_state_push(GameState new_state) {
    if (new_state >= STATE_COUNT) return;
    if (stack_top >= STATE_STACK_SIZE) return;  // Stack overflow

    // Push current state onto stack
    state_stack[stack_top] = current_state;
    stack_top++;

    // Transition to new state
    game_state_set(new_state);
}

/*
 * Pop state stack and return to previous state
 * Used when exiting temporary states
 */
void game_state_pop(void) {
    if (stack_top == 0) return;  // Stack underflow

    // Pop previous state
    stack_top--;
    GameState popped_state = state_stack[stack_top];

    // Transition back
    game_state_set(popped_state);
}

/*
 * Get the current game state
 *
 * @return Current state
 */
GameState game_state_get_current(void) {
    return current_state;
}

/*
 * Get the previous game state
 *
 * @return Previous state
 */
GameState game_state_get_previous(void) {
    return previous_state;
}

/*
 * Check if current state is a gameplay state
 *
 * @return true if in gameplay
 */
bool game_state_is_gameplay(void) {
    return current_state == STATE_GAMEPLAY;
}

/*
 * Check if game is paused
 *
 * @return true if paused
 */
bool game_state_is_paused(void) {
    return current_state == STATE_PAUSE || current_state == STATE_MAP;
}
