/*
 * Super Metroid DSi Port - Ending State
 *
 * Credits and ending sequence.
 */

#include "game_state.h"
#include "input.h"
#include "save.h"
#include "graphics.h"
#include "audio.h"

static uint32_t ending_timer = 0;
static uint16_t credits_scroll_y = 0;

/*
 * Enter ending state
 */
void state_ending_enter(void) {
    ending_timer = 0;
    credits_scroll_y = 0;

    // TODO: Stop gameplay music
    // TODO: Start ending music
    // TODO: Load ending graphics

    // TODO: Display completion stats
    // - Playtime
    // - Item collection percentage
    // - Ending rank (based on time and items)
}

/*
 * Update ending state
 */
void state_ending_update(void) {
    ending_timer++;

    // Scroll credits
    if (ending_timer % 2 == 0) {  // Scroll every other frame
        credits_scroll_y++;
    }

    // Skip to end
    if (input_check(KEY_START, INPUT_PRESSED) ||
        input_check(KEY_A, INPUT_PRESSED)) {
        // TODO: Fast-forward to end of credits
        credits_scroll_y += 100;
    }

    // TODO: Check if credits are done scrolling
    // After credits end, return to title
    if (credits_scroll_y > 2000) {  // Arbitrary end point
        game_state_set(STATE_TITLE);
    }
}

/*
 * Render ending state
 */
void state_ending_render(void) {
    // TODO: Render ending background

    // TODO: Render completion stats at top

    // TODO: Render scrolling credits

    // TODO: Render "THE END" message at conclusion
}

/*
 * Exit ending state
 */
void state_ending_exit(void) {
    ending_timer = 0;
    credits_scroll_y = 0;

    // TODO: Stop ending music
    // TODO: Cleanup ending graphics
}
