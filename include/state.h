/**
 * state.h - Game state manager
 *
 * Function pointer table for enter/exit/update/render per GameStateID.
 * Deferred transitions (set pending, apply at frame boundary).
 *
 * Implemented in: source/state.c (M6)
 */

#ifndef STATE_H
#define STATE_H

#include "sm_types.h"

/* State handler function pointers */
typedef struct {
    void (*enter)(void);
    void (*exit)(void);
    void (*update)(void);
    void (*render)(void);
} StateHandlers;

void        state_init(void);
void        state_set(GameStateID new_state);    /* Deferred: applies next frame */
void        state_update(void);                  /* Dispatch to current update */
void        state_render(void);                  /* Dispatch to current render */
GameStateID state_current(void);

/* Replace handlers for a state (call after state_init) */
void        state_set_handlers(GameStateID id, StateHandlers handlers);

#endif /* STATE_H */
