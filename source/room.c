/**
 * room.c - Room loading and collision map
 *
 * Single global room, O(1) tile collision queries, VRAM upload.
 * Currently uses a hardcoded test room (M7 verification).
 * Will be replaced with bin2o-embedded ROM data when asset pipeline is complete.
 */

#include "room.h"
#include "graphics.h"
#include <string.h>
#include <stdio.h>

/* The single global room instance */
RoomData g_current_room;

/* ========================================================================
 * Static BG map buffer for VRAM upload
 * 512x512 BG = 64x64 tile entries = 8192 bytes.
 * Too large for DTCM stack (~16KB total), so it's static.
 * ======================================================================== */

static u16 vram_bgmap[64 * 64];

/* ========================================================================
 * Test Room Data (M7 verification)
 *
 * 16x12 metatile room = 256x192 pixels (fits one DS screen).
 * Layout: solid walls on edges, floor on bottom 2 rows,
 * a platform in the middle, everything else is air.
 * ======================================================================== */

/* 4bpp linear 8x8 test tiles (32 bytes each) */
static const u8 test_tile_empty[32] = { 0 };

/* Solid tile: every pixel = palette index 1 */
static const u8 test_tile_solid[32] = {
    0x11, 0x11, 0x11, 0x11,
    0x11, 0x11, 0x11, 0x11,
    0x11, 0x11, 0x11, 0x11,
    0x11, 0x11, 0x11, 0x11,
    0x11, 0x11, 0x11, 0x11,
    0x11, 0x11, 0x11, 0x11,
    0x11, 0x11, 0x11, 0x11,
    0x11, 0x11, 0x11, 0x11
};

/* Platform tile: every pixel = palette index 2 */
static const u8 test_tile_platform[32] = {
    0x22, 0x22, 0x22, 0x22,
    0x22, 0x22, 0x22, 0x22,
    0x22, 0x22, 0x22, 0x22,
    0x22, 0x22, 0x22, 0x22,
    0x22, 0x22, 0x22, 0x22,
    0x22, 0x22, 0x22, 0x22,
    0x22, 0x22, 0x22, 0x22,
    0x22, 0x22, 0x22, 0x22
};

/* Test palette (BGR555) */
static const u16 test_palette[16] = {
    RGB15(0, 0, 0),       /* 0: black (backdrop) */
    RGB15(8, 8, 24),      /* 1: dark blue (solid ground/wall) */
    RGB15(4, 20, 4),      /* 2: green (platform) */
    RGB15(31, 31, 31),    /* 3: white */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/* Build the hardcoded test room */
static bool load_test_room(uint8_t area_id, uint8_t room_id) {
    g_current_room.area_id = area_id;
    g_current_room.room_id = room_id;
    g_current_room.width_tiles = 16;
    g_current_room.height_tiles = 12;
    g_current_room.tileset_id = 0;
    g_current_room.door_count = 0;

    /* Enemy spawn points for testing */
    g_current_room.spawns[0] = (EnemySpawnData){ 1, 64, 148, 0, 0 };  /* Zoomer left */
    g_current_room.spawns[1] = (EnemySpawnData){ 1, 192, 148, 0, 0 }; /* Zoomer right */
    g_current_room.spawns[2] = (EnemySpawnData){ 3, 128, 48, 0, 0 };  /* Waver center */
    g_current_room.spawn_count = 3;

    memset(g_current_room.collision, COLL_AIR, sizeof(g_current_room.collision));
    memset(g_current_room.bts, 0, sizeof(g_current_room.bts));
    memset(g_current_room.tilemap, 0, sizeof(g_current_room.tilemap));

    int w = g_current_room.width_tiles;
    int h = g_current_room.height_tiles;

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int idx = y * w + x;

            /* Floor: bottom 2 rows */
            if (y >= h - 2) {
                g_current_room.collision[idx] = COLL_SOLID;
                g_current_room.tilemap[idx] = 1;  /* solid metatile */
            }
            /* Walls: left and right edges */
            else if (x == 0 || x == w - 1) {
                g_current_room.collision[idx] = COLL_SOLID;
                g_current_room.tilemap[idx] = 1;
            }
            /* Platform: middle of room */
            else if (y == 6 && x >= 5 && x <= 10) {
                g_current_room.collision[idx] = COLL_SOLID;
                g_current_room.tilemap[idx] = 2;  /* platform metatile */
            }
            /* Air */
            else {
                g_current_room.tilemap[idx] = 0;  /* empty metatile */
            }
        }
    }

    /* Scroll bounds (0 since room fits one screen exactly) */
    g_current_room.scroll_max_x = (w * TILE_SIZE) - SCREEN_WIDTH;
    g_current_room.scroll_max_y = (h * TILE_SIZE) - SCREEN_HEIGHT;
    if (g_current_room.scroll_max_x < 0) g_current_room.scroll_max_x = 0;
    if (g_current_room.scroll_max_y < 0) g_current_room.scroll_max_y = 0;

    g_current_room.loaded = true;
    return true;
}

/* ========================================================================
 * Public API
 * ======================================================================== */

void room_init(void) {
    memset(&g_current_room, 0, sizeof(g_current_room));
}

bool room_load(uint8_t area_id, uint8_t room_id) {
    /* Unload previous room if any */
    if (g_current_room.loaded) {
        room_unload();
    }

    /* Currently all loads produce the test room.
     * When the asset pipeline is complete, this will look up
     * bin2o-embedded room data by (area_id, room_id). */
    if (!load_test_room(area_id, room_id)) {
        return false;
    }

    /* Upload graphics to VRAM */
    room_upload_to_vram();

    iprintf("Room %d:%d (%dx%d)\n",
            area_id, room_id,
            g_current_room.width_tiles,
            g_current_room.height_tiles);

    return true;
}

void room_unload(void) {
    g_current_room.loaded = false;
    g_current_room.width_tiles = 0;
    g_current_room.height_tiles = 0;
    g_current_room.door_count = 0;
    g_current_room.spawn_count = 0;
}

uint8_t room_get_collision(int tile_x, int tile_y) {
    if (!g_current_room.loaded) return COLL_SOLID;
    if (tile_x < 0 || tile_x >= g_current_room.width_tiles) return COLL_SOLID;
    if (tile_y < 0 || tile_y >= g_current_room.height_tiles) return COLL_SOLID;
    return g_current_room.collision[tile_y * g_current_room.width_tiles + tile_x];
}

uint8_t room_get_bts(int tile_x, int tile_y) {
    if (!g_current_room.loaded) return 0;
    if (tile_x < 0 || tile_x >= g_current_room.width_tiles) return 0;
    if (tile_y < 0 || tile_y >= g_current_room.height_tiles) return 0;
    return g_current_room.bts[tile_y * g_current_room.width_tiles + tile_x];
}

/* ========================================================================
 * VRAM Upload
 *
 * Uploads tileset, palette, and tilemap to the main engine BG.
 * Each 16x16 metatile expands to 2x2 entries in the 8x8 BG map.
 *
 * DS 512x512 BG map layout: 4 contiguous 32x32 blocks (8KB total)
 *   Block 0: rows 0-31, cols 0-31
 *   Block 1: rows 0-31, cols 32-63
 *   Block 2: rows 32-63, cols 0-31
 *   Block 3: rows 32-63, cols 32-63
 *
 * BG map entry (16 bits):
 *   Bits 0-9:   Tile index
 *   Bit 10:     H-flip
 *   Bit 11:     V-flip
 *   Bits 12-15: Palette number
 * ======================================================================== */

void room_upload_to_vram(void) {
    if (!g_current_room.loaded) return;

    /* 1. Upload test tileset (3 tiles: empty, solid, platform) */
    u8 tileset[32 * 3];
    memcpy(&tileset[0],  test_tile_empty,    32);
    memcpy(&tileset[32], test_tile_solid,    32);
    memcpy(&tileset[64], test_tile_platform, 32);
    graphics_load_bg_tileset(BG_LAYER_LEVEL, tileset, sizeof(tileset));

    /* 2. Upload palette */
    graphics_load_bg_palette(0, test_palette);

    /* 3. Build 8x8 BG map from metatile tilemap */
    int w = g_current_room.width_tiles;
    int h = g_current_room.height_tiles;

    memset(vram_bgmap, 0, sizeof(vram_bgmap));

    for (int my = 0; my < h && my * 2 < 64; my++) {
        for (int mx = 0; mx < w && mx * 2 < 64; mx++) {
            uint16_t metatile = g_current_room.tilemap[my * w + mx];

            /* Simple mapping: metatile N -> all four 8x8 sub-tiles use tile N.
             * When real metatile definitions are available, this will look up
             * a MetatileDef table for proper 4-tile expansion. */
            uint16_t entry = metatile & 0x03FF;  /* tile index, palette 0, no flip */

            /* Expand to 2x2 in the BG map */
            for (int dy = 0; dy < 2; dy++) {
                for (int dx = 0; dx < 2; dx++) {
                    int tx = mx * 2 + dx;
                    int ty = my * 2 + dy;

                    /* 512x512 map: 4 blocks of 32x32 entries */
                    int block = (tx >= 32 ? 1 : 0) + (ty >= 32 ? 2 : 0);
                    int local_x = tx & 31;
                    int local_y = ty & 31;
                    int offset = block * 1024 + local_y * 32 + local_x;

                    vram_bgmap[offset] = entry;
                }
            }
        }
    }

    /* 4. Upload BG map to VRAM */
    graphics_load_bg_tilemap(BG_LAYER_LEVEL, vram_bgmap, sizeof(vram_bgmap));
}
