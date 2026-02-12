/**
 * room.h - Room loading and collision map
 *
 * Loads room data from embedded blobs. Provides O(1) tile collision query.
 * Single global room -- unload previous before loading next.
 *
 * Implemented in: source/room.c (M7)
 */

#ifndef ROOM_H
#define ROOM_H

#include "sm_types.h"
#include "sm_config.h"
#include "physics.h"

/* Door connection data */
typedef struct {
    uint8_t  dest_area;
    uint8_t  dest_room;
    uint8_t  direction;     /* DIR_LEFT, DIR_RIGHT, DIR_UP, DIR_DOWN */
    uint8_t  door_type;     /* 0=blue, 1=red, 2=green, 3=yellow, 4=gray */
    uint16_t door_x;        /* Door X position in room tiles */
    uint16_t door_y;        /* Door Y position in room tiles */
    uint16_t spawn_x;       /* Samus spawn X in dest room (pixels) */
    uint16_t spawn_y;       /* Samus spawn Y in dest room (pixels) */
} DoorData;

/* Enemy spawn point (consumed by enemy system at room load) */
typedef struct {
    uint16_t enemy_id;
    int16_t  x;             /* Spawn X position (pixels) */
    int16_t  y;             /* Spawn Y position (pixels) */
    uint16_t param;         /* Initial parameter */
    uint16_t properties;
} EnemySpawnData;

/* Room data structure (single global instance, never allocated) */
typedef struct {
    uint16_t width_tiles;       /* Room width in 16x16 metatiles */
    uint16_t height_tiles;      /* Room height in 16x16 metatiles */
    uint8_t  area_id;           /* Area index (for tileset/palette switching) */
    uint8_t  room_id;           /* Room index within area */
    uint8_t  tileset_id;        /* Which tileset to use */
    bool     loaded;            /* Is a room currently loaded? */

    /* Collision map: collision[y * width_tiles + x] */
    uint8_t  collision[MAX_ROOM_WIDTH_TILES * MAX_ROOM_HEIGHT_TILES];
    /* Block Type Specifier (slope angles, special block types) */
    uint8_t  bts[MAX_ROOM_WIDTH_TILES * MAX_ROOM_HEIGHT_TILES];
    /* Metatile tilemap indices */
    uint16_t tilemap[MAX_ROOM_WIDTH_TILES * MAX_ROOM_HEIGHT_TILES];

    /* Doors */
    DoorData doors[MAX_DOORS];
    uint8_t  door_count;

    /* Enemy spawn points */
    EnemySpawnData spawns[MAX_ENEMIES];
    uint8_t  spawn_count;

    /* Items */
    ItemData items[MAX_ITEMS];
    uint8_t  item_count;

    /* Crumble block timers (parallel to collision array) */
    uint8_t  crumble_timer[MAX_ROOM_WIDTH_TILES * MAX_ROOM_HEIGHT_TILES];

    /* Scroll bounds (pixels, 0 if room fits on one screen) */
    int scroll_max_x;
    int scroll_max_y;
} RoomData;

extern RoomData g_current_room;

/* Initialize room system (zero state) */
void    room_init(void);

/* Load room by area+room ID. Unloads previous room first. */
bool    room_load(uint8_t area_id, uint8_t room_id);

/* Unload current room */
void    room_unload(void);

/* O(1) collision query. Returns COLL_SOLID for out-of-bounds. */
uint8_t room_get_collision(int tile_x, int tile_y);

/* O(1) BTS query. Returns 0 for out-of-bounds. */
uint8_t room_get_bts(int tile_x, int tile_y);

/* O(1) set collision type at runtime (for breakable blocks) */
void    room_set_collision(int tile_x, int tile_y, uint8_t new_type);

/* Check if body overlaps any door. Returns DoorData* or NULL. */
const DoorData* room_check_door_collision(const PhysicsBody* body);

/* Check if body overlaps an item. Grants it if so. Returns item type or ITEM_NONE. */
ItemTypeID room_check_item_pickup(const PhysicsBody* body);

/* Update crumble block timers. Call once per frame during gameplay. */
void    room_update_crumble_blocks(void);

/* Upload current room tilemap/tileset/palette to VRAM */
void    room_upload_to_vram(void);

#endif /* ROOM_H */
