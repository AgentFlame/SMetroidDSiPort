#include <nds.h>
#include <stdio.h>

int main(int argc, char* argv[]) {
    defaultExceptionHandler();

    consoleDemoInit();

    iprintf("=========================\n");
    iprintf("  Super Metroid DS Port\n");
    iprintf("=========================\n\n");
    iprintf("Build system working!\n");
    iprintf("Target: NTR (original DS)\n\n");
    iprintf("Press A for response\n");
    iprintf("Press START to exit\n\n");

    fprintf(stderr, "SuperMetroidDS: ROM started\n");

    int pressCount = 0;

    while (pmMainLoop()) {
        swiWaitForVBlank();
        scanKeys();

        int pressed = keysDown();

        if (pressed & KEY_A) {
            pressCount++;
            iprintf("A pressed! Count: %d\n", pressCount);
            fprintf(stderr, "DEBUG: A count=%d\n", pressCount);
        }

        if (pressed & KEY_START) {
            iprintf("Goodbye!\n");
            fprintf(stderr, "SuperMetroidDS: Clean exit\n");
            break;
        }
    }

    return 0;
}
