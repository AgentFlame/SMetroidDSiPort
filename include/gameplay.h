/**
 * gameplay.h - Gameplay state logic
 *
 * Extracted from main.c (M17f). Contains gameplay_enter/exit/update/render,
 * door transitions, item pickup, weapon firing, boss management.
 *
 * Call gameplay_register_states() after state_init() to wire everything up.
 */

#ifndef GAMEPLAY_H
#define GAMEPLAY_H

#include "sm_types.h"

/* Register gameplay, pause, and death state handlers with state manager */
void gameplay_register_states(void);

/* Global progress (readable by HUD and other modules) */
extern uint32_t g_game_time_frames;

#endif /* GAMEPLAY_H */
