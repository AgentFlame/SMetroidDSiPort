#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <nds.h>
#include <stdint.h>
#include <stdbool.h>

// Framebuffer dimensions
#define FRAMEBUFFER_WIDTH  256
#define FRAMEBUFFER_HEIGHT 224
#define SCREEN_TOP_HEIGHT  192
#define SCREEN_BOT_HEIGHT  32

// Background layer count (SNES Mode 1)
#define BG_LAYER_COUNT 4

// Maximum hardware sprites on DSi
#define MAX_SPRITES 128

// Sprite structure
typedef struct {
    int16_t x, y;
    uint16_t tile_index;
    uint8_t palette;
    uint8_t priority;
    bool flip_h, flip_v;
    bool visible;
} Sprite;

// Graphics initialization and core functions
void graphics_init(void);
void graphics_vsync(void);
void graphics_clear_framebuffer(uint16_t color);

// Background layer functions
void bg_load_tileset(uint8_t layer, uint16_t* tiles, uint16_t tile_count);
void bg_load_tilemap(uint8_t layer, uint16_t* tilemap, uint16_t w, uint16_t h);
void bg_set_scroll(uint8_t layer, int16_t x, int16_t y);
void bg_set_priority(uint8_t layer, uint8_t priority);
void bg_set_visible(uint8_t layer, bool visible);

// Sprite functions
void sprite_init(void);
void sprite_add(Sprite* s);
void sprite_clear_all(void);
void sprite_render_all(void);
void sprite_sort_by_priority(void);

// Palette functions (BGR555 format)
void palette_load(uint16_t* pal_data, uint16_t offset, uint16_t count);
void palette_animate(uint8_t pal_index, uint16_t* colors, uint8_t frame);
void palette_set_color(uint16_t index, uint16_t color);
uint16_t palette_get_color(uint16_t index);

// Screen effects
void effect_fade_in(uint16_t frames);
void effect_fade_out(uint16_t frames);
void effect_fade_to_color(uint16_t color, uint16_t frames);
void effect_flash(uint16_t color, uint16_t duration);
void effect_shake(uint16_t intensity, uint16_t duration);
void effect_update(void); // Call each frame to update active effects

// HUD rendering (for Agent B to implement in detail)
struct Player; // Forward declaration
void hud_init(void);
void hud_render(struct Player* p);
void hud_set_visibility(bool visible);

// Screen transitions
void transition_door(uint8_t direction, void (*callback)(void));
void transition_elevator(bool going_down, void (*callback)(void));
void transition_fade_to_black(void (*callback)(void));

// Utility functions
uint16_t rgb_to_bgr555(uint8_t r, uint8_t g, uint8_t b);
void bgr555_to_rgb(uint16_t color, uint8_t* r, uint8_t* g, uint8_t* b);

#endif // GRAPHICS_H
