/**
 * gameplay.c - Gameplay state logic
 *
 * Extracted from main.c (M17f). Contains all game state handlers:
 * title, file select, gameplay, pause, death, ending.
 *
 * Also contains: door transitions, weapon firing, item pickup,
 * save station interaction, boss management.
 */

#include "gameplay.h"

#include <nds.h>
#include <stdio.h>
#include <string.h>

#include "sm_config.h"
#include "sm_physics_constants.h"
#include "fixed_math.h"
#include "input.h"
#include "graphics.h"
#include "room.h"
#include "physics.h"
#include "player.h"
#include "enemy.h"
#include "projectile.h"
#include "boss.h"
#include "camera.h"
#include "audio.h"
#include "save.h"
#include "state.h"
#include "hud.h"

/* ========================================================================
 * Global Progress
 * ======================================================================== */

uint32_t g_game_time_frames;       /* Non-static: accessed by hud.c */
static uint16_t g_boss_flags;
static int g_active_save_slot;
static bool g_boss_was_active;

/* ========================================================================
 * Weapon Helpers
 * ======================================================================== */

static ProjectileTypeID get_beam_type(void) {
    if (g_player.equipment & EQUIP_PLASMA_BEAM) return PROJ_PLASMA_BEAM;
    if (g_player.equipment & EQUIP_SPAZER_BEAM) return PROJ_SPAZER_BEAM;
    if (g_player.equipment & EQUIP_WAVE_BEAM)   return PROJ_WAVE_BEAM;
    if (g_player.equipment & EQUIP_ICE_BEAM)    return PROJ_ICE_BEAM;
    return PROJ_POWER_BEAM;
}

static bool can_fire(void) {
    switch (g_player.state) {
        case PSTATE_STANDING:
        case PSTATE_RUNNING:
        case PSTATE_JUMPING:
        case PSTATE_SPIN_JUMPING:
        case PSTATE_FALLING:
        case PSTATE_CROUCHING:
            return true;
        default:
            return false;
    }
}

/* ========================================================================
 * Door Transition State Machine (with fade)
 * ======================================================================== */

typedef enum {
    TRANS_NONE = 0,
    TRANS_FADEOUT,
    TRANS_LOAD,
    TRANS_FADEIN
} TransState;

static TransState trans_state;
static int trans_timer;
static DoorData trans_door;

#define FADE_FRAMES 8

static void gameplay_exit(void);  /* Forward declaration */

static void start_door_transition(const DoorData* door) {
    trans_door = *door;
    trans_state = TRANS_FADEOUT;
    trans_timer = FADE_FRAMES;
}

/* Returns true while transition is active */
static bool update_door_transition(void) {
    if (trans_state == TRANS_NONE) return false;

    switch (trans_state) {
        case TRANS_FADEOUT: {
            int level = -16 + (trans_timer * 16 / FADE_FRAMES);
            graphics_set_brightness(level);
            graphics_set_brightness_sub(level);
            trans_timer--;
            if (trans_timer <= 0) {
                graphics_set_brightness(-16);
                graphics_set_brightness_sub(-16);
                trans_state = TRANS_LOAD;
            }
            break;
        }

        case TRANS_LOAD: {
            /* Clear entities for room change (NOT a full state exit --
             * gameplay_initialized must stay true for pause/resume). */
            enemy_clear_all();
            projectile_clear_all();
            boss_init();

            room_load(trans_door.dest_area, trans_door.dest_room);

            g_player.body.pos.x = INT_TO_FX(trans_door.spawn_x);
            g_player.body.pos.y = INT_TO_FX(trans_door.spawn_y);
            g_player.body.vel.x = 0;
            g_player.body.vel.y = 0;

            camera_init();

            for (int i = 0; i < g_current_room.spawn_count; i++) {
                EnemySpawnData* s = &g_current_room.spawns[i];
                enemy_spawn((EnemyTypeID)s->enemy_id,
                            INT_TO_FX(s->x), INT_TO_FX(s->y));
            }

            /* Auto-spawn boss in boss rooms */
            g_boss_was_active = false;
            if (trans_door.dest_area == 0 && trans_door.dest_room == 3) {
                if (!(g_boss_flags & BOSS_FLAG_SPORE_SPAWN)) {
                    boss_spawn(BOSS_SPORE_SPAWN, INT_TO_FX(128), INT_TO_FX(48));
                    g_boss_was_active = true;
                }
            }

            fprintf(stderr, "Door -> room %d:%d spawn(%d,%d)\n",
                    trans_door.dest_area, trans_door.dest_room,
                    trans_door.spawn_x, trans_door.spawn_y);

            trans_state = TRANS_FADEIN;
            trans_timer = FADE_FRAMES;
            break;
        }

        case TRANS_FADEIN: {
            int level = -16 + ((FADE_FRAMES - trans_timer) * 16 / FADE_FRAMES);
            graphics_set_brightness(level);
            graphics_set_brightness_sub(level);
            trans_timer--;
            if (trans_timer <= 0) {
                graphics_set_brightness(0);
                graphics_set_brightness_sub(0);
                trans_state = TRANS_NONE;
            }
            break;
        }

        default:
            break;
    }
    return true;
}

/* ========================================================================
 * Gameplay State Handlers
 * ======================================================================== */

static bool gameplay_initialized;
static bool gameplay_pausing;

static void gameplay_enter(void) {
    /* Resume from pause: room already loaded, skip re-initialization */
    if (gameplay_initialized && g_current_room.loaded) {
        fprintf(stderr, "Gameplay: resumed from pause\n");
        return;
    }

    consoleClear();
    player_init();
    camera_init();
    enemy_pool_init();
    projectile_pool_init();
    boss_init();
    trans_state = TRANS_NONE;
    g_boss_was_active = false;

    if (!g_current_room.loaded) room_load(0, 0);

    /* Spawn enemies from room data */
    for (int i = 0; i < g_current_room.spawn_count; i++) {
        EnemySpawnData* s = &g_current_room.spawns[i];
        enemy_spawn((EnemyTypeID)s->enemy_id,
                    INT_TO_FX(s->x), INT_TO_FX(s->y));
    }

    /* Auto-spawn boss in boss rooms */
    if (g_current_room.area_id == 0 && g_current_room.room_id == 3) {
        if (!(g_boss_flags & BOSS_FLAG_SPORE_SPAWN)) {
            boss_spawn(BOSS_SPORE_SPAWN, INT_TO_FX(128), INT_TO_FX(48));
            g_boss_was_active = true;
        }
    }

    gameplay_initialized = true;

    fprintf(stderr, "Gameplay: room %d:%d enemies=%d\n",
            g_current_room.area_id, g_current_room.room_id,
            enemy_get_count());
}

static void gameplay_exit(void) {
    /* When pausing, preserve all game state for seamless resume */
    if (gameplay_pausing) {
        gameplay_pausing = false;
        return;
    }

    /* Full teardown for real state changes (death, ending, etc.) */
    enemy_clear_all();
    projectile_clear_all();
    boss_init();
    gameplay_initialized = false;
}

static void gameplay_update(void) {
    /* Handle active door transition */
    if (update_door_transition()) {
        return;
    }

    /* Check for death state transition */
    if (g_player.state == PSTATE_DEATH && g_player.anim.frame_timer == 0) {
        state_set(STATE_DEATH);
        return;
    }

    /* Pause */
    if (input_pressed(KEY_START) && g_player.alive) {
        gameplay_pausing = true;
        state_set(STATE_PAUSE);
        return;
    }

    /* Game timer */
    g_game_time_frames++;

    player_update();

    /* Save station interaction: UP on save tile */
    if (input_pressed(KEY_UP) && g_player.body.contact.on_ground) {
        int tx = FX_TO_INT(g_player.body.pos.x) >> TILE_SHIFT;
        int ty = FX_TO_INT(g_player.body.pos.y + g_player.body.hitbox.half_h) >> TILE_SHIFT;
        if (room_get_collision(tx, ty) == COLL_SPECIAL_SAVE) {
            SaveData sd;
            memset(&sd, 0, sizeof(sd));
            sd.hp = g_player.hp;
            sd.hp_max = g_player.hp_max;
            sd.missiles = g_player.missiles;
            sd.missiles_max = g_player.missiles_max;
            sd.supers = g_player.supers;
            sd.supers_max = g_player.supers_max;
            sd.power_bombs = g_player.power_bombs;
            sd.power_bombs_max = g_player.power_bombs_max;
            sd.equipment = g_player.equipment;
            sd.area_id = g_current_room.area_id;
            sd.save_station_id = g_current_room.room_id;
            sd.boss_flags = g_boss_flags;
            sd.time_hours = g_game_time_frames / (60 * 60 * 60);
            sd.time_minutes = (g_game_time_frames / (60 * 60)) % 60;
            sd.time_seconds = (g_game_time_frames / 60) % 60;
            sd.time_frames = g_game_time_frames % 60;
            save_write(g_active_save_slot, &sd);
            fprintf(stderr, "Saved to slot %d\n", g_active_save_slot);
        }
    }

    /* Boss death detection */
    if (g_boss_was_active && !boss_is_active()) {
        g_boss_was_active = false;
        if (g_current_room.area_id == 0 && g_current_room.room_id == 3) {
            g_boss_flags |= BOSS_FLAG_SPORE_SPAWN;
        }
        camera_shake(30, 4);
        fprintf(stderr, "Boss defeated! flags=0x%04x\n", g_boss_flags);
    }

    /* Door transition check (locked during boss fight) */
    const DoorData* door = NULL;
    if (!boss_is_active()) {
        door = room_check_door_collision(&g_player.body);
    }
    if (door) {
        bool enter = false;
        switch (door->direction) {
            case DIR_RIGHT: enter = input_held(KEY_RIGHT); break;
            case DIR_LEFT:  enter = input_held(KEY_LEFT);  break;
            case DIR_UP:    enter = input_held(KEY_UP);    break;
            case DIR_DOWN:  enter = input_held(KEY_DOWN);  break;
        }
        if (enter) {
            start_door_transition(door);
            return;
        }
    }

    /* Weapon firing */
    if (can_fire()) {
        if (input_pressed(KEY_X)) {
            ProjectileTypeID beam = get_beam_type();
            fx32 speed = INT_TO_FX(4);
            fx32 vx = (g_player.facing == DIR_RIGHT) ? speed : -speed;
            projectile_spawn(beam, PROJ_OWNER_PLAYER,
                           g_player.body.pos.x, g_player.body.pos.y,
                           vx, 0);
        }
        if (input_pressed(KEY_R) && g_player.missiles > 0) {
            g_player.missiles--;
            fx32 speed = INT_TO_FX(5);
            fx32 vx = (g_player.facing == DIR_RIGHT) ? speed : -speed;
            projectile_spawn(PROJ_MISSILE, PROJ_OWNER_PLAYER,
                           g_player.body.pos.x, g_player.body.pos.y,
                           vx, 0);
        }
        if (input_pressed(KEY_L) && g_player.supers > 0) {
            g_player.supers--;
            fx32 speed = INT_TO_FX(5);
            fx32 vx = (g_player.facing == DIR_RIGHT) ? speed : -speed;
            projectile_spawn(PROJ_SUPER_MISSILE, PROJ_OWNER_PLAYER,
                           g_player.body.pos.x, g_player.body.pos.y,
                           vx, 0);
        }
    }

    /* Bombs in morphball */
    if (g_player.state == PSTATE_MORPHBALL &&
        (g_player.equipment & EQUIP_BOMBS) &&
        input_pressed(KEY_B)) {
        projectile_spawn(PROJ_BOMB, PROJ_OWNER_PLAYER,
                       g_player.body.pos.x, g_player.body.pos.y, 0, 0);
    }

    /* Item pickup */
    ItemTypeID pickup = room_check_item_pickup(&g_player.body);
    if (pickup != ITEM_NONE) {
        switch (pickup) {
            case ITEM_ENERGY_TANK:
                g_player.hp_max += ENERGY_TANK_VALUE;
                g_player.hp = g_player.hp_max;
                break;
            case ITEM_MISSILE_TANK:
                g_player.missiles_max += 5;
                g_player.missiles += 5;
                break;
            case ITEM_SUPER_TANK:
                g_player.supers_max += 5;
                g_player.supers += 5;
                break;
            case ITEM_PB_TANK:
                g_player.power_bombs_max += 5;
                g_player.power_bombs += 5;
                break;
            case ITEM_MORPH_BALL:    g_player.equipment |= EQUIP_MORPH_BALL;   break;
            case ITEM_BOMBS:         g_player.equipment |= EQUIP_BOMBS;        break;
            case ITEM_HI_JUMP:       g_player.equipment |= EQUIP_HI_JUMP;     break;
            case ITEM_SPEED_BOOST:   g_player.equipment |= EQUIP_SPEED_BOOST;  break;
            case ITEM_VARIA_SUIT:    g_player.equipment |= EQUIP_VARIA_SUIT;   break;
            case ITEM_GRAVITY_SUIT:  g_player.equipment |= EQUIP_GRAVITY_SUIT; break;
            case ITEM_SPACE_JUMP:    g_player.equipment |= EQUIP_SPACE_JUMP;   break;
            case ITEM_SCREW_ATTACK:  g_player.equipment |= EQUIP_SCREW_ATTACK; break;
            case ITEM_CHARGE_BEAM:   g_player.equipment |= EQUIP_CHARGE_BEAM;  break;
            case ITEM_ICE_BEAM:      g_player.equipment |= EQUIP_ICE_BEAM;     break;
            case ITEM_WAVE_BEAM:     g_player.equipment |= EQUIP_WAVE_BEAM;    break;
            case ITEM_SPAZER_BEAM:   g_player.equipment |= EQUIP_SPAZER_BEAM;  break;
            case ITEM_PLASMA_BEAM:   g_player.equipment |= EQUIP_PLASMA_BEAM;  break;
            case ITEM_GRAPPLE:       g_player.equipment |= EQUIP_GRAPPLE;      break;
            case ITEM_XRAY:          g_player.equipment |= EQUIP_XRAY;         break;
            case ITEM_RESERVE_TANK:
                g_player.reserve_hp_max += RESERVE_TANK_VALUE;
                break;
            default: break;
        }
        fprintf(stderr, "Item pickup: type %d\n", pickup);
    }

    /* Crumble blocks */
    room_update_crumble_blocks();

    enemy_update_all();
    boss_update();
    projectile_update_all();
    camera_update();
}

static void gameplay_render(void) {
    camera_apply();
    player_render();
    enemy_render_all();
    boss_render();
    projectile_render_all();
    hud_render();
}

/* ========================================================================
 * Title State Handlers
 * ======================================================================== */

static void title_enter(void) {
    consoleClear();
    graphics_set_brightness(-16);
    graphics_set_brightness_sub(-16);

    iprintf("\x1b[6;4HSUPER METROID DS");
    iprintf("\x1b[10;6HPRESS START");
    iprintf("\x1b[18;2Hv0.17 - M17 Integration");

    audio_play_music(MUSIC_TITLE);
    fprintf(stderr, "STATE_TITLE entered\n");

    trans_state = TRANS_FADEIN;
    trans_timer = FADE_FRAMES;
}

static void title_exit(void) {
    consoleClear();
    graphics_set_brightness(0);
    graphics_set_brightness_sub(0);
}

static void title_update(void) {
    if (trans_state != TRANS_NONE) {
        update_door_transition();
        return;
    }

    if (input_pressed(KEY_START)) {
        state_set(STATE_FILE_SELECT);
    }
}

static void title_render(void) {
    /* Static screen */
}

/* ========================================================================
 * File Select State Handlers
 * ======================================================================== */

static int file_select_cursor;

static void file_select_enter(void) {
    consoleClear();
    file_select_cursor = 0;

    iprintf("\x1b[2;6HSELECT FILE\n\n");
    for (int i = 0; i < SAVE_SLOT_COUNT; i++) {
        if (save_slot_valid(i)) {
            SaveData sd;
            save_read(i, &sd);
            iprintf("  %c File %d: HP %d  %d:%02d\n",
                    (i == 0) ? '>' : ' ',
                    i + 1, sd.hp,
                    sd.time_hours, sd.time_minutes);
        } else {
            iprintf("  %c File %d: [empty]\n",
                    (i == 0) ? '>' : ' ', i + 1);
        }
    }
    iprintf("\n  A=Load/New  B=Back");

    fprintf(stderr, "STATE_FILE_SELECT entered\n");
}

static void file_select_exit(void) {
    consoleClear();
}

static void file_select_update(void) {
    if (input_pressed(KEY_DOWN)) {
        file_select_cursor = (file_select_cursor + 1) % SAVE_SLOT_COUNT;
    }
    if (input_pressed(KEY_UP)) {
        file_select_cursor = (file_select_cursor + SAVE_SLOT_COUNT - 1) % SAVE_SLOT_COUNT;
    }

    /* Refresh cursor display */
    for (int i = 0; i < SAVE_SLOT_COUNT; i++) {
        iprintf("\x1b[%d;2H%c", 4 + i, (i == file_select_cursor) ? '>' : ' ');
    }

    if (input_pressed(KEY_A)) {
        g_active_save_slot = file_select_cursor;
        if (save_slot_valid(file_select_cursor)) {
            SaveData sd;
            save_read(file_select_cursor, &sd);
            g_boss_flags = sd.boss_flags;
            g_game_time_frames = (uint32_t)sd.time_hours * 60 * 60 * 60
                               + (uint32_t)sd.time_minutes * 60 * 60
                               + (uint32_t)sd.time_seconds * 60
                               + sd.time_frames;
        } else {
            g_boss_flags = 0;
            g_game_time_frames = 0;
        }
        state_set(STATE_GAMEPLAY);
    }

    if (input_pressed(KEY_B)) {
        state_set(STATE_TITLE);
    }
}

static void file_select_render(void) {
    /* Static screen */
}

/* ========================================================================
 * Pause State Handlers
 * ======================================================================== */

static void pause_enter(void) {
    graphics_set_brightness(-8);
    consoleClear();
    iprintf("\x1b[2;8HPAUSED\n\n");
    iprintf("  HP:  %d / %d\n", g_player.hp, g_player.hp_max);
    iprintf("  MIS: %d / %d\n", g_player.missiles, g_player.missiles_max);
    iprintf("  SUP: %d / %d\n", g_player.supers, g_player.supers_max);
    iprintf("  PB:  %d / %d\n", g_player.power_bombs, g_player.power_bombs_max);
    iprintf("\n  Room: %d:%d\n",
            g_current_room.area_id, g_current_room.room_id);
    iprintf("\n  START = Resume");
    fprintf(stderr, "STATE_PAUSE entered\n");
}

static void pause_exit(void) {
    graphics_set_brightness(0);
    consoleClear();
}

static void pause_update(void) {
    if (input_pressed(KEY_START)) {
        state_set(STATE_GAMEPLAY);
    }
}

static void pause_render(void) {
    /* Static screen */
}

/* ========================================================================
 * Death State Handlers
 * ======================================================================== */

static void death_enter(void) {
    consoleClear();
    iprintf("\x1b[10;8HGAME OVER");
    iprintf("\x1b[12;5HPress A to continue");
    audio_stop_music();
    fprintf(stderr, "STATE_DEATH entered\n");
}

static void death_exit(void) {
    consoleClear();
}

static void death_update(void) {
    if (input_pressed(KEY_A)) {
        state_set(STATE_FILE_SELECT);
    }
}

static void death_render(void) {
    /* Static screen */
}

/* ========================================================================
 * Ending State Handlers
 * ======================================================================== */

static void ending_enter(void) {
    consoleClear();
    graphics_set_brightness(0);
    graphics_set_brightness_sub(0);

    uint32_t total_secs = g_game_time_frames / 60;
    uint32_t hours = total_secs / 3600;
    uint32_t mins  = (total_secs / 60) % 60;
    uint32_t secs  = total_secs % 60;

    iprintf("\x1b[4;6HGAME CLEAR!");
    iprintf("\x1b[8;4HPlay Time: %d:%02d:%02d",
            (int)hours, (int)mins, (int)secs);
    iprintf("\x1b[10;4HBoss Flags: 0x%04x", g_boss_flags);

    iprintf("\x1b[14;4HPress A for title");

    audio_stop_music();
    fprintf(stderr, "STATE_ENDING entered\n");
}

static void ending_exit(void) {
    consoleClear();
}

static void ending_update(void) {
    if (input_pressed(KEY_A)) {
        state_set(STATE_TITLE);
    }
}

static void ending_render(void) {
    /* Static screen */
}

/* ========================================================================
 * Public: Register All State Handlers
 * ======================================================================== */

void gameplay_register_states(void) {
    state_set_handlers(STATE_TITLE, (StateHandlers){
        title_enter, title_exit, title_update, title_render
    });
    state_set_handlers(STATE_FILE_SELECT, (StateHandlers){
        file_select_enter, file_select_exit, file_select_update, file_select_render
    });
    state_set_handlers(STATE_GAMEPLAY, (StateHandlers){
        gameplay_enter, gameplay_exit, gameplay_update, gameplay_render
    });
    state_set_handlers(STATE_PAUSE, (StateHandlers){
        pause_enter, pause_exit, pause_update, pause_render
    });
    state_set_handlers(STATE_DEATH, (StateHandlers){
        death_enter, death_exit, death_update, death_render
    });
    state_set_handlers(STATE_ENDING, (StateHandlers){
        ending_enter, ending_exit, ending_update, ending_render
    });
}
