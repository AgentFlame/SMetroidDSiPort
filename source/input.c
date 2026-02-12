/**
 * input.c - Input system with buffering
 *
 * Wraps libnds scanKeys/keysDown/keysHeld/keysUp.
 * Circular buffer of last INPUT_BUFFER_FRAMES frames for advanced techniques.
 * Per-key hold duration tracking.
 *
 * NOTE: The caller (main loop) must call scanKeys() BEFORE input_update().
 * This module does NOT call scanKeys() itself.
 */

#include "input.h"

/* Circular buffer storing pressed keys for each of the last 8 frames */
static uint32_t press_buffer[INPUT_BUFFER_FRAMES];
static int buffer_index;

/* Per-key hold duration counter (indexed by bit position 0-15) */
static uint16_t hold_duration[16];

/* Cached current-frame state (set by input_update) */
static uint32_t cur_pressed;
static uint32_t cur_held;
static uint32_t cur_released;

void input_update(void) {
    /* Read current frame state from libnds (scanKeys already called) */
    cur_pressed  = keysDown();
    cur_held     = keysHeld();
    cur_released = keysUp();

    /* Store pressed keys in circular buffer */
    press_buffer[buffer_index] = cur_pressed;
    buffer_index = (buffer_index + 1) % INPUT_BUFFER_FRAMES;

    /* Update per-key hold durations */
    for (int bit = 0; bit < 16; bit++) {
        if (cur_held & (1 << bit)) {
            if (hold_duration[bit] < 0xFFFF) {
                hold_duration[bit]++;
            }
        } else {
            hold_duration[bit] = 0;
        }
    }
}

bool input_pressed(int key) {
    return (cur_pressed & key) != 0;
}

bool input_held(int key) {
    return (cur_held & key) != 0;
}

bool input_released(int key) {
    return (cur_released & key) != 0;
}

bool input_buffered(int key) {
    for (int i = 0; i < INPUT_BUFFER_WINDOW; i++) {
        int idx = (buffer_index - 1 - i + INPUT_BUFFER_FRAMES) % INPUT_BUFFER_FRAMES;
        if (press_buffer[idx] & key) {
            return true;
        }
    }
    return false;
}

int input_held_frames(int key) {
    /* Find the lowest set bit in key to get the bit position */
    for (int bit = 0; bit < 16; bit++) {
        if (key & (1 << bit)) {
            return hold_duration[bit];
        }
    }
    return 0;
}
