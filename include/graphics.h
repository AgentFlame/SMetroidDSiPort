/**
 * graphics.h - Hardware rendering foundation
 *
 * VRAM bank setup, shadow OAM, background tile/map loading,
 * sprite management, VBlank commit.
 *
 * Implemented in: source/graphics.c (M4)
 */

#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <nds.h>
#include "sm_types.h"
#include "sm_config.h"

/* Initialize video hardware: VRAM banks, video modes, BG layers, OAM */
void graphics_init(void);

/* Call at start of frame (clear shadow OAM) */
void graphics_begin_frame(void);

/* Call at end of frame (DMA shadow OAM + scroll regs to hardware) */
void graphics_end_frame(void);

/* Load tileset (tile graphics / CHR) into BG VRAM for a layer */
void graphics_load_bg_tileset(int layer, const void* data, uint32_t size);

/* Load tilemap (screen map) into BG VRAM for a layer */
void graphics_load_bg_tilemap(int layer, const void* data, uint32_t size);

/* Load 16-color palette to BG palette RAM (slot 0-15) */
void graphics_load_bg_palette(int palette_idx, const u16* palette);

/* Load sprite tile data to OBJ VRAM at the given tile offset */
void graphics_load_sprite_tiles(const void* data, uint32_t size, int tile_offset);

/* Load 16-color palette to OBJ palette RAM (slot 0-15) */
void graphics_load_sprite_palette(int palette_idx, const u16* palette);

/* Set BG scroll (applied at end_frame) */
void graphics_set_bg_scroll(int layer, int scroll_x, int scroll_y);

/* Set sprite in shadow OAM */
void graphics_set_sprite(int oam_idx, int x, int y, int tile_id,
                         int palette, int priority, bool hflip, bool vflip);

/* Hide a specific sprite */
void graphics_hide_sprite(int oam_idx);

/* Screen brightness control for fades.
 * level: -16 (black) to 0 (normal) to +16 (white) */
void graphics_set_brightness(int level);
void graphics_set_brightness_sub(int level);

#endif /* GRAPHICS_H */
