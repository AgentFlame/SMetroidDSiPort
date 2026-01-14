#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>
#include <stdbool.h>

// Music track IDs (from SNES ROM analysis)
typedef enum {
    MUSIC_TITLE = 0x05,
    MUSIC_CERES = 0x06,
    MUSIC_ITEM_ROOM = 0x09,
    MUSIC_CRATERIA_UNDERGROUND = 0x0C,
    MUSIC_CRATERIA_SURFACE = 0x0D,
    MUSIC_BRINSTAR_GREEN = 0x0F,
    MUSIC_BRINSTAR_RED = 0x12,
    MUSIC_NORFAIR_ANCIENT = 0x15,
    MUSIC_NORFAIR_HOT = 0x18,
    MUSIC_WRECKED_SHIP_UNPOWERED = 0x1B,
    MUSIC_WRECKED_SHIP_POWERED = 0x1E,
    MUSIC_MARIDIA_EXTERIOR = 0x21,
    MUSIC_MARIDIA_INTERIOR = 0x24,
    MUSIC_TOURIAN = 0x27,
    MUSIC_BOSS_1 = 0x2A,
    MUSIC_BOSS_2 = 0x2D,
    MUSIC_RIDLEY = 0x30,
    MUSIC_ESCAPE = 0x33,
    MUSIC_ENDING = 0x36
} MusicTrack;

// Sound effect IDs (Samus)
typedef enum {
    SFX_FOOTSTEP = 0x01,
    SFX_JUMP = 0x02,
    SFX_LAND = 0x03,
    SFX_DAMAGE = 0x04,
    SFX_HEALTH_PICKUP = 0x05,
    SFX_MORPH = 0x06,
    SFX_UNMORPH = 0x07,
    SFX_WALL_JUMP = 0x08
} SamusSFX;

// Sound effect IDs (Weapons)
typedef enum {
    SFX_BEAM_SHOT = 0x10,
    SFX_CHARGE_BEAM = 0x11,
    SFX_ICE_BEAM = 0x12,
    SFX_WAVE_BEAM = 0x13,
    SFX_SPAZER = 0x14,
    SFX_PLASMA_BEAM = 0x15,
    SFX_MISSILE = 0x16,
    SFX_SUPER_MISSILE = 0x17,
    SFX_BOMB_PLACE = 0x18,
    SFX_BOMB_EXPLODE = 0x19,
    SFX_POWER_BOMB = 0x1A
} WeaponSFX;

// Sound effect IDs (Environment)
typedef enum {
    SFX_DOOR_OPEN = 0x30,
    SFX_DOOR_CLOSE = 0x31,
    SFX_ELEVATOR_START = 0x32,
    SFX_ELEVATOR_STOP = 0x33,
    SFX_SAVE_STATION = 0x34,
    SFX_ENERGY_RECHARGE = 0x35,
    SFX_LAVA_BUBBLE = 0x36,
    SFX_RAIN = 0x37
} EnvironmentSFX;

// Priority levels for SFX
typedef enum {
    SFX_PRIORITY_HIGH = 0,      // Player sounds (damage, critical events)
    SFX_PRIORITY_MEDIUM = 1,    // Weapons, enemies
    SFX_PRIORITY_LOW = 2        // Ambient, environment
} SFXPriority;

// Audio system functions
void audio_init(void);
void audio_update(void);
void audio_shutdown(void);

// Music functions
void music_play(MusicTrack track, bool loop);
void music_stop(void);
void music_pause(void);
void music_resume(void);
void music_fade_out(uint16_t frames);
void music_crossfade(MusicTrack new_track, uint16_t frames);
void music_set_volume(uint8_t volume); // 0-127

// SFX functions
void sfx_play(uint16_t sfx_id, SFXPriority priority);
void sfx_stop(uint16_t sfx_id);
void sfx_stop_all(void);
void sfx_set_volume(uint8_t volume); // 0-127

// Query functions
bool music_is_playing(void);
MusicTrack music_get_current(void);

#endif // AUDIO_H
