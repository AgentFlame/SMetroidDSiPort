# Agent B: Graphics & Integration Lead
## Model: Sonnet | Token Budget: 150K-300K

---

## Role Summary

You are responsible for all rendering systems, DSi hardware integration, and final project integration. You bridge the gap between Agent A's gameplay logic and Agent C's converted assets.

You also serve as the **integration coordinator** in the final phase, bringing all systems together.

---

## Day 1: Architecture & Setup

### Task 1.1: System Architecture Design
**Prerequisites**: None
**Deliverables**:
- [ ] Create `docs/architecture.md`
- [ ] Define module boundaries and interfaces
- [ ] Memory allocation strategy (16MB budget)
  - Player/physics: ~1MB
  - Enemies: ~2MB
  - Graphics (loaded): ~4MB
  - Audio (loaded): ~2MB
  - Level data (current room): ~1MB
  - Buffers/stack: ~2MB
  - Reserve: ~4MB
- [ ] Define header file structure

### Task 1.2: Project Scaffold
**Prerequisites**: Agent C completes environment setup (1.1)
**Deliverables**:
- [ ] Create project directory structure
- [ ] Set up Makefile for devkitARM
- [ ] Create main.c entry point
- [ ] Verify "hello world" compiles and runs in emulator
- [ ] Set up build configurations (debug, release)

### Task 1.3: Header File Stubs
**Prerequisites**: Task 1.1
**Deliverables**:
- [ ] Create all major header files with function signatures:
  - `include/graphics.h`
  - `include/audio.h`
  - `include/input.h`
  - `include/game_state.h`
  - `include/physics.h` (for Agent A)
  - `include/player.h` (for Agent A)
  - `include/enemies.h` (for Agent A)
  - `include/room.h`
  - `include/save.h`

---

## Day 2: Rendering Foundation

### Task 2.1: Dual-Screen Framebuffer
**Prerequisites**: Task 1.2
**Deliverables**:
- [ ] Implement `graphics/framebuffer.c`
- [ ] Create 256×224 render target in RAM
- [ ] Blit rows 32-223 to top screen (192 pixels)
- [ ] Blit rows 0-31 to bottom screen (32 pixels)
- [ ] VSync at 60 FPS
- [ ] Test with solid color fills

### Task 2.2: Background Layer System
**Prerequisites**: Task 2.1, Agent C provides tileset format (2.1)
**Deliverables**:
- [ ] Implement `graphics/background.c`
- [ ] 4 background layers (as per SNES)
- [ ] Layer priorities
- [ ] Tilemap rendering from RAM
- [ ] Tile flipping (H/V)
- [ ] Per-tile palette selection

### Task 2.3: Palette System
**Prerequisites**: Task 2.1, Agent C provides palettes (2.1)
**Deliverables**:
- [ ] Implement `graphics/palette.c`
- [ ] Load palettes from converted data
- [ ] Palette animation support (lava glow, water shimmer)
- [ ] Palette swaps (damage flash, suit colors)
- [ ] 256 color mode setup

---

## Day 3: Sprite & Effects

### Task 3.1: Sprite Renderer
**Prerequisites**: Task 2.1, Agent C provides sprite format (2.1)
**Deliverables**:
- [ ] Implement `graphics/sprites.c`
- [ ] Sprite allocation (128 hardware sprites on DSi)
- [ ] Priority sorting (player > enemies > items > effects)
- [ ] Sprite flipping (H/V)
- [ ] Large sprite assembly (Samus is multi-sprite)
- [ ] OAM management

### Task 3.2: Screen Effects
**Prerequisites**: Task 2.1
**Deliverables**:
- [ ] Implement `graphics/effects.c`
- [ ] Screen fade (in/out, color)
- [ ] Screen flash (damage, explosion)
- [ ] Palette cycling (animated backgrounds)
- [ ] Heat wave effect (Norfair)
- [ ] Rain effect (Crateria surface)
- [ ] Mode 7 style effects (if needed for intro)

### Task 3.3: HUD Rendering
**Prerequisites**: Task 2.1
**Deliverables**:
- [ ] Implement `graphics/hud.c`
- [ ] Energy display (current/max)
- [ ] Missile count
- [ ] Super missile count
- [ ] Power bomb count
- [ ] Mini-map (if in top 32 pixels)
- [ ] Reserve tank indicator
- [ ] HUD positioning for split-screen layout

### Task 3.4: Screen Transitions
**Prerequisites**: Task 2.1
**Deliverables**:
- [ ] Implement `graphics/transitions.c`
- [ ] Door transition (fade, scroll, fade in)
- [ ] Elevator transition (long scroll)
- [ ] Death transition (explosion, fade)
- [ ] Save room glow effect

---

## Day 4: World Systems

### Task 4.1: Room Loader
**Prerequisites**: Agent C provides room format (2.3)
**Deliverables**:
- [ ] Implement `world/room.c`
- [ ] Load room tilemap from converted data
- [ ] Load room collision data
- [ ] Load room enemy spawns
- [ ] Load room item placements
- [ ] Load room scroll boundaries
- [ ] Unload previous room to free memory

### Task 4.2: Camera System
**Prerequisites**: Task 4.1, Agent A provides player position
**Deliverables**:
- [ ] Implement `world/camera.c`
- [ ] Follow player with smoothing
- [ ] Respect room scroll boundaries
- [ ] Scroll lock regions
- [ ] Boss room lock-in
- [ ] Camera shake (explosions)

### Task 4.3: Door System
**Prerequisites**: Task 4.1
**Deliverables**:
- [ ] Implement `world/doors.c`
- [ ] Door types (blue, red, green, yellow, gray)
- [ ] Door opening animation
- [ ] Door lock/unlock state
- [ ] Transition trigger zones
- [ ] Door scroll direction

### Task 4.4: Environmental Interactions
**Prerequisites**: Task 4.1, Agent A provides collision
**Deliverables**:
- [ ] Implement `world/environment.c`
- [ ] Breakable blocks (shot, bomb, power bomb, speed)
- [ ] Crumble blocks (timer after step)
- [ ] Conveyor blocks
- [ ] Spike damage
- [ ] Lava/acid damage (per frame in contact)
- [ ] Water entry/exit

---

## Day 4: Input & State

### Task 4.5: Input System
**Prerequisites**: Task 1.2
**Deliverables**:
- [ ] Implement `input/input.c`
- [ ] Button mapping:
  - D-pad → D-pad
  - A → Jump
  - B → Shoot
  - X → Dash (or toggle)
  - Y → Cancel/back
  - L → Aim up
  - R → Aim down
  - Start → Pause
  - Select → Item select
- [ ] Input buffering (3-5 frames for techniques)
- [ ] Input state (pressed, held, released)

### Task 4.6: Game State Manager
**Prerequisites**: Task 1.2
**Deliverables**:
- [ ] Implement `game/state_manager.c`
- [ ] States: boot, title, file_select, gameplay, pause, map, gameover, ending
- [ ] State transitions
- [ ] Per-state update and render calls
- [ ] Pause freeze (gameplay pauses, not music)

### Task 4.7: Save System
**Prerequisites**: Task 1.2
**Deliverables**:
- [ ] Implement `game/save.c`
- [ ] Save data structure:
  - Current room
  - Player position
  - Collected items bitmask
  - Defeated bosses bitmask
  - Energy/ammo counts
  - Playtime
  - Map reveal data
- [ ] Save to SD card or NAND
- [ ] 3 save slots
- [ ] Load validation (checksum)

---

## Day 5: Integration Begin

### Task 5.1: Player-Graphics Integration
**Prerequisites**: Agent A provides player state machine (4.1)
**Deliverables**:
- [ ] Implement `player/animation.c`
- [ ] Animation controller driven by player state
- [ ] Animation frame timing
- [ ] Animation blending (if needed)
- [ ] Weapon firing animations
- [ ] Damage/death animations

### Task 5.2: Enemy-Graphics Integration
**Prerequisites**: Agent A provides enemy base (5.1)
**Deliverables**:
- [ ] Enemy sprite rendering
- [ ] Enemy animation by state
- [ ] Death explosion effects
- [ ] Boss sprite assembly (Kraid is huge)
- [ ] Boss health bars (if adding)

### Task 5.3: Pause Screen & Map
**Prerequisites**: Task 4.6, Agent C provides map data
**Deliverables**:
- [ ] Implement `ui/pause.c`
- [ ] Equipment display
- [ ] Samus suit viewer
- [ ] Implement `ui/map.c`
- [ ] Area map with fog of war
- [ ] Room-by-room reveal
- [ ] Item dots
- [ ] Navigation between areas

---

## Day 5-6: Full Integration

### Task 5.4: Title Screen & Menus
**Prerequisites**: Task 4.6, Agent C provides assets
**Deliverables**:
- [ ] Title screen graphics and animation
- [ ] "Press Start" prompt
- [ ] File select screen (3 files + copy/delete)
- [ ] Demo playback (optional)

### Task 5.5: Main Game Loop Integration
**Prerequisites**: All Agent A physics complete, Tasks 4.1-4.7 complete
**Deliverables**:
- [ ] Implement complete gameplay loop:
  1. Read input
  2. Update player physics (Agent A)
  3. Update enemies (Agent A)
  4. Update world (doors, items)
  5. Update camera
  6. Render backgrounds
  7. Render sprites
  8. Render HUD
  9. Flip buffers
- [ ] Verify 60 FPS

### Task 5.6: Ending Sequences
**Prerequisites**: Agent A provides escape trigger
**Deliverables**:
- [ ] Escape timer display
- [ ] Escape sequence rooms
- [ ] Ship escape cutscene
- [ ] Credits roll
- [ ] Ending variations (based on time/items)
- [ ] THE END screen

---

## Day 6: Performance & Polish

### Task 6.1: Performance Profiling
**Prerequisites**: Task 5.5 complete
**Deliverables**:
- [ ] Frame time breakdown
- [ ] Identify any >16ms frames
- [ ] Memory usage audit
- [ ] Sprite count audit (max per room)

### Task 6.2: Performance Optimization
**Prerequisites**: Task 6.1
**Deliverables**:
- [ ] Optimize render loops
- [ ] Reduce overdraw
- [ ] Pool allocators for particles/effects
- [ ] Sprite culling (off-camera)
- [ ] Asset streaming if needed

### Task 6.3: Visual Polish
**Prerequisites**: Task 5.5 complete
**Deliverables**:
- [ ] Screen transition timing feels good
- [ ] HUD is readable
- [ ] No visual glitches
- [ ] All effects render correctly

---

## Day 7: Final Build

### Task 7.1: Integration Bug Fixes
**Prerequisites**: Agent C's bug reports, Agent A's sign-off
**Deliverables**:
- [ ] Fix any integration issues
- [ ] Fix any rendering bugs
- [ ] Fix any state transition bugs

### Task 7.2: Release Build
**Prerequisites**: Task 7.1
**Deliverables**:
- [ ] Compile release build (optimizations on)
- [ ] Strip debug symbols
- [ ] Verify .nds file size
- [ ] Test on emulator (DeSmuME, melonDS)
- [ ] Test on hardware if available

### Task 7.3: Final Verification
**Prerequisites**: Task 7.2
**Deliverables**:
- [ ] Full playthrough on release build
- [ ] All areas accessible
- [ ] All bosses beatable
- [ ] Ending plays
- [ ] No crashes

---

## File Structure

```
src/
├── main.c
├── graphics/
│   ├── framebuffer.c
│   ├── background.c
│   ├── palette.c
│   ├── sprites.c
│   ├── effects.c
│   ├── hud.c
│   ├── transitions.c
│   └── graphics.h
├── world/
│   ├── room.c
│   ├── camera.c
│   ├── doors.c
│   ├── environment.c
│   └── world.h
├── input/
│   ├── input.c
│   └── input.h
├── game/
│   ├── state_manager.c
│   ├── save.c
│   └── game.h
├── ui/
│   ├── pause.c
│   ├── map.c
│   └── ui.h
├── player/
│   └── animation.c
include/
├── graphics.h
├── audio.h
├── input.h
├── game_state.h
├── physics.h
├── player.h
├── enemies.h
├── room.h
└── save.h
docs/
└── architecture.md
```

---

## Dependencies FROM Other Agents

| Dependency | From Agent | Blocking Task |
|------------|------------|---------------|
| Environment setup | Agent C | Task 1.2 |
| Tileset format | Agent C | Task 2.2 |
| Palette data | Agent C | Task 2.3 |
| Sprite format | Agent C | Task 3.1 |
| Room data format | Agent C | Task 4.1 |
| Map data | Agent C | Task 5.3 |
| Title screen assets | Agent C | Task 5.4 |
| Player state machine | Agent A | Task 5.1 |
| Enemy base system | Agent A | Task 5.2 |
| All physics complete | Agent A | Task 5.5 |
| Bug reports | Agent C | Task 7.1 |
| Physics sign-off | Agent A | Task 7.1 |

## Dependencies TO Other Agents

| Your Output | To Agent | Their Blocking Task |
|-------------|----------|---------------------|
| Project scaffold | Agent C | Asset integration |
| Collision system | Agent A | Enemy AI |
| Header stubs | Agent A | Physics implementation |
| Animation controller | Agent A | Player state sync |
| Integration complete | Agent C | Final testing |

---

*Agent B: You're the glue. Keep Agent A unblocked, coordinate with Agent C, and own the final build.*
