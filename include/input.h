#ifndef INPUT_H
#define INPUT_H

#include <nds.h>
#include <stdint.h>
#include <stdbool.h>

// Input check types
typedef enum {
    INPUT_PRESSED,  // Button was just pressed this frame
    INPUT_HELD,     // Button is currently held down
    INPUT_RELEASED  // Button was just released this frame
} InputCheckType;

// Input buffer size (for advanced techniques)
#define INPUT_BUFFER_SIZE 8

// Input state structure
typedef struct {
    uint16_t pressed;   // Buttons pressed this frame
    uint16_t held;      // Buttons currently held
    uint16_t released;  // Buttons released this frame
    uint16_t previous;  // Previous frame's buttons

    // Input buffer for technique windows (3-5 frame buffering)
    uint8_t buffer[INPUT_BUFFER_SIZE];
    uint8_t buffer_index;

    // Touch screen (optional)
    touchPosition touch;
    bool touch_active;
} InputState;

// Button mappings (DSi → SNES functionality)
// Uses libnds KEY_* defines:
// KEY_A, KEY_B, KEY_X, KEY_Y
// KEY_L, KEY_R
// KEY_START, KEY_SELECT
// KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT

// Input system functions
void input_init(void);
void input_update(void);
void input_shutdown(void);

// Button state queries
bool input_check(uint16_t button, InputCheckType type);
bool input_check_any(uint16_t button_mask, InputCheckType type);
bool input_check_all(uint16_t button_mask, InputCheckType type);

// Input buffer queries (for technique windows)
bool input_buffered(uint16_t button, uint8_t frames_back);
void input_clear_buffer(void);

// Touch screen queries (optional)
bool input_touch_active(void);
void input_get_touch(int16_t* x, int16_t* y);

// Utility functions
uint16_t input_get_pressed(void);
uint16_t input_get_held(void);
uint16_t input_get_released(void);

// Global input state (extern, defined in input.c)
extern InputState g_input;

#endif // INPUT_H
