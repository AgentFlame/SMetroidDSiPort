# Graphics System Implementation Instructions (M4)

## Purpose

Initialize NDS video hardware, manage VRAM banks, and provide shadow OAM and scroll register API. All rendering is hardware tile/sprite compositing. NO software framebuffer.

This is an Opus-level milestone. Wrong VRAM bank assignments cascade into every subsequent milestone. Verify bank configuration twice before committing.

## Dependencies

### Headers Required
```c
#include <nds.h>
#include "sm_types.h"
#include "sm_config.h"
#include "graphics.h"
```

### Build System
- Source file: `source/graphics.c`
- Header file: `include/graphics.h` (already exists, declarations only)
- Auto-discovered by Makefile (no manual addition needed)

## VRAM Bank Configuration

### Bank Layout (from sm_config.h)

| Bank | Size | Address | Purpose | Config Constant |
|------|------|---------|---------|-----------------|
| A | 128KB | 0x06000000 | Main BG slot 0 | `VRAM_A_MAIN_BG_0x06000000` |
| B | 128KB | 0x06020000 | Main BG slot 1 | `VRAM_B_MAIN_BG_0x06020000` |
| C | 128KB | Sub BG | HUD + map tiles | `VRAM_C_SUB_BG` |
| D | 128KB | 0x06400000 | Main OBJ | `VRAM_D_MAIN_SPRITE` |
| E | 64KB | LCD | Extended palette staging | `VRAM_E_LCD` |
| H | 32KB | 0x06600000 | Sub OBJ | `VRAM_H_SUB_SPRITE` |
| I | 16KB | 0x06208000 | Sub BG maps | `VRAM_I_SUB_BG_0x06208000` |

### Usage Strategy

**Main Engine (Top Screen - 192px gameplay):**
- Bank A: Level tileset data (tiles/CHR)
- Bank B: Level tilemap data (screen map + parallax)
- Bank D: Sprite data (Samus + enemies + projectiles)

**Sub Engine (Bottom Screen - 32px HUD + 160px extra):**
- Bank C: HUD tiles
- Bank H: HUD sprites (item icons, energy bars)
- Bank I: HUD tilemap

**Bank E:**
- Staging area for extended palette transfers (if needed)
- Can be reconfigured at runtime for temporary operations

### Configuration Constants (Copy to sm_config.h)

```c
/* VRAM Bank Assignments */
#define VRAM_A_CONFIG   VRAM_A_MAIN_BG_0x06000000
#define VRAM_B_CONFIG   VRAM_B_MAIN_BG_0x06020000
#define VRAM_C_CONFIG   VRAM_C_SUB_BG
#define VRAM_D_CONFIG   VRAM_D_MAIN_SPRITE
#define VRAM_E_CONFIG   VRAM_E_LCD
#define VRAM_H_CONFIG   VRAM_H_SUB_SPRITE
#define VRAM_I_CONFIG   VRAM_I_SUB_BG_0x06208000

/* BG Layer Assignments */
#define BG_LAYER_LEVEL     0  /* Main BG0: primary level tilemap */
#define BG_LAYER_PARALLAX  1  /* Main BG1: background parallax */
#define BG_LAYER_FG        2  /* Main BG2: foreground effects */
#define BG_LAYER_DEBUG     3  /* Main BG3: debug console */

/* Video Modes */
#define MAIN_VIDEO_MODE  MODE_0_2D  /* Tiled mode, 4 BG layers */
#define SUB_VIDEO_MODE   MODE_0_2D  /* Tiled mode, 4 BG layers */
```

## Data Structures for graphics.c

### Internal State (Static Variables)

```c
/* Background layer IDs (returned by bgInit) */
static int bg_main[4];    /* Main engine BG0-3 */
static int bg_sub[4];     /* Sub engine BG0-3 */

/* Shadow scroll registers (applied at VBlank) */
static int bg_scroll_x[4];  /* Per-layer horizontal scroll */
static int bg_scroll_y[4];  /* Per-layer vertical scroll */

/* Shadow OAM management */
static int oam_used_count;  /* Number of active sprites this frame */

/* Initialization flag */
static bool graphics_initialized = false;
```

### Notes on OAM Management

Use libnds `oamSet()` + `oamUpdate()` for OAM. libnds maintains internal shadow OAM and DMAs to hardware at `oamUpdate()`.

**Do NOT manually manage OAM memory.** libnds handles:
- Shadow buffer allocation
- DMA transfer at VBlank
- Double-buffering for tearing prevention

## Function Implementations

### graphics_init()

**Purpose:** One-time initialization of video hardware, VRAM banks, and BG layers.

**Implementation:**

```c
void graphics_init(void) {
    /* 1. Configure main engine video mode */
    videoSetMode(
        MAIN_VIDEO_MODE |
        DISPLAY_BG0_ACTIVE |  /* Level layer */
        DISPLAY_BG1_ACTIVE |  /* Parallax layer */
        DISPLAY_BG2_ACTIVE |  /* FG effects layer */
        DISPLAY_BG3_ACTIVE |  /* Debug console */
        DISPLAY_SPR_ACTIVE |  /* Sprites enabled */
        DISPLAY_SPR_1D        /* 1D sprite mapping */
    );

    /* 2. Configure sub engine video mode */
    videoSetModeSub(
        SUB_VIDEO_MODE |
        DISPLAY_BG0_ACTIVE |  /* HUD base layer */
        DISPLAY_BG1_ACTIVE |  /* HUD overlay */
        DISPLAY_SPR_ACTIVE |  /* HUD sprites */
        DISPLAY_SPR_1D        /* 1D sprite mapping */
    );

    /* 3. Set VRAM banks (ONCE, NEVER reconfigured) */
    vramSetBankA(VRAM_A_CONFIG);
    vramSetBankB(VRAM_B_CONFIG);
    vramSetBankC(VRAM_C_CONFIG);
    vramSetBankD(VRAM_D_CONFIG);
    vramSetBankE(VRAM_E_CONFIG);
    vramSetBankH(VRAM_H_CONFIG);
    vramSetBankI(VRAM_I_CONFIG);

    /* 4. Initialize BG layers - Main Engine */
    bg_main[0] = bgInit(0, BgType_Text4bpp, BgSize_T_256x256, 0, 1);  /* Map base 0, tile base 1 */
    bg_main[1] = bgInit(1, BgType_Text4bpp, BgSize_T_256x256, 2, 1);  /* Map base 2, tile base 1 */
    bg_main[2] = bgInit(2, BgType_Text4bpp, BgSize_T_256x256, 4, 1);  /* Map base 4, tile base 1 */
    bg_main[3] = bgInit(3, BgType_Text4bpp, BgSize_T_256x256, 6, 1);  /* Map base 6, tile base 1 */

    /* 5. Initialize BG layers - Sub Engine */
    bg_sub[0] = bgInitSub(0, BgType_Text4bpp, BgSize_T_256x256, 0, 1);
    bg_sub[1] = bgInitSub(1, BgType_Text4bpp, BgSize_T_256x256, 2, 1);

    /* 6. Initialize sprite systems */
    oamInit(&oamMain, SpriteMapping_1D_32, false);  /* 1D mapping, 32-byte boundary */
    oamInit(&oamSub, SpriteMapping_1D_32, false);

    /* 7. Zero scroll registers */
    for (int i = 0; i < 4; i++) {
        bg_scroll_x[i] = 0;
        bg_scroll_y[i] = 0;
    }

    /* 8. Initialize OAM counter */
    oam_used_count = 0;

    /* 9. Optional: Initialize debug console on sub engine BG3 */
    /* consoleDemoInit(); */  /* Uncomment if needed for debug text */

    /* 10. Set initialization flag */
    graphics_initialized = true;
}
```

**Critical Details:**

- `BgType_Text4bpp`: 16-color tiled mode (4 bits per pixel)
- `BgSize_T_256x256`: 256x256 pixel tilemap (32x32 tiles)
- Map base: tilemap location (units of 2KB)
- Tile base: tile data location (units of 16KB)
- `SpriteMapping_1D_32`: Sprites use linear addressing, 32-byte boundary alignment
- `false` parameter: No extended palettes initially

### graphics_begin_frame()

**Purpose:** Prepare for new frame rendering. Reset OAM allocation counter, clear shadow OAM to hidden state.

**Implementation:**

```c
void graphics_begin_frame(void) {
    /* Reset OAM allocation counter */
    oam_used_count = 0;

    /* libnds oamClear() hides all sprites by setting them off-screen */
    oamClear(&oamMain, 0, 128);  /* Clear all 128 main OAM entries */
    oamClear(&oamSub, 0, 128);   /* Clear all 128 sub OAM entries */
}
```

**Why Clear OAM Each Frame:**

Sprites are stateless from frame to frame. Every active sprite must be explicitly set each frame. Clearing prevents ghosting from previous frame.

### graphics_end_frame()

**Purpose:** Finalize frame. Copy shadow OAM to hardware, apply scroll registers.

**Implementation:**

```c
void graphics_end_frame(void) {
    /* 1. DMA shadow OAM to hardware (libnds handles shadow buffer) */
    oamUpdate(&oamMain);
    oamUpdate(&oamSub);

    /* 2. Apply scroll registers to hardware */
    REG_BG0HOFS = bg_scroll_x[0];
    REG_BG0VOFS = bg_scroll_y[0];
    REG_BG1HOFS = bg_scroll_x[1];
    REG_BG1VOFS = bg_scroll_y[1];
    REG_BG2HOFS = bg_scroll_x[2];
    REG_BG2VOFS = bg_scroll_y[2];
    REG_BG3HOFS = bg_scroll_x[3];
    REG_BG3VOFS = bg_scroll_y[3];

    /* Sub engine scroll (if needed) */
    REG_BG0HOFS_SUB = 0;
    REG_BG0VOFS_SUB = 0;
    REG_BG1HOFS_SUB = 0;
    REG_BG1VOFS_SUB = 0;
}
```

**Timing:**

Call after all sprites and BG updates are staged. `oamUpdate()` should be called during VBlank for tear-free rendering.

### graphics_load_bg_tileset()

**Purpose:** Load tile graphics data (CHR) to VRAM BG tile base.

**Signature:**

```c
void graphics_load_bg_tileset(int layer, const void* data, size_t size);
```

**Implementation:**

```c
void graphics_load_bg_tileset(int layer, const void* data, size_t size) {
    /* Get VRAM pointer for layer's tile base */
    u16* vram_ptr = bgGetGfxPtr(bg_main[layer]);

    /* DMA copy tile data to VRAM */
    dmaCopy(data, vram_ptr, size);
}
```

**Notes:**

- `layer`: 0-3 (BG_LAYER_LEVEL, BG_LAYER_PARALLAX, etc.)
- `data`: Pointer to 4bpp tile data (2 pixels per byte)
- `size`: Byte count (must be multiple of 32 bytes per tile)
- DS tiles are 8x8 pixels, 32 bytes per tile in 4bpp mode

### graphics_load_bg_tilemap()

**Purpose:** Load tilemap (screen map) to VRAM BG map base.

**Signature:**

```c
void graphics_load_bg_tilemap(int layer, const void* data, size_t size);
```

**Implementation:**

```c
void graphics_load_bg_tilemap(int layer, const void* data, size_t size) {
    /* Get VRAM pointer for layer's map base */
    u16* vram_ptr = bgGetMapPtr(bg_main[layer]);

    /* DMA copy tilemap data to VRAM */
    dmaCopy(data, vram_ptr, size);
}
```

**Notes:**

- Tilemap entries are 16-bit:
  - Bits 0-9: Tile index (0-1023)
  - Bits 10-11: Reserved
  - Bits 12-15: Palette index (0-15)
- 32x32 tiles = 2KB per map

### graphics_load_bg_palette()

**Purpose:** Load 16-color palette to BG palette RAM.

**Signature:**

```c
void graphics_load_bg_palette(int palette_idx, const u16* palette);
```

**Implementation:**

```c
void graphics_load_bg_palette(int palette_idx, const u16* palette) {
    /* BG palette is 16 slots of 16 colors each */
    u16* pal_ptr = BG_PALETTE + (palette_idx * 16);

    /* Copy 16 colors (32 bytes) */
    dmaCopy(palette, pal_ptr, 32);
}
```

**Notes:**

- `palette_idx`: 0-15 (palette slot)
- `palette`: 16 entries of BGR555 format (15-bit color)
- SNES BGR555 palettes are directly compatible (no conversion needed)

### graphics_load_sprite_tiles()

**Purpose:** Load sprite tile data to OBJ VRAM.

**Signature:**

```c
void graphics_load_sprite_tiles(const void* data, size_t size, int offset);
```

**Implementation:**

```c
void graphics_load_sprite_tiles(const void* data, size_t size, int offset) {
    /* Get OBJ VRAM pointer (main engine sprites) */
    u16* vram_ptr = oamGetGfxPtr(&oamMain, offset);

    /* DMA copy sprite tile data */
    dmaCopy(data, vram_ptr, size);
}
```

**Notes:**

- `offset`: Tile offset in OBJ VRAM (in 1D mapping, this is byte offset / 32)
- Sprite tiles are 8x8, 32 bytes per tile in 4bpp mode
- 1D mapping: tiles are sequential in memory

### graphics_load_sprite_palette()

**Purpose:** Load 16-color palette to OBJ palette RAM.

**Signature:**

```c
void graphics_load_sprite_palette(int palette_idx, const u16* palette);
```

**Implementation:**

```c
void graphics_load_sprite_palette(int palette_idx, const u16* palette) {
    /* OBJ palette is 16 slots of 16 colors each */
    u16* pal_ptr = SPRITE_PALETTE + (palette_idx * 16);

    /* Copy 16 colors (32 bytes) */
    dmaCopy(palette, pal_ptr, 32);
}
```

### graphics_set_sprite()

**Purpose:** Set sprite attributes in shadow OAM.

**Signature:**

```c
void graphics_set_sprite(int oam_idx, int x, int y, int tile_id, int palette,
                          int priority, bool hflip, bool vflip);
```

**Implementation:**

```c
void graphics_set_sprite(int oam_idx, int x, int y, int tile_id, int palette,
                          int priority, bool hflip, bool vflip) {
    /* Calculate tile pointer offset (1D mapping, 32 bytes per tile) */
    u16* tile_ptr = oamGetGfxPtr(&oamMain, tile_id);

    /* Set sprite in shadow OAM */
    oamSet(
        &oamMain,              /* OAM buffer */
        oam_idx,               /* OAM entry index (0-127) */
        x, y,                  /* Screen position */
        priority,              /* Priority (0-3, 0=highest) */
        palette,               /* Palette index (0-15) */
        SpriteSize_16x16,      /* Sprite size enum */
        SpriteColorFormat_16Color,  /* 4bpp mode */
        tile_ptr,              /* VRAM tile pointer */
        -1,                    /* Affine index (-1 = none) */
        false,                 /* Double size (affine only) */
        false,                 /* Hide sprite */
        hflip, vflip,          /* Flip flags */
        false                  /* Mosaic effect */
    );

    /* Track OAM usage */
    if (oam_idx >= oam_used_count) {
        oam_used_count = oam_idx + 1;
    }
}
```

**Notes:**

- `oam_idx`: 0-127 (OAM entry index)
- `x`, `y`: Screen coordinates (0-255, 0-191)
- `tile_id`: Tile offset in OBJ VRAM (1D mapping)
- `palette`: 0-15 (palette slot)
- `priority`: 0-3 (0=front, 3=back)
- Sprite sizes available: 8x8, 16x16, 32x32, 64x64, 16x8, 32x8, 32x16, 64x32, 8x16, 8x32, 16x32, 32x64

### graphics_set_scroll()

**Purpose:** Set BG layer scroll position (applied at `graphics_end_frame()`).

**Signature:**

```c
void graphics_set_scroll(int layer, int x, int y);
```

**Implementation:**

```c
void graphics_set_scroll(int layer, int x, int y) {
    bg_scroll_x[layer] = x;
    bg_scroll_y[layer] = y;
}
```

**Notes:**

- Scroll is 9-bit (0-511 range)
- Wraps at tilemap boundary (256 or 512 depending on map size)
- Applied to hardware at `graphics_end_frame()`

## Critical Constraints

### VRAM Bank Rules

1. **Banks set ONCE at init.** Never reconfigured during gameplay.
2. **No overlapping mappings.** Each bank has one purpose.
3. **Bank E is LCD mode.** Can be reconfigured for temporary operations, but restore before frame end.

### OAM Rules

1. **Use libnds OAM API exclusively.** Do not write to OAM memory directly.
2. **Clear OAM each frame.** Sprites are stateless.
3. **128 sprite limit per engine.** Track usage, refuse overflow.
4. **1D mapping only.** Simpler tile addressing, less VRAM waste.

### Rendering Rules

1. **No software framebuffer.** Hardware compositing only.
2. **No per-frame VRAM reallocation.** Tileset/tilemap slots are static.
3. **No floating-point.** All positions are integers.
4. **DMA for all VRAM writes.** Faster than CPU copy, doesn't stall bus.

### Performance Constraints

1. **60 fps target.** ~1.1M cycles per frame (67 MHz ARM9).
2. **VBlank window is ~1140 cycles.** DMA OAM/palette updates during VBlank.
3. **No unbounded loops in graphics code.** All operations O(1) or O(n) with small fixed n.

## Test Criteria

### Initialization Test

```c
graphics_init();
/* Expected: Both screens black, no garbage pixels, no crash */
```

### Static Tilemap Test

```c
graphics_init();
/* Load test tileset (64 tiles, 2KB) */
u8 test_tiles[2048];
graphics_load_bg_tileset(BG_LAYER_LEVEL, test_tiles, 2048);

/* Load test tilemap (32x32, 2KB) */
u16 test_map[1024];
graphics_load_bg_tilemap(BG_LAYER_LEVEL, test_map, 2048);

/* Load test palette (16 colors) */
u16 test_palette[16];
graphics_load_bg_palette(0, test_palette);

while (pmMainLoop()) {
    swiWaitForVBlank();
    scanKeys();
    if (keysDown() & KEY_START) break;
}
/* Expected: Static tilemap visible on top screen, no flicker */
```

### Sprite Test

```c
graphics_init();
graphics_load_sprite_tiles(sprite_data, 512, 0);  /* 16 tiles */
graphics_load_sprite_palette(0, sprite_palette);

int x = 128, y = 96;

while (pmMainLoop()) {
    swiWaitForVBlank();
    scanKeys();

    graphics_begin_frame();
    graphics_set_sprite(0, x, y, 0, 0, 0, false, false);
    graphics_end_frame();

    if (keysDown() & KEY_START) break;
}
/* Expected: 16x16 sprite at screen center, no garbage */
```

### Scroll Test

```c
graphics_init();
/* Load tileset/tilemap as above */

int scroll_x = 0;

while (pmMainLoop()) {
    swiWaitForVBlank();
    scanKeys();

    scroll_x += 1;
    if (scroll_x >= 256) scroll_x = 0;

    graphics_begin_frame();
    graphics_set_scroll(BG_LAYER_LEVEL, scroll_x, 0);
    graphics_end_frame();

    if (keysDown() & KEY_START) break;
}
/* Expected: Tilemap scrolls smoothly at 60fps, no tearing */
```

## Common Pitfalls

### VRAM Bank Misconfiguration

**Problem:** Wrong bank mapping causes garbage rendering or silent data corruption.

**Solution:** Triple-check bank config against memory map. Use constants from `sm_config.h`.

### OAM Overflow

**Problem:** Setting sprite at index > 127 corrupts OAM memory.

**Solution:** Track `oam_used_count`, refuse setting sprite if `oam_idx >= 128`.

### Forgetting oamUpdate()

**Problem:** Sprites not visible despite `oamSet()` calls.

**Solution:** Call `oamUpdate(&oamMain)` in `graphics_end_frame()`.

### DMA During Rendering

**Problem:** DMA during active scanline causes tearing.

**Solution:** All DMA (OAM, palettes, scroll) happens during VBlank. Call `graphics_end_frame()` right after `swiWaitForVBlank()`.

### Wrong Tile Offset Calculation

**Problem:** Sprite tiles appear corrupted or wrong.

**Solution:** In 1D mapping, tile offset = (tile_id * 32) bytes. Use `oamGetGfxPtr(&oamMain, tile_id)` for correct pointer.

### Tilemap Entry Format Errors

**Problem:** Wrong tile appears, or palette is incorrect.

**Solution:** Tilemap entry format:
```
Bits 0-9:   Tile index (0-1023)
Bits 12-15: Palette index (0-15)
Bits 10-11: Flip flags (optional)
```

### BG Init Map/Tile Base Conflicts

**Problem:** Tilemap overwrites tile data, or vice versa.

**Solution:** Map base and tile base must not overlap. Each map base is 2KB, tile base is 16KB. Use separate base values (see `graphics_init()` example).

## File Structure Summary

### include/graphics.h

```c
#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <nds.h>
#include "sm_types.h"

/* Initialization and frame management */
void graphics_init(void);
void graphics_begin_frame(void);
void graphics_end_frame(void);

/* BG layer management */
void graphics_load_bg_tileset(int layer, const void* data, size_t size);
void graphics_load_bg_tilemap(int layer, const void* data, size_t size);
void graphics_load_bg_palette(int palette_idx, const u16* palette);
void graphics_set_scroll(int layer, int x, int y);

/* Sprite management */
void graphics_load_sprite_tiles(const void* data, size_t size, int offset);
void graphics_load_sprite_palette(int palette_idx, const u16* palette);
void graphics_set_sprite(int oam_idx, int x, int y, int tile_id, int palette,
                          int priority, bool hflip, bool vflip);

#endif /* GRAPHICS_H */
```

### source/graphics.c

- Include headers (nds.h, sm_types.h, sm_config.h, graphics.h)
- Static state variables (bg_main[], bg_sub[], bg_scroll_x[], bg_scroll_y[], oam_used_count, graphics_initialized)
- Function implementations (as detailed above)

### include/sm_config.h

- VRAM bank config constants
- BG layer assignment constants
- Video mode constants

## Implementation Checklist

- [ ] Create `include/sm_config.h` with VRAM/BG/video mode constants
- [ ] Verify `include/graphics.h` has all function declarations
- [ ] Create `source/graphics.c` with static state variables
- [ ] Implement `graphics_init()` with VRAM bank setup
- [ ] Implement `graphics_begin_frame()` with OAM clear
- [ ] Implement `graphics_end_frame()` with OAM update and scroll apply
- [ ] Implement `graphics_load_bg_tileset()`
- [ ] Implement `graphics_load_bg_tilemap()`
- [ ] Implement `graphics_load_bg_palette()`
- [ ] Implement `graphics_load_sprite_tiles()`
- [ ] Implement `graphics_load_sprite_palette()`
- [ ] Implement `graphics_set_sprite()`
- [ ] Implement `graphics_set_scroll()`
- [ ] Build test: `make clean && make`
- [ ] Initialization test (both screens black, no crash)
- [ ] Static tilemap test (visible on top screen)
- [ ] Sprite test (16x16 sprite at center)
- [ ] Scroll test (smooth 60fps horizontal scroll)
- [ ] Commit with message: `feat: implement M4 graphics hardware rendering foundation`

## Final Notes

This is foundational code. Every subsequent milestone depends on correct VRAM bank assignment and OAM management. Test exhaustively before moving forward.

If VRAM bank config changes after this milestone, ALL graphics code must be audited for pointer invalidation.

Hardware rendering constraints are NON-NEGOTIABLE. No software framebuffer, no dynamic VRAM reallocation, no floating-point. These are ARM9 67 MHz + 4 MB RAM hard limits.
