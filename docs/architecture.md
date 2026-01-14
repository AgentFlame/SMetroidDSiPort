# Super Metroid DSi Port - System Architecture

## Overview

This document defines the system architecture for the Super Metroid DSi port, including module boundaries, memory allocation, data flow, and initialization strategy.

**Target Platform**: Nintendo DSi (ARM9 @ 133 MHz, 16MB RAM, dual screens)
**Display Strategy**: 256×224 internal framebuffer → top 192 rows on top screen, bottom 32 rows on bottom screen
**Frame Rate**: 60 FPS (16.67ms per frame)
**Development**: devkitARM + libnds

---

## System Architecture Diagram

```
┌──────────────────────────────────────────────────────────────┐
│                        MAIN LOOP                             │
│  (60 FPS VSync, state machine dispatch)                      │
└────────────┬────────────────────────────────┬────────────────┘
             │                                │
    ┌────────▼────────┐              ┌───────▼────────┐
    │  INPUT SYSTEM   │              │  GAME STATE    │
    │  - Button scan  │              │  - State mgmt  │
    │  - Buffering    │              │  - Pause/menu  │
    │  - Touch (opt)  │              │  - Transitions │
    └────────┬────────┘              └───────┬────────┘
             │                                │
    ┌────────▼────────────────────────────────▼────────┐
    │              GAMEPLAY LAYER                       │
    │  ┌──────────────┐  ┌──────────────┐             │
    │  │   PLAYER     │  │   ENEMIES    │             │
    │  │  - Physics   │  │  - AI logic  │             │
    │  │  - Weapons   │  │  - Collision │             │
    │  │  - Animation │  │  - Behavior  │             │
    │  └──────┬───────┘  └──────┬───────┘             │
    │         │                  │                      │
    │  ┌──────▼──────────────────▼───────┐             │
    │  │       PHYSICS ENGINE            │             │
    │  │  - Gravity (air/water/lava)     │             │
    │  │  - Collision detection          │             │
    │  │  - Movement integration         │             │
    │  │  - Momentum preservation        │             │
    │  └──────┬──────────────────────────┘             │
    └─────────┼──────────────────────────────────────┘
              │
    ┌─────────▼─────────────────────────────────────┐
    │              WORLD SYSTEM                      │
    │  - Room loader (current room only)             │
    │  - Door transitions                            │
    │  - Collision map                               │
    │  - Item collection                             │
    │  - Environment hazards                         │
    └─────────┬──────────────────────────────────────┘
              │
    ┌─────────▼─────────────────────────────────────┐
    │           GRAPHICS ENGINE                      │
    │  ┌──────────────┐  ┌──────────────┐           │
    │  │  BACKGROUND  │  │   SPRITES    │           │
    │  │  - 4 layers  │  │  - 128 max   │           │
    │  │  - Palettes  │  │  - Priority  │           │
    │  │  - Scrolling │  │  - Assembly  │           │
    │  └──────┬───────┘  └──────┬───────┘           │
    │         │                  │                    │
    │  ┌──────▼──────────────────▼───────┐           │
    │  │   FRAMEBUFFER (256×224)         │           │
    │  │  - Double buffered              │           │
    │  │  - Split to dual screens        │           │
    │  └──────┬──────────────────────────┘           │
    │         │                                       │
    │  ┌──────▼────────┐  ┌──────────────┐           │
    │  │   HUD/UI      │  │   EFFECTS    │           │
    │  │  - Energy bar │  │  - Fade      │           │
    │  │  - Ammo count │  │  - Flash     │           │
    │  │  - Map (opt)  │  │  - Palette   │           │
    │  └───────────────┘  └──────────────┘           │
    └─────────┬──────────────────────────────────────┘
              │
    ┌─────────▼─────────────────────────────────────┐
    │            AUDIO ENGINE                        │
    │  - Music playback (streaming/sequenced)        │
    │  - SFX with priority                           │
    │  - Area music switching                        │
    │  - maxmod integration                          │
    └────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────┐
│                     SUPPORT SYSTEMS                          │
│  - Save/Load (SD or NAND)                                    │
│  - Memory manager (pool allocators)                          │
│  - Asset loader (streaming from ROM)                         │
└──────────────────────────────────────────────────────────────┘
```

---

## Module Breakdown

### 1. Main Loop & State Management

**Files**: `src/main.c`, `src/game/state_manager.c`
**Headers**: `include/game_state.h`

**Responsibilities**:
- Initialize all systems in correct order
- Run main 60 FPS game loop with VSync
- Dispatch to current game state (title, gameplay, pause, map, etc.)
- Handle state transitions

**States**:
- `STATE_BOOT` - Initial startup
- `STATE_TITLE` - Title screen with demo
- `STATE_FILE_SELECT` - Save file selection
- `STATE_GAMEPLAY` - Active gameplay
- `STATE_PAUSE` - Paused (inventory/equipment)
- `STATE_MAP` - Full screen map
- `STATE_DEATH` - Death animation
- `STATE_ENDING` - Credits/ending

**Dependencies**: All modules (initializes them)
**Called by**: Hardware entry point
**Calls**: Current state's update/render functions

---

### 2. Input System

**Files**: `src/input/input.c`
**Headers**: `include/input.h`

**Responsibilities**:
- Scan DSi buttons each frame
- Maintain button state (pressed, held, released)
- Input buffering (3-5 frame window for techniques)
- Touch screen input (optional for map/menu)

**Button Mapping**:
```
DSi → SNES Function
───────────────────
D-pad → D-pad (movement)
A → A (jump)
B → B (shoot/run)
X → X (dash/item cancel)
Y → Y (item select)
L → L (aim diagonal up)
R → R (aim diagonal down)
Start → Start (pause)
Select → Select (item toggle)
```

**Interface**:
```c
typedef struct {
    uint16_t pressed;   // Buttons pressed this frame
    uint16_t held;      // Buttons held
    uint16_t released;  // Buttons released this frame
    uint8_t buffer[8];  // Input buffer for techniques
} InputState;

void input_init(void);
void input_update(void);
bool input_check(uint16_t button, InputCheckType type);
```

**Dependencies**: libnds key scan
**Called by**: Main loop (pre-update)
**Calls**: None

---

### 3. Physics Engine

**Files**: `src/physics/physics.c`, `src/physics/collision.c`
**Headers**: `include/physics.h`

**Responsibilities**:
- Gravity application (air: 0.07168 px/f², water: 0.02048 px/f²)
- Velocity integration (16.16 fixed-point)
- Collision detection (tile-based, slopes)
- Momentum preservation
- Environmental physics changes (water, lava, quicksand)

**Critical Constants** (from NTSC values):
```c
#define GRAVITY_AIR_NTSC    0x125C   // 0.07168 px/frame²
#define GRAVITY_WATER_NTSC  0x053F   // 0.02048 px/frame²
#define TERMINAL_VELOCITY   0x50000  // 5.02 px/frame
#define WALK_SPEED          0x18000  // 1.5 px/frame
#define RUN_SPEED           0x20000  // 2.0 px/frame
#define JUMP_INITIAL_VEL    0x49000  // 4.57 px/frame
```

**Collision Types** (from BTS data):
- Air ($00), Solid ($01-$0F)
- Slopes ($10-$2F, 8 angle variants)
- Breakable blocks ($30-$34)
- Crumble blocks ($40)
- Hazards: Spike ($50), Water ($60), Lava ($70), Acid ($78)
- Grapple points ($80)

**Interface**:
```c
typedef struct {
    int32_t x, y;           // 16.16 fixed-point position
    int32_t vx, vy;         // 16.16 fixed-point velocity
    uint16_t width, height; // Collision box size
    uint8_t flags;          // Physics flags (in_air, in_water, etc.)
} PhysicsBody;

void physics_init(void);
void physics_apply_gravity(PhysicsBody* body, uint8_t environment);
void physics_integrate_velocity(PhysicsBody* body);
bool physics_check_collision(PhysicsBody* body, int32_t dx, int32_t dy);
uint8_t physics_get_tile_type(int tile_x, int tile_y);
```

**Dependencies**: Room collision data
**Called by**: Player, enemies
**Calls**: Room system (collision queries)

---

### 4. Player System

**Files**: `src/player/player.c`, `src/player/animation.c`, `src/player/weapons.c`
**Headers**: `include/player.h`

**Responsibilities**:
- Samus state machine (standing, running, jumping, morphball, etc.)
- Animation control based on state
- Weapon firing (beams, missiles, bombs)
- Equipment management (suits, items)
- Health/ammo tracking

**Player States**:
```
STANDING, WALKING, RUNNING, JUMPING, SPIN_JUMP, FALLING
MORPHBALL, MORPHBALL_JUMP, AIMING_UP, AIMING_DOWN
CROUCHING, DAMAGE, DEATH, DOOR_TRANSITION
WALL_JUMP, SHINESPARK_CHARGE, SHINESPARK_ACTIVE
```

**Equipment Flags**:
- Suits: Varia, Gravity
- Beams: Wave, Ice, Spazer, Plasma, Charge
- Missiles: Missiles, Super Missiles
- Boots: Hi-Jump, Speed Booster, Space Jump
- Misc: Morph Ball, Bombs, Power Bombs, Grapple, X-Ray, Spring Ball, Screw Attack

**Interface**:
```c
typedef struct {
    PhysicsBody physics;
    PlayerState state;
    PlayerState prev_state;

    uint16_t health;
    uint16_t max_health;
    uint16_t missiles;
    uint16_t super_missiles;
    uint16_t power_bombs;
    uint16_t reserve_energy;

    uint32_t equipped_items;
    uint32_t equipped_beams;

    uint16_t animation_frame;
    uint16_t animation_timer;

    int16_t aim_angle;
    bool is_shooting;
} Player;

void player_init(void);
void player_update(Player* p, InputState* input);
void player_render(Player* p);
void player_take_damage(Player* p, uint16_t damage);
```

**Dependencies**: Physics, input, graphics
**Called by**: Gameplay state
**Calls**: Physics engine, weapon system

---

### 5. Enemy System

**Files**: `src/enemies/enemy_manager.c`, `src/enemies/enemy_ai.c`, `src/enemies/bosses/*.c`
**Headers**: `include/enemies.h`

**Responsibilities**:
- Enemy spawning from room data
- Enemy AI state machines
- Enemy collision with player/projectiles
- Boss-specific logic (Kraid, Ridley, Phantoon, Draygon, Mother Brain)
- Drop item generation

**Enemy Structure**:
```c
#define MAX_ENEMIES 32

typedef struct Enemy {
    uint16_t enemy_id;
    int32_t x, y;           // 16.16 fixed-point
    int32_t vx, vy;         // 16.16 fixed-point
    int16_t hp;
    uint16_t state;
    uint16_t timer;
    uint16_t frame;
    uint8_t palette;
    uint8_t flags;

    void (*update)(struct Enemy*);
    void (*render)(struct Enemy*);
    void (*on_hit)(struct Enemy*, int damage);

    uint8_t data[16];  // Enemy-specific data
} Enemy;
```

**Boss HP** (from ROM analysis):
- Kraid: 1000, Phantoon: 2500, Draygon: 6000, Ridley: 18000
- Mother Brain: 3000 (phase 1), 18000 (phase 2), 36000 (phase 3)

**Interface**:
```c
void enemies_init(void);
void enemies_spawn(uint16_t id, int x, int y);
void enemies_update_all(void);
void enemies_render_all(void);
void enemies_clear_all(void);
Enemy* enemies_check_collision(int x, int y, int w, int h);
```

**Dependencies**: Physics, graphics
**Called by**: Gameplay state, room loader
**Calls**: Physics engine for movement

---

### 6. World System

**Files**: `src/world/room.c`, `src/world/camera.c`, `src/world/doors.c`, `src/world/environment.c`
**Headers**: `include/room.h`

**Responsibilities**:
- Room loading/unloading (one room active at a time)
- Collision map management
- Door transitions
- Item placement and collection
- Camera scrolling with boundaries
- Environmental effects (lava rise/fall, rain, etc.)

**Room Structure**:
```c
typedef struct {
    uint8_t area_id;        // 0-6 (Crateria, Brinstar, Norfair, etc.)
    uint8_t room_id;
    uint8_t width_screens;  // Room width in screens (1 screen = 16×14 tiles)
    uint8_t height_screens; // Room height in screens

    uint16_t* tilemap;      // Decompressed 16×16 metatile indices
    uint8_t* collision;     // BTS collision type per tile
    uint8_t* scroll_data;   // Per-screen scroll flags (red/blue/green)

    PLM* plms;              // Power-up/load modules
    uint8_t plm_count;

    EnemySpawn* enemy_spawns;
    uint8_t enemy_count;

    Door* doors;
    uint8_t door_count;

    RoomFX* fx;             // Visual effects (rain, lava, fog)
} Room;
```

**Door Types**:
- Blue: Any weapon opens
- Red: 5 missiles required
- Green: 1 super missile required
- Yellow: 1 power bomb required
- Gray: Defeat all enemies

**Interface**:
```c
void room_init(void);
void room_load(uint8_t area_id, uint8_t room_id);
void room_unload(void);
void room_update(void);
uint8_t room_get_collision(int tile_x, int tile_y);
void room_break_block(int tile_x, int tile_y);
```

**Dependencies**: Graphics (for rendering), asset loader
**Called by**: Game state, door transitions
**Calls**: Graphics for tilemap, enemy spawner

---

### 7. Graphics Engine

**Files**: `src/graphics/framebuffer.c`, `src/graphics/background.c`, `src/graphics/sprites.c`, `src/graphics/palette.c`, `src/graphics/effects.c`, `src/graphics/hud.c`, `src/graphics/transitions.c`
**Headers**: `include/graphics.h`

**Responsibilities**:
- Dual-screen framebuffer (256×224 → split rendering)
- Background layer rendering (4 layers, priority sorting)
- Sprite rendering (128 hardware sprites, OAM management)
- Palette management and animation
- Screen effects (fade, flash, shake)
- HUD rendering (energy, ammo, map)
- Screen transitions (doors, elevators)

**Framebuffer Strategy**:
```
Internal buffer: 256×224 pixels
├─ Rows 0-31   → DSi bottom screen (32 pixels, HUD area)
└─ Rows 32-223 → DSi top screen (192 pixels, gameplay area)

VSync: 60 FPS (16.67ms budget per frame)
Double buffered to prevent tearing
```

**Background Layers** (from SNES Mode 1):
- BG1: Main level tilemap (16×16 metatiles)
- BG2: Parallax background
- BG3: HUD/UI layer
- BG4: Unused (or additional parallax)

**Sprite Priority**:
1. HUD elements (highest)
2. Player projectiles
3. Player (Samus)
4. Enemy projectiles
5. Enemies
6. Items/pickups
7. Background effects (lowest)

**Interface**:
```c
void graphics_init(void);
void graphics_vsync(void);
void graphics_clear_framebuffer(uint16_t color);

// Background
void bg_load_tileset(uint8_t layer, uint16_t* tiles, uint16_t tile_count);
void bg_load_tilemap(uint8_t layer, uint16_t* tilemap, uint16_t w, uint16_t h);
void bg_set_scroll(uint8_t layer, int16_t x, int16_t y);

// Sprites
typedef struct {
    int16_t x, y;
    uint16_t tile_index;
    uint8_t palette;
    uint8_t priority;
    bool flip_h, flip_v;
} Sprite;

void sprite_init(void);
void sprite_add(Sprite* s);
void sprite_clear_all(void);
void sprite_render_all(void);

// Palette
void palette_load(uint16_t* pal_data, uint16_t offset, uint16_t count);
void palette_animate(uint8_t pal_index, uint16_t* colors, uint8_t frame);

// Effects
void effect_fade_in(uint16_t frames);
void effect_fade_out(uint16_t frames);
void effect_flash(uint16_t color, uint16_t duration);
void effect_shake(uint16_t intensity, uint16_t duration);

// HUD
void hud_render(Player* p);
void hud_set_visibility(bool visible);
```

**Dependencies**: Asset data (tiles, palettes)
**Called by**: All rendering code
**Calls**: libnds hardware functions

---

### 8. Audio Engine

**Files**: `src/audio/audio.c`, `src/audio/music.c`, `src/audio/sfx.c`
**Headers**: `include/audio.h`

**Responsibilities**:
- Music playback (maxmod library)
- Sound effect playback with priority
- Area-based music switching
- Boss music triggers
- Audio crossfading

**Audio Priorities**:
- High (0-1): Player sounds (damage, jump, land)
- Medium (2-5): Weapons and enemy sounds
- Low (6-7): Ambient/environmental

**Music Tracks** (from ROM, bank $CF-$DE):
- Title theme, Crateria (surface/underground), Brinstar (green/red)
- Norfair (ancient/hot), Wrecked Ship (powered/unpowered)
- Maridia (exterior/interior), Tourian
- Boss themes (standard, mini-boss, Ridley-specific)
- Item room, escape sequence, ending

**Interface**:
```c
void audio_init(void);
void audio_update(void);

// Music
void music_play(uint8_t track_id, bool loop);
void music_stop(void);
void music_fade_out(uint16_t frames);
void music_crossfade(uint8_t new_track, uint16_t frames);

// SFX
void sfx_play(uint8_t sfx_id, uint8_t priority);
void sfx_stop(uint8_t sfx_id);
void sfx_stop_all(void);
```

**Dependencies**: maxmod, converted audio assets
**Called by**: Game state, player, enemies, world events
**Calls**: maxmod API

---

### 9. Save System

**Files**: `src/game/save.c`
**Headers**: `include/save.h`

**Responsibilities**:
- Save game state to SD card or DSi NAND
- Load game state
- 3 save slots + copy/delete functions
- Data validation (checksum)

**Save Data Structure**:
```c
typedef struct {
    uint32_t magic;          // "SMDS" (Super Metroid DSi)
    uint16_t version;
    uint16_t checksum;

    uint8_t current_area;
    uint8_t current_room;
    uint16_t player_x;
    uint16_t player_y;

    uint16_t energy;
    uint16_t max_energy;
    uint16_t missiles;
    uint16_t max_missiles;
    uint16_t super_missiles;
    uint16_t max_super_missiles;
    uint16_t power_bombs;
    uint16_t max_power_bombs;
    uint16_t reserve_energy;
    uint16_t max_reserve_energy;

    uint32_t equipped_items;
    uint32_t equipped_beams;

    uint8_t items_collected[32];   // Bitmask of collected items
    uint8_t bosses_defeated[8];    // Bitmask of defeated bosses
    uint8_t doors_opened[64];      // Bitmask of opened doors
    uint8_t map_revealed[256];     // Bitmask of revealed map tiles

    uint32_t playtime_frames;      // Total playtime in frames

    uint8_t padding[128];          // Future expansion
} SaveData;
```

**Interface**:
```c
void save_init(void);
bool save_write(uint8_t slot, SaveData* data);
bool save_read(uint8_t slot, SaveData* data);
bool save_delete(uint8_t slot);
bool save_copy(uint8_t src_slot, uint8_t dst_slot);
bool save_exists(uint8_t slot);
```

**Dependencies**: File I/O (SD card or NAND)
**Called by**: Game state
**Calls**: FAT filesystem API

---

## Memory Allocation Strategy

**Total DSi RAM**: 16 MB (16,777,216 bytes)

### Memory Budget Breakdown

| System | Size | Notes |
|--------|------|-------|
| **Player/Physics** | ~1 MB | Player state, physics buffers, projectiles |
| **Enemies** | ~2 MB | 32 enemy slots @ ~64 KB each (AI + animation data) |
| **Graphics (loaded)** | ~4 MB | Current area tileset + Samus sprites + enemy sprites |
| **Audio (loaded)** | ~2 MB | Current music track + sound effects (maxmod soundbank) |
| **Level Data** | ~1 MB | Current room tilemap + collision + PLMs |
| **Framebuffer** | ~256 KB | 256×224×2 bytes (double buffered, 16-bit color) |
| **Stack/Heap** | ~2 MB | General allocations, temporary buffers |
| **Code/Data** | ~1.5 MB | Compiled .nds code section |
| **Reserve** | ~2.25 MB | Safety margin for overhead and future expansion |
| **Total** | 16 MB | |

### Memory Management Strategy

**Static Allocation**:
- Framebuffers (compile-time allocation)
- Player struct (single global instance)
- Enemy array (32 slots pre-allocated)

**Dynamic Allocation** (pool allocators):
- Projectiles (player shots, enemy shots) - 128 projectile pool
- Particles (explosions, effects) - 256 particle pool
- Room data (load on room transition, free previous room)

**Streaming**:
- Audio: Stream music from ROM or use compressed formats
- Graphics: Load only current area tileset + active enemy sprites
- Level: Load only current room (unload previous on door transition)

**Memory Profiling Points**:
1. After room load (check peak allocation)
2. During boss fights (max enemies + effects)
3. During escape sequence (particle-heavy)

---

## Data Flow

### Frame Update Sequence (60 FPS)

```
1. VSync Wait (16.67ms budget)
2. Input Scan
   └─> Update InputState (pressed, held, released)
3. State Update (dispatch based on current state)
   ├─> GAMEPLAY state:
   │   ├─> Player Update
   │   │   ├─> Read input
   │   │   ├─> Update state machine
   │   │   ├─> Physics integration
   │   │   ├─> Collision checks
   │   │   ├─> Weapon firing
   │   │   └─> Animation update
   │   ├─> Enemy Update (all active)
   │   │   ├─> AI logic
   │   │   ├─> Physics integration
   │   │   ├─> Collision checks
   │   │   └─> Animation update
   │   ├─> World Update
   │   │   ├─> Door checks
   │   │   ├─> Item collection checks
   │   │   ├─> Crumble block timers
   │   │   └─> Environmental effects
   │   └─> Camera Update
   │       ├─> Follow player
   │       ├─> Apply scroll bounds
   │       └─> Screen shake effects
   └─> Other states: title, pause, map, etc.
4. Audio Update
   ├─> Music system tick
   └─> SFX priority management
5. Render
   ├─> Clear framebuffer
   ├─> Render backgrounds (BG1, BG2)
   │   └─> Apply camera scroll offset
   ├─> Render sprites (all layers, priority sorted)
   │   ├─> Enemy sprites
   │   ├─> Player sprite
   │   ├─> Projectiles
   │   └─> Effects
   ├─> Render HUD (BG3)
   │   ├─> Energy bar
   │   ├─> Ammo counters
   │   └─> Reserve tank
   └─> Apply screen effects (fade, flash)
6. Flip Framebuffer (swap buffers)
7. Goto 1
```

---

## Module Dependencies

### Dependency Graph

```
main.c
  ├─> game_state.h (state_manager)
  │     ├─> input.h
  │     ├─> player.h
  │     ├─> enemies.h
  │     ├─> room.h
  │     ├─> graphics.h
  │     ├─> audio.h
  │     └─> save.h
  │
  ├─> input.h
  │     └─> [libnds]
  │
  ├─> player.h
  │     ├─> physics.h
  │     ├─> input.h
  │     └─> graphics.h
  │
  ├─> enemies.h
  │     ├─> physics.h
  │     └─> graphics.h
  │
  ├─> physics.h
  │     └─> room.h (collision queries)
  │
  ├─> room.h
  │     ├─> graphics.h (rendering)
  │     └─> enemies.h (spawning)
  │
  ├─> graphics.h
  │     └─> [libnds]
  │
  ├─> audio.h
  │     └─> [maxmod]
  │
  └─> save.h
        └─> [FAT filesystem]
```

### Initialization Order

Critical initialization sequence (dependencies must be initialized first):

```c
int main(void) {
    // 1. Hardware
    powerOn(POWER_ALL_2D);
    videoSetMode(MODE_5_2D);
    vramSetBankA(VRAM_A_MAIN_BG);
    // ...DSi-specific init

    // 2. Core systems (no dependencies)
    input_init();
    graphics_init();
    audio_init();

    // 3. Game systems (depend on core)
    physics_init();
    save_init();

    // 4. Gameplay systems (depend on game systems)
    player_init();
    enemies_init();
    room_init();

    // 5. State manager (depends on everything)
    game_state_init();
    game_state_set(STATE_TITLE);

    // 6. Main loop
    while (1) {
        swiWaitForVBlank();

        input_update();
        game_state_update();
        audio_update();
        game_state_render();

        graphics_vsync();
    }

    return 0;
}
```

---

## DSi-Specific Considerations

### Hardware Constraints

**ARM9 CPU**: 133 MHz (vs SNES @ 3.58 MHz)
- **Advantage**: ~37× faster CPU, can afford more complex logic
- **Challenge**: Must still hit 60 FPS with dual-screen rendering

**RAM**: 16 MB (vs SNES @ 128 KB)
- **Advantage**: Can load entire areas, don't need clever streaming
- **Challenge**: Asset size management (decompressed graphics/audio)

**VRAM**: 656 KB
- **Advantage**: Sufficient for multiple tilesets + sprites
- **Challenge**: Need efficient palette management (256 colors)

**Audio**: Hardware mixing, maxmod library
- **Advantage**: Better audio quality than SNES SPC700
- **Challenge**: Converting SNES music/SFX format

### Dual-Screen Strategy

**Top Screen (256×192)**: Main gameplay area (rows 32-223)
**Bottom Screen (256×192)**: HUD area (rows 0-31) + optional touch controls

**Rendering approach**:
1. Render to single 256×224 internal framebuffer
2. Blit rows 32-223 to top screen (192 pixels)
3. Blit rows 0-31 to bottom screen, stretch or add extra UI

**Alternative** (optional future enhancement):
- Use bottom screen for full-size map during gameplay
- Touch screen for weapon switching

### Performance Targets

**Frame Budget**: 16.67ms @ 60 FPS
- Input: <0.1ms
- Update (player + enemies + world): <5ms
- Render: <10ms
- Audio: <1ms
- Overhead: ~0.6ms reserve

**Optimization strategies**:
- Sprite culling (don't render off-camera)
- Collision checks only for nearby tiles
- Enemy AI simplification when off-screen
- Pool allocators to reduce malloc overhead

---

## Testing & Validation

### Physics Verification

**Frame-perfect comparison** with original SNES:
- Jump height (standing, running, spin)
- Fall speed and terminal velocity
- Horizontal speed curves (walk, run, dash)
- Wall jump timing window
- Shinespark charge timing
- Momentum preservation through door transitions

**Test techniques**:
- Mockball (morph while running)
- Short charge (shinespark with reduced runway)
- Infinite bomb jump
- Wall jump ladder climb
- Crystal flash

### Integration Tests

**Room-by-room validation**:
- All 200+ rooms load correctly
- All enemies spawn and behave
- All items are collectible
- All doors function
- No memory leaks after 10+ room transitions

**Boss validation**:
- All bosses beatable with standard equipment
- Boss patterns match original timing
- No softlocks or crashes

### Performance Tests

**Target metrics**:
- 60 FPS in all rooms (including boss rooms)
- No audio dropouts or crackling
- Memory usage < 14 MB (leaving 2 MB reserve)
- Load time < 1 second per room transition

---

## Future Enhancements (Post-MVP)

**Quality of Life**:
- Touch screen map (real-time navigation)
- Quick weapon switch via touch
- Save anywhere (vs save stations only)

**Visual Enhancements**:
- Higher resolution backgrounds (upscaled)
- Sprite smoothing/filtering
- CRT scanline effect (optional)

**Gameplay**:
- New game modes (randomizer, boss rush)
- Achievement system
- Speedrun timer with splits

**Technical**:
- Wi-Fi leaderboards (via DSi Wi-Fi)
- Screenshot feature
- Debug mode with RAM viewer

---

## Summary

This architecture balances **fidelity to the original** with **DSi-specific optimizations**:

- **Physics**: Frame-perfect translation from 65C816 assembly
- **Graphics**: Native DSi rendering with dual-screen support
- **Memory**: Efficient allocation with streaming for large assets
- **Performance**: 60 FPS with overhead for effects and polish

**Critical path**: Physics engine (Agent A) must be precise. Everything else can be iterated.

**Integration points**: Clear module boundaries allow parallel development by 3 agents.

---

*Architecture designed for Super Metroid DSi Port*
*Agent B: Graphics & Integration Lead*
