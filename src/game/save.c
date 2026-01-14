/*
 * Super Metroid DSi Port - Save System
 *
 * Handles save/load operations to DSi SD card or NAND storage.
 * Supports 3 save slots with data validation and integrity checking.
 */

#include "save.h"
#include "player.h"
#include "room.h"
#include <stdio.h>
#include <string.h>
#include <fat.h>

// Save file paths
#define SAVE_BASE_PATH "sd:/smetroid/"
#define SAVE_FILE_FORMAT "sd:/smetroid/save%d.sav"

// Current save slot (for quick save/load)
static uint8_t current_slot = 0;

// Save directory initialization flag
static bool save_dir_initialized = false;

/*
 * Initialize the save system
 * Sets up FAT filesystem and creates save directory if needed
 */
void save_init(void) {
    // Initialize FAT filesystem
    if (!fatInitDefault()) {
        // FAT init failed - try alternate init
        // This is non-fatal, saves just won't work
        save_dir_initialized = false;
        return;
    }

    // Create save directory if it doesn't exist
    mkdir(SAVE_BASE_PATH, 0777);

    save_dir_initialized = true;
    current_slot = 0;
}

/*
 * Shutdown the save system
 */
void save_shutdown(void) {
    save_dir_initialized = false;
}

/*
 * Calculate checksum for save data
 * Uses simple XOR checksum for data validation
 *
 * @param data Save data to checksum
 * @return 16-bit checksum value
 */
uint16_t save_data_calculate_checksum(SaveData* data) {
    if (!data) return 0;

    uint16_t checksum = 0;
    uint8_t* bytes = (uint8_t*)data;

    // Skip the checksum field itself (offset 6, 2 bytes)
    for (size_t i = 0; i < sizeof(SaveData); i++) {
        if (i >= 6 && i < 8) continue;  // Skip checksum field
        checksum ^= bytes[i];
        checksum = (checksum << 1) | (checksum >> 15);  // Rotate left
    }

    return checksum;
}

/*
 * Validate save data integrity
 *
 * @param data Save data to validate
 * @return true if data is valid
 */
bool save_data_validate(SaveData* data) {
    if (!data) return false;

    // Check magic numbers
    if (data->magic != SAVE_MAGIC) return false;
    if (data->end_magic != SAVE_MAGIC) return false;

    // Check version
    if (data->version > SAVE_VERSION) return false;

    // Verify checksum
    uint16_t calculated = save_data_calculate_checksum(data);
    if (calculated != data->checksum) return false;

    return true;
}

/*
 * Initialize save data with default values
 *
 * @param data Save data structure to initialize
 */
void save_data_init(SaveData* data) {
    if (!data) return;

    memset(data, 0, sizeof(SaveData));

    data->magic = SAVE_MAGIC;
    data->version = SAVE_VERSION;
    data->end_magic = SAVE_MAGIC;

    // Default starting values (from original Super Metroid)
    data->energy = 99;
    data->max_energy = 99;
    data->missiles = 0;
    data->max_missiles = 0;
    data->super_missiles = 0;
    data->max_super_missiles = 0;
    data->power_bombs = 0;
    data->max_power_bombs = 0;
    data->reserve_energy = 0;
    data->max_reserve_energy = 0;

    // Starting location (Ceres Station or Ship)
    data->current_area = 0;  // Crateria
    data->current_room = 0;  // Landing site
    data->player_x = 128;
    data->player_y = 128;

    // No equipment at start
    data->equipped_items = 0;
    data->equipped_beams = 0;

    // Calculate checksum
    data->checksum = save_data_calculate_checksum(data);
}

/*
 * Copy current game state to save data
 *
 * @param data Save data to populate
 */
void save_data_from_game_state(SaveData* data) {
    if (!data) return;

    // Get player data
    Player* player = &g_player;

    data->energy = player->health;
    data->max_energy = player->max_health;
    data->missiles = player->missiles;
    data->max_missiles = player->max_missiles;
    data->super_missiles = player->super_missiles;
    data->max_super_missiles = player->max_super_missiles;
    data->power_bombs = player->power_bombs;
    data->max_power_bombs = player->max_power_bombs;
    data->reserve_energy = player->reserve_energy;
    data->max_reserve_energy = player->max_reserve_energy;

    data->equipped_items = player->equipped_items;
    data->equipped_beams = player->equipped_beams;

    // Get room data (extern from room.h)
    extern uint8_t g_current_area;
    extern uint8_t g_current_room;

    data->current_area = g_current_area;
    data->current_room = g_current_room;

    // Get player position
    int32_t px, py;
    player_get_position(player, &px, &py);
    data->player_x = (uint16_t)(px >> 16);  // Convert from 16.16 fixed to int
    data->player_y = (uint16_t)(py >> 16);

    // Calculate item percentage
    data->items_collected_percent = save_calculate_item_percentage(data);

    // Update checksum
    data->checksum = save_data_calculate_checksum(data);
}

/*
 * Apply save data to current game state
 *
 * @param data Save data to load from
 */
void save_data_to_game_state(SaveData* data) {
    if (!data || !save_data_validate(data)) return;

    // Apply to player
    Player* player = &g_player;

    player->health = data->energy;
    player->max_health = data->max_energy;
    player->missiles = data->missiles;
    player->max_missiles = data->max_missiles;
    player->super_missiles = data->super_missiles;
    player->max_super_missiles = data->max_super_missiles;
    player->power_bombs = data->power_bombs;
    player->max_power_bombs = data->max_power_bombs;
    player->reserve_energy = data->reserve_energy;
    player->max_reserve_energy = data->max_reserve_energy;

    player->equipped_items = data->equipped_items;
    player->equipped_beams = data->equipped_beams;

    // Set player position
    int32_t px = ((int32_t)data->player_x) << 16;  // Convert to 16.16 fixed
    int32_t py = ((int32_t)data->player_y) << 16;
    player_set_position(player, px, py);

    // Load room will be handled by room system
    // extern void room_load(uint8_t area_id, uint8_t room_id);
    // room_load(data->current_area, data->current_room);
}

/*
 * Write save data to a slot
 *
 * @param slot Save slot (0-2)
 * @param data Save data to write
 * @return true if save succeeded
 */
bool save_write(uint8_t slot, SaveData* data) {
    if (!save_dir_initialized) return false;
    if (slot >= SAVE_SLOT_COUNT) return false;
    if (!data) return false;

    // Update magic and checksum
    data->magic = SAVE_MAGIC;
    data->end_magic = SAVE_MAGIC;
    data->checksum = save_data_calculate_checksum(data);

    // Build save file path
    char filepath[64];
    snprintf(filepath, sizeof(filepath), SAVE_FILE_FORMAT, slot);

    // Open file for writing
    FILE* file = fopen(filepath, "wb");
    if (!file) return false;

    // Write save data
    size_t written = fwrite(data, sizeof(SaveData), 1, file);
    fclose(file);

    return written == 1;
}

/*
 * Read save data from a slot
 *
 * @param slot Save slot (0-2)
 * @param data Save data structure to fill
 * @return true if load succeeded
 */
bool save_read(uint8_t slot, SaveData* data) {
    if (!save_dir_initialized) return false;
    if (slot >= SAVE_SLOT_COUNT) return false;
    if (!data) return false;

    // Build save file path
    char filepath[64];
    snprintf(filepath, sizeof(filepath), SAVE_FILE_FORMAT, slot);

    // Open file for reading
    FILE* file = fopen(filepath, "rb");
    if (!file) return false;

    // Read save data
    size_t read = fread(data, sizeof(SaveData), 1, file);
    fclose(file);

    if (read != 1) return false;

    // Validate data
    return save_data_validate(data);
}

/*
 * Delete a save file
 *
 * @param slot Save slot (0-2)
 * @return true if delete succeeded
 */
bool save_delete(uint8_t slot) {
    if (!save_dir_initialized) return false;
    if (slot >= SAVE_SLOT_COUNT) return false;

    char filepath[64];
    snprintf(filepath, sizeof(filepath), SAVE_FILE_FORMAT, slot);

    return remove(filepath) == 0;
}

/*
 * Copy save data from one slot to another
 *
 * @param src_slot Source slot (0-2)
 * @param dst_slot Destination slot (0-2)
 * @return true if copy succeeded
 */
bool save_copy(uint8_t src_slot, uint8_t dst_slot) {
    if (src_slot >= SAVE_SLOT_COUNT) return false;
    if (dst_slot >= SAVE_SLOT_COUNT) return false;
    if (src_slot == dst_slot) return false;

    SaveData data;
    if (!save_read(src_slot, &data)) return false;
    return save_write(dst_slot, &data);
}

/*
 * Check if a save file exists
 *
 * @param slot Save slot (0-2)
 * @return true if save exists
 */
bool save_exists(uint8_t slot) {
    if (!save_dir_initialized) return false;
    if (slot >= SAVE_SLOT_COUNT) return false;

    char filepath[64];
    snprintf(filepath, sizeof(filepath), SAVE_FILE_FORMAT, slot);

    FILE* file = fopen(filepath, "rb");
    if (!file) return false;

    fclose(file);
    return true;
}

/*
 * Quick save to current slot
 *
 * @return true if save succeeded
 */
bool save_quick_save(void) {
    SaveData data;
    save_data_init(&data);
    save_data_from_game_state(&data);
    return save_write(current_slot, &data);
}

/*
 * Quick load from current slot
 *
 * @return true if load succeeded
 */
bool save_quick_load(void) {
    SaveData data;
    if (!save_read(current_slot, &data)) return false;
    save_data_to_game_state(&data);
    return true;
}

/*
 * Set the current save slot for quick operations
 *
 * @param slot Save slot (0-2)
 */
void save_set_current_slot(uint8_t slot) {
    if (slot < SAVE_SLOT_COUNT) {
        current_slot = slot;
    }
}

/*
 * Get the current save slot
 *
 * @return Current save slot (0-2)
 */
uint8_t save_get_current_slot(void) {
    return current_slot;
}

/*
 * Mark an item as collected
 *
 * @param data Save data
 * @param item_id Item ID (0-255)
 */
void save_mark_item_collected(SaveData* data, uint8_t item_id) {
    if (!data) return;
    uint8_t byte = item_id / 8;
    uint8_t bit = item_id % 8;
    if (byte < 32) {
        data->items_collected[byte] |= (1 << bit);
    }
}

/*
 * Mark a boss as defeated
 *
 * @param data Save data
 * @param boss_id Boss ID (0-63)
 */
void save_mark_boss_defeated(SaveData* data, uint8_t boss_id) {
    if (!data) return;
    uint8_t byte = boss_id / 8;
    uint8_t bit = boss_id % 8;
    if (byte < 8) {
        data->bosses_defeated[byte] |= (1 << bit);
    }
}

/*
 * Mark a door as opened
 *
 * @param data Save data
 * @param door_id Door ID (0-511)
 */
void save_mark_door_opened(SaveData* data, uint16_t door_id) {
    if (!data) return;
    uint8_t byte = door_id / 8;
    uint8_t bit = door_id % 8;
    if (byte < 64) {
        data->doors_opened[byte] |= (1 << bit);
    }
}

/*
 * Mark a map tile as revealed
 *
 * @param data Save data
 * @param map_tile_id Map tile ID (0-2047)
 */
void save_mark_map_revealed(SaveData* data, uint16_t map_tile_id) {
    if (!data) return;
    uint16_t byte = map_tile_id / 8;
    uint8_t bit = map_tile_id % 8;
    if (byte < 256) {
        data->map_revealed[byte] |= (1 << bit);
    }
}

/*
 * Check if an item is collected
 *
 * @param data Save data
 * @param item_id Item ID (0-255)
 * @return true if collected
 */
bool save_is_item_collected(SaveData* data, uint8_t item_id) {
    if (!data) return false;
    uint8_t byte = item_id / 8;
    uint8_t bit = item_id % 8;
    if (byte >= 32) return false;
    return (data->items_collected[byte] & (1 << bit)) != 0;
}

/*
 * Check if a boss is defeated
 *
 * @param data Save data
 * @param boss_id Boss ID (0-63)
 * @return true if defeated
 */
bool save_is_boss_defeated(SaveData* data, uint8_t boss_id) {
    if (!data) return false;
    uint8_t byte = boss_id / 8;
    uint8_t bit = boss_id % 8;
    if (byte >= 8) return false;
    return (data->bosses_defeated[byte] & (1 << bit)) != 0;
}

/*
 * Check if a door is opened
 *
 * @param data Save data
 * @param door_id Door ID (0-511)
 * @return true if opened
 */
bool save_is_door_opened(SaveData* data, uint16_t door_id) {
    if (!data) return false;
    uint8_t byte = door_id / 8;
    uint8_t bit = door_id % 8;
    if (byte >= 64) return false;
    return (data->doors_opened[byte] & (1 << bit)) != 0;
}

/*
 * Check if a map tile is revealed
 *
 * @param data Save data
 * @param map_tile_id Map tile ID (0-2047)
 * @return true if revealed
 */
bool save_is_map_revealed(SaveData* data, uint16_t map_tile_id) {
    if (!data) return false;
    uint16_t byte = map_tile_id / 8;
    uint8_t bit = map_tile_id % 8;
    if (byte >= 256) return false;
    return (data->map_revealed[byte] & (1 << bit)) != 0;
}

/*
 * Update playtime (call once per frame during gameplay)
 *
 * @param data Save data
 */
void save_update_playtime(SaveData* data) {
    if (!data) return;
    data->playtime_frames++;
}

/*
 * Get playtime as formatted string (HH:MM:SS)
 *
 * @param data Save data
 * @param buffer Output buffer
 * @param buffer_size Buffer size
 */
void save_get_playtime_string(SaveData* data, char* buffer, uint8_t buffer_size) {
    if (!data || !buffer || buffer_size < 9) return;

    uint32_t total_seconds = data->playtime_frames / 60;
    uint32_t hours = total_seconds / 3600;
    uint32_t minutes = (total_seconds % 3600) / 60;
    uint32_t seconds = total_seconds % 60;

    snprintf(buffer, buffer_size, "%02u:%02u:%02u", hours, minutes, seconds);
}

/*
 * Calculate item collection percentage
 *
 * @param data Save data
 * @return Percentage (0-100)
 */
uint8_t save_calculate_item_percentage(SaveData* data) {
    if (!data) return 0;

    // Count collected items
    uint16_t collected = 0;
    for (int i = 0; i < 32; i++) {
        uint8_t byte = data->items_collected[i];
        // Count set bits
        while (byte) {
            collected += byte & 1;
            byte >>= 1;
        }
    }

    // Total collectible items in Super Metroid: 100
    const uint16_t TOTAL_ITEMS = 100;

    return (uint8_t)((collected * 100) / TOTAL_ITEMS);
}

/*
 * Get save file metadata for display
 *
 * @param slot Save slot (0-2)
 * @param meta Metadata structure to fill
 * @return true if metadata retrieved
 */
bool save_get_metadata(uint8_t slot, SaveMetadata* meta) {
    if (!meta) return false;

    memset(meta, 0, sizeof(SaveMetadata));

    if (!save_exists(slot)) {
        meta->exists = false;
        return true;
    }

    SaveData data;
    if (!save_read(slot, &data)) {
        meta->exists = false;
        return false;
    }

    meta->exists = true;
    meta->energy = data.energy;
    meta->max_energy = data.max_energy;
    meta->current_area = data.current_area;
    meta->playtime_frames = data.playtime_frames;
    meta->items_percent = data.items_collected_percent;

    return true;
}
