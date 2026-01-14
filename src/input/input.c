/*
 * Super Metroid DSi Port - Input System
 *
 * Handles button scanning, input buffering, and touch screen support.
 * Maps DSi buttons to SNES functionality with advanced technique support.
 */

#include "input.h"
#include <string.h>

// Global input state
InputState g_input;

/*
 * Initialize the input system
 * Sets up button scanning and clears all state
 */
void input_init(void) {
    memset(&g_input, 0, sizeof(InputState));
    g_input.buffer_index = 0;
    g_input.touch_active = false;
}

/*
 * Update input state (call once per frame)
 * Scans hardware buttons and updates pressed/held/released states
 */
void input_update(void) {
    // Store previous frame's held buttons
    g_input.previous = g_input.held;

    // Scan current button state
    scanKeys();
    uint16_t current = keysHeld();

    // Calculate state changes
    g_input.pressed = keysDown();       // Buttons pressed this frame
    g_input.held = current;             // Buttons currently held
    g_input.released = keysUp();        // Buttons released this frame

    // Update input buffer (circular buffer for technique windows)
    g_input.buffer[g_input.buffer_index] = (uint8_t)(current & 0xFF);
    g_input.buffer_index = (g_input.buffer_index + 1) % INPUT_BUFFER_SIZE;

    // Update touch screen state
    touchRead(&g_input.touch);
    g_input.touch_active = (keysHeld() & KEY_TOUCH) != 0;
}

/*
 * Shutdown the input system
 */
void input_shutdown(void) {
    memset(&g_input, 0, sizeof(InputState));
}

/*
 * Check if a button matches the given state
 *
 * @param button Button to check (KEY_A, KEY_B, etc.)
 * @param type Check type (PRESSED, HELD, or RELEASED)
 * @return true if the button matches the state
 */
bool input_check(uint16_t button, InputCheckType type) {
    switch (type) {
        case INPUT_PRESSED:
            return (g_input.pressed & button) != 0;
        case INPUT_HELD:
            return (g_input.held & button) != 0;
        case INPUT_RELEASED:
            return (g_input.released & button) != 0;
        default:
            return false;
    }
}

/*
 * Check if any button in the mask matches the given state
 *
 * @param button_mask Bitmask of buttons to check
 * @param type Check type (PRESSED, HELD, or RELEASED)
 * @return true if any button in the mask matches the state
 */
bool input_check_any(uint16_t button_mask, InputCheckType type) {
    switch (type) {
        case INPUT_PRESSED:
            return (g_input.pressed & button_mask) != 0;
        case INPUT_HELD:
            return (g_input.held & button_mask) != 0;
        case INPUT_RELEASED:
            return (g_input.released & button_mask) != 0;
        default:
            return false;
    }
}

/*
 * Check if all buttons in the mask match the given state
 *
 * @param button_mask Bitmask of buttons to check
 * @param type Check type (PRESSED, HELD, or RELEASED)
 * @return true if all buttons in the mask match the state
 */
bool input_check_all(uint16_t button_mask, InputCheckType type) {
    switch (type) {
        case INPUT_PRESSED:
            return (g_input.pressed & button_mask) == button_mask;
        case INPUT_HELD:
            return (g_input.held & button_mask) == button_mask;
        case INPUT_RELEASED:
            return (g_input.released & button_mask) == button_mask;
        default:
            return false;
    }
}

/*
 * Check if a button was pressed within the last N frames (buffered input)
 * Used for technique windows (e.g., wall jump, quick morph)
 *
 * @param button Button to check
 * @param frames_back Number of frames to look back (1-7)
 * @return true if button was pressed in the window
 */
bool input_buffered(uint16_t button, uint8_t frames_back) {
    if (frames_back == 0 || frames_back >= INPUT_BUFFER_SIZE) {
        return false;
    }

    // Convert button mask to low byte (buffer only stores low 8 buttons)
    uint8_t button_byte = (uint8_t)(button & 0xFF);

    // Check backwards through buffer
    for (uint8_t i = 1; i <= frames_back; i++) {
        int index = (g_input.buffer_index - i + INPUT_BUFFER_SIZE) % INPUT_BUFFER_SIZE;
        if (g_input.buffer[index] & button_byte) {
            return true;
        }
    }

    return false;
}

/*
 * Clear the input buffer
 * Used when transitioning states to prevent carryover inputs
 */
void input_clear_buffer(void) {
    memset(g_input.buffer, 0, INPUT_BUFFER_SIZE);
    g_input.buffer_index = 0;
}

/*
 * Check if touch screen is currently being touched
 *
 * @return true if touch screen is active
 */
bool input_touch_active(void) {
    return g_input.touch_active;
}

/*
 * Get current touch screen coordinates
 *
 * @param x Output X coordinate (0-255)
 * @param y Output Y coordinate (0-191)
 */
void input_get_touch(int16_t* x, int16_t* y) {
    if (x) *x = g_input.touch.px;
    if (y) *y = g_input.touch.py;
}

/*
 * Get pressed button mask
 *
 * @return Bitmask of buttons pressed this frame
 */
uint16_t input_get_pressed(void) {
    return g_input.pressed;
}

/*
 * Get held button mask
 *
 * @return Bitmask of buttons currently held
 */
uint16_t input_get_held(void) {
    return g_input.held;
}

/*
 * Get released button mask
 *
 * @return Bitmask of buttons released this frame
 */
uint16_t input_get_released(void) {
    return g_input.released;
}
