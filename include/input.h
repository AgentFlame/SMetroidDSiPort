/**
 * input.h - Input system
 *
 * Wraps libnds scanKeys/keysDown/keysHeld/keysUp with buffering.
 * Circular buffer of last INPUT_BUFFER_FRAMES frames for advanced techniques.
 *
 * Implemented in: source/input.c (M5)
 */

#ifndef INPUT_H
#define INPUT_H

#include "sm_types.h"
#include "sm_config.h"

/* Call once per frame after scanKeys() */
void input_update(void);

/* Standard input queries (current frame) */
bool input_pressed(int key);    /* Rising edge (this frame only) */
bool input_held(int key);       /* Currently held */
bool input_released(int key);   /* Falling edge (this frame only) */

/* Buffered press: true if key was pressed within INPUT_BUFFER_WINDOW frames */
bool input_buffered(int key);

/* How many consecutive frames has key been held? */
int input_held_frames(int key);

#endif /* INPUT_H */
