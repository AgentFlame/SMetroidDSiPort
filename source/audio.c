/**
 * audio.c - Audio system (M14)
 *
 * Stub implementation that tracks music/SFX state.
 * When audio assets are added to the AUDIO directory,
 * the Makefile auto-links -lmm9 and this file gets
 * upgraded to real Maxmod playback.
 *
 * Current behavior:
 * - Tracks current music ID for area-based switching
 * - SFX calls are no-ops (no soundbank)
 * - Debug output via fprintf(stderr, ...)
 */

#include "audio.h"
#include <stdio.h>

/* ========================================================================
 * State
 * ======================================================================== */

static MusicID current_music = MUSIC_NONE;

/* ========================================================================
 * Public API
 * ======================================================================== */

void audio_init(void) {
    current_music = MUSIC_NONE;
    fprintf(stderr, "audio: init (stub - no soundbank)\n");
}

void audio_play_music(MusicID id) {
    if (id < 0 || id >= MUSIC_COUNT) return;
    if (id == current_music) return;

    current_music = id;
    fprintf(stderr, "audio: play music %d\n", id);
}

void audio_stop_music(void) {
    if (current_music == MUSIC_NONE) return;

    fprintf(stderr, "audio: stop music\n");
    current_music = MUSIC_NONE;
}

void audio_play_sfx(SfxID id) {
    if (id <= SFX_NONE || id >= SFX_COUNT) return;

    fprintf(stderr, "audio: sfx %d\n", id);
    /* No-op until soundbank is built */
}

MusicID audio_get_current_music(void) {
    return current_music;
}
