# M0: Project Scaffolding - Instruction File

## Purpose

Create the foundational header files, type definitions, and project structure that all subsequent milestones depend on. This milestone produces no runtime code beyond boot verification -- it establishes the type system, constants, and module API contracts.

## What Was Created

### Core Headers (`include/`)

| File | Contents |
|------|----------|
| `sm_types.h` | fx32, Vec2fx, AABBfx, Direction, GameStateID, pool constants, collision types, equipment bitfield |
| `sm_config.h` | VRAM bank assignments, BG layer assignments, video modes, room limits, input buffer config |
| `sm_physics_constants.h` | All SNES NTSC physics constants as fx32 (gravity, terminal velocity, jump velocities, speeds, hitboxes) |

### Module Stub Headers (`include/`)

Each module has a header with include guards, type definitions, and function declarations:

- `fixed_math.h` -- 16.16 fixed-point math library (M1)
- `input.h` -- Input system with buffering (M5)
- `graphics.h` -- Hardware rendering foundation (M4)
- `room.h` -- Room loading + collision map (M7)
- `physics.h` -- Physics engine (M8)
- `player.h` -- Player state machine (M9)
- `enemy.h` -- Enemy pool + AI (M11)
- `projectile.h` -- Weapon projectile pool (M12)
- `camera.h` -- Camera system (M10)
- `audio.h` -- Audio system / Maxmod (M14)
- `save.h` -- Save system / FAT (M15)
- `state.h` -- Game state manager (M6)
- `hud.h` -- HUD display (M16)

### Instruction Files (`docs/instructions/`)

Self-contained markdown files for sub-agents implementing each milestone.

### Build Verification

- Clean build with zero warnings
- ROM boots and displays scaffold info + fixed-point verification
- All headers included from main.c

## Key Design Decisions

1. **fx32 = int32_t with 16.16 format** -- matches SNES subpixel system exactly
2. **fx_mul/fx_div are inline in sm_types.h** -- available everywhere, no link dependency
3. **Pool sizes are #defines in sm_types.h** -- every module sees the same limits
4. **VRAM bank assignments are in sm_config.h** -- single source of truth for graphics
5. **Physics constants are exact hex values** -- no float conversion, directly from SNES disassembly
6. **SCREEN_WIDTH/SCREEN_HEIGHT not redefined** -- libnds already defines them

## Verification

```
make clean && make
```

Expected output: zero warnings, `SuperMetroidDS.nds` produced, boots to scaffold screen.
