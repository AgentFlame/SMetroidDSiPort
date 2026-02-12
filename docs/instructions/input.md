# M5: Input System - Instruction File

## Purpose

Wrap libnds `scanKeys`/`keysDown`/`keysHeld`/`keysUp` with an input buffer for advanced technique timing (wall jumps, shinespark). Circular buffer of last 8 frames.

## Dependencies

- `include/sm_types.h` (bool, stdint)
- `include/sm_config.h` (INPUT_BUFFER_FRAMES=8, INPUT_BUFFER_WINDOW=5)
- `include/input.h` (function declarations)
- libnds: `scanKeys()`, `keysDown()`, `keysHeld()`, `keysUp()`, `KEY_*` constants

## Shared Types

```c
#define INPUT_BUFFER_FRAMES  8
#define INPUT_BUFFER_WINDOW  5
```

libnds key constants: `KEY_A`, `KEY_B`, `KEY_X`, `KEY_Y`, `KEY_L`, `KEY_R`, `KEY_UP`, `KEY_DOWN`, `KEY_LEFT`, `KEY_RIGHT`, `KEY_START`, `KEY_SELECT`, `KEY_TOUCH`.

## Data Structures

```c
/* Circular buffer storing pressed keys for each of the last 8 frames */
static uint32_t press_buffer[INPUT_BUFFER_FRAMES];
static int buffer_index;

/* Per-key hold duration (12 keys tracked) */
static uint16_t hold_duration[16];  /* indexed by bit position in keysHeld() */

/* Cached current-frame state */
static uint32_t cur_pressed;
static uint32_t cur_held;
static uint32_t cur_released;
```

## API (from input.h)

```c
void input_update(void);          /* Call once per frame after scanKeys() */
bool input_pressed(int key);      /* Rising edge this frame */
bool input_held(int key);         /* Currently held */
bool input_released(int key);     /* Falling edge this frame */
bool input_buffered(int key);     /* Pressed within last INPUT_BUFFER_WINDOW frames */
int  input_held_frames(int key);  /* Consecutive frames held */
```

## Implementation

### input_update()
```
1. cur_pressed  = keysDown();
2. cur_held     = keysHeld();
3. cur_released = keysUp();
4. press_buffer[buffer_index] = cur_pressed;
5. buffer_index = (buffer_index + 1) % INPUT_BUFFER_FRAMES;
6. For each key bit (0-15):
   if (cur_held & (1 << bit)) hold_duration[bit]++;
   else hold_duration[bit] = 0;
```

### input_pressed/held/released()
```c
bool input_pressed(int key)  { return (cur_pressed & key) != 0; }
bool input_held(int key)     { return (cur_held & key) != 0; }
bool input_released(int key) { return (cur_released & key) != 0; }
```

### input_buffered()
Check last INPUT_BUFFER_WINDOW entries in circular buffer:
```c
for (int i = 0; i < INPUT_BUFFER_WINDOW; i++) {
    int idx = (buffer_index - 1 - i + INPUT_BUFFER_FRAMES) % INPUT_BUFFER_FRAMES;
    if (press_buffer[idx] & key) return true;
}
return false;
```

### input_held_frames()
Find the bit position of `key` and return `hold_duration[bit]`.

## Constraints

- No malloc. All buffers are static.
- No float.
- `scanKeys()` must be called by the caller (main loop) before `input_update()`.
- This module does NOT call `scanKeys()` itself.

## Test Criteria

- Press A: `input_pressed(KEY_A)` true for exactly 1 frame, `input_held(KEY_A)` true while held.
- Release A: `input_released(KEY_A)` true for exactly 1 frame.
- After pressing A, `input_buffered(KEY_A)` stays true for INPUT_BUFFER_WINDOW frames.
- `input_held_frames(KEY_A)` increments each frame while A is held.
