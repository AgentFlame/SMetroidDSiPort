#ifndef SAVE_H
#define SAVE_H

#include <stdint.h>
#include <stdbool.h>

// Save file magic number
#define SAVE_MAGIC 0x534D4453  // "SMDS" (Super Metroid DSi)
#define SAVE_VERSION 0x0001

// Number of save slots
#define SAVE_SLOT_COUNT 3

// Save data structure (aligned for DSi NAND/SD card)
typedef struct {
    uint32_t magic;          // "SMDS" magic number
    uint16_t version;        // Save format version
    uint16_t checksum;       // Data integrity checksum

    // Current location
    uint8_t current_area;
    uint8_t current_room;
    uint16_t player_x;
    uint16_t player_y;

    // Player stats
    uint16_t energy;
    uint16_t max_energy;
    uint16_t missiles;
    uint16_t max_missiles;
    uint16_t super_missiles;
    uint16_t max_super_missiles;
    uint16_t power_bombs;
    uint16_t max_power_bombs;
    uint16_t reserve_energy;
    uint16_t max_reserve_energy;

    // Equipment
    uint32_t equipped_items;  // Bitmask of ItemFlags
    uint16_t equipped_beams;  // Bitmask of BeamFlags

    // Collection tracking (bitmasks)
    uint8_t items_collected[32];   // 256 items max
    uint8_t bosses_defeated[8];    // 64 bosses max
    uint8_t doors_opened[64];      // 512 doors max
    uint8_t map_revealed[256];     // 2048 map tiles max

    // Playtime
    uint32_t playtime_frames;      // Total playtime in frames (÷60 = seconds)

    // Game completion tracking
    uint8_t items_collected_percent;
    uint8_t game_completion_flags;

    // Expansion room
    uint8_t padding[128];

    // End marker
    uint32_t end_magic;  // Should match SAVE_MAGIC
} SaveData;

// Save system functions
void save_init(void);
void save_shutdown(void);

// Save/Load operations
bool save_write(uint8_t slot, SaveData* data);
bool save_read(uint8_t slot, SaveData* data);
bool save_delete(uint8_t slot);
bool save_copy(uint8_t src_slot, uint8_t dst_slot);
bool save_exists(uint8_t slot);

// Save data management
void save_data_init(SaveData* data);
void save_data_from_game_state(SaveData* data);
void save_data_to_game_state(SaveData* data);
uint16_t save_data_calculate_checksum(SaveData* data);
bool save_data_validate(SaveData* data);

// Quick save/load (uses last selected slot)
bool save_quick_save(void);
bool save_quick_load(void);
void save_set_current_slot(uint8_t slot);
uint8_t save_get_current_slot(void);

// Collection tracking
void save_mark_item_collected(SaveData* data, uint8_t item_id);
void save_mark_boss_defeated(SaveData* data, uint8_t boss_id);
void save_mark_door_opened(SaveData* data, uint16_t door_id);
void save_mark_map_revealed(SaveData* data, uint16_t map_tile_id);

bool save_is_item_collected(SaveData* data, uint8_t item_id);
bool save_is_boss_defeated(SaveData* data, uint8_t boss_id);
bool save_is_door_opened(SaveData* data, uint16_t door_id);
bool save_is_map_revealed(SaveData* data, uint16_t map_tile_id);

// Playtime utilities
void save_update_playtime(SaveData* data);
void save_get_playtime_string(SaveData* data, char* buffer, uint8_t buffer_size);

// Item percentage calculation
uint8_t save_calculate_item_percentage(SaveData* data);

// Save file metadata (for file select screen)
typedef struct {
    bool exists;
    uint16_t energy;
    uint16_t max_energy;
    uint8_t current_area;
    uint32_t playtime_frames;
    uint8_t items_percent;
} SaveMetadata;

bool save_get_metadata(uint8_t slot, SaveMetadata* meta);

#endif // SAVE_H
