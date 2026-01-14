/*
 * Super Metroid DSi Port - Main Entry Point
 *
 * Initializes all systems and runs the main game loop.
 * Target: Nintendo DSi (ARM9 @ 133 MHz, 16MB RAM)
 */

#include <nds.h>
#include "game_state.h"
#include "input.h"
#include "graphics.h"
#include "audio.h"
#include "physics.h"
#include "save.h"
#include "player.h"
#include "enemies.h"
#include "room.h"

/*
 * Main entry point
 */
int main(void) {
    // 1. Hardware initialization
    powerOn(POWER_ALL_2D);
    videoSetMode(MODE_5_2D);
    videoSetModeSub(MODE_0_2D);

    vramSetBankA(VRAM_A_MAIN_BG);
    vramSetBankC(VRAM_C_SUB_BG);

    // 2. Core systems (no dependencies)
    input_init();
    // graphics_init();  // TODO: Uncomment when graphics system is implemented
    // audio_init();     // TODO: Uncomment when audio system is implemented

    // 3. Game systems (depend on core)
    // physics_init();   // TODO: Uncomment when physics system is implemented
    save_init();

    // 4. Gameplay systems (depend on game systems)
    // player_init();    // TODO: Uncomment when player system is implemented
    // enemies_init();   // TODO: Uncomment when enemy system is implemented
    // room_init();      // TODO: Uncomment when room system is implemented

    // 5. State manager (depends on everything)
    game_state_init();
    game_state_set(STATE_TITLE);

    // 6. Main game loop (60 FPS)
    while (1) {
        // Wait for vertical blank (16.67ms frame budget)
        swiWaitForVBlank();

        // Update input state
        input_update();

        // Update current game state
        game_state_update();

        // Update audio
        // audio_update();  // TODO: Uncomment when audio system is implemented

        // Render current game state
        game_state_render();

        // Swap framebuffers
        // graphics_vsync();  // TODO: Uncomment when graphics system is implemented
    }

    // Cleanup (never reached, but good practice)
    game_state_shutdown();
    input_shutdown();
    save_shutdown();

    return 0;
}
