/**
 * state.c - Game state manager
 *
 * Function pointer dispatch for game states.
 * Deferred transitions: state_set() marks pending, applied at next state_update().
 */

#include "state.h"
#include <nds.h>
#include <stdio.h>

/* State handler table */
static StateHandlers state_table[STATE_COUNT];

/* Current and pending state */
static GameStateID current_state;
static GameStateID pending_state;
static bool transition_pending;

/* ========================================================================
 * Stub state handlers (placeholders until real states are implemented)
 * ======================================================================== */

static void stub_enter_title(void)       { iprintf(">> State: TITLE\n"); }
static void stub_enter_file_select(void) { iprintf(">> State: FILE SELECT\n"); }
static void stub_enter_gameplay(void)    { iprintf(">> State: GAMEPLAY\n"); }
static void stub_enter_pause(void)       { iprintf(">> State: PAUSE\n"); }
static void stub_enter_map(void)         { iprintf(">> State: MAP\n"); }
static void stub_enter_death(void)       { iprintf(">> State: DEATH\n"); }
static void stub_enter_ending(void)      { iprintf(">> State: ENDING\n"); }

static void stub_exit(void) { /* nothing */ }
static void stub_update(void) { /* nothing */ }
static void stub_render(void) { /* nothing */ }

/* ========================================================================
 * Public API
 * ======================================================================== */

void state_init(void) {
    /* Register stub handlers for all states */
    state_table[STATE_TITLE]       = (StateHandlers){ stub_enter_title, stub_exit, stub_update, stub_render };
    state_table[STATE_FILE_SELECT] = (StateHandlers){ stub_enter_file_select, stub_exit, stub_update, stub_render };
    state_table[STATE_GAMEPLAY]    = (StateHandlers){ stub_enter_gameplay, stub_exit, stub_update, stub_render };
    state_table[STATE_PAUSE]       = (StateHandlers){ stub_enter_pause, stub_exit, stub_update, stub_render };
    state_table[STATE_MAP]         = (StateHandlers){ stub_enter_map, stub_exit, stub_update, stub_render };
    state_table[STATE_DEATH]       = (StateHandlers){ stub_enter_death, stub_exit, stub_update, stub_render };
    state_table[STATE_ENDING]      = (StateHandlers){ stub_enter_ending, stub_exit, stub_update, stub_render };

    current_state = STATE_TITLE;
    transition_pending = false;

    /* Do NOT enter initial state here -- let state_set() + state_update() handle it.
     * This avoids entering with stub handlers before gameplay_register_states(). */
}

void state_set(GameStateID new_state) {
    if (new_state < STATE_COUNT) {
        pending_state = new_state;
        transition_pending = true;
    }
}

void state_update(void) {
    /* Apply deferred transition at frame boundary */
    if (transition_pending) {
        if (state_table[current_state].exit) {
            state_table[current_state].exit();
        }

        current_state = pending_state;
        transition_pending = false;

        if (state_table[current_state].enter) {
            state_table[current_state].enter();
        }
    }

    /* Dispatch update to current state */
    if (state_table[current_state].update) {
        state_table[current_state].update();
    }
}

void state_render(void) {
    if (state_table[current_state].render) {
        state_table[current_state].render();
    }
}

GameStateID state_current(void) {
    return current_state;
}

void state_set_handlers(GameStateID id, StateHandlers handlers) {
    if (id < STATE_COUNT) {
        state_table[id] = handlers;
    }
}
