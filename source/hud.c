/**
 * hud.c - HUD display (sub engine / bottom screen)
 *
 * Text-based HUD using sub engine console.
 * Displays: HP, missiles, supers, power bombs, game timer, room ID.
 * Uses ANSI escape codes for cursor positioning to avoid scroll.
 *
 * The console is initialized in main.c on sub BG3.
 * HUD writes to the top 3 rows each frame during gameplay.
 */

#include "hud.h"
#include "gameplay.h"
#include "player.h"
#include "room.h"
#include <stdio.h>

void hud_init(void) {
    /* Console already initialized in main.c */
}

void hud_update(void) {
    /* Values read directly from g_player in render */
}

void hud_render(void) {
    /* Row 0: HP and missiles */
    iprintf("\x1b[0;0H");
    iprintf("HP:%4d/%-4d M:%3d/%-3d",
            g_player.hp, g_player.hp_max,
            g_player.missiles, g_player.missiles_max);

    /* Row 1: supers and power bombs */
    iprintf("\x1b[1;0H");
    iprintf("S: %3d/%-3d PB:%3d/%-3d",
            g_player.supers, g_player.supers_max,
            g_player.power_bombs, g_player.power_bombs_max);

    /* Row 2: game timer and room ID */
    uint32_t total_secs = g_game_time_frames / 60;
    uint32_t hours = total_secs / 3600;
    uint32_t mins  = (total_secs / 60) % 60;
    uint32_t secs  = total_secs % 60;
    iprintf("\x1b[2;0H");
    iprintf("TIME:%d:%02d:%02d  RM:%d:%d",
            (int)hours, (int)mins, (int)secs,
            g_current_room.area_id, g_current_room.room_id);
}
