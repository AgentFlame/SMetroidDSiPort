/**
 * audio.h - Audio system (Maxmod)
 *
 * Music and SFX playback with priority system.
 * Area-based music switching on room transitions.
 *
 * Implemented in: source/audio.c (M14)
 */

#ifndef AUDIO_H
#define AUDIO_H

#include "sm_types.h"

/* Music track IDs (mapped to Maxmod bank indices) */
typedef enum {
    MUSIC_NONE = 0,
    MUSIC_TITLE,
    MUSIC_CRATERIA_SURFACE,
    MUSIC_CRATERIA_UNDERGROUND,
    MUSIC_BRINSTAR_GREEN,
    MUSIC_BRINSTAR_RED,
    MUSIC_NORFAIR_UPPER,
    MUSIC_NORFAIR_LOWER,
    MUSIC_WRECKED_SHIP,
    MUSIC_MARIDIA,
    MUSIC_TOURIAN,
    MUSIC_BOSS,
    MUSIC_MINIBOSS,
    MUSIC_ITEM_ROOM,
    MUSIC_ESCAPE,
    MUSIC_ENDING,
    MUSIC_COUNT
} MusicID;

/* SFX IDs */
typedef enum {
    SFX_NONE = 0,
    SFX_BEAM,
    SFX_MISSILE,
    SFX_SUPER_MISSILE,
    SFX_BOMB,
    SFX_POWER_BOMB,
    SFX_JUMP,
    SFX_LAND,
    SFX_DAMAGE,
    SFX_ENEMY_HIT,
    SFX_ENEMY_DEATH,
    SFX_DOOR,
    SFX_ITEM,
    SFX_SAVE,
    SFX_COUNT
} SfxID;

void audio_init(void);
void audio_play_music(MusicID id);
void audio_stop_music(void);
void audio_play_sfx(SfxID id);
MusicID audio_get_current_music(void);

#endif /* AUDIO_H */
