# Agent A: Core Systems Engineer
## Model: Opus 4.5 | Token Budget: 300K-600K

---

## Role Summary

You are responsible for the most technically demanding work: translating SNES 65C816 assembly to ARM, implementing physics with frame-perfect accuracy, and building all gameplay logic including enemy AI and boss battles.

**Your work is the critical path.** Graphics and assets can be placeholder, but if physics feel wrong, the port fails.

---

## Day 1: Analysis & Disassembly

### Task 1.1: Physics Code Review
**Prerequisites**: None
**Deliverables**:
- [ ] Obtain and load Super Metroid ROM
- [ ] Identify physics routine entry points in ROM
- [ ] Document memory addresses for:
  - Player position (X, Y)
  - Player velocity (X, Y)
  - Gravity constant(s)
  - Jump force values
  - Speed caps
- [ ] Create `docs/physics_memory_map.md`

### Task 1.2: Begin Disassembly
**Prerequisites**: Task 1.1
**Deliverables**:
- [ ] Disassemble core movement loop
- [ ] Disassemble gravity application routine
- [ ] Disassemble horizontal acceleration/deceleration
- [ ] Document in `docs/physics_disassembly.md`
- [ ] Note any lookup tables used (slope data, etc.)

### Task 1.3: Coordinate with Agent C
**Prerequisites**: Agent C completes ROM extraction (1.2)
**Action**: Review Agent C's ROM analysis for physics-related data locations

---

## Day 2: Physics Engine Foundation

### Task 2.1: Gravity System
**Prerequisites**: Task 1.2
**Deliverables**:
- [ ] Implement `physics/gravity.c`
- [ ] Normal gravity (air)
- [ ] Water gravity (reduced)
- [ ] Lava/acid gravity (same as normal, damage separate)
- [ ] Space/zero-g zones
- [ ] Terminal velocity cap
- [ ] Unit tests comparing to original values

### Task 2.2: Horizontal Movement
**Prerequisites**: Task 1.2
**Deliverables**:
- [ ] Implement `physics/movement.c`
- [ ] Walking acceleration curve
- [ ] Running acceleration curve
- [ ] Deceleration (ground friction)
- [ ] Air control (reduced acceleration)
- [ ] Moonwalk (backward running)
- [ ] Speed cap enforcement

### Task 2.3: Collision Foundation
**Prerequisites**: Agent C completes level data format (2.3)
**Deliverables**:
- [ ] Implement `physics/collision.c`
- [ ] Tile-based collision detection
- [ ] Bounding box calculations
- [ ] Collision response (push-out)
- [ ] Slope detection (identify slope tiles)

---

## Day 3: Advanced Physics

### Task 3.1: Jump Mechanics
**Prerequisites**: Tasks 2.1, 2.2, 2.3
**Deliverables**:
- [ ] Implement `physics/jump.c`
- [ ] Variable jump height (hold duration)
- [ ] Spin jump (different arc/height)
- [ ] Wall jump detection and response
- [ ] Wall jump timing window (frames)
- [ ] Space jump (infinite spin jumps)
- [ ] Hi-jump boots modifier

### Task 3.2: Slope Physics
**Prerequisites**: Task 2.3
**Deliverables**:
- [ ] Slope angle detection
- [ ] Velocity adjustment on slopes
- [ ] Slope collision response
- [ ] Running up vs down slopes
- [ ] Crouch-slide on steep slopes

### Task 3.3: Morph Ball Physics
**Prerequisites**: Tasks 2.1, 2.2, 2.3
**Deliverables**:
- [ ] Implement `physics/morphball.c`
- [ ] Smaller hitbox
- [ ] Ball momentum preservation
- [ ] Spring ball jump
- [ ] Bomb jump physics
- [ ] IBJ (infinite bomb jump) must be possible

### Task 3.4: Speed Booster & Shinespark
**Prerequisites**: Task 2.2
**Deliverables**:
- [ ] Implement `physics/speedbooster.c`
- [ ] Speed booster charge threshold
- [ ] Blue suit state
- [ ] Shinespark charge and storage
- [ ] Shinespark directional launch
- [ ] Shinespark distance/damage
- [ ] Shinespark chaining (store mid-spark)

---

## Day 4: Player Systems

### Task 4.1: Samus State Machine
**Prerequisites**: All Day 3 physics tasks
**Deliverables**:
- [ ] Implement `player/state_machine.c`
- [ ] States: standing, walking, running, jumping, falling, morphball, crouching, aiming, damage, death
- [ ] State transition rules
- [ ] Animation state sync (coordinate with Agent B)
- [ ] Invincibility frames after damage

### Task 4.2: Weapon Systems
**Prerequisites**: Task 4.1
**Deliverables**:
- [ ] Implement `player/weapons.c`
- [ ] Power beam (base)
- [ ] Charge beam (hold and release)
- [ ] Ice beam (freeze enemies)
- [ ] Wave beam (pass through walls)
- [ ] Spazer (3-way spread)
- [ ] Plasma beam (pierce enemies)
- [ ] Beam stacking logic (ice+wave+plasma)
- [ ] Missiles (ammo tracking)
- [ ] Super missiles
- [ ] Power bombs (screen-wide)

### Task 4.3: Suit & Health Systems
**Prerequisites**: Task 4.1
**Deliverables**:
- [ ] Implement `player/health.c`
- [ ] Energy tank system (99 HP each)
- [ ] Reserve tanks
- [ ] Damage calculation by enemy/hazard
- [ ] Suit damage reduction (Varia: 50%, Gravity: 75%)
- [ ] Hazard immunity (Varia: heat, Gravity: water physics)

---

## Day 5: Enemy AI

### Task 5.1: Enemy Base System
**Prerequisites**: Agent B completes collision system (4.3)
**Deliverables**:
- [ ] Implement `enemies/enemy_base.c`
- [ ] Enemy state machine template
- [ ] Health and damage handling
- [ ] Death animation trigger
- [ ] Drop table (health, missiles, supers, etc.)
- [ ] Collision with player
- [ ] Collision with weapons

### Task 5.2: Standard Enemy AI
**Prerequisites**: Task 5.1
**Deliverables**:
- [ ] Implement common enemy patterns:
- [ ] Crawlers (Zoomers, Geemers)
- [ ] Flyers (Wavers, Rinkas)
- [ ] Jumpers (Sidehoppers, Dessgeegas)
- [ ] Shooters (Space Pirates, Zebesians)
- [ ] Floaters (Metroids, Covern)
- [ ] ~20-30 enemy types total

### Task 5.3: Mini-Boss AI
**Prerequisites**: Task 5.1
**Deliverables**:
- [ ] Spore Spawn (swinging, opening pattern)
- [ ] Crocomire (push-back mechanic)
- [ ] Botwoon (snake movement)
- [ ] Golden Torizo (aggressive variant)
- [ ] Bomb Torizo (intro boss)

---

## Day 5-6: Boss AI

### Task 5.4: Major Boss - Kraid
**Prerequisites**: Task 5.1
**Deliverables**:
- [ ] Implement `enemies/bosses/kraid.c`
- [ ] Rising entrance sequence
- [ ] Projectile patterns (fingernails, belly spikes)
- [ ] Mouth vulnerability window
- [ ] Roar and flinch states
- [ ] Death sequence

### Task 5.5: Major Boss - Phantoon
**Prerequisites**: Task 5.1
**Deliverables**:
- [ ] Implement `enemies/bosses/phantoon.c`
- [ ] Visibility states (visible, fading, invisible)
- [ ] Flame attack patterns (circle, wave)
- [ ] Rage mode (after super missile)
- [ ] Eye vulnerability

### Task 5.6: Major Boss - Draygon
**Prerequisites**: Task 5.1
**Deliverables**:
- [ ] Implement `enemies/bosses/draygon.c`
- [ ] Swooping attack pattern
- [ ] Grab attack (drain health)
- [ ] Gunk spit (slow player)
- [ ] Turret interaction (electric kill)

### Task 5.7: Major Boss - Ridley
**Prerequisites**: Task 5.1
**Deliverables**:
- [ ] Implement `enemies/bosses/ridley.c`
- [ ] Flight patterns
- [ ] Tail swipe
- [ ] Fireball attacks (single, spread)
- [ ] Grab and drag attack
- [ ] Pogo attack
- [ ] Health-based aggression scaling

### Task 5.8: Final Boss - Mother Brain
**Prerequisites**: Task 5.1, all other bosses complete
**Deliverables**:
- [ ] Implement `enemies/bosses/mother_brain.c`
- [ ] Phase 1: Brain in tank (turrets, rinkas)
- [ ] Tank destruction sequence
- [ ] Phase 2: Standing form
- [ ] Beam attacks (ring, laser)
- [ ] Bomb drop attack
- [ ] Hyper beam sequence (player gets hyper beam)
- [ ] Death and escape trigger

---

## Day 6: Physics Verification

### Task 6.1: Frame Comparison Tool
**Prerequisites**: Agent C provides test utilities
**Deliverables**:
- [ ] Input replay system
- [ ] Position logging per frame
- [ ] Velocity logging per frame
- [ ] Comparison script vs original recordings

### Task 6.2: Speedrun Technique Verification
**Prerequisites**: Task 6.1, all physics complete
**Deliverables**:
- [ ] Test: Mockball (morph while running, maintain speed)
- [ ] Test: Short charge (shinespark in short distance)
- [ ] Test: Wall jump (all surfaces that should work)
- [ ] Test: IBJ (infinite bomb jump)
- [ ] Test: Shinespark chains
- [ ] Test: Crystal flash (emergency heal)
- [ ] Test: Arm pumping (faster climbing)
- [ ] Document any deviations from original

### Task 6.3: Physics Bug Fixes
**Prerequisites**: Task 6.2
**Deliverables**:
- [ ] Fix any techniques that don't work
- [ ] Adjust values to match original behavior
- [ ] Re-verify after fixes

---

## Day 7: Final Polish

### Task 7.1: Final Physics Review
**Prerequisites**: Task 6.3
**Deliverables**:
- [ ] Full playthrough for physics feel
- [ ] Sign off on movement quality
- [ ] Document any known minor deviations

### Task 7.2: AI Bug Fixes
**Prerequisites**: Agent C's bug reports
**Deliverables**:
- [ ] Fix any reported enemy AI issues
- [ ] Fix any boss pattern bugs
- [ ] Final boss playtest

### Task 7.3: Handoff
**Prerequisites**: All above complete
**Deliverables**:
- [ ] All physics code documented
- [ ] All enemy AI documented
- [ ] Sign-off for integration

---

## File Structure

```
src/
├── physics/
│   ├── gravity.c
│   ├── movement.c
│   ├── collision.c
│   ├── jump.c
│   ├── morphball.c
│   ├── speedbooster.c
│   └── physics.h
├── player/
│   ├── state_machine.c
│   ├── weapons.c
│   ├── health.c
│   └── player.h
├── enemies/
│   ├── enemy_base.c
│   ├── enemy_base.h
│   ├── standard/
│   │   └── [enemy_type].c
│   └── bosses/
│       ├── kraid.c
│       ├── phantoon.c
│       ├── draygon.c
│       ├── ridley.c
│       └── mother_brain.c
docs/
├── physics_memory_map.md
├── physics_disassembly.md
└── enemy_ai_patterns.md
```

---

## Dependencies FROM Other Agents

| Dependency | From Agent | Blocking Task |
|------------|------------|---------------|
| ROM memory map | Agent C | Task 1.3 |
| Level collision format | Agent C | Task 2.3 |
| Sprite collision boxes | Agent B | Task 5.1 |
| Test utilities | Agent C | Task 6.1 |
| Bug reports | Agent C | Task 7.2 |

## Dependencies TO Other Agents

| Your Output | To Agent | Their Blocking Task |
|-------------|----------|---------------------|
| Physics system complete | Agent B | Room scrolling bounds |
| Player state machine | Agent B | Animation controller |
| Enemy base system | Agent B | Enemy sprite rendering |
| All physics complete | Agent B | Final integration |

---

*Agent A: You are the foundation. Take your time on physics—everything else can wait for you.*
