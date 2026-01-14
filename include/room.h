#ifndef ROOM_H
#define ROOM_H

#include "enemies.h"
#include <stdint.h>
#include <stdbool.h>

// Area IDs (from ROM analysis)
typedef enum {
    AREA_CRATERIA = 0,
    AREA_BRINSTAR = 1,
    AREA_NORFAIR = 2,
    AREA_WRECKED_SHIP = 3,
    AREA_MARIDIA = 4,
    AREA_TOURIAN = 5,
    AREA_CERES = 6,
    AREA_DEBUG = 7
} AreaID;

// Screen size constants (16x16 metatiles)
#define SCREEN_TILES_WIDTH 16
#define SCREEN_TILES_HEIGHT 14
#define METATILE_SIZE 16  // 16x16 pixels per metatile

// Door types
typedef enum {
    DOOR_BLUE,      // Opens with any weapon
    DOOR_RED,       // Requires 5 missiles
    DOOR_GREEN,     // Requires 1 super missile
    DOOR_YELLOW,    // Requires 1 power bomb
    DOOR_GRAY,      // Opens when all enemies defeated
    DOOR_EYE        // Opens when eye sensor shot
} DoorType;

// Door direction
typedef enum {
    DOOR_LEFT,
    DOOR_RIGHT,
    DOOR_UP,
    DOOR_DOWN
} DoorDirection;

// Scroll region types
typedef enum {
    SCROLL_RED = 0,    // No scroll (boundary)
    SCROLL_BLUE = 1,   // Normal scroll allowed
    SCROLL_GREEN = 2   // Always visible
} ScrollType;

// PLM (Power-up/Load Module) types
typedef enum {
    PLM_ITEM_ENERGY_TANK,
    PLM_ITEM_MISSILE,
    PLM_ITEM_SUPER_MISSILE,
    PLM_ITEM_POWER_BOMB,
    PLM_ITEM_MORPH_BALL,
    PLM_ITEM_BOMBS,
    PLM_ITEM_CHARGE_BEAM,
    PLM_ITEM_VARIA_SUIT,
    PLM_ITEM_GRAVITY_SUIT,
    PLM_ITEM_HI_JUMP_BOOTS,
    PLM_ITEM_SPEED_BOOSTER,
    PLM_ITEM_GRAPPLE_BEAM,
    PLM_ITEM_XRAY_SCOPE,
    PLM_ITEM_SPACE_JUMP,
    PLM_ITEM_SPRING_BALL,
    PLM_ITEM_SCREW_ATTACK,
    PLM_BLOCK_SHOT,
    PLM_BLOCK_BOMB,
    PLM_BLOCK_POWER_BOMB,
    PLM_BLOCK_SUPER_MISSILE,
    PLM_BLOCK_SPEED,
    PLM_DOOR,
    PLM_TRIGGER,
    PLM_EFFECT
} PLMType;

// PLM structure
typedef struct {
    PLMType plm_id;
    uint8_t x, y;        // Tile coordinates
    uint16_t argument;   // PLM-specific data
    bool collected;      // For items
    bool active;
} PLM;

// Door structure
typedef struct {
    uint16_t dest_room_id;
    uint8_t dest_area_id;
    DoorType door_type;
    DoorDirection direction;
    uint16_t scroll_x, scroll_y;
    uint16_t spawn_x, spawn_y;
    uint8_t x, y;        // Door location in tiles
    bool is_open;
} Door;

// Room FX types
typedef enum {
    FX_NONE,
    FX_RAIN,
    FX_FOG,
    FX_LAVA,
    FX_WATER,
    FX_HEAT,
    FX_EARTHQUAKE
} RoomFXType;

// Room FX structure
typedef struct {
    RoomFXType fx_type;
    uint16_t y_position;
    uint16_t target_y;
    uint16_t speed;
    uint8_t timer;
    uint8_t intensity;
} RoomFX;

// Room structure
typedef struct {
    uint8_t area_id;
    uint8_t room_id;
    uint8_t width_screens;   // Width in screens (1 screen = 16×14 tiles)
    uint8_t height_screens;  // Height in screens

    // Tilemap data (decompressed)
    uint16_t* tilemap;       // 16×16 metatile indices (width×height tiles)
    uint8_t* collision;      // BTS collision type per metatile
    uint8_t* scroll_data;    // Per-screen scroll flags

    // Objects
    PLM* plms;
    uint8_t plm_count;

    // Enemies
    EnemySpawn* enemy_spawns;
    uint8_t enemy_count;

    // Doors
    Door* doors;
    uint8_t door_count;

    // Effects
    RoomFX fx;

    // State tracking
    bool visited;
    bool cleared;  // All enemies defeated
} Room;

// Room system functions
void room_init(void);
void room_load(uint8_t area_id, uint8_t room_id);
void room_unload(void);
void room_update(void);
void room_render(void);
void room_shutdown(void);

// Room queries
Room* room_get_current(void);
uint8_t room_get_area_id(void);
uint8_t room_get_room_id(void);
uint16_t room_get_width_pixels(void);
uint16_t room_get_height_pixels(void);

// Collision queries
uint8_t room_get_collision(int tile_x, int tile_y);
bool room_is_solid(int tile_x, int tile_y);
bool room_is_spike(int tile_x, int tile_y);
bool room_is_water(int tile_x, int tile_y);
bool room_is_lava(int tile_x, int tile_y);

// Block manipulation
void room_break_block(int tile_x, int tile_y);
void room_set_tile(int tile_x, int tile_y, uint16_t tile_index, uint8_t collision);

// PLM functions
PLM* room_get_plm(uint8_t index);
void room_collect_plm(PLM* plm);
void room_update_plms(void);

// Door functions
Door* room_get_door(uint8_t index);
bool room_check_door_collision(int x, int y, int w, int h, Door** out_door);
void room_open_door(Door* door);
void room_transition_to_door(Door* door);

// Enemy spawning
void room_spawn_enemies(void);
void room_clear_enemies(void);

// Scroll boundaries
void room_get_scroll_bounds(int screen_x, int screen_y, int* min_x, int* max_x, int* min_y, int* max_y);
ScrollType room_get_scroll_type(int screen_x, int screen_y);

// Room state persistence
void room_mark_visited(void);
void room_mark_cleared(void);
bool room_is_visited(uint8_t area_id, uint8_t room_id);
bool room_is_cleared(uint8_t area_id, uint8_t room_id);

// Global current room
extern Room* g_current_room;

#endif // ROOM_H
