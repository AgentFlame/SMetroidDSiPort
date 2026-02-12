/**
 * room.c - Room loading and collision map
 *
 * Single global room, O(1) tile collision queries, VRAM upload.
 * Multi-room world with door connections (M17a).
 * Currently uses hardcoded test rooms.
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
 * Test Tile / Palette Data
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

/* Hazard tile: every pixel = palette index 3 (white / red tint) */
static const u8 test_tile_hazard[32] = {
    0x33, 0x33, 0x33, 0x33,
    0x33, 0x33, 0x33, 0x33,
    0x33, 0x33, 0x33, 0x33,
    0x33, 0x33, 0x33, 0x33,
    0x33, 0x33, 0x33, 0x33,
    0x33, 0x33, 0x33, 0x33,
    0x33, 0x33, 0x33, 0x33,
    0x33, 0x33, 0x33, 0x33
};

/* Test palette (BGR555) */
static const u16 test_palette[16] = {
    RGB15(0, 0, 0),       /* 0: black (backdrop) */
    RGB15(8, 8, 24),      /* 1: dark blue (solid ground/wall) */
    RGB15(4, 20, 4),      /* 2: green (platform) */
    RGB15(31, 8, 8),      /* 3: red (hazard/special) */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/* ========================================================================
 * Helper: fill room edges (walls + floor)
 * ======================================================================== */

static void fill_room_shell(int w, int h) {
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int idx = y * w + x;

            /* Floor: bottom 2 rows */
            if (y >= h - 2) {
                g_current_room.collision[idx] = COLL_SOLID;
                g_current_room.tilemap[idx] = 1;
            }
            /* Left wall */
            else if (x == 0) {
                g_current_room.collision[idx] = COLL_SOLID;
                g_current_room.tilemap[idx] = 1;
            }
            /* Right wall */
            else if (x == w - 1) {
                g_current_room.collision[idx] = COLL_SOLID;
                g_current_room.tilemap[idx] = 1;
            }
            /* Air */
            else {
                g_current_room.collision[idx] = COLL_AIR;
                g_current_room.tilemap[idx] = 0;
            }
        }
    }
}

/* Punch a 1x3 door opening in a wall column.
 * Samus is 40px tall (half_h=20); 3 metatiles = 48px gives clearance. */
static void punch_door_opening(int door_x, int door_y, int w) {
    for (int dy = 0; dy < 3; dy++) {
        int idx = (door_y + dy) * w + door_x;
        g_current_room.collision[idx] = COLL_AIR;
        g_current_room.tilemap[idx] = 0;
    }
}

/* ========================================================================
 * Room (0,0): Crateria Test - 16x12
 *
 * Same layout as original M7 test room to preserve existing tests.
 * Added: door on right wall -> room (0,1).
 * ======================================================================== */

static void load_room_0_0(void) {
    g_current_room.width_tiles = 16;
    g_current_room.height_tiles = 12;
    g_current_room.tileset_id = 0;

    int w = 16, h = 12;

    memset(g_current_room.collision, COLL_AIR, sizeof(g_current_room.collision));
    memset(g_current_room.bts, 0, sizeof(g_current_room.bts));
    memset(g_current_room.tilemap, 0, sizeof(g_current_room.tilemap));

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int idx = y * w + x;

            /* Floor: bottom 2 rows */
            if (y >= h - 2) {
                g_current_room.collision[idx] = COLL_SOLID;
                g_current_room.tilemap[idx] = 1;
            }
            /* Walls: left and right edges */
            else if (x == 0 || x == w - 1) {
                g_current_room.collision[idx] = COLL_SOLID;
                g_current_room.tilemap[idx] = 1;
            }
            /* Platform: middle of room */
            else if (y == 6 && x >= 5 && x <= 10) {
                g_current_room.collision[idx] = COLL_SOLID;
                g_current_room.tilemap[idx] = 2;
            }
        }
    }

    /* Save station tile at (3, 9) - just above floor */
    {
        int idx = 9 * w + 3;
        g_current_room.collision[idx] = COLL_SPECIAL_SAVE;
        g_current_room.tilemap[idx] = 3;  /* hazard tile for visibility */
    }

    /* Door opening on right wall: tiles (15, 7) and (15, 8) */
    punch_door_opening(15, 7, w);

    /* Doors */
    g_current_room.doors[0] = (DoorData){
        .dest_area = 0, .dest_room = 1,
        .direction = DIR_RIGHT, .door_type = 0,
        .door_x = 15, .door_y = 7,
        .spawn_x = 32, .spawn_y = 148
    };
    g_current_room.door_count = 1;

    /* Enemy spawns */
    g_current_room.spawns[0] = (EnemySpawnData){ 1, 64, 148, 0, 0 };   /* Zoomer left */
    g_current_room.spawns[1] = (EnemySpawnData){ 1, 192, 148, 0, 0 };  /* Zoomer right */
    g_current_room.spawns[2] = (EnemySpawnData){ 3, 128, 48, 0, 0 };   /* Waver center */
    g_current_room.spawn_count = 3;
}

/* ========================================================================
 * Room (0,1): Wide Corridor - 32x12
 *
 * Camera scrolls horizontally. Different platform layout.
 * Doors: left -> room (0,0), right -> room (0,2).
 * ======================================================================== */

static void load_room_0_1(void) {
    g_current_room.width_tiles = 32;
    g_current_room.height_tiles = 12;
    g_current_room.tileset_id = 0;

    int w = 32, h = 12;

    memset(g_current_room.collision, COLL_AIR, sizeof(g_current_room.collision));
    memset(g_current_room.bts, 0, sizeof(g_current_room.bts));
    memset(g_current_room.tilemap, 0, sizeof(g_current_room.tilemap));

    fill_room_shell(w, h);

    /* Platforms at different heights for variety */
    /* Low platform left side */
    for (int x = 6; x <= 10; x++) {
        int idx = 8 * w + x;
        g_current_room.collision[idx] = COLL_SOLID;
        g_current_room.tilemap[idx] = 2;
    }
    /* High platform center */
    for (int x = 14; x <= 18; x++) {
        int idx = 5 * w + x;
        g_current_room.collision[idx] = COLL_SOLID;
        g_current_room.tilemap[idx] = 2;
    }
    /* Mid platform right side */
    for (int x = 22; x <= 26; x++) {
        int idx = 7 * w + x;
        g_current_room.collision[idx] = COLL_SOLID;
        g_current_room.tilemap[idx] = 2;
    }

    /* Spike row on floor near center (tiles 12-16 at row 9, just above floor) */
    for (int x = 12; x <= 16; x++) {
        int idx = 9 * w + x;
        g_current_room.collision[idx] = COLL_HAZARD_SPIKE;
        g_current_room.tilemap[idx] = 3;  /* hazard tile */
    }

    /* Shot blocks guarding items (tiles 20-21 at row 8) */
    for (int x = 20; x <= 21; x++) {
        int idx = 8 * w + x;
        g_current_room.collision[idx] = COLL_SPECIAL_SHOT;
        g_current_room.tilemap[idx] = 3;
    }

    /* Bomb block (tile 22 at row 8) */
    {
        int idx = 8 * w + 22;
        g_current_room.collision[idx] = COLL_SPECIAL_BOMB;
        g_current_room.tilemap[idx] = 3;
    }

    /* Crumble block bridge (tiles 3-5 at row 5) */
    for (int x = 3; x <= 5; x++) {
        int idx = 5 * w + x;
        g_current_room.collision[idx] = COLL_SPECIAL_CRUMBLE;
        g_current_room.tilemap[idx] = 2;  /* looks like platform */
    }

    /* Door openings */
    punch_door_opening(0, 7, w);   /* Left wall */
    punch_door_opening(31, 7, w);  /* Right wall */

    /* Doors */
    g_current_room.doors[0] = (DoorData){
        .dest_area = 0, .dest_room = 0,
        .direction = DIR_LEFT, .door_type = 0,
        .door_x = 0, .door_y = 7,
        .spawn_x = 224, .spawn_y = 148
    };
    g_current_room.doors[1] = (DoorData){
        .dest_area = 0, .dest_room = 2,
        .direction = DIR_RIGHT, .door_type = 0,
        .door_x = 31, .door_y = 7,
        .spawn_x = 32, .spawn_y = 300
    };
    g_current_room.door_count = 2;

    /* Enemy spawns - different types to distinguish rooms */
    g_current_room.spawns[0] = (EnemySpawnData){ 2, 128, 148, 0, 0 };  /* Geemer left */
    g_current_room.spawns[1] = (EnemySpawnData){ 5, 256, 120, 0, 0 };  /* Sidehopper center */
    g_current_room.spawns[2] = (EnemySpawnData){ 1, 400, 148, 0, 0 };  /* Zoomer right */
    g_current_room.spawn_count = 3;

    /* Items behind breakable blocks */
    g_current_room.items[0] = (ItemData){
        ITEM_MISSILE_TANK, INT_TO_FX(336), INT_TO_FX(140), false
    };
    g_current_room.items[1] = (ItemData){
        ITEM_ENERGY_TANK, INT_TO_FX(368), INT_TO_FX(140), false
    };
    g_current_room.item_count = 2;
}

/* ========================================================================
 * Room (0,2): Tall Shaft - 16x24
 *
 * Camera scrolls vertically. Platforms form a vertical climb.
 * Door: left -> room (0,1).
 * ======================================================================== */

static void load_room_0_2(void) {
    g_current_room.width_tiles = 16;
    g_current_room.height_tiles = 24;
    g_current_room.tileset_id = 0;

    int w = 16, h = 24;

    memset(g_current_room.collision, COLL_AIR, sizeof(g_current_room.collision));
    memset(g_current_room.bts, 0, sizeof(g_current_room.bts));
    memset(g_current_room.tilemap, 0, sizeof(g_current_room.tilemap));

    fill_room_shell(w, h);

    /* Ceiling: top row */
    for (int x = 0; x < w; x++) {
        int idx = 0 * w + x;
        g_current_room.collision[idx] = COLL_SOLID;
        g_current_room.tilemap[idx] = 1;
    }

    /* Staircase platforms going up */
    /* Bottom platform */
    for (int x = 3; x <= 8; x++) {
        int idx = 20 * w + x;
        g_current_room.collision[idx] = COLL_SOLID;
        g_current_room.tilemap[idx] = 2;
    }
    /* Mid-low platform */
    for (int x = 8; x <= 13; x++) {
        int idx = 16 * w + x;
        g_current_room.collision[idx] = COLL_SOLID;
        g_current_room.tilemap[idx] = 2;
    }
    /* Mid-high platform */
    for (int x = 3; x <= 8; x++) {
        int idx = 12 * w + x;
        g_current_room.collision[idx] = COLL_SOLID;
        g_current_room.tilemap[idx] = 2;
    }
    /* Top platform (extended to x=14 for right door access) */
    for (int x = 8; x <= 14; x++) {
        int idx = 8 * w + x;
        g_current_room.collision[idx] = COLL_SOLID;
        g_current_room.tilemap[idx] = 2;
    }
    /* High ledge */
    for (int x = 3; x <= 8; x++) {
        int idx = 4 * w + x;
        g_current_room.collision[idx] = COLL_SOLID;
        g_current_room.tilemap[idx] = 2;
    }

    /* Lava pool at bottom (row 21, above floor).
     * Starts at x=4 to leave safe landing zone near left door. */
    for (int x = 4; x < w - 1; x++) {
        int idx = 21 * w + x;
        g_current_room.collision[idx] = COLL_HAZARD_LAVA;
        g_current_room.tilemap[idx] = 3;  /* hazard tile */
    }

    /* Door opening on left wall near bottom: tiles (0, 19) and (0, 20) */
    punch_door_opening(0, 19, w);
    /* Door opening on right wall: tiles (15, 5)-(15, 7), aligned with top platform */
    punch_door_opening(15, 5, w);

    /* Doors */
    g_current_room.doors[0] = (DoorData){
        .dest_area = 0, .dest_room = 1,
        .direction = DIR_LEFT, .door_type = 0,
        .door_x = 0, .door_y = 19,
        .spawn_x = 480, .spawn_y = 148
    };
    g_current_room.doors[1] = (DoorData){
        .dest_area = 0, .dest_room = 3,
        .direction = DIR_RIGHT, .door_type = 0,
        .door_x = 15, .door_y = 5,
        .spawn_x = 32, .spawn_y = 140
    };
    g_current_room.door_count = 2;

    /* Enemy spawns - wavers for vertical room */
    g_current_room.spawns[0] = (EnemySpawnData){ 3, 128, 80, 0, 0 };   /* Waver top */
    g_current_room.spawns[1] = (EnemySpawnData){ 3, 128, 240, 0, 0 };  /* Waver bottom */
    g_current_room.spawn_count = 2;
}

/* ========================================================================
 * Room (0,3): Boss Chamber - 16x12
 *
 * Auto-spawns boss if not defeated. Door locked during fight.
 * Door: left -> room (0,2).
 * ======================================================================== */

static void load_room_0_3(void) {
    g_current_room.width_tiles = 16;
    g_current_room.height_tiles = 12;
    g_current_room.tileset_id = 0;

    int w = 16, h = 12;

    memset(g_current_room.collision, COLL_AIR, sizeof(g_current_room.collision));
    memset(g_current_room.bts, 0, sizeof(g_current_room.bts));
    memset(g_current_room.tilemap, 0, sizeof(g_current_room.tilemap));

    fill_room_shell(w, h);

    /* Ceiling */
    for (int x = 0; x < w; x++) {
        int idx = 0 * w + x;
        g_current_room.collision[idx] = COLL_SOLID;
        g_current_room.tilemap[idx] = 1;
    }

    /* Door opening on left wall: tiles (0, 7) and (0, 8) */
    punch_door_opening(0, 7, w);

    /* Doors */
    g_current_room.doors[0] = (DoorData){
        .dest_area = 0, .dest_room = 2,
        .direction = DIR_LEFT, .door_type = 0,
        .door_x = 0, .door_y = 7,
        .spawn_x = 224, .spawn_y = 108
    };
    g_current_room.door_count = 1;

    /* No enemy spawns - boss spawned separately by gameplay code */
    g_current_room.spawn_count = 0;
}

/* ========================================================================
 * Room Dispatch Table
 * ======================================================================== */

typedef void (*RoomLoadFn)(void);

typedef struct {
    uint8_t area_id;
    uint8_t room_id;
    RoomLoadFn load_fn;
} RoomTableEntry;

static const RoomTableEntry room_table[] = {
    { 0, 0, load_room_0_0 },
    { 0, 1, load_room_0_1 },
    { 0, 2, load_room_0_2 },
    { 0, 3, load_room_0_3 },
};

#define ROOM_TABLE_COUNT (sizeof(room_table) / sizeof(room_table[0]))

static const RoomTableEntry* find_room(uint8_t area_id, uint8_t room_id) {
    for (int i = 0; i < (int)ROOM_TABLE_COUNT; i++) {
        if (room_table[i].area_id == area_id &&
            room_table[i].room_id == room_id) {
            return &room_table[i];
        }
    }
    return NULL;
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

    const RoomTableEntry* entry = find_room(area_id, room_id);
    if (!entry) return false;

    /* Set area/room IDs before load function */
    g_current_room.area_id = area_id;
    g_current_room.room_id = room_id;

    /* Call room-specific load function */
    entry->load_fn();

    /* Compute scroll bounds */
    int w = g_current_room.width_tiles;
    int h = g_current_room.height_tiles;
    g_current_room.scroll_max_x = (w * TILE_SIZE) - SCREEN_WIDTH;
    g_current_room.scroll_max_y = (h * TILE_SIZE) - SCREEN_HEIGHT;
    if (g_current_room.scroll_max_x < 0) g_current_room.scroll_max_x = 0;
    if (g_current_room.scroll_max_y < 0) g_current_room.scroll_max_y = 0;

    g_current_room.loaded = true;

    /* Upload graphics to VRAM */
    room_upload_to_vram();

    fprintf(stderr, "Room %d:%d (%dx%d) doors=%d spawns=%d\n",
            area_id, room_id,
            g_current_room.width_tiles,
            g_current_room.height_tiles,
            g_current_room.door_count,
            g_current_room.spawn_count);

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

void room_set_collision(int tile_x, int tile_y, uint8_t new_type) {
    if (!g_current_room.loaded) return;
    if (tile_x < 0 || tile_x >= g_current_room.width_tiles) return;
    if (tile_y < 0 || tile_y >= g_current_room.height_tiles) return;
    g_current_room.collision[tile_y * g_current_room.width_tiles + tile_x] = new_type;
}

/* ========================================================================
 * Door Collision
 *
 * Each door occupies 1 tile wide, 2 tiles tall.
 * Check if the player's center tile overlaps any door region.
 * ======================================================================== */

const DoorData* room_check_door_collision(const PhysicsBody* body) {
    if (!g_current_room.loaded) return NULL;

    /* Player center in tile coordinates */
    int px = FX_TO_INT(body->pos.x) >> TILE_SHIFT;
    int py = FX_TO_INT(body->pos.y) >> TILE_SHIFT;

    for (int i = 0; i < g_current_room.door_count; i++) {
        const DoorData* d = &g_current_room.doors[i];

        /* Door occupies tile (door_x, door_y) to (door_x, door_y+2) */
        if (px == d->door_x && py >= d->door_y && py <= d->door_y + 2) {
            return d;
        }
    }
    return NULL;
}

/* ========================================================================
 * Item Pickup
 * ======================================================================== */

ItemTypeID room_check_item_pickup(const PhysicsBody* body) {
    if (!g_current_room.loaded) return ITEM_NONE;

    for (int i = 0; i < g_current_room.item_count; i++) {
        ItemData* item = &g_current_room.items[i];
        if (item->collected) continue;

        /* Simple AABB overlap: item is 16x16 (half = 8) */
        fx32 dx = body->pos.x - item->x;
        fx32 dy = body->pos.y - item->y;
        if (dx < 0) dx = -dx;
        if (dy < 0) dy = -dy;

        fx32 check_w = body->hitbox.half_w + INT_TO_FX(8);
        fx32 check_h = body->hitbox.half_h + INT_TO_FX(8);

        if (dx < check_w && dy < check_h) {
            item->collected = true;
            return item->type;
        }
    }
    return ITEM_NONE;
}

/* ========================================================================
 * Crumble Block Update
 * ======================================================================== */

void room_update_crumble_blocks(void) {
    if (!g_current_room.loaded) return;

    int w = g_current_room.width_tiles;
    int h = g_current_room.height_tiles;

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int idx = y * w + x;
            if (g_current_room.crumble_timer[idx] > 0) {
                g_current_room.crumble_timer[idx]--;
                if (g_current_room.crumble_timer[idx] == 0) {
                    g_current_room.collision[idx] = COLL_AIR;
                    g_current_room.tilemap[idx] = 0;
                    /* VRAM update deferred to next room_upload_to_vram call
                     * or inline update of the specific BG map entry */
                }
            }
        }
    }
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

    /* 1. Upload test tileset (4 tiles: empty, solid, platform, hazard)
     * MUST be static: DMA cannot access DTCM (stack memory).
     * Local arrays live on the DTCM stack, which is tightly coupled
     * to the CPU and not visible on the main bus that DMA uses. */
    static u8 tileset[32 * 4];
    memcpy(&tileset[0],   test_tile_empty,    32);
    memcpy(&tileset[32],  test_tile_solid,    32);
    memcpy(&tileset[64],  test_tile_platform, 32);
    memcpy(&tileset[96],  test_tile_hazard,   32);
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
