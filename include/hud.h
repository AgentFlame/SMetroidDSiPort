/**
 * hud.h - HUD display (sub engine / bottom screen)
 *
 * Energy, missiles, supers, power bombs, mini-map.
 * Rendered on sub engine BG layers.
 *
 * Implemented in: source/hud.c (M16)
 */

#ifndef HUD_H
#define HUD_H

#include "sm_types.h"

void hud_init(void);
void hud_update(void);   /* Refresh displayed values from player state */
void hud_render(void);   /* Commit to sub engine */

#endif /* HUD_H */
