# Super Metroid DS Port - System Architecture

## Overview

**Target Platform:** Nintendo DS, NTR mode (67 MHz ARM9, 4 MB RAM, dual 256x192 screens)
**Rendering:** Hardware tile backgrounds + OAM sprites (no software framebuffer)
**Frame Rate:** 60 fps (fixed timestep, VBlank-synced, ~1.1M cycles/frame)
**Memory:** All allocation at startup, static pools, no malloc during gameplay
**Math:** 16.16 fixed-point integer arithmetic (no floating-point)

For build commands, code standards, and AI constraints, see `claude.md`.
For SNES game data, see: `physics_data.md`, `graphics_data.md`, `audio_data.md`, `level_data.md`, `enemy_data.md`, `rom_memory_map.md`.

---

## Dual-Screen Layout

The SNES renders at 256x224. The DS has two 256x192 screens. The 224 scanlines are split across both screens using independent hardware 2D engines:

```
Top Screen (Main Engine, 256x192)
┌────────────────────────────┐
│                            │
│   Gameplay Area            │
│   192 rows of the world    │
│   BG layers + OAM sprites  │
│                            │
└────────────────────────────┘

Bottom Screen (Sub Engine, 256x192)
┌────────────────────────────┐
│ HUD Strip (32px)           │  <- Energy, ammo, mini-map
├────────────────────────────┤
│                            │
│   Extra Space (160px)      │
│   Full map / touch UI      │
│                            │
└────────────────────────────┘
```

Each engine has its own:
- 4 BG layers (tile-based, hardware-scrolled)
- 128-sprite OAM
- Independent VRAM banks
- Independent palette RAM

No pixel data is copied between engines. Each renders independently from its own VRAM.

---

## Core Loop

Fixed timestep, VBlank-synced. One simulation tick per frame, no delta time.

```c
int main(int argc, char* argv[]) {
    defaultExceptionHandler();  // Must be first -- enables crash debugging

    // Hardware init
    // Video mode setup, VRAM bank assignment, OAM init, etc.

    // System init (order matters)
    // input, graphics, audio, physics, save, player, enemies, room, state

    while (pmMainLoop()) {
        swiWaitForVBlank();

        // 1. Input
        scanKeys();

        // 2. Simulation (fixed step)
        //    state_update() dispatches to current game state
        //    which updates player, enemies, projectiles, physics, triggers

        // 3. Camera update

        // 4. Renderer prep (build OAM table, compute scroll offsets)
        //    Game logic does NOT write hardware registers directly.
        //    All changes are batched and committed during VBlank.

        // 5. Audio commands

        // 6. Hardware commit (OAM upload, scroll registers, palette updates)
    }

    return 0;
}
```

**Key rules:**
- `pmMainLoop()` replaces `while(1)` -- required for calico power management
- `scanKeys()` exactly once per frame -- multiple calls break delta tracking
- Hardware register writes batched to VBlank -- prevents tearing

---

## Rendering Architecture

**Hardware tile/sprite compositing only. No software framebuffer.**

The SNES was tile/sprite-based. The DS is tile/sprite-based. This is a direct architectural match.

### Background Layers (per engine)

| Layer | Purpose | SNES Equivalent |
|-------|---------|-----------------|
| BG0 | Main level tilemap (16x16 metatiles) | BG1 |
| BG1 | Parallax background | BG2 |
| BG2 | Foreground overlay / effects | BG3 |
| BG3 | HUD / debug text (sub engine) | -- |

- Update only changed tiles, not entire tilemap
- Hardware handles scrolling via register writes
- Tileset and tilemap data loaded into VRAM at room load

### Sprites (OAM)

- 128 hardware sprites per engine (OAM limit)
- Preallocated sprite pool, never dynamically allocated
- Update positions/attributes during simulation; commit to OAM during VBlank
- Priority sorting: HUD > player projectiles > player > enemy projectiles > enemies > items > effects

### VRAM Layout

Assign banks at startup and keep them fixed. Example allocation:

| Bank | Size | Assignment |
|------|------|------------|
| A (128 KB) | Main engine BG tiles | Level tileset |
| B (128 KB) | Main engine BG maps | Level tilemap + parallax |
| C (128 KB) | Sub engine BG | HUD + map tiles |
| D (128 KB) | Main engine sprite tiles | Samus + enemies + projectiles |
| E (64 KB) | Palettes | BG + sprite palettes |
| F (16 KB) | Extended palettes | Additional palettes if needed |
| G (16 KB) | Extended palettes | Additional palettes if needed |
| H (32 KB) | Sub engine sprite tiles | HUD icons |
| I (16 KB) | Sub engine BG maps | HUD tilemap |

Never reconfigure VRAM mid-frame.

---

## Memory Management

### NTR Memory Budget (4 MB total)

| System | Estimate | Notes |
|--------|----------|-------|
| Code + static data | ~200 KB | Compiled ARM9 binary |
| Player state | ~2 KB | Single global instance |
| Enemy pool | ~4 KB | 16 slots x ~256 bytes each |
| Projectile pool | ~4 KB | 32 slots |
| Particle pool | ~3 KB | 48 slots |
| Room data (loaded) | ~64 KB | Current room tilemap + collision + PLMs |
| Tileset (loaded) | ~128 KB | Current area 4bpp tiles in RAM staging |
| Sprite frames (loaded) | ~256 KB | Samus + current enemies |
| Audio soundbank | ~512 KB | Maxmod bank (compressed) |
| Stack (DTCM) | ~16 KB | User + IRQ + SVC stacks |
| VRAM (separate) | 656 KB | Not counted against main RAM |
| **Estimated used** | **~1.2 MB** | |
| **Available** | **~2.8 MB** | Comfortable margin |

### Allocation Strategy

**At startup (before game loop):**
- All entity pools (enemies, projectiles, particles) as static arrays
- Framebuffer: none needed (hardware rendering)
- Player struct: single global

**On room transition:**
- Unload previous room data
- Load new room into same static buffers
- Spawn enemies from room data into pool
- Load tileset into VRAM if area changed

**During gameplay:**
- Zero heap allocation
- Entity spawn/despawn uses pool index management (swap-remove)
- No linked lists, no pointer graphs

```c
#define MAX_ENEMIES     16
#define MAX_PROJECTILES 32
#define MAX_PARTICLES   48

static Enemy enemies[MAX_ENEMIES];
static int activeEnemyCount;

// Spawn: append to end
int spawnEnemy(/* ... */) {
    if (activeEnemyCount >= MAX_ENEMIES) return -1;
    enemies[activeEnemyCount] = /* ... */;
    return activeEnemyCount++;
}

// Remove: swap with last (no gaps, no fragmentation)
void removeEnemy(int index) {
    enemies[index] = enemies[activeEnemyCount - 1];
    activeEnemyCount--;
}
```

---

## Module Breakdown

### Game States

| State | Description |
|-------|-------------|
| `STATE_TITLE` | Title screen |
| `STATE_FILE_SELECT` | Save file selection |
| `STATE_GAMEPLAY` | Active gameplay |
| `STATE_PAUSE` | Inventory/equipment |
| `STATE_MAP` | Full screen map |
| `STATE_DEATH` | Death animation |
| `STATE_ENDING` | Credits |

State manager dispatches `update()` and `render()` to current state.

### Module Dependencies

```
main.c
├── state_manager  (dispatches to current state)
│   ├── input      (scanKeys, button state)
│   ├── player     (state machine, physics, weapons, animation)
│   │   └── physics (gravity, velocity, collision)
│   │       └── room (collision map queries)
│   ├── enemies    (AI, spawning, collision)
│   │   └── physics
│   ├── room       (load/unload, tilemap, doors, items)
│   ├── graphics   (BG layers, OAM, palettes, effects)
│   ├── audio      (maxmod music + SFX)
│   └── save       (FAT filesystem)
```

### Physics Engine

Uses 16.16 fixed-point matching exact SNES constants. See `docs/physics_data.md` for all values.

- Gravity varies by environment (air, water, lava)
- Collision is tile-coordinate lookup: convert position to tile coords, check overlapping tiles
- Slopes use 8 angle variants from BTS data
- No per-pixel collision, no full-map iteration

### Input System

```c
// Called once per frame, in this order:
scanKeys();
int pressed = keysDown();   // Rising edge
int held = keysHeld();      // Currently held
int released = keysUp();    // Falling edge
```

Button mapping: DS D-pad/A/B/X/Y/L/R/Start/Select map directly to SNES equivalents.
Input buffering (3-5 frame window) for advanced techniques (wall jump, shinespark).

### Audio Engine

Maxmod library (`-lmm9`) for music and SFX.
- Music tracks converted from SNES BRR/SPC at build time
- SFX with priority system (player > weapons > enemies > ambient)
- Area-based music switching on room transitions
- See `docs/audio_data.md` for track list and format details

### Save System

FAT filesystem on SD card (via `-lfat`).
- 3 save slots
- Checksum validation
- Saves: position, health, ammo, equipment, items collected, bosses defeated, map revealed, playtime

---

## Frame Update Sequence

```
1. swiWaitForVBlank()     -- sync to 60 Hz
2. scanKeys()             -- read input hardware
3. State dispatch:
   GAMEPLAY:
   ├── Player update (input -> state machine -> physics -> collision -> animation)
   ├── Enemy update (AI -> physics -> collision -> animation)
   ├── Projectile update (movement -> collision -> lifetime)
   ├── World update (door checks, item collection, crumble timers, environment)
   └── Camera update (follow player, scroll bounds, screen shake)
4. Audio update (music tick, SFX priority)
5. Renderer prep (build OAM table from entity positions, compute BG scroll offsets)
6. Hardware commit during VBlank (OAM upload, scroll registers, palette writes)
7. Loop
```

---

## Asset Pipeline

All assets preprocessed at build time. No runtime format conversion.

| Asset Type | Build Tool | Source Format | Runtime Format |
|-----------|-----------|---------------|---------------|
| Tilesets | grit | PNG + .grit | 4bpp/8bpp tiles, native palette |
| Tilemaps | Custom (Python) | SNES ROM data | DS-native tilemap entries |
| Sprites | grit | PNG + .grit | 4bpp/8bpp OAM-ready tiles |
| Animation | Custom (Python) | SNES ROM data | Frame index tables |
| Level data | Custom (Python) | SNES ROM data | Pre-chunked room structs |
| Audio | mmutil | WAV/MOD/XM | Maxmod soundbank |
| Binary data | bin2o | Any binary | Embedded in ROM |

SNES-specific conversions:
- 4bpp planar tiles -> 4bpp linear (bitplane deinterleaving)
- BGR555 palettes are directly compatible (no conversion needed)
- LC_LZ2 compressed room data -> pre-decompressed at build time

---

## Performance Budget

```
67,000,000 cycles/sec / 60 fps = ~1,116,000 cycles/frame

Estimated per-frame costs:
  Input:          ~1,000 cycles    (<0.1%)
  Player update:  ~50,000 cycles   (~4.5%)
  Enemy update:   ~80,000 cycles   (~7.2%, 16 enemies worst case)
  Physics:        ~30,000 cycles   (~2.7%)
  World update:   ~20,000 cycles   (~1.8%)
  Camera:         ~5,000 cycles    (~0.4%)
  Audio:          ~10,000 cycles   (~0.9%)
  OAM prep:       ~15,000 cycles   (~1.3%)
  ─────────────────────────────────
  Total:         ~211,000 cycles   (~19%)
  Headroom:      ~905,000 cycles   (~81%)
```

The headroom is enormous. If a Super Metroid port struggles at 60 fps on this hardware, it is an implementation failure, not a hardware limitation. The only way to fail is architectural malpractice (software framebuffer, per-pixel collision, floating-point math, unbounded entity counts).

---

## Testing Strategy

### Physics Verification
- Frame-perfect comparison with original SNES for: jump height, fall speed, horizontal speed curves, wall jump timing, shinespark charge timing
- Test techniques: mockball, short charge, infinite bomb jump, wall jump ladder, crystal flash

### Room Validation
- All rooms load and render correctly
- All enemies spawn with correct behavior
- All items collectible, all doors functional
- No memory leaks across room transitions

### Performance
- 60 fps in all rooms including boss fights
- No audio dropouts
- Memory usage stable over extended play
