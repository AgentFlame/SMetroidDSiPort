/*
 * Super Metroid DSi Port - Demo
 *
 * Test harness to verify build system and basic functionality.
 * This replaces the full game initialization temporarily.
 */

#include <nds.h>
#include <stdio.h>

int main(void) {
    // Initialize console (handles all hardware setup automatically)
    consoleDemoInit();

    // Print welcome messages
    printf("Super Metroid DSi Port\n");
    printf("======================\n\n");
    printf("Build successful!\n");
    printf("Console initialized and working.\n\n");

    // Demo loop - just print Hello World periodically
    int frame_counter = 0;
    while(1) {
        // Wait for vertical blank (16.67ms frame budget)
        swiWaitForVBlank();

        frame_counter++;

        // Print "Hello World!" every 60 frames (approximately 1 second)
        if (frame_counter % 60 == 0) {
            printf("Hello World! (Frame: %d)\n", frame_counter);
        }

        // Run for 300 frames then exit (5 seconds)
        if (frame_counter >= 300) {
            printf("\nDemo complete. Exiting...\n");
            break;
        }
    }

    return 0;
}
