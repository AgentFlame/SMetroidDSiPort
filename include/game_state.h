#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <stdint.h>
#include <stdbool.h>

// Game state enumeration
typedef enum {
    STATE_BOOT,          // Initial startup/loading
    STATE_TITLE,         // Title screen with demo
    STATE_FILE_SELECT,   // Save file selection menu
    STATE_GAMEPLAY,      // Active gameplay
    STATE_PAUSE,         // Paused (equipment/inventory menu)
    STATE_MAP,           // Full-screen map view
    STATE_DEATH,         // Death animation and respawn
    STATE_GAMEOVER,      // Game over screen
    STATE_ENDING,        // Credits and ending sequence
    STATE_COUNT          // Total number of states
} GameState;

// State function pointers
typedef void (*StateUpdateFunc)(void);
typedef void (*StateRenderFunc)(void);
typedef void (*StateEnterFunc)(void);
typedef void (*StateExitFunc)(void);

// State definition structure
typedef struct {
    GameState state_id;
    StateEnterFunc on_enter;
    StateUpdateFunc on_update;
    StateRenderFunc on_render;
    StateExitFunc on_exit;
} StateDefinition;

// Game state manager functions
void game_state_init(void);
void game_state_update(void);
void game_state_render(void);
void game_state_shutdown(void);

// State transitions
void game_state_set(GameState new_state);
void game_state_push(GameState new_state); // Push current, switch to new
void game_state_pop(void);                  // Return to previous state
GameState game_state_get_current(void);
GameState game_state_get_previous(void);

// State queries
bool game_state_is_gameplay(void);
bool game_state_is_paused(void);

// Individual state update/render functions (implemented per-state)
// Title state
void state_title_enter(void);
void state_title_update(void);
void state_title_render(void);
void state_title_exit(void);

// File select state
void state_file_select_enter(void);
void state_file_select_update(void);
void state_file_select_render(void);
void state_file_select_exit(void);

// Gameplay state
void state_gameplay_enter(void);
void state_gameplay_update(void);
void state_gameplay_render(void);
void state_gameplay_exit(void);

// Pause state
void state_pause_enter(void);
void state_pause_update(void);
void state_pause_render(void);
void state_pause_exit(void);

// Map state
void state_map_enter(void);
void state_map_update(void);
void state_map_render(void);
void state_map_exit(void);

// Death state
void state_death_enter(void);
void state_death_update(void);
void state_death_render(void);
void state_death_exit(void);

// Ending state
void state_ending_enter(void);
void state_ending_update(void);
void state_ending_render(void);
void state_ending_exit(void);

#endif // GAME_STATE_H
