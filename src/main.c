#include <nds.h>
#include <stdio.h>

int main(void) {
    consoleDemoInit();

    printf("Super Metroid DSi Port\n");
    printf("Press START to exit\n");

    while(1) {
        swiWaitForVBlank();
        scanKeys();

        int keys = keysDown();
        if(keys & KEY_START) break;
    }

    return 0;
}
