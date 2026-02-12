/**
 * save.c - Save system via FAT filesystem (M15)
 *
 * SNES-compatible save format. The .sav file is the exact 8KB
 * SNES Super Metroid SRAM layout byte-for-byte.
 *
 * To transfer saves:
 *   SNES -> DS: Copy .srm as SuperMetroidDS.sav alongside the .nds
 *   DS -> SNES: Copy SuperMetroidDS.sav as .srm
 *
 * Backed by libfat filesystem (SD card on flash carts, emulator FS).
 * Falls back to in-memory-only (no persistence) if FAT unavailable.
 */

#include "save.h"
#include <nds.h>
#include <nds/dldi.h>
#include <fat.h>
#include <string.h>
#include <stdio.h>

/* ========================================================================
 * SNES SRAM Geometry
 * ======================================================================== */

#define SNES_SRAM_SIZE  0x2000   /* 8KB total */
#define SNES_SLOT_SIZE  0x065C   /* 1628 bytes per slot */

/* Slot start offsets within SRAM */
#define SNES_SLOT_0     0x0010
#define SNES_SLOT_1     0x066C
#define SNES_SLOT_2     0x0CC8

/* Checksum storage (2 bytes per slot: hi, lo) */
#define CHK_PRIMARY     0x0000   /* slots at +0, +2, +4 */
#define COMP_PRIMARY    0x0008   /* complements at +0, +2, +4 */
#define CHK_REDUNDANT   0x1FF0
#define COMP_REDUNDANT  0x1FF8

/* ========================================================================
 * Within-Slot Offsets (SNES save buffer $7E:D7C0 base)
 *
 * The slot is a copy of RAM $7E:D7C0-$7E:DE1B.
 * First 96 bytes (0x00-0x5F) = Samus data ($7E:09A2-$7E:0A01)
 * ======================================================================== */

/* Equipment */
#define S_EQUIPPED_ITEMS   0x00   /* uint16 bitmask */
#define S_COLLECTED_ITEMS  0x02
#define S_EQUIPPED_BEAMS   0x04
#define S_COLLECTED_BEAMS  0x06

/* Controller mappings (11 words) */
#define S_CTRL_UP          0x08
#define S_CTRL_DOWN        0x0A
#define S_CTRL_LEFT        0x0C
#define S_CTRL_RIGHT       0x0E
#define S_CTRL_SHOT        0x10
#define S_CTRL_JUMP        0x12
#define S_CTRL_DASH        0x14
#define S_CTRL_ITEM_CANCEL 0x16
#define S_CTRL_ITEM_SELECT 0x18
#define S_CTRL_ANGLE_DOWN  0x1A
#define S_CTRL_ANGLE_UP    0x1C

/* Misc */
#define S_RESERVE_MODE     0x1E   /* 1=auto, 2=manual */

/* Player stats */
#define S_HP               0x20
#define S_HP_MAX           0x22
#define S_MISSILES         0x24
#define S_MISSILES_MAX     0x26
#define S_SUPERS           0x28
#define S_SUPERS_MAX       0x2A
#define S_PB               0x2C
#define S_PB_MAX           0x2E
#define S_HUD_ITEM         0x30   /* Selected HUD weapon */
#define S_RESERVE_MAX      0x32
#define S_RESERVE_HP       0x34

/* Time */
#define S_TIME_FRAMES      0x38
#define S_TIME_SECONDS     0x3A
#define S_TIME_MINUTES     0x3C
#define S_TIME_HOURS       0x3E

/* Event/boss data (after Samus data block) */
#define S_EVENTS           0x60   /* 8 bytes of event flags */
#define S_BOSSES           0x68   /* 7 areas * 2 bytes = 14 bytes */

/* Item/door collection bits */
#define S_ITEM_BITS        0xB0   /* ~24 bytes */
#define S_DOOR_BITS        0xF0   /* ~24 bytes */

/* Game state / position */
#define S_GAME_STATE       0x154  /* $0998: 5=normal gameplay */
#define S_SAVE_STATION     0x156
#define S_AREA_ID          0x158

/* Compressed map data (rest of slot) */
#define S_MAP_DATA         0x15C  /* through 0x065B */

/* ========================================================================
 * Static Data
 * ======================================================================== */

#define SAVE_FILE_NAME "SuperMetroidDS.sav"

static const uint16_t slot_offsets[3] = {
    SNES_SLOT_0, SNES_SLOT_1, SNES_SLOT_2
};

/* In-memory image of the full 8KB SNES SRAM.
 * Loaded from .sav file at init, flushed back on write/delete. */
static uint8_t sram_image[SNES_SRAM_SIZE];

/* Working buffer for building/reading a slot */
static uint8_t slot_buf[SNES_SLOT_SIZE];

/* Whether FAT filesystem is available for persistence */
static bool fat_available;

/* ========================================================================
 * FAT File I/O
 * ======================================================================== */

static void sram_load_from_file(void) {
    FILE* f = fopen(SAVE_FILE_NAME, "rb");
    if (f) {
        fread(sram_image, 1, SNES_SRAM_SIZE, f);
        fclose(f);
        fprintf(stderr, "save: loaded %s\n", SAVE_FILE_NAME);
    }
    /* If file doesn't exist, sram_image stays zeroed (no saves) */
}

static void sram_flush_to_file(void) {
    if (!fat_available) return;
    FILE* f = fopen(SAVE_FILE_NAME, "wb");
    if (f) {
        fwrite(sram_image, 1, SNES_SRAM_SIZE, f);
        fclose(f);
    }
}

/* ========================================================================
 * SRAM Image Access (operate on in-memory buffer)
 * ======================================================================== */

static void sram_read_bytes(uint32_t offset, void* dst, uint32_t len) {
    memcpy(dst, &sram_image[offset], len);
}

static void sram_write_bytes(uint32_t offset, const void* src, uint32_t len) {
    memcpy(&sram_image[offset], src, len);
}

static void sram_zero(uint32_t offset, uint32_t len) {
    memset(&sram_image[offset], 0, len);
}

static void sram_write_u8(uint32_t offset, uint8_t val) {
    sram_image[offset] = val;
}

static uint8_t sram_read_u8(uint32_t offset) {
    return sram_image[offset];
}

/* ========================================================================
 * Little-Endian Helpers (for slot_buf in normal RAM)
 * ======================================================================== */

static void buf_put_u16(uint16_t offset, uint16_t val) {
    slot_buf[offset]     = val & 0xFF;
    slot_buf[offset + 1] = (val >> 8) & 0xFF;
}

static uint16_t buf_get_u16(uint16_t offset) {
    return (uint16_t)slot_buf[offset] | ((uint16_t)slot_buf[offset + 1] << 8);
}

/* ========================================================================
 * SNES Checksum (alternating-byte accumulator)
 *
 * Even bytes accumulate in 'high', odd bytes in 'low'.
 * Overflow from high carries into low; overflow from low is discarded.
 * ======================================================================== */

static void snes_checksum(const uint8_t* data, uint32_t len,
                           uint8_t* out_hi, uint8_t* out_lo) {
    uint16_t high = 0, low = 0;

    for (uint32_t i = 0; i < len; i += 2) {
        high += data[i];
        if (high > 0xFF) {
            high &= 0xFF;
            low++;
        }
        if (i + 1 < len) {
            low += data[i + 1];
            if (low > 0xFF) {
                low &= 0xFF;
            }
        }
    }

    *out_hi = (uint8_t)high;
    *out_lo = (uint8_t)low;
}

/* Write checksum + complement to all 4 SRAM locations */
static void write_checksums(int slot, uint8_t chk_hi, uint8_t chk_lo) {
    uint8_t comp_hi = chk_hi ^ 0xFF;
    uint8_t comp_lo = chk_lo ^ 0xFF;

    /* Primary pair */
    sram_write_u8(CHK_PRIMARY  + slot * 2,     chk_hi);
    sram_write_u8(CHK_PRIMARY  + slot * 2 + 1, chk_lo);
    sram_write_u8(COMP_PRIMARY + slot * 2,     comp_hi);
    sram_write_u8(COMP_PRIMARY + slot * 2 + 1, comp_lo);

    /* Redundant pair */
    sram_write_u8(CHK_REDUNDANT  + slot * 2,     chk_hi);
    sram_write_u8(CHK_REDUNDANT  + slot * 2 + 1, chk_lo);
    sram_write_u8(COMP_REDUNDANT + slot * 2,     comp_hi);
    sram_write_u8(COMP_REDUNDANT + slot * 2 + 1, comp_lo);
}

/* Validate checksum for a slot. Fills slot_buf on success. */
static bool validate_checksum(int slot) {
    sram_read_bytes(slot_offsets[slot], slot_buf, SNES_SLOT_SIZE);

    uint8_t calc_hi, calc_lo;
    snes_checksum(slot_buf, SNES_SLOT_SIZE, &calc_hi, &calc_lo);

    /* Try primary pair */
    uint8_t chk_hi  = sram_read_u8(CHK_PRIMARY  + slot * 2);
    uint8_t chk_lo  = sram_read_u8(CHK_PRIMARY  + slot * 2 + 1);
    uint8_t comp_hi = sram_read_u8(COMP_PRIMARY + slot * 2);
    uint8_t comp_lo = sram_read_u8(COMP_PRIMARY + slot * 2 + 1);

    if (chk_hi == calc_hi && chk_lo == calc_lo &&
        comp_hi == (uint8_t)(calc_hi ^ 0xFF) &&
        comp_lo == (uint8_t)(calc_lo ^ 0xFF)) {
        return true;
    }

    /* Try redundant pair */
    chk_hi  = sram_read_u8(CHK_REDUNDANT  + slot * 2);
    chk_lo  = sram_read_u8(CHK_REDUNDANT  + slot * 2 + 1);
    comp_hi = sram_read_u8(COMP_REDUNDANT + slot * 2);
    comp_lo = sram_read_u8(COMP_REDUNDANT + slot * 2 + 1);

    if (chk_hi == calc_hi && chk_lo == calc_lo &&
        comp_hi == (uint8_t)(calc_hi ^ 0xFF) &&
        comp_lo == (uint8_t)(calc_lo ^ 0xFF)) {
        return true;
    }

    return false;
}

/* ========================================================================
 * Equipment Conversion: our EQUIP_* flags <-> SNES items + beams
 * ======================================================================== */

/* SNES item bits ($09A2) */
#define SNES_ITEM_VARIA     0x0001
#define SNES_ITEM_SPRING    0x0002
#define SNES_ITEM_MORPH     0x0004
#define SNES_ITEM_SCREW     0x0008
#define SNES_ITEM_GRAVITY   0x0020
#define SNES_ITEM_HIJUMP    0x0100
#define SNES_ITEM_SPACE     0x0200
#define SNES_ITEM_BOMBS     0x1000
#define SNES_ITEM_SPEED     0x2000
#define SNES_ITEM_GRAPPLE   0x4000
#define SNES_ITEM_XRAY      0x8000

/* SNES beam bits ($09A6) */
#define SNES_BEAM_WAVE      0x0001
#define SNES_BEAM_ICE       0x0002
#define SNES_BEAM_SPAZER    0x0004
#define SNES_BEAM_PLASMA    0x0008
#define SNES_BEAM_CHARGE    0x1000

static void equip_to_snes(uint32_t equip, uint16_t* items, uint16_t* beams) {
    uint16_t i = 0, b = 0;

    if (equip & EQUIP_VARIA_SUIT)   i |= SNES_ITEM_VARIA;
    if (equip & EQUIP_SPRING_BALL)  i |= SNES_ITEM_SPRING;
    if (equip & EQUIP_MORPH_BALL)   i |= SNES_ITEM_MORPH;
    if (equip & EQUIP_SCREW_ATTACK) i |= SNES_ITEM_SCREW;
    if (equip & EQUIP_GRAVITY_SUIT) i |= SNES_ITEM_GRAVITY;
    if (equip & EQUIP_HI_JUMP)     i |= SNES_ITEM_HIJUMP;
    if (equip & EQUIP_SPACE_JUMP)   i |= SNES_ITEM_SPACE;
    if (equip & EQUIP_BOMBS)        i |= SNES_ITEM_BOMBS;
    if (equip & EQUIP_SPEED_BOOST)  i |= SNES_ITEM_SPEED;
    if (equip & EQUIP_GRAPPLE)      i |= SNES_ITEM_GRAPPLE;
    if (equip & EQUIP_XRAY)         i |= SNES_ITEM_XRAY;

    if (equip & EQUIP_WAVE_BEAM)    b |= SNES_BEAM_WAVE;
    if (equip & EQUIP_ICE_BEAM)     b |= SNES_BEAM_ICE;
    if (equip & EQUIP_SPAZER_BEAM)  b |= SNES_BEAM_SPAZER;
    if (equip & EQUIP_PLASMA_BEAM)  b |= SNES_BEAM_PLASMA;
    if (equip & EQUIP_CHARGE_BEAM)  b |= SNES_BEAM_CHARGE;

    *items = i;
    *beams = b;
}

static uint32_t snes_to_equip(uint16_t items, uint16_t beams) {
    uint32_t e = 0;

    if (items & SNES_ITEM_VARIA)   e |= EQUIP_VARIA_SUIT;
    if (items & SNES_ITEM_SPRING)  e |= EQUIP_SPRING_BALL;
    if (items & SNES_ITEM_MORPH)   e |= EQUIP_MORPH_BALL;
    if (items & SNES_ITEM_SCREW)   e |= EQUIP_SCREW_ATTACK;
    if (items & SNES_ITEM_GRAVITY) e |= EQUIP_GRAVITY_SUIT;
    if (items & SNES_ITEM_HIJUMP)  e |= EQUIP_HI_JUMP;
    if (items & SNES_ITEM_SPACE)   e |= EQUIP_SPACE_JUMP;
    if (items & SNES_ITEM_BOMBS)   e |= EQUIP_BOMBS;
    if (items & SNES_ITEM_SPEED)   e |= EQUIP_SPEED_BOOST;
    if (items & SNES_ITEM_GRAPPLE) e |= EQUIP_GRAPPLE;
    if (items & SNES_ITEM_XRAY)    e |= EQUIP_XRAY;

    if (beams & SNES_BEAM_WAVE)    e |= EQUIP_WAVE_BEAM;
    if (beams & SNES_BEAM_ICE)     e |= EQUIP_ICE_BEAM;
    if (beams & SNES_BEAM_SPAZER)  e |= EQUIP_SPAZER_BEAM;
    if (beams & SNES_BEAM_PLASMA)  e |= EQUIP_PLASMA_BEAM;
    if (beams & SNES_BEAM_CHARGE)  e |= EQUIP_CHARGE_BEAM;

    return e;
}

/* ========================================================================
 * Boss Flag Conversion: our flags <-> SNES per-area bytes
 *
 * SNES stores boss defeat bits per area (7 areas * 2 bytes):
 *   Crateria[2]=Bomb Torizo, Brinstar[0]=Kraid [1]=Spore Spawn,
 *   Norfair[0]=Ridley [1]=Crocomire [2]=Golden Torizo,
 *   Wrecked Ship[0]=Phantoon, Maridia[0]=Draygon [1]=Botwoon,
 *   Tourian[1]=Mother Brain
 * ======================================================================== */

#define AREA_BOSS_BYTES 14  /* 7 areas * 2 bytes */

static void boss_flags_to_snes(uint16_t flags, uint8_t area_bosses[AREA_BOSS_BYTES]) {
    memset(area_bosses, 0, AREA_BOSS_BYTES);

    /* Crateria (area 0) */
    if (flags & BOSS_FLAG_BOMB_TORIZO)   area_bosses[0] |= 0x04;
    /* Brinstar (area 1) */
    if (flags & BOSS_FLAG_KRAID)         area_bosses[2] |= 0x01;
    if (flags & BOSS_FLAG_SPORE_SPAWN)   area_bosses[2] |= 0x02;
    /* Norfair (area 2) */
    if (flags & BOSS_FLAG_RIDLEY)        area_bosses[4] |= 0x01;
    if (flags & BOSS_FLAG_CROCOMIRE)     area_bosses[4] |= 0x02;
    if (flags & BOSS_FLAG_GOLDEN_TORIZO) area_bosses[4] |= 0x04;
    /* Wrecked Ship (area 3) */
    if (flags & BOSS_FLAG_PHANTOON)      area_bosses[6] |= 0x01;
    /* Maridia (area 4) */
    if (flags & BOSS_FLAG_DRAYGON)       area_bosses[8] |= 0x01;
    if (flags & BOSS_FLAG_BOTWOON)       area_bosses[8] |= 0x02;
    /* Tourian (area 5) */
    if (flags & BOSS_FLAG_MOTHER_BRAIN)  area_bosses[10] |= 0x02;
}

static uint16_t snes_to_boss_flags(const uint8_t area_bosses[AREA_BOSS_BYTES]) {
    uint16_t flags = 0;

    if (area_bosses[0] & 0x04) flags |= BOSS_FLAG_BOMB_TORIZO;
    if (area_bosses[2] & 0x01) flags |= BOSS_FLAG_KRAID;
    if (area_bosses[2] & 0x02) flags |= BOSS_FLAG_SPORE_SPAWN;
    if (area_bosses[4] & 0x01) flags |= BOSS_FLAG_RIDLEY;
    if (area_bosses[4] & 0x02) flags |= BOSS_FLAG_CROCOMIRE;
    if (area_bosses[4] & 0x04) flags |= BOSS_FLAG_GOLDEN_TORIZO;
    if (area_bosses[6] & 0x01) flags |= BOSS_FLAG_PHANTOON;
    if (area_bosses[8] & 0x01) flags |= BOSS_FLAG_DRAYGON;
    if (area_bosses[8] & 0x02) flags |= BOSS_FLAG_BOTWOON;
    if (area_bosses[10] & 0x02) flags |= BOSS_FLAG_MOTHER_BRAIN;

    return flags;
}

/* ========================================================================
 * SNES Default Controller Mapping
 * ======================================================================== */

/* SNES joypad bit values */
#define SNES_BTN_A      0x0080
#define SNES_BTN_B      0x8000
#define SNES_BTN_X      0x0040
#define SNES_BTN_Y      0x4000
#define SNES_BTN_L      0x0020
#define SNES_BTN_R      0x0010
#define SNES_BTN_SELECT 0x2000

static void write_default_controls(void) {
    buf_put_u16(S_CTRL_SHOT,        SNES_BTN_Y);
    buf_put_u16(S_CTRL_JUMP,        SNES_BTN_A);
    buf_put_u16(S_CTRL_DASH,        SNES_BTN_B);
    buf_put_u16(S_CTRL_ITEM_CANCEL, SNES_BTN_X);
    buf_put_u16(S_CTRL_ITEM_SELECT, SNES_BTN_SELECT);
    buf_put_u16(S_CTRL_ANGLE_DOWN,  SNES_BTN_L);
    buf_put_u16(S_CTRL_ANGLE_UP,    SNES_BTN_R);
}

/* ========================================================================
 * Public API
 * ======================================================================== */

void save_init(void) {
    memset(sram_image, 0, SNES_SRAM_SIZE);
    fat_available = false;

    /* Only attempt FAT if DLDI driver was actually patched by a loader.
     * __dldi_start is in ARM7 WRAM (0x0380B000) -- NOT ARM9-accessible.
     * Use dldiDumpInternal() to safely copy the DLDI header via PXI.
     * The unpatched stub has disc.features == 0. Calling fatInitDefault()
     * on an unpatched DLDI can hang (e.g. melonDS without SD configured). */
    DLDI_INTERFACE dldi_copy;
    if (dldiDumpInternal(&dldi_copy) && dldi_copy.disc.features != 0) {
        fat_available = fatInitDefault();
        if (fat_available) {
            sram_load_from_file();
        }
    }

    fprintf(stderr, "save: init, persistence=%s (%u bytes/slot)\n",
            fat_available ? "FAT" : "none", SNES_SLOT_SIZE);
}

bool save_write(int slot, const SaveData* data) {
    if (slot < 0 || slot >= SAVE_SLOT_COUNT) return false;
    if (data == NULL) return false;

    /* Clear buffer to zero (unpopulated fields stay 0) */
    memset(slot_buf, 0, SNES_SLOT_SIZE);

    /* --- Equipment -> SNES items + beams --- */
    uint16_t snes_items, snes_beams;
    equip_to_snes(data->equipment, &snes_items, &snes_beams);

    /* Write both equipped and collected (identical for us) */
    buf_put_u16(S_EQUIPPED_ITEMS,  snes_items);
    buf_put_u16(S_COLLECTED_ITEMS, snes_items);
    buf_put_u16(S_EQUIPPED_BEAMS,  snes_beams);
    buf_put_u16(S_COLLECTED_BEAMS, snes_beams);

    /* Controller defaults (so SNES emulators get sensible controls) */
    write_default_controls();

    /* Reserve mode: 1=auto */
    buf_put_u16(S_RESERVE_MODE, 1);

    /* --- Player stats --- */
    buf_put_u16(S_HP,           data->hp);
    buf_put_u16(S_HP_MAX,       data->hp_max);
    buf_put_u16(S_MISSILES,     data->missiles);
    buf_put_u16(S_MISSILES_MAX, data->missiles_max);
    buf_put_u16(S_SUPERS,       data->supers);
    buf_put_u16(S_SUPERS_MAX,   data->supers_max);
    buf_put_u16(S_PB,           data->power_bombs);
    buf_put_u16(S_PB_MAX,       data->power_bombs_max);
    buf_put_u16(S_RESERVE_MAX,  data->reserve_hp_max);
    buf_put_u16(S_RESERVE_HP,   data->reserve_hp);

    /* --- Game time --- */
    buf_put_u16(S_TIME_FRAMES,  data->time_frames);
    buf_put_u16(S_TIME_SECONDS, data->time_seconds);
    buf_put_u16(S_TIME_MINUTES, data->time_minutes);
    buf_put_u16(S_TIME_HOURS,   data->time_hours);

    /* --- Boss flags -> SNES area-based bits --- */
    uint8_t area_bosses[AREA_BOSS_BYTES];
    boss_flags_to_snes(data->boss_flags, area_bosses);
    memcpy(&slot_buf[S_BOSSES], area_bosses, AREA_BOSS_BYTES);

    /* --- Game state / position --- */
    buf_put_u16(S_GAME_STATE,   5);  /* 5 = normal gameplay */
    buf_put_u16(S_SAVE_STATION, data->save_station_id);
    buf_put_u16(S_AREA_ID,      data->area_id);

    /* Item/door/map bits: left zeroed until those systems are implemented.
     * An SNES save loaded here will lose map/item/door progress, but
     * stats, equipment, position, bosses, and time are preserved. */

    /* --- Write to SRAM image --- */
    sram_write_bytes(slot_offsets[slot], slot_buf, SNES_SLOT_SIZE);

    /* Calculate and store checksums */
    uint8_t chk_hi, chk_lo;
    snes_checksum(slot_buf, SNES_SLOT_SIZE, &chk_hi, &chk_lo);
    write_checksums(slot, chk_hi, chk_lo);

    /* Flush to filesystem */
    sram_flush_to_file();

    fprintf(stderr, "save: write slot %d (chk=%02X%02X)\n",
            slot, chk_hi, chk_lo);
    return true;
}

bool save_read(int slot, SaveData* data) {
    if (slot < 0 || slot >= SAVE_SLOT_COUNT) return false;
    if (data == NULL) return false;

    /* Validate checksum (also fills slot_buf) */
    if (!validate_checksum(slot)) return false;

    memset(data, 0, sizeof(SaveData));

    /* --- SNES items + beams -> our equipment --- */
    uint16_t snes_items = buf_get_u16(S_COLLECTED_ITEMS);
    uint16_t snes_beams = buf_get_u16(S_COLLECTED_BEAMS);
    data->equipment = snes_to_equip(snes_items, snes_beams);

    /* --- Player stats --- */
    data->hp             = buf_get_u16(S_HP);
    data->hp_max         = buf_get_u16(S_HP_MAX);
    data->missiles       = buf_get_u16(S_MISSILES);
    data->missiles_max   = buf_get_u16(S_MISSILES_MAX);
    data->supers         = buf_get_u16(S_SUPERS);
    data->supers_max     = buf_get_u16(S_SUPERS_MAX);
    data->power_bombs    = buf_get_u16(S_PB);
    data->power_bombs_max = buf_get_u16(S_PB_MAX);
    data->reserve_hp     = buf_get_u16(S_RESERVE_HP);
    data->reserve_hp_max = buf_get_u16(S_RESERVE_MAX);

    /* --- Game time --- */
    data->time_frames  = buf_get_u16(S_TIME_FRAMES);
    data->time_seconds = buf_get_u16(S_TIME_SECONDS);
    data->time_minutes = buf_get_u16(S_TIME_MINUTES);
    data->time_hours   = buf_get_u16(S_TIME_HOURS);

    /* --- Boss flags --- */
    data->boss_flags = snes_to_boss_flags(&slot_buf[S_BOSSES]);

    /* --- Position --- */
    data->area_id         = buf_get_u16(S_AREA_ID);
    data->save_station_id = buf_get_u16(S_SAVE_STATION);

    return true;
}

bool save_slot_valid(int slot) {
    if (slot < 0 || slot >= SAVE_SLOT_COUNT) return false;
    return validate_checksum(slot);
}

void save_delete(int slot) {
    if (slot < 0 || slot >= SAVE_SLOT_COUNT) return;

    /* Zero the slot data */
    sram_zero(slot_offsets[slot], SNES_SLOT_SIZE);

    /* Invalidate checksums: write 0 for both checksum and complement.
     * Since complement should be chk^0xFF, storing 0 for both
     * guarantees validation fails. */
    write_checksums(slot, 0, 0);

    /* Flush to filesystem */
    sram_flush_to_file();

    fprintf(stderr, "save: delete slot %d\n", slot);
}
