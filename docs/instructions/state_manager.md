# M6: Game State Manager - Instruction File

## Purpose

Manage game states (title, gameplay, pause, etc.) with function pointer dispatch. Deferred transitions prevent state changes mid-frame.

## Dependencies

- `include/sm_types.h` (GameStateID, STATE_COUNT)
- `include/state.h` (StateHandlers struct, function declarations)

## Shared Types

```c
typedef enum {
    STATE_TITLE = 0,
    STATE_FILE_SELECT,
    STATE_GAMEPLAY,
    STATE_PAUSE,
    STATE_MAP,
    STATE_DEATH,
    STATE_ENDING,
    STATE_COUNT
} GameStateID;

typedef struct {
    void (*enter)(void);
    void (*exit)(void);
    void (*update)(void);
    void (*render)(void);
} StateHandlers;
```

## Data Structures

```c
/* Handler table: one entry per GameStateID */
static StateHandlers state_table[STATE_COUNT];

/* Current and pending state */
static GameStateID current_state;
static GameStateID pending_state;
static bool transition_pending;
```

## API

```c
void        state_init(void);                    /* Register all state handlers */
void        state_set(GameStateID new_state);    /* Deferred transition */
void        state_update(void);                  /* Apply pending + dispatch update */
void        state_render(void);                  /* Dispatch render */
GameStateID state_current(void);                 /* Query current */
```

## Implementation

### state_init()
Register handler functions for each state. Initially, all states can use stub handlers that display state name via iprintf.

```c
/* Example stub handlers */
static void title_enter(void)  { iprintf(">> TITLE\n"); }
static void title_exit(void)   { }
static void title_update(void) { if (input_pressed(KEY_START)) state_set(STATE_GAMEPLAY); }
static void title_render(void) { }

/* Register */
state_table[STATE_TITLE] = (StateHandlers){ title_enter, title_exit, title_update, title_render };
/* ... repeat for all states ... */

current_state = STATE_TITLE;
transition_pending = false;
state_table[current_state].enter();
```

### state_set()
```c
void state_set(GameStateID new_state) {
    pending_state = new_state;
    transition_pending = true;
}
```

### state_update()
```c
void state_update(void) {
    /* Apply deferred transition */
    if (transition_pending) {
        state_table[current_state].exit();
        current_state = pending_state;
        state_table[current_state].enter();
        transition_pending = false;
    }
    state_table[current_state].update();
}
```

### state_render()
```c
void state_render(void) {
    state_table[current_state].render();
}
```

## Constraints

- No malloc. All handler table is static.
- Transitions are deferred -- `state_set()` just marks pending, actual transition happens at start of next `state_update()`.
- States must not call `state_set()` from within `enter()` (would cause recursive transition).

## Test Criteria

- Boot to STATE_TITLE, press START -> transitions to STATE_GAMEPLAY.
- STATE_GAMEPLAY, press START -> STATE_PAUSE.
- STATE_PAUSE, press START -> back to STATE_GAMEPLAY.
- No crashes on any transition. Enter/exit called exactly once per transition.
