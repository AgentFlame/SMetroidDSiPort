/**
 * hud.c - HUD display (sub engine / bottom screen)
 *
 * Text-based HUD using sub engine console.
 * Displays: HP, missiles, supers, power bombs.
 * Uses ANSI escape codes for cursor positioning to avoid scroll.
 *
 * The console is initialized in main.c on sub BG3.
 * HUD writes to the top 2 rows each frame during gameplay.
 */

#include "hud.h"
#include "player.h"
#include <stdio.h>

void hud_init(void) {
    /* Console already initialized in main.c */
}

void hud_update(void) {
    /* Values read directly from g_player in render */
}

void hud_render(void) {
    /* Position cursor at row 0, col 0 */
    iprintf("\x1b[0;0H");
    iprintf("HP:%4d/%-4d M:%3d/%-3d",
            g_player.hp, g_player.hp_max,
            g_player.missiles, g_player.missiles_max);
    iprintf("\x1b[1;0H");
    iprintf("S: %3d/%-3d PB:%3d/%-3d",
            g_player.supers, g_player.supers_max,
            g_player.power_bombs, g_player.power_bombs_max);
}
