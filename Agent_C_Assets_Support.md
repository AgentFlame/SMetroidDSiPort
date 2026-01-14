# Agent C: Asset Pipeline & Support
## Model: Haiku | Token Budget: 100K-200K

---

## Role Summary

You are responsible for all data extraction and conversion, development environment setup, testing infrastructure, and documentation. You enable the other agents to do their work.

**Your work starts first and ends last.** You set up the environment, provide the data, test the results, and document everything.

---

## Day 1: Environment & Extraction

### Task 1.1: Development Environment Setup
**Prerequisites**: None
**Deliverables**:
- [ ] Install devkitPro/devkitARM
  - Windows: devkitPro installer
  - Or: pacman -S devkitARM
- [ ] Install libnds
- [ ] Install grit (graphics conversion)
- [ ] Install mmutil (audio conversion)
- [ ] Set environment variables (DEVKITPRO, DEVKITARM)
- [ ] Install emulator (DeSmuME or melonDS)
- [ ] Create `docs/environment_setup.md`
- [ ] Verify: compile libnds example, run in emulator

### Task 1.2: ROM Analysis & Extraction
**Prerequisites**: User provides ROM file
**Deliverables**:
- [ ] Create `docs/rom_memory_map.md` with:
  - ROM header information
  - Graphics data ranges
  - Audio data ranges
  - Level/room data ranges
  - Enemy data ranges
  - Text/string data
  - Physics lookup tables
- [ ] Extract raw graphics data to `raw/graphics/`
- [ ] Extract raw audio data to `raw/audio/`
- [ ] Extract level data to `raw/levels/`
- [ ] Extract enemy data to `raw/enemies/`
- [ ] Create extraction tools in `tools/` for reproducibility

### Task 1.3: Coordinate with Agents A & B
**Prerequisites**: Task 1.2 complete
**Action**:
- [ ] Share ROM map with Agent A (physics tables)
- [ ] Share ROM map with Agent B (for architecture planning)
- [ ] Respond to any data location questions

---

## Day 2: Graphics Conversion

### Task 2.1: Tile & Tileset Conversion
**Prerequisites**: Task 1.2
**Deliverables**:
- [ ] Create `tools/tile_converter.py` (or C)
- [ ] Convert SNES 4bpp tiles → DSi format
- [ ] Handle 8x8 and 16x16 tiles
- [ ] Output to `assets/tilesets/`
- [ ] Document format in `docs/tileset_format.md`
- [ ] Converted tilesets:
  - [ ] Crateria (surface, underground)
  - [ ] Brinstar (green, red, pink)
  - [ ] Norfair (heated, hot)
  - [ ] Wrecked Ship (powered, unpowered)
  - [ ] Maridia (sand, water)
  - [ ] Tourian
  - [ ] Ceres Station
  - [ ] Common (doors, items, effects)

### Task 2.2: Palette Conversion
**Prerequisites**: Task 1.2
**Deliverables**:
- [ ] Create `tools/palette_converter.py`
- [ ] Convert SNES 15-bit → DSi 15-bit (BGR555)
- [ ] Extract all palettes
- [ ] Identify palette animation sequences
- [ ] Output to `assets/palettes/`
- [ ] Document in `docs/palette_format.md`

### Task 2.3: Sprite Conversion
**Prerequisites**: Task 1.2
**Deliverables**:
- [ ] Create `tools/sprite_converter.py`
- [ ] Convert sprite sheets:
  - [ ] Samus (all suits, all poses)
  - [ ] Morph ball + bombs
  - [ ] Beam projectiles
  - [ ] Missile/super missile
  - [ ] Power bomb explosion
  - [ ] Item pickups
  - [ ] All standard enemies
  - [ ] All boss sprites
  - [ ] Effects (explosions, particles)
- [ ] Output to `assets/sprites/`
- [ ] Generate sprite metadata (frames, sizes, hotspots)
- [ ] Document in `docs/sprite_format.md`

---

## Day 2-3: Level Data Conversion

### Task 2.4: Room Data Conversion
**Prerequisites**: Task 1.2
**Deliverables**:
- [ ] Create `tools/room_converter.py`
- [ ] Parse SNES room format
- [ ] Convert to DSi-friendly format:
  ```c
  struct Room {
      uint16_t width_tiles;
      uint16_t height_tiles;
      uint16_t* tilemap;      // layer 1
      uint16_t* tilemap_bg;   // layer 2
      uint8_t* collision;     // collision types per tile
      uint8_t scroll_x_min, scroll_x_max;
      uint8_t scroll_y_min, scroll_y_max;
      EnemySpawn* enemies;
      ItemPlacement* items;
      DoorData* doors;
  };
  ```
- [ ] Convert all rooms:
  - [ ] Crateria (~25 rooms)
  - [ ] Brinstar (~40 rooms)
  - [ ] Norfair (~45 rooms)
  - [ ] Wrecked Ship (~20 rooms)
  - [ ] Maridia (~35 rooms)
  - [ ] Tourian (~15 rooms)
  - [ ] Ceres (~10 rooms)
- [ ] Output to `assets/rooms/`
- [ ] Document in `docs/room_format.md`

### Task 2.5: Collision Data
**Prerequisites**: Task 2.4
**Deliverables**:
- [ ] Extract collision types:
  - Solid
  - Air
  - Slope (various angles)
  - Spike
  - Lava/acid
  - Water
  - Shot-block
  - Bomb-block
  - Power-bomb-block
  - Speed-block
  - Crumble
  - Grapple point
- [ ] Map to collision enum
- [ ] Verify collision data accuracy

### Task 2.6: Map Data
**Prerequisites**: Task 2.4
**Deliverables**:
- [ ] Extract map tile data per area
- [ ] Create map reveal flags per room
- [ ] Item dot positions
- [ ] Save station positions
- [ ] Output to `assets/maps/`

---

## Day 3: Audio Conversion

### Task 3.1: Sample Extraction
**Prerequisites**: Task 1.2
**Deliverables**:
- [ ] Create `tools/spc_extractor.py`
- [ ] Extract BRR samples from SPC data
- [ ] Convert to WAV or DSi-native format
- [ ] Output to `assets/audio/samples/`
- [ ] Catalog all samples with descriptions

### Task 3.2: Music Conversion
**Prerequisites**: Task 3.1
**Deliverables**:
- [ ] Determine approach:
  - Option A: Convert to MOD/XM format
  - Option B: Stream pre-rendered audio
  - Option C: Port SPC sequences to maxmod
- [ ] Convert all music tracks:
  - [ ] Title theme
  - [ ] Item fanfare
  - [ ] Crateria (surface, underground)
  - [ ] Brinstar (green, red)
  - [ ] Norfair
  - [ ] Wrecked Ship (unpowered, powered)
  - [ ] Maridia
  - [ ] Tourian
  - [ ] Boss theme
  - [ ] Mini-boss theme
  - [ ] Ridley theme
  - [ ] Escape theme
  - [ ] Ending theme
- [ ] Output to `assets/audio/music/`

### Task 3.3: Sound Effect Conversion
**Prerequisites**: Task 3.1
**Deliverables**:
- [ ] Extract and convert all SFX:
  - [ ] Beam shots (all types)
  - [ ] Missile launch/hit
  - [ ] Bomb place/explode
  - [ ] Door open/close
  - [ ] Item pickup (various)
  - [ ] Damage taken
  - [ ] Jump/land
  - [ ] Morphball transform
  - [ ] Enemy hit/death
  - [ ] Menu sounds
- [ ] Output to `assets/audio/sfx/`
- [ ] Create SFX index header

---

## Day 3-4: Audio Engine Support

### Task 3.4: Audio Engine Implementation
**Prerequisites**: Task 3.2, Task 3.3
**Deliverables**:
- [ ] Implement `audio/audio.c`
- [ ] Initialize maxmod (or chosen library)
- [ ] Music playback functions:
  ```c
  void audio_play_music(MusicTrack track);
  void audio_stop_music();
  void audio_fade_music(int frames);
  ```
- [ ] SFX playback functions:
  ```c
  void audio_play_sfx(SFX sfx);
  void audio_play_sfx_at(SFX sfx, int x, int y); // positional
  ```
- [ ] Priority system (don't cut important sounds)
- [ ] Test playback in emulator

---

## Day 4: Testing Infrastructure

### Task 4.1: Test Room
**Prerequisites**: Agent B provides renderer, Agent C provides room data
**Deliverables**:
- [ ] Create test room with:
  - All tile types
  - All collision types
  - Sample enemies
  - Items
  - Doors
- [ ] Quick-load test room for development

### Task 4.2: Debug Overlay
**Prerequisites**: Agent B provides HUD system
**Deliverables**:
- [ ] Implement `debug/overlay.c`
- [ ] Toggle with button combo (L+R+Select)
- [ ] Display:
  - FPS counter
  - Player position (X, Y)
  - Player velocity
  - Current room ID
  - Memory usage
  - Sprite count
- [ ] Collision box visualization
- [ ] Hitbox display

### Task 4.3: Input Recording/Playback
**Prerequisites**: Agent B provides input system
**Deliverables**:
- [ ] Implement `debug/input_record.c`
- [ ] Record input to file
- [ ] Playback input from file
- [ ] For physics verification (Agent A)

---

## Day 5-6: Testing

### Task 5.1: Room-by-Room Testing
**Prerequisites**: Agent B's room loader working
**Deliverables**:
- [ ] Test each room loads:
  - [ ] Correct tilemap
  - [ ] Correct collision
  - [ ] Enemies spawn
  - [ ] Items present
  - [ ] Doors work
- [ ] Document issues in `bugs/room_bugs.md`

### Task 5.2: Item Collection Testing
**Prerequisites**: Agent A's player systems
**Deliverables**:
- [ ] Test every item is:
  - [ ] Visible
  - [ ] Collectible
  - [ ] Grants correct ability
  - [ ] Save state correct
- [ ] 100% item checklist
- [ ] Document issues in `bugs/item_bugs.md`

### Task 5.3: Enemy Testing
**Prerequisites**: Agent A's enemy AI
**Deliverables**:
- [ ] Test each enemy type:
  - [ ] Spawns correctly
  - [ ] Moves correctly
  - [ ] Takes damage
  - [ ] Dies properly
  - [ ] Drops items
- [ ] Test each boss:
  - [ ] All phases work
  - [ ] Can be defeated
  - [ ] Rewards granted
- [ ] Document issues in `bugs/enemy_bugs.md`

### Task 5.4: Progression Testing
**Prerequisites**: Task 5.1, 5.2, 5.3
**Deliverables**:
- [ ] Full playthrough from Ceres to Tourian
- [ ] Verify all required paths accessible
- [ ] Verify boss order doesn't softlock
- [ ] Verify escape sequence works
- [ ] Verify ending plays
- [ ] Note any sequence breaks that should/shouldn't work

---

## Day 6: Bug Documentation

### Task 6.1: Bug Report Compilation
**Prerequisites**: Tasks 5.1-5.4
**Deliverables**:
- [ ] Compile all bug reports
- [ ] Categorize:
  - Critical (blocks progress)
  - Major (significant issue)
  - Minor (cosmetic/polish)
- [ ] Prioritize for Agent A and B
- [ ] Create `bugs/bug_summary.md`

### Task 6.2: Bug Reproduction
**Prerequisites**: Task 6.1
**Deliverables**:
- [ ] Create repro steps for each bug
- [ ] Create input recordings for physics bugs
- [ ] Provide save states at bug locations
- [ ] Assist Agents A/B with debugging

---

## Day 7: Documentation

### Task 7.1: README
**Prerequisites**: Agent B's final build
**Deliverables**:
- [ ] Create `README.md`:
  - Project description
  - Installation/running instructions
  - Controls reference
  - Known issues
  - Credits

### Task 7.2: Build Instructions
**Prerequisites**: Agent B's final build
**Deliverables**:
- [ ] Create `docs/building.md`:
  - Environment setup
  - Dependencies
  - Build commands
  - Emulator setup
  - Hardware installation

### Task 7.3: Asset Pipeline Documentation
**Prerequisites**: All assets converted
**Deliverables**:
- [ ] Create `docs/asset_pipeline.md`:
  - How to re-extract from ROM
  - How to modify assets
  - Format specifications
  - Tool usage

### Task 7.4: Final Testing
**Prerequisites**: Agent B's release build
**Deliverables**:
- [ ] Final playthrough on release build
- [ ] Verify no regressions
- [ ] Sign off on release

---

## File Structure

```
tools/
├── tile_converter.py
├── palette_converter.py
├── sprite_converter.py
├── room_converter.py
├── spc_extractor.py
└── build_assets.sh

raw/
├── graphics/
├── audio/
├── levels/
└── enemies/

assets/
├── tilesets/
├── palettes/
├── sprites/
├── rooms/
├── maps/
└── audio/
    ├── samples/
    ├── music/
    └── sfx/

src/
├── audio/
│   ├── audio.c
│   └── audio.h
└── debug/
    ├── overlay.c
    └── input_record.c

docs/
├── environment_setup.md
├── rom_memory_map.md
├── tileset_format.md
├── palette_format.md
├── sprite_format.md
├── room_format.md
├── architecture.md
├── building.md
├── asset_pipeline.md
└── README.md

bugs/
├── room_bugs.md
├── item_bugs.md
├── enemy_bugs.md
└── bug_summary.md
```

---

## Dependencies FROM Other Agents

| Dependency | From Agent | Blocking Task |
|------------|------------|---------------|
| Renderer working | Agent B | Task 4.1 |
| HUD system | Agent B | Task 4.2 |
| Input system | Agent B | Task 4.3 |
| Room loader | Agent B | Task 5.1 |
| Player systems | Agent A | Task 5.2 |
| Enemy AI | Agent A | Task 5.3 |
| Release build | Agent B | Task 7.1, 7.2, 7.4 |

## Dependencies TO Other Agents

| Your Output | To Agent | Their Blocking Task |
|-------------|----------|---------------------|
| Environment ready | Agent B | Project scaffold |
| ROM memory map | Agent A | Physics disassembly |
| Tileset format | Agent B | Background rendering |
| Sprite format | Agent B | Sprite rendering |
| Room format | Agent B | Room loader |
| Room format | Agent A | Collision system |
| Audio engine | Agent B | Game integration |
| Test utilities | Agent A | Physics verification |
| Bug reports | Agent A, B | Final fixes |

---

## Tools Reference

### devkitARM
```bash
# Compile
make

# Clean
make clean
```

### grit (Graphics)
```bash
# Convert PNG to DSi format
grit image.png -ftc -gB4 -p
```

### mmutil (Audio)
```bash
# Convert to maxmod soundbank
mmutil -o soundbank.bin *.wav
```

### Emulators
- **DeSmuME**: Best compatibility, good debugger
- **melonDS**: Most accurate, hardware-like

---

*Agent C: You're the foundation and the finish. Without you, nothing starts; without your testing, nothing ships.*
