/**
 * camera.h - Camera system
 *
 * Follows player with dead zone, clamped to room scroll bounds.
 * Applies scroll offsets to BG layers via graphics module.
 *
 * Implemented in: source/camera.c (M10)
 */

#ifndef CAMERA_H
#define CAMERA_H

#include "sm_types.h"

/* Camera state */
typedef struct {
    fx32 x;             /* Top-left corner X in world space (fx32) */
    fx32 y;             /* Top-left corner Y in world space (fx32) */
    fx32 target_x;      /* Desired position (before smoothing) */
    fx32 target_y;
    int  shake_frames;  /* Screen shake remaining */
    int  shake_mag;     /* Shake magnitude in pixels */
} Camera;

extern Camera g_camera;

void camera_init(void);
void camera_update(void);    /* Follow player, clamp to room bounds */
void camera_shake(int frames, int magnitude);
void camera_apply(void);     /* Write scroll offsets to graphics module */

#endif /* CAMERA_H */
