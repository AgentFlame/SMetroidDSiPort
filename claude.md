# Super Metroid DS Port - Rules for AI Agents and Developers

## Project Overview

Native port of Super Metroid (SNES, 1994) to Nintendo DS in NTR mode. This is NOT an emulator -- it is a ground-up reimplementation using hardware tile/sprite rendering, integer math, and static memory pools. Target: 60 fps on original DS hardware (67 MHz ARM9, 4 MB RAM).

## Build Command (CRITICAL)

You MUST build through devkitPro's own MSYS2 shell. No other shell works on Windows.

```bash
/c/devkitPro/msys2/usr/bin/bash.exe -lc 'cd /home/Infernal/Documents/Code/SMetroidDSiPort && make 2>&1'
```

Clean build:
```bash
/c/devkitPro/msys2/usr/bin/bash.exe -lc 'cd /home/Infernal/Documents/Code/SMetroidDSiPort && make clean && make 2>&1'
```

**Why:** The `make` binary's internal `$(MAKE)` variable resolves to `/opt/devkitpro/msys2/usr/bin/make`, which only exists via an fstab mount inside devkitPro's MSYS2. Setting env vars in Git Bash / PowerShell / CMD does not fix this.

**Path mapping inside devkitPro MSYS2:**
- `C:\Users\` -> `/home/`
- `C:\devkitPro\` -> `/opt/devkitpro/`

## Makefile Rules

- The Makefile is based on the official ARM9 template (`$DEVKITPRO/examples/nds/templates/arm9/`).
- **Only edit the top section** (above `ifneq ($(BUILD),$(notdir $(CURDIR)))`). Everything below is build machinery driven by `ds_rules` -> `base_rules` -> `base_tools`.
- New source files in `source/` are auto-discovered. No Makefile edit needed.
- Library link order matters: dependents first. `-lcalico_ds9` is appended automatically -- never add it manually.
- `-specs=ds_arm9.specs` in LDFLAGS is a symbolic name. `ds_rules` substitutes it with `$(CALICO)/share/ds9.specs` at link time. Do not rename it.

## Code Standards

- **Language:** Pure C. Smaller binary, no RTTI/exception overhead.
- **Main loop:** `while (pmMainLoop())` -- never `while(1)`. Required for calico power management.
- **Frame sync:** `swiWaitForVBlank()` once per frame.
- **Input:** `scanKeys()` then `keysDown()`/`keysHeld()`/`keysUp()` -- exactly once per frame, in that order.
- **Console output:** `iprintf()` for on-screen text (not `printf` -- avoids ~8KB float bloat). `fprintf(stderr, ...)` for emulator debug console.
- **Exception handler:** Call `defaultExceptionHandler()` at the top of `main()`.
- **Target mode:** NTR (original DS mode) for maximum compatibility. No TWL/DSi-enhanced features.
- **Math:** 16.16 fixed-point integer arithmetic. No floating-point (ARM9 has no FPU).
- **Memory:** All allocation at startup. No malloc/free during gameplay. Static pools with swap-remove.
- **Rendering:** Hardware tile layers + OAM sprites exclusively. No software framebuffer.

## AI Code Generation Constraints (HARD RULES)

These prevent AI drift on embedded targets. Violating any of these will cause performance failures or crashes on hardware.

1. **No dynamic allocation after init.** All pools, buffers, and arrays allocated at startup. No `malloc`/`free`/`new`/`delete` during gameplay.
2. **No STL containers.** No `vector`, `map`, `set`, `list`. Use fixed-size C arrays with active-count tracking.
3. **No recursion.** DTCM stack is ~16 KB total. Overflow is silent and corrupts adjacent data.
4. **No per-frame memory growth.** Active entity counts may change; total allocated memory must not.
5. **No floating-point math.** ARM9 has no FPU. Soft-float is 10-100x slower. Use integer or 16.16 fixed-point. Document any exception with rationale.
6. **Tile-based collision only.** Convert to tile coordinates, check overlapping tiles. No per-pixel collision. No full-map iteration.
7. **No software framebuffer.** Use hardware BG layers and OAM sprites. Never memcpy a pixel buffer to VRAM per frame.
8. **All structs POD-style.** No virtual functions, no inheritance, no RTTI, no exceptions. C structs only.
9. **Fixed-size arrays preferred.** Static allocation with compile-time size constants. Track active counts separately.
10. **No per-frame heap usage.** Even "temporary" allocations introduce fragmentation risk and unpredictable stalls.

## Architecture Quick Reference

```
+---------------------------------------------------+
|              Your Application (source/)            |
+---------------------------------------------------+
|                    libnds (-lnds9)                 |
|  Console, 2D graphics, sprites, backgrounds,      |
|  touchscreen, sound, FAT filesystem, NitroFS       |
+---------------------------------------------------+
|                    calico (-lcalico_ds9)           |
|  crt0, ARM7 firmware, IRQ, threading, PM,          |
|  inter-processor communication (PXI), BIOS calls   |
+---------------------------------------------------+
|                    Hardware                         |
|  ARM946E-S (67MHz), ARM7TDMI (33MHz), VRAM, DMA   |
+---------------------------------------------------+
```

**Link chain:** `your .o files -> -lnds9 -> -lcalico_ds9 (auto-appended) -> hardware`

**NTR Hardware:**
- 4 MB main RAM (`0x2000000` - `0x23FFFFF`)
- 656 KB VRAM (banks A-I)
- 32 KB ITCM + 16 KB DTCM
- Two 256x192 screens (main + sub), one with touch
- ARM9 at 67 MHz, ARM7 at 33 MHz
- ~1.1M cycles/frame at 60 fps

**Dual-Screen Strategy:**
- Main engine (top screen): 192px gameplay area with BG layers + OAM sprites
- Sub engine (bottom screen): 32px HUD strip + extra space for map/touch UI
- Each engine has independent BG layers, OAM (128 sprites), and VRAM banks
- Full 256x224 SNES resolution covered across both screens using pure hardware rendering

## Game-Specific Constants

Physics use 16.16 fixed-point matching exact SNES values. See `docs/physics_data.md` for full list.

```c
#define GRAVITY_AIR     0x125C   // 0.07168 px/frame^2
#define GRAVITY_WATER   0x053F   // 0.02048 px/frame^2
#define TERMINAL_VEL    0x50000  // 5.02 px/frame
#define WALK_SPEED      0x18000  // 1.5 px/frame
#define RUN_SPEED       0x20000  // 2.0 px/frame
#define JUMP_INITIAL    0x49000  // 4.57 px/frame
```

See also: `docs/enemy_data.md` (AI, boss HP), `docs/level_data.md` (rooms, collision), `docs/graphics_data.md` (tile formats), `docs/audio_data.md` (music, SFX).

## Toolchain

- devkitARM r67 (GCC 15.2.0), libnds 2.0.0, calico, ndstool 2.3.1
- Compiler prefix: `arm-none-eabi-`
- Architecture flags: `-march=armv5te -mtune=arm946e-s` (ARM946E-S = DS ARM9 CPU)
- Debug: `-g` always on. Symbols in `.elf`, not `.nds`. Use `arm-none-eabi-addr2line -e SuperMetroidDS.elf <addr>` for crash diagnosis.

## Known Pitfalls

### Build System
1. **MUST use devkitPro's MSYS2 shell** -- fstab mounts are required. No other shell works.
2. **MUST use `include $(DEVKITARM)/ds_rules`** -- don't write custom compile/link rules.
3. **`-specs=ds_arm9.specs` is symbolic** -- doesn't exist as a file. ds_rules replaces it at link time.
4. **`$(MAKE)` stores absolute path** -- sub-make fails outside devkitPro MSYS2.
5. **ndstool doesn't support `--version`** -- don't use it for version checking.

### Code Patterns
6. **Use `pmMainLoop()` not `while(1)`** -- required for calico power management.
7. **`defaultExceptionHandler()` must be first** -- enables crash debugging.
8. **`scanKeys()` exactly once per frame** -- multiple calls break keysDown() delta tracking.
9. **Use `iprintf()` not `printf()`** -- saves ~8KB, no soft-float dependency.
10. **DTCM stack overflow is silent** -- ~16KB total, corrupts adjacent data.

### Architecture
11. **No software framebuffer** -- use hardware tile/sprite compositing.
12. **No malloc during gameplay** -- fragmentation is permanent on 4MB fixed RAM.
13. **No floating-point** -- ARM9 has no FPU, soft-float is 10-100x slower.
14. **No recursion in game loop** -- DTCM stack is tiny.
15. **No per-pixel collision** -- use tile-coordinate lookup (O(1) per tile).

### Game-Specific
16. **SNES 4bpp planar must be converted to linear** for DS -- bitplane interleaving.
17. **BGR555 palettes are directly compatible** between SNES and DS (no conversion needed).
18. **Physics MUST use 16.16 fixed-point** with exact SNES constants for feel accuracy.
19. **Room loading must be one-at-a-time** -- unload previous on transition.

## Project Structure

```
SMetroidDSiPort/
├── claude.md              # THIS FILE - master reference
├── Makefile               # Based on official ARM9 template (include ds_rules)
├── docs/
│   ├── architecture.md    # System architecture (NTR, hardware rendering)
│   ├── rom_analysis.md    # ROM header & structure
│   ├── rom_memory_map.md  # Bank-by-bank ROM map
│   ├── physics_data.md    # SNES physics constants (16.16 fixed-point)
│   ├── graphics_data.md   # SNES graphics format specs
│   ├── audio_data.md      # SNES audio data & format
│   ├── level_data.md      # Room structure & collision types
│   └── enemy_data.md      # Enemy AI & boss data
├── source/                # .c source files (auto-discovered by Makefile)
│   └── main.c
├── include/               # Header files
├── data/                  # Binary assets (embedded via bin2o)
├── graphics/              # PNG + .grit files (converted via grit)
├── audio/                 # Audio files (converted via mmutil)
└── tools/                 # Asset conversion scripts (Python)
```

## Git Commit Policy

All code changes must be committed with a brief description.

**Format:**
```
<type>: <brief description>

Co-Authored-By: Claude Opus 4.6 <noreply@anthropic.com>
```

**Types:** `feat:` `fix:` `refactor:` `perf:` `docs:` `test:` `build:` `asset:`

## Debugging

```bash
# Crash diagnosis (from exception handler pc address)
arm-none-eabi-addr2line -e SuperMetroidDS.elf 0x02004A3C

# Emulator debug output (goes to DeSmuME TTY, melonDS console)
fprintf(stderr, "debug: %d\n", value);

# ROM inspection
/c/devkitPro/msys2/usr/bin/bash.exe -lc 'ndstool -i /home/Infernal/Documents/Code/SMetroidDSiPort/SuperMetroidDS.nds'
```

## External References

- [P.JBoy's SM Disassembly](https://patrickjohnston.org/bank/index.html)
- [Kejardon's RAM Map](https://jathys.zophar.net/supermetroid/kejardon/RAMMap.txt)
- [GBATEK](https://problemkaputt.de/gbatek.htm) -- DS hardware reference
- [devkitPro libnds docs](https://libnds.devkitpro.org/)
- [SimpleNDS reference project](../SimpleNDS/) -- working NDS hello world (see its `docs/STANDARDS.md` for deep toolchain reference)
