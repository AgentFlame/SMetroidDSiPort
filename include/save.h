/**
 * save.h - Save system (SNES-compatible SRAM format)
 *
 * The DS SRAM uses the exact same 8KB layout as the original SNES
 * Super Metroid. Save files are plug-and-play compatible:
 * rename .sav <-> .srm to transfer between DS and SNES emulators.
 *
 * SNES SRAM layout (8KB):
 *   0x0000-0x000F: Checksums + complements for 3 slots
 *   0x0010-0x066B: Slot 0 (0x065C bytes)
 *   0x066C-0x0CC7: Slot 1
 *   0x0CC8-0x1323: Slot 2
 *   0x1FF0-0x1FFF: Redundant checksums + complements
 *
 * Internally, save.c converts between our game-facing SaveData struct
 * and the SNES byte-level format. Game code never touches raw SRAM.
 *
 * Implemented in: source/save.c (M15)
 */

#ifndef SAVE_H
#define SAVE_H

#include "sm_types.h"

#define SAVE_SLOT_COUNT  3

/* Boss defeat flags (internal representation, one bit per boss).
 * Converted to/from SNES per-area boss bytes in save.c. */
#define BOSS_FLAG_BOMB_TORIZO    (1 << 0)
#define BOSS_FLAG_SPORE_SPAWN    (1 << 1)
#define BOSS_FLAG_KRAID          (1 << 2)
#define BOSS_FLAG_CROCOMIRE      (1 << 3)
#define BOSS_FLAG_GOLDEN_TORIZO  (1 << 4)
#define BOSS_FLAG_PHANTOON       (1 << 5)
#define BOSS_FLAG_DRAYGON        (1 << 6)
#define BOSS_FLAG_BOTWOON        (1 << 7)
#define BOSS_FLAG_RIDLEY         (1 << 8)
#define BOSS_FLAG_MOTHER_BRAIN   (1 << 9)

/* Game-facing save data structure.
 * Fields use natural C types. The SNES byte-level encoding
 * (equipment bitmasks, area-based boss bytes, alternating-byte
 * checksum) is handled transparently in save_write/save_read. */
typedef struct {
    /* Player stats */
    uint16_t hp;
    uint16_t hp_max;
    uint16_t missiles;
    uint16_t missiles_max;
    uint16_t supers;
    uint16_t supers_max;
    uint16_t power_bombs;
    uint16_t power_bombs_max;
    uint16_t reserve_hp;
    uint16_t reserve_hp_max;

    /* Equipment (EQUIP_* flags from sm_types.h) */
    uint32_t equipment;

    /* Position */
    uint16_t area_id;           /* 0=Crateria..6=Ceres */
    uint16_t save_station_id;   /* Save station index within area */

    /* Progress */
    uint16_t boss_flags;        /* BOSS_FLAG_* bits */

    /* Game time (SNES format: separate fields) */
    uint16_t time_hours;        /* 0-99 */
    uint16_t time_minutes;      /* 0-59 */
    uint16_t time_seconds;      /* 0-59 */
    uint16_t time_frames;       /* 0-59 */
} SaveData;

void save_init(void);
bool save_write(int slot, const SaveData* data);
bool save_read(int slot, SaveData* data);
bool save_slot_valid(int slot);
void save_delete(int slot);

#endif /* SAVE_H */
