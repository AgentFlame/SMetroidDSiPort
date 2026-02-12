/**
 * camera.c - Camera system
 *
 * Follows player with dead zone, clamped to room scroll bounds.
 * Screen shake for boss hits / explosions.
 * Applies scroll to BG layers: level 1:1, parallax 1:2.
 */

#include "camera.h"
#include "player.h"
#include "room.h"
#include "graphics.h"
#include "sm_config.h"

/* Dead zone: half-extents from screen center.
 * Camera won't move until player leaves this rectangle. */
#define DEAD_ZONE_HALF_X   INT_TO_FX(32)
#define DEAD_ZONE_HALF_Y   INT_TO_FX(24)

Camera g_camera;

/* Simple PRNG for screen shake (quality doesn't matter) */
static uint32_t shake_seed = 7919;

static int shake_rand(void) {
    shake_seed ^= shake_seed << 13;
    shake_seed ^= shake_seed >> 17;
    shake_seed ^= shake_seed << 5;
    return (int)(shake_seed & 0x7FFF);
}

void camera_init(void) {
    g_camera.x = 0;
    g_camera.y = 0;
    g_camera.target_x = 0;
    g_camera.target_y = 0;
    g_camera.shake_frames = 0;
    g_camera.shake_mag = 0;
}

void camera_update(void) {
    /* Screen center in world space */
    fx32 screen_cx = g_camera.x + INT_TO_FX(SCREEN_WIDTH / 2);
    fx32 screen_cy = g_camera.y + INT_TO_FX(SCREEN_HEIGHT / 2);

    /* Player offset from screen center */
    fx32 dx = g_player.body.pos.x - screen_cx;
    fx32 dy = g_player.body.pos.y - screen_cy;

    /* Only move camera when player leaves dead zone */
    if (dx > DEAD_ZONE_HALF_X) {
        g_camera.x += dx - DEAD_ZONE_HALF_X;
    } else if (dx < -DEAD_ZONE_HALF_X) {
        g_camera.x += dx + DEAD_ZONE_HALF_X;
    }

    if (dy > DEAD_ZONE_HALF_Y) {
        g_camera.y += dy - DEAD_ZONE_HALF_Y;
    } else if (dy < -DEAD_ZONE_HALF_Y) {
        g_camera.y += dy + DEAD_ZONE_HALF_Y;
    }

    /* Clamp to room scroll bounds */
    if (g_camera.x < 0) g_camera.x = 0;
    if (g_camera.y < 0) g_camera.y = 0;

    fx32 max_x = INT_TO_FX(g_current_room.scroll_max_x);
    fx32 max_y = INT_TO_FX(g_current_room.scroll_max_y);

    if (g_camera.x > max_x) g_camera.x = max_x;
    if (g_camera.y > max_y) g_camera.y = max_y;

    /* Decrement shake timer */
    if (g_camera.shake_frames > 0) {
        g_camera.shake_frames--;
    }
}

void camera_shake(int frames, int magnitude) {
    g_camera.shake_frames = frames;
    g_camera.shake_mag = magnitude;
}

void camera_apply(void) {
    int sx = FX_TO_INT(g_camera.x);
    int sy = FX_TO_INT(g_camera.y);

    /* Apply screen shake */
    if (g_camera.shake_frames > 0) {
        int mag = g_camera.shake_mag;
        if (mag > 0) {
            sx += (shake_rand() % (mag * 2 + 1)) - mag;
            sy += (shake_rand() % (mag * 2 + 1)) - mag;
        }
    }

    /* Level BG scrolls 1:1 */
    graphics_set_bg_scroll(BG_LAYER_LEVEL, sx, sy);

    /* Parallax BG scrolls at half rate */
    graphics_set_bg_scroll(BG_LAYER_PARALLAX, sx >> 1, sy >> 1);

    /* Foreground at 1:1 */
    graphics_set_bg_scroll(BG_LAYER_FG, sx, sy);
}
