# Super Metroid → Nintendo DSi Port
## Project Plan & Team Structure

---

## Project Overview

**Goal**: Port Super Metroid (SNES) to Nintendo DSi homebrew
**Source**: SNES ROM (personal copy)
**Target**: DSi via devkitARM + libnds
**Display Strategy**: 256×224 buffer → top 192 rows on top screen, top 32 rows on bottom screen
**Estimated Duration**: 5-7 days with 3 AI coders working in parallel
**Estimated Tokens**: 600K-1.2M total across all agents

---

## Team Structure

### Agent A: Core Systems Engineer (Opus 4.5)
**Focus**: Assembly translation, physics, game logic
**Token Budget**: 300K-600K
**Why Opus**: Requires deep understanding of 65C816→ARM translation, behavioral fidelity

### Agent B: Graphics & Integration Lead (Sonnet)
**Focus**: Rendering pipeline, DSi hardware integration, memory management
**Token Budget**: 150K-300K
**Why Sonnet**: System architecture, moderate complexity, good reasoning

### Agent C: Asset Pipeline & Support (Haiku)
**Focus**: Data extraction, format conversion, documentation, testing utilities
**Token Budget**: 100K-200K
**Why Haiku**: Algorithmic tasks, high volume, cost-efficient

---

## Phase 1: Setup & Extraction (Day 1)

### 1.1 Environment Setup
**Owner**: Agent C
**Dependencies**: None
**Deliverables**:
- [ ] devkitARM toolchain installed and configured
- [ ] libnds project template created
- [ ] Build system verified (compiles empty DSi app)
- [ ] Emulator testing environment (DeSmuME/melonDS)

### 1.2 ROM Analysis & Data Extraction
**Owner**: Agent C
**Dependencies**: 1.1
**Deliverables**:
- [ ] ROM header analysis and memory map documentation
- [ ] Sprite/tile data locations identified
- [ ] Palette data extracted
- [ ] Level/room data structure documented
- [ ] Audio sample and sequence data located
- [ ] Physics constants and tables identified

### 1.3 Architecture Design
**Owner**: Agent B
**Dependencies**: 1.2
**Deliverables**:
- [ ] System architecture document
- [ ] Memory layout plan for DSi (16MB allocation strategy)
- [ ] Module dependency graph
- [ ] Header files for all major systems (stubs)

---

## Phase 2: Asset Conversion (Days 1-2)

### 2.1 Graphics Conversion
**Owner**: Agent C
**Dependencies**: 1.2
**Deliverables**:
- [ ] Tile format converter (SNES 4bpp → DSi format)
- [ ] Palette converter (SNES 15-bit → DSi 15-bit)
- [ ] Sprite sheet extractor and reorganizer
- [ ] Background/tilemap converter
- [ ] All Samus animation frames converted
- [ ] All enemy sprites converted
- [ ] All environment tilesets converted

### 2.2 Audio Conversion
**Owner**: Agent C
**Dependencies**: 1.2
**Deliverables**:
- [ ] SPC700 sample extraction tool
- [ ] Audio samples converted to DSi-compatible format
- [ ] Music sequence data parsed
- [ ] Sound effect catalog created
- [ ] Basic audio playback test

### 2.3 Level Data Conversion
**Owner**: Agent C
**Dependencies**: 1.2
**Deliverables**:
- [ ] Room structure parser
- [ ] Collision map extractor
- [ ] Door/transition data converter
- [ ] Item placement data converter
- [ ] Enemy spawn data converter
- [ ] All 6 areas converted (Crateria, Brinstar, Norfair, Wrecked Ship, Maridia, Tourian)

---

## Phase 3: Core Engine Development (Days 2-4)

### 3.1 Rendering Engine
**Owner**: Agent B
**Dependencies**: 2.1, 1.3
**Deliverables**:
- [ ] Dual-screen framebuffer system (256×224 → split display)
- [ ] Background layer renderer (4 layers)
- [ ] Sprite renderer with priority sorting
- [ ] Palette animation system
- [ ] Screen transition effects (doors, elevators)
- [ ] HUD rendering system
- [ ] 60 FPS vsync loop

### 3.2 Physics Engine (CRITICAL PATH)
**Owner**: Agent A
**Dependencies**: 1.2
**Deliverables**:
- [ ] 65C816 physics code fully disassembled and documented
- [ ] Gravity system (normal, water, lava, space)
- [ ] Horizontal movement (run, walk, moonwalk)
- [ ] Jump mechanics (variable height, spin jump, wall jump)
- [ ] Collision detection (tiles, slopes, one-way platforms)
- [ ] Momentum preservation system
- [ ] Morph ball physics
- [ ] Shinespark/speed booster physics
- [ ] All values matched to original frame-perfect behavior

### 3.3 Game State & Memory
**Owner**: Agent B
**Dependencies**: 1.3
**Deliverables**:
- [ ] Save/load system (DSi NAND or SD card)
- [ ] Game state manager (title, gameplay, pause, map, death)
- [ ] Room state tracking (visited, cleared, items collected)
- [ ] Equipment/inventory system
- [ ] SRAM format compatibility (optional)

### 3.4 Input System
**Owner**: Agent B
**Dependencies**: 1.3
**Deliverables**:
- [ ] Button mapping (SNES → DSi)
- [ ] Input buffering for advanced techniques
- [ ] Touch screen support (optional map/inventory)
- [ ] Input replay system (for testing)

---

## Phase 4: Gameplay Systems (Days 3-5)

### 4.1 Player Systems
**Owner**: Agent A
**Dependencies**: 3.2
**Deliverables**:
- [ ] Samus state machine (standing, running, jumping, morphball, etc.)
- [ ] Animation controller
- [ ] Weapon system (beam types, missiles, super missiles, power bombs)
- [ ] Beam stacking/combination logic
- [ ] Suit damage reduction
- [ ] Health/ammo management
- [ ] Death and respawn

### 4.2 Enemy AI
**Owner**: Agent A
**Dependencies**: 3.2, 2.3
**Deliverables**:
- [ ] Enemy base class with common behaviors
- [ ] All standard enemy AI patterns converted
- [ ] Boss AI: Ridley
- [ ] Boss AI: Kraid
- [ ] Boss AI: Phantoon
- [ ] Boss AI: Draygon
- [ ] Boss AI: Mother Brain (all phases)
- [ ] Boss AI: Crocomire, Spore Spawn, Botwoon
- [ ] Mini-boss patterns (Torizo, etc.)

### 4.3 World Systems
**Owner**: Agent B
**Dependencies**: 2.3, 3.1
**Deliverables**:
- [ ] Room loader and renderer
- [ ] Door transition system
- [ ] Elevator system
- [ ] Scrolling camera with bounds
- [ ] Environmental hazards (lava, acid, spikes)
- [ ] Breakable blocks (all types)
- [ ] Item collection and powerup grants
- [ ] Map reveal system

### 4.4 Audio Engine
**Owner**: Agent C (with Agent B oversight)
**Dependencies**: 2.2
**Deliverables**:
- [ ] Music playback system (streaming or sequenced)
- [ ] Sound effect system with priority
- [ ] Area-based music switching
- [ ] Boss music triggers
- [ ] Environmental audio (rain, lava bubbles)

---

## Phase 5: Integration & Testing (Days 5-6)

### 5.1 System Integration
**Owner**: Agent B (coordinator)
**Dependencies**: All Phase 4 tasks
**Deliverables**:
- [ ] All systems integrated into main game loop
- [ ] Title screen and menu functional
- [ ] Full game playable start to finish
- [ ] Pause menu and map screen
- [ ] Ending sequences

### 5.2 Physics Verification
**Owner**: Agent A
**Dependencies**: 5.1
**Deliverables**:
- [ ] Frame-by-frame comparison tool
- [ ] Jump height verification (all jump types)
- [ ] Speed values verification
- [ ] Shinespark distance/timing verification
- [ ] Mockball technique working
- [ ] Wall jump timing verified
- [ ] Known speedrun techniques tested

### 5.3 Compatibility Testing
**Owner**: Agent C
**Dependencies**: 5.1
**Deliverables**:
- [ ] All rooms load correctly
- [ ] All enemies spawn and behave correctly
- [ ] All items collectible
- [ ] All bosses defeatable
- [ ] All doors function
- [ ] Save/load works correctly
- [ ] No memory leaks over extended play

### 5.4 Performance Testing
**Owner**: Agent B
**Dependencies**: 5.1
**Deliverables**:
- [ ] Stable 60 FPS in all rooms
- [ ] No audio glitches
- [ ] Memory usage within bounds
- [ ] CPU usage profiled and optimized

---

## Phase 6: Polish & Finalization (Day 7)

### 6.1 Bug Fixes
**Owner**: All agents
**Dependencies**: 5.2, 5.3, 5.4
**Deliverables**:
- [ ] All critical bugs fixed
- [ ] All major bugs fixed
- [ ] Known issues documented

### 6.2 Final Build
**Owner**: Agent B
**Dependencies**: 6.1
**Deliverables**:
- [ ] Release build compiled
- [ ] ROM size optimized
- [ ] Final testing on hardware (if available)
- [ ] .nds file ready for distribution

### 6.3 Documentation
**Owner**: Agent C
**Dependencies**: 6.2
**Deliverables**:
- [ ] README with installation instructions
- [ ] Controls reference
- [ ] Known issues list
- [ ] Build instructions for future development

---

## Dependency Graph

```
Phase 1: Setup
    1.1 Environment ──┬──> 1.2 ROM Analysis ──> 1.3 Architecture
                      │
Phase 2: Assets       ▼
    ┌─────────────────┴─────────────────┐
    │                                   │
    2.1 Graphics    2.2 Audio    2.3 Levels
    │               │            │
Phase 3: Engine     ▼            ▼
    │    ┌──────────┴────────────┘
    ▼    ▼
    3.1 Rendering ◄──── 3.2 Physics (CRITICAL)
    │                   │
    3.3 Game State      │
    │                   │
    3.4 Input           │
    │                   │
Phase 4: Gameplay       ▼
    │    ┌──────────────┴──────────────┐
    ▼    ▼                             ▼
    4.3 World    4.1 Player    4.2 Enemies
    │            │             │
    4.4 Audio    │             │
    │            │             │
Phase 5: Testing ▼             ▼
    └────────────┴─────────────┘
                 │
                 ▼
         5.1 Integration
         │
    ┌────┼────┐
    ▼    ▼    ▼
   5.2  5.3  5.4
    │    │    │
    └────┴────┘
         │
Phase 6: Polish
         ▼
        6.1 Bug Fixes
         │
         ▼
        6.2 Final Build
         │
         ▼
        6.3 Documentation
```

---

## Parallel Work Streams

### Day 1
| Agent A (Opus) | Agent B (Sonnet) | Agent C (Haiku) |
|----------------|------------------|-----------------|
| Review physics code | Architecture design | Environment setup |
| Begin disassembly | Memory layout | ROM extraction |
| Document physics | System headers | Asset location |

### Day 2
| Agent A (Opus) | Agent B (Sonnet) | Agent C (Haiku) |
|----------------|------------------|-----------------|
| Physics engine | Rendering engine | Graphics conversion |
| Gravity/movement | Framebuffer setup | Audio conversion |
| Collision system | Sprite renderer | Level data parsing |

### Day 3
| Agent A (Opus) | Agent B (Sonnet) | Agent C (Haiku) |
|----------------|------------------|-----------------|
| Jump mechanics | Background layers | Finish asset conversion |
| Advanced physics | Screen transitions | Audio engine basics |
| Morph ball | HUD system | Testing utilities |

### Day 4
| Agent A (Opus) | Agent B (Sonnet) | Agent C (Haiku) |
|----------------|------------------|-----------------|
| Player state machine | Room loader | Audio engine polish |
| Weapon systems | Door transitions | Asset verification |
| Begin enemy AI | Camera system | Support tasks |

### Day 5
| Agent A (Opus) | Agent B (Sonnet) | Agent C (Haiku) |
|----------------|------------------|-----------------|
| Enemy AI (bosses) | World systems | Compatibility testing |
| Boss patterns | Integration start | Room-by-room tests |
| AI refinement | Save/load | Bug documentation |

### Day 6
| Agent A (Opus) | Agent B (Sonnet) | Agent C (Haiku) |
|----------------|------------------|-----------------|
| Physics verification | Full integration | Continued testing |
| Speedrun tech tests | Performance tuning | Bug reproduction |
| Fix physics bugs | Optimization | Test automation |

### Day 7
| Agent A (Opus) | Agent B (Sonnet) | Agent C (Haiku) |
|----------------|------------------|-----------------|
| Final physics fixes | Final build | Documentation |
| Review/sign-off | Release prep | README/controls |
| Done | Done | Done |

---

## Risk Factors

### High Risk
- **Physics fidelity**: If jump/momentum feels wrong, the game feels wrong
  - Mitigation: Extensive frame comparison, speedrunner technique testing

- **Boss AI complexity**: Mother Brain alone has 3+ phases with unique logic
  - Mitigation: Prioritize bosses early, allow extra time

### Medium Risk
- **Audio timing**: Music synced to gameplay events
  - Mitigation: Use reference recordings, frame-accurate triggering

- **Memory constraints**: DSi has 16MB but sloppy code adds up
  - Mitigation: Regular memory profiling, streaming for large assets

### Low Risk
- **Graphics conversion**: Well-understood process
- **Input handling**: Simple button mapping
- **Save system**: Standard file I/O

---

## Success Criteria

### Minimum Viable Product
- [ ] Playable from Ceres Station to Mother Brain
- [ ] All items collectible
- [ ] All bosses defeatable
- [ ] Runs at stable 60 FPS
- [ ] Save/load functional

### Full Release
- [ ] All MVP criteria met
- [ ] Physics match original (wall jump, shinespark, mockball work)
- [ ] All audio plays correctly
- [ ] No major bugs
- [ ] Clean code and documentation

---

## Token Budget Summary

| Agent | Role | Token Budget | Model |
|-------|------|--------------|-------|
| A | Core Systems | 300K-600K | Opus 4.5 |
| B | Graphics/Integration | 150K-300K | Sonnet |
| C | Assets/Support | 100K-200K | Haiku |
| **Total** | | **550K-1.1M** | |

---

## Getting Started

1. **Human**: Provide ROM file and confirm legal understanding
2. **Agent C**: Run environment setup (Phase 1.1)
3. **Agent C**: Begin ROM extraction while Agent B designs architecture
4. **Agent A**: Start physics analysis once ROM map is available
5. **All agents**: Follow parallel work streams above

---

*Plan generated for Super Metroid DSi Port Project*
*Estimated completion: 5-7 days with 3 AI coders*
