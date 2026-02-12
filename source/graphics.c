/**
 * graphics.c - Hardware rendering foundation
 *
 * VRAM bank configuration, BG layer management, shadow OAM,
 * sprite management, and VBlank commit.
 *
 * CRITICAL: VRAM banks are set ONCE at init and never reconfigured.
 * All rendering is hardware tile/sprite compositing -- NO software framebuffer.
 */

#include "graphics.h"
#include <string.h>

/* ========================================================================
 * Internal State
 * ======================================================================== */

/* BG layer IDs returned by bgInit/bgInitSub */
static int bg_main[4];
static int bg_sub[4];

/* Shadow scroll registers (applied to hardware at end_frame) */
static int bg_scroll_x[4];
static int bg_scroll_y[4];

/* OAM management */
static int oam_used_count;

/* ========================================================================
 * Initialization
 * ======================================================================== */

void graphics_init(void) {
    /* 1. Configure main engine video mode (top screen) */
    videoSetMode(
        MAIN_VIDEO_MODE |
        DISPLAY_BG0_ACTIVE |
        DISPLAY_BG1_ACTIVE |
        DISPLAY_BG2_ACTIVE |
        DISPLAY_SPR_ACTIVE |
        DISPLAY_SPR_1D
    );

    /* 2. Configure sub engine video mode (bottom screen) */
    videoSetModeSub(
        SUB_VIDEO_MODE |
        DISPLAY_BG0_ACTIVE |
        DISPLAY_BG1_ACTIVE |
        DISPLAY_BG3_ACTIVE |
        DISPLAY_SPR_ACTIVE |
        DISPLAY_SPR_1D
    );

    /* 3. Set VRAM banks -- ONCE, NEVER reconfigured */
    vramSetBankA(VRAM_A_CONFIG);
    vramSetBankB(VRAM_B_CONFIG);
    vramSetBankC(VRAM_C_CONFIG);
    vramSetBankD(VRAM_D_CONFIG);
    vramSetBankE(VRAM_E_CONFIG);
    vramSetBankH(VRAM_H_CONFIG);
    vramSetBankI(VRAM_I_CONFIG);

    /* 4. Initialize main engine BG layers
     * BG0: Level tilemap (map base 0, tile base 1)
     * BG1: Parallax background (map base 2, tile base 1)
     * BG2: Foreground effects (map base 4, tile base 1)
     * BG3: Not used for gameplay (debug console will use consoleDemoInit)
     *
     * Map base unit = 2KB, Tile base unit = 16KB
     * Tile base 1 = 16KB offset into BG VRAM (shared across layers)
     */
    bg_main[BG_LAYER_LEVEL]    = bgInit(0, BgType_Text4bpp, BgSize_T_512x512, 0, 1);
    bg_main[BG_LAYER_PARALLAX] = bgInit(1, BgType_Text4bpp, BgSize_T_256x256, 4, 1);
    bg_main[BG_LAYER_FG]       = bgInit(2, BgType_Text4bpp, BgSize_T_256x256, 6, 1);
    /* BG3 reserved -- consoleDemoInit() sets it up if needed */
    bg_main[BG_LAYER_DEBUG]    = -1;

    /* 5. Initialize sub engine BG layers (bottom screen) */
    bg_sub[SUB_BG_LAYER_HUD]  = bgInitSub(0, BgType_Text4bpp, BgSize_T_256x256, 0, 1);
    bg_sub[SUB_BG_LAYER_MAP]  = bgInitSub(1, BgType_Text4bpp, BgSize_T_256x256, 2, 1);
    bg_sub[2] = -1;
    bg_sub[SUB_BG_LAYER_TEXT] = -1;  /* Console will init this */

    /* 6. Set BG priorities (lower number = closer to viewer)
     * Level layer on top of parallax, FG on top of level */
    bgSetPriority(bg_main[BG_LAYER_LEVEL], 1);
    bgSetPriority(bg_main[BG_LAYER_PARALLAX], 3);
    bgSetPriority(bg_main[BG_LAYER_FG], 0);

    /* 7. Initialize sprite OAM systems */
    oamInit(&oamMain, SpriteMapping_1D_32, false);
    oamInit(&oamSub, SpriteMapping_1D_32, false);

    /* 8. Zero scroll registers */
    for (int i = 0; i < 4; i++) {
        bg_scroll_x[i] = 0;
        bg_scroll_y[i] = 0;
    }

    oam_used_count = 0;
}

/* ========================================================================
 * Per-Frame Management
 * ======================================================================== */

void graphics_begin_frame(void) {
    oam_used_count = 0;

    /* Hide all sprites by clearing OAM entries */
    oamClear(&oamMain, 0, 128);
    oamClear(&oamSub, 0, 128);
}

void graphics_end_frame(void) {
    /* DMA shadow OAM to hardware */
    oamUpdate(&oamMain);
    oamUpdate(&oamSub);

    /* Apply scroll registers to main engine BGs */
    if (bg_main[BG_LAYER_LEVEL] >= 0) {
        bgSetScroll(bg_main[BG_LAYER_LEVEL],
                    bg_scroll_x[BG_LAYER_LEVEL],
                    bg_scroll_y[BG_LAYER_LEVEL]);
    }
    if (bg_main[BG_LAYER_PARALLAX] >= 0) {
        bgSetScroll(bg_main[BG_LAYER_PARALLAX],
                    bg_scroll_x[BG_LAYER_PARALLAX],
                    bg_scroll_y[BG_LAYER_PARALLAX]);
    }
    if (bg_main[BG_LAYER_FG] >= 0) {
        bgSetScroll(bg_main[BG_LAYER_FG],
                    bg_scroll_x[BG_LAYER_FG],
                    bg_scroll_y[BG_LAYER_FG]);
    }
    bgUpdate();
}

/* ========================================================================
 * BG Tile/Map/Palette Loading
 * ======================================================================== */

void graphics_load_bg_tileset(int layer, const void* data, uint32_t size) {
    if (layer < 0 || layer > 3 || bg_main[layer] < 0) return;
    DC_FlushRange(data, size);
    dmaCopy(data, bgGetGfxPtr(bg_main[layer]), size);
}

void graphics_load_bg_tilemap(int layer, const void* data, uint32_t size) {
    if (layer < 0 || layer > 3 || bg_main[layer] < 0) return;
    DC_FlushRange(data, size);
    dmaCopy(data, bgGetMapPtr(bg_main[layer]), size);
}

void graphics_load_bg_palette(int palette_idx, const u16* palette) {
    if (palette_idx < 0 || palette_idx > 15) return;
    DC_FlushRange(palette, 32);
    dmaCopy(palette, BG_PALETTE + (palette_idx * 16), 32);
}

/* ========================================================================
 * Sprite Tile/Palette Loading
 * ======================================================================== */

void graphics_load_sprite_tiles(const void* data, uint32_t size, int tile_offset) {
    u16* dest = oamGetGfxPtr(&oamMain, tile_offset);
    if (dest) {
        DC_FlushRange(data, size);
        dmaCopy(data, dest, size);
    }
}

void graphics_load_sprite_palette(int palette_idx, const u16* palette) {
    if (palette_idx < 0 || palette_idx > 15) return;
    DC_FlushRange(palette, 32);
    dmaCopy(palette, SPRITE_PALETTE + (palette_idx * 16), 32);
}

/* ========================================================================
 * Scroll Management
 * ======================================================================== */

void graphics_set_bg_scroll(int layer, int scroll_x, int scroll_y) {
    if (layer < 0 || layer > 3) return;
    bg_scroll_x[layer] = scroll_x;
    bg_scroll_y[layer] = scroll_y;
}

/* ========================================================================
 * Sprite Management
 * ======================================================================== */

void graphics_set_sprite(int oam_idx, int x, int y, int tile_id,
                         int palette, int priority, bool hflip, bool vflip) {
    if (oam_idx < 0 || oam_idx >= 128) return;

    u16* gfx_ptr = oamGetGfxPtr(&oamMain, tile_id);

    oamSet(&oamMain,
           oam_idx,
           x, y,
           priority,
           palette,
           SpriteSize_16x16,
           SpriteColorFormat_16Color,
           gfx_ptr,
           -1,      /* No affine transform */
           false,    /* Not double-size */
           false,    /* Not hidden */
           hflip,
           vflip,
           false);   /* No mosaic */

    if (oam_idx >= oam_used_count) {
        oam_used_count = oam_idx + 1;
    }
}

void graphics_hide_sprite(int oam_idx) {
    if (oam_idx < 0 || oam_idx >= 128) return;
    oamClearSprite(&oamMain, oam_idx);
}

/* ========================================================================
 * Screen Brightness (for fade transitions)
 *
 * REG_MASTER_BRIGHT format (0x0400006C main, 0x0400106C sub):
 *   Bits 0-4:  Intensity (0-16)
 *   Bits 14-15: Mode (0=off, 1=brighten/white, 2=darken/black, 3=reserved)
 *
 * level: -16 (full black) to 0 (normal) to +16 (full white)
 * ======================================================================== */

void graphics_set_brightness(int level) {
    if (level == 0) {
        REG_MASTER_BRIGHT = 0;
    } else if (level < 0) {
        int intensity = -level;
        if (intensity > 16) intensity = 16;
        REG_MASTER_BRIGHT = (2 << 14) | intensity;
    } else {
        int intensity = level;
        if (intensity > 16) intensity = 16;
        REG_MASTER_BRIGHT = (1 << 14) | intensity;
    }
}

void graphics_set_brightness_sub(int level) {
    if (level == 0) {
        REG_MASTER_BRIGHT_SUB = 0;
    } else if (level < 0) {
        int intensity = -level;
        if (intensity > 16) intensity = 16;
        REG_MASTER_BRIGHT_SUB = (2 << 14) | intensity;
    } else {
        int intensity = level;
        if (intensity > 16) intensity = 16;
        REG_MASTER_BRIGHT_SUB = (1 << 14) | intensity;
    }
}
