/**
 * sm_config.h - Hardware configuration for Super Metroid DS
 *
 * Screen dimensions, VRAM bank assignments, frame timing.
 * All values are compile-time constants -- no runtime configuration.
 */

#ifndef SM_CONFIG_H
#define SM_CONFIG_H

#include <nds.h>

/* ========================================================================
 * Screen Dimensions
 * SCREEN_WIDTH (256) and SCREEN_HEIGHT (192) are defined by libnds.
 * ======================================================================== */

#define SNES_WIDTH         256
#define SNES_HEIGHT        224

/* HUD strip on bottom screen */
#define HUD_HEIGHT          32
#define MAP_AREA_HEIGHT    (SCREEN_HEIGHT - HUD_HEIGHT)  /* 160px */

/* ========================================================================
 * Frame Timing
 * ======================================================================== */

#define TARGET_FPS          60
#define ARM9_CLOCK_HZ   67028000   /* 67.028 MHz */
#define CYCLES_PER_FRAME (ARM9_CLOCK_HZ / TARGET_FPS)  /* ~1,117,133 */

/* ========================================================================
 * VRAM Bank Assignments
 *
 * Set once at startup, never reconfigured.
 * See docs/architecture.md for rationale.
 *
 * Bank  Size     Assignment                  libnds constant
 * ----  ------   ----------                  ---------------
 * A     128 KB   Main BG (tiles+maps)        VRAM_A_MAIN_BG_0x06000000
 * B     128 KB   Main OBJ (sprites)          VRAM_B_MAIN_SPRITE_0x06400000
 * C     128 KB   Sub BG (HUD + map tiles)    VRAM_C_SUB_BG
 * D     128 KB   Sub OBJ (HUD sprites)       VRAM_D_SUB_SPRITE
 * E      64 KB   LCD mode (available)        VRAM_E_LCD
 * H      32 KB   Sub BG (extra)              VRAM_H_SUB_BG
 * I      16 KB   Sub BG maps                 VRAM_I_SUB_BG_0x06208000
 *
 * Note: Palettes use dedicated palette RAM (BG_PALETTE, SPRITE_PALETTE),
 * not VRAM banks. Bank E is LCD mode (unused) and available for staging.
 * ======================================================================== */

#define VRAM_A_CONFIG   VRAM_A_MAIN_BG_0x06000000
#define VRAM_B_CONFIG   VRAM_B_MAIN_SPRITE_0x06400000
#define VRAM_C_CONFIG   VRAM_C_SUB_BG
#define VRAM_D_CONFIG   VRAM_D_SUB_SPRITE
#define VRAM_E_CONFIG   VRAM_E_LCD
#define VRAM_H_CONFIG   VRAM_H_SUB_BG
#define VRAM_I_CONFIG   VRAM_I_SUB_BG_0x06208000

/* ========================================================================
 * BG Layer Assignments (Main Engine - Top Screen)
 * ======================================================================== */

#define BG_LAYER_LEVEL     0   /* Main level tilemap (16x16 metatiles) */
#define BG_LAYER_PARALLAX  1   /* Parallax background */
#define BG_LAYER_FG        2   /* Foreground overlay / effects */
#define BG_LAYER_DEBUG     3   /* Debug text (only in debug builds) */

/* ========================================================================
 * BG Layer Assignments (Sub Engine - Bottom Screen)
 * ======================================================================== */

#define SUB_BG_LAYER_HUD   0   /* HUD strip */
#define SUB_BG_LAYER_MAP   1   /* Area map */
#define SUB_BG_LAYER_TEXT  3   /* Console text */

/* ========================================================================
 * Video Modes
 * ======================================================================== */

#define MAIN_VIDEO_MODE  MODE_0_2D   /* 4 tiled BG layers */
#define SUB_VIDEO_MODE   MODE_0_2D

/* ========================================================================
 * Room / World Limits
 * ======================================================================== */

#define MAX_ROOM_WIDTH_TILES   64   /* Max room width in metatiles (16px each) */
#define MAX_ROOM_HEIGHT_TILES  32   /* Max room height in metatiles */
#define MAX_ROOM_WIDTH_PX    (MAX_ROOM_WIDTH_TILES * 16)   /* 1024px */
#define MAX_ROOM_HEIGHT_PX   (MAX_ROOM_HEIGHT_TILES * 16)  /* 512px */

/* ========================================================================
 * Input Buffering
 * ======================================================================== */

#define INPUT_BUFFER_FRAMES  8   /* Circular buffer for input history */
#define INPUT_BUFFER_WINDOW  5   /* Frames a buffered press stays valid */

#endif /* SM_CONFIG_H */
