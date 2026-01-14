#include <nds.h>
#include "converted_tiles_bin.h"

int main(void) {
    // Set up video mode 0 for 2D graphics on the main engine
    videoSetMode(MODE_0_2D);

    // VRAM bank A will be used for background tiles
    vramSetBankA(VRAM_A_MAIN_BG);

    // Get a pointer to the background graphics memory (for tiles)
    u16* bg_gfx = (u16*)BG_GFX;

    // Set up a simple palette
    // Color 0 is transparent by default
    // We will set color 1 to white
    BG_PALETTE[1] = RGB15(31, 31, 31);

    // Copy our tile data into VRAM
    // The converted_tiles_bin array is from the auto-generated header
    dmaCopy(converted_tiles_bin, bg_gfx, converted_tiles_bin_size);

    // Get a pointer to the background map memory
    u16* bg_map = (u16*)bgGetMapPtr(bgInit(0, BgType_Text4bpp, BgSize_T_256x256, 0, 0));

    // Set up a tilemap entry to display our tile
    // Tile 0 at the top-left of the screen
    bg_map[0] = 0;

    while(1) {
        swiWaitForVBlank();
    }

    return 0;
}
