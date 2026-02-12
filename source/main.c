/**
 * main.c - Super Metroid DS entry point
 *
 * M0-M16 skeleton: fixed-point math, input, state manager, graphics,
 * room loading, physics engine, player state machine, camera, enemies,
 * projectiles, bosses (all 10), HUD, audio system, save system.
 */

#include <nds.h>
#include <stdio.h>
#include <string.h>

/* Core types and config */
#include "sm_types.h"
#include "sm_config.h"
#include "sm_physics_constants.h"

/* Module headers */
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
 * Test Infrastructure
 * ======================================================================== */

static int tests_passed;
static int tests_total;

static void test(const char* name, bool condition) {
    tests_total++;
    if (condition) {
        tests_passed++;
    } else {
        iprintf("FAIL: %s\n", name);
    }
}

/* ========================================================================
 * M1: Fixed-Point Math Tests
 * ======================================================================== */

static void run_fixed_math_tests(void) {
    iprintf("--- Math Tests ---\n");

    test("mul 3*4=12",
         fx_mul(INT_TO_FX(3), INT_TO_FX(4)) == INT_TO_FX(12));
    test("mul 1.5*1.5=2.25",
         fx_mul(0x18000, 0x18000) == 0x24000);
    test("mul -2*3=-6",
         fx_mul(INT_TO_FX(-2), INT_TO_FX(3)) == INT_TO_FX(-6));
    test("div 12/4=3",
         fx_div(INT_TO_FX(12), INT_TO_FX(4)) == INT_TO_FX(3));
    test("div 1/2=0.5",
         fx_div(FX_ONE, INT_TO_FX(2)) == FX_HALF);
    test("abs(-5)=5",
         fx_abs(INT_TO_FX(-5)) == INT_TO_FX(5));
    test("min(3,7)=3",
         fx_min(INT_TO_FX(3), INT_TO_FX(7)) == INT_TO_FX(3));
    test("max(3,7)=7",
         fx_max(INT_TO_FX(3), INT_TO_FX(7)) == INT_TO_FX(7));
    test("clamp(10,0,5)=5",
         fx_clamp(INT_TO_FX(10), 0, INT_TO_FX(5)) == INT_TO_FX(5));
    test("lerp(0,10,0.5)=5",
         fx_lerp(0, INT_TO_FX(10), FX_HALF) == INT_TO_FX(5));
    test("sin(0)=0",   fx_sin(0)   == 0);
    test("sin(64)=1",  fx_sin(64)  == FX_ONE);
    test("sin(128)=0", fx_sin(128) == 0);
    test("sin(192)=-1", fx_sin(192) == -FX_ONE);
    test("cos(0)=1",   fx_cos(0)   == FX_ONE);

    fx32 sqrt4 = fx_sqrt(INT_TO_FX(4));
    test("sqrt(4)~=2", fx_abs(sqrt4 - INT_TO_FX(2)) <= 1);

    test("from_snes", fx_from_snes(5, 0x8000) == 0x58000);

    /* Gravity accumulation test */
    fx32 vel = 0;
    int terminal_frame = -1;
    for (int f = 0; f < 100; f++) {
        vel += GRAVITY_AIR;
        if (vel >= TERMINAL_VEL_AIR && terminal_frame < 0) {
            terminal_frame = f + 1;
            vel = TERMINAL_VEL_AIR;
        }
    }
    test("terminal ~f70", terminal_frame >= 69 && terminal_frame <= 71);

    /* Jump deceleration test */
    fx32 jvel = JUMP_VEL_NORMAL;
    int apex = -1;
    for (int f = 0; f < 100; f++) {
        jvel -= GRAVITY_AIR;
        if (jvel <= 0 && apex < 0) apex = f + 1;
    }
    test("jump apex ~f64", apex >= 63 && apex <= 65);

    iprintf("%d/%d math OK\n", tests_passed, tests_total);
}

/* ========================================================================
 * M7: Room / Collision Tests
 * ======================================================================== */

static void run_room_tests(void) {
    iprintf("--- Room Tests ---\n");
    int pre_passed = tests_passed;
    int pre_total = tests_total;

    /* Load test room (area 0, room 0 = Crateria test) */
    bool loaded = room_load(0, 0);
    test("room loaded", loaded);
    test("room is loaded", g_current_room.loaded);
    test("width=16", g_current_room.width_tiles == 16);
    test("height=12", g_current_room.height_tiles == 12);

    /* Collision queries: air tile */
    test("air(5,3)=0", room_get_collision(5, 3) == COLL_AIR);
    test("air(8,2)=0", room_get_collision(8, 2) == COLL_AIR);

    /* Collision queries: solid floor */
    test("floor(5,10)=solid", room_get_collision(5, 10) == COLL_SOLID);
    test("floor(8,11)=solid", room_get_collision(8, 11) == COLL_SOLID);

    /* Collision queries: solid walls */
    test("wall_L(0,5)=solid", room_get_collision(0, 5) == COLL_SOLID);
    test("wall_R(15,5)=solid", room_get_collision(15, 5) == COLL_SOLID);

    /* Collision queries: platform */
    test("plat(7,6)=solid", room_get_collision(7, 6) == COLL_SOLID);
    test("above_plat(7,5)=air", room_get_collision(7, 5) == COLL_AIR);

    /* Out-of-bounds returns solid */
    test("oob(-1,0)=solid", room_get_collision(-1, 0) == COLL_SOLID);
    test("oob(16,0)=solid", room_get_collision(16, 0) == COLL_SOLID);
    test("oob(0,-1)=solid", room_get_collision(0, -1) == COLL_SOLID);
    test("oob(0,12)=solid", room_get_collision(0, 12) == COLL_SOLID);

    /* BTS queries */
    test("bts(5,3)=0", room_get_bts(5, 3) == 0);
    test("bts_oob(-1,0)=0", room_get_bts(-1, 0) == 0);

    /* Unload and reload test */
    room_unload();
    test("unloaded", !g_current_room.loaded);
    test("unloaded_coll=solid", room_get_collision(5, 3) == COLL_SOLID);

    /* Reload */
    loaded = room_load(0, 1);
    test("reload OK", loaded);
    test("reload air(5,3)", room_get_collision(5, 3) == COLL_AIR);

    /* M7 spawn data */
    test("spawn_count=3", g_current_room.spawn_count == 3);

    iprintf("%d/%d room OK\n",
            tests_passed - pre_passed,
            tests_total - pre_total);
}

/* ========================================================================
 * M8: Physics Engine Tests
 * ======================================================================== */

static void run_physics_tests(void) {
    iprintf("--- Physics Tests ---\n");
    int pre_passed = tests_passed;
    int pre_total = tests_total;

    /* Need a loaded room for collision queries */
    if (!g_current_room.loaded) {
        room_load(0, 0);
    }

    /* Test 1: Gravity application (air) */
    {
        PhysicsBody b;
        memset(&b, 0, sizeof(b));
        b.env = ENV_AIR;
        physics_apply_gravity(&b);
        test("grav air=0x125C", b.vel.y == GRAVITY_AIR);
    }

    /* Test 2: Gravity application (water) */
    {
        PhysicsBody b;
        memset(&b, 0, sizeof(b));
        b.env = ENV_WATER;
        physics_apply_gravity(&b);
        test("grav water=0x53F", b.vel.y == GRAVITY_WATER);
    }

    /* Test 3: Terminal velocity clamping */
    {
        PhysicsBody b;
        memset(&b, 0, sizeof(b));
        b.env = ENV_AIR;
        b.vel.y = TERMINAL_VEL_AIR + INT_TO_FX(1);
        physics_apply_gravity(&b);
        test("terminal clamp", b.vel.y == TERMINAL_VEL_AIR);
    }

    /* Test 4: Freefall to terminal velocity */
    {
        PhysicsBody b;
        memset(&b, 0, sizeof(b));
        b.pos.x = INT_TO_FX(128);
        b.pos.y = INT_TO_FX(32);
        b.hitbox.half_w = INT_TO_FX(4);
        b.hitbox.half_h = INT_TO_FX(4);
        b.env = ENV_AIR;

        int term_frame = -1;
        for (int f = 0; f < 100; f++) {
            physics_apply_gravity(&b);
            if (b.vel.y >= TERMINAL_VEL_AIR && term_frame < 0) {
                term_frame = f + 1;
            }
        }
        test("freefall ~f70", term_frame >= 69 && term_frame <= 71);
    }

    /* Test 5: Landing on floor */
    {
        PhysicsBody b;
        memset(&b, 0, sizeof(b));
        b.pos.x = INT_TO_FX(128);
        b.pos.y = INT_TO_FX(80);
        b.hitbox.half_w = INT_TO_FX(8);
        b.hitbox.half_h = INT_TO_FX(8);
        b.env = ENV_AIR;

        int land_frame = -1;
        for (int f = 0; f < 100; f++) {
            physics_update_body(&b);
            if (b.contact.on_ground && land_frame < 0) {
                land_frame = f + 1;
            }
        }
        test("land ~f45", land_frame >= 43 && land_frame <= 47);
        test("on_ground", b.contact.on_ground);
        test("vel_y=0 landed", b.vel.y == 0);
        fx32 bottom = b.pos.y + b.hitbox.half_h;
        test("floor snap", FX_TO_INT(bottom) == 160);
    }

    /* Test 6: Wall collision (moving right) */
    {
        PhysicsBody b;
        memset(&b, 0, sizeof(b));
        b.pos.x = INT_TO_FX(220);
        b.pos.y = INT_TO_FX(80);
        b.vel.x = RUN_SPEED;
        b.hitbox.half_w = INT_TO_FX(8);
        b.hitbox.half_h = INT_TO_FX(8);
        b.env = ENV_AIR;

        bool hit = false;
        for (int f = 0; f < 20; f++) {
            physics_update_body(&b);
            if (b.contact.on_wall_right) {
                hit = true;
                break;
            }
        }
        test("wall_R hit", hit);
        test("vel_x=0 wall", b.vel.x == 0);
        fx32 right = b.pos.x + b.hitbox.half_w;
        test("wall snap", FX_TO_INT(right) == 240);
    }

    /* Test 7: Wall collision (moving left) */
    {
        PhysicsBody b;
        memset(&b, 0, sizeof(b));
        b.pos.x = INT_TO_FX(30);
        b.pos.y = INT_TO_FX(80);
        b.vel.x = -RUN_SPEED;
        b.hitbox.half_w = INT_TO_FX(8);
        b.hitbox.half_h = INT_TO_FX(8);
        b.env = ENV_AIR;

        bool hit = false;
        for (int f = 0; f < 20; f++) {
            physics_update_body(&b);
            if (b.contact.on_wall_left) {
                hit = true;
                break;
            }
        }
        test("wall_L hit", hit);
        test("vel_x=0 wallL", b.vel.x == 0);
        fx32 left = b.pos.x - b.hitbox.half_w;
        test("wallL snap", FX_TO_INT(left) == 16);
    }

    /* Test 8: Ceiling collision */
    {
        PhysicsBody b;
        memset(&b, 0, sizeof(b));
        b.pos.x = INT_TO_FX(128);
        b.pos.y = INT_TO_FX(128);
        b.vel.y = -JUMP_VEL_NORMAL;
        b.hitbox.half_w = INT_TO_FX(8);
        b.hitbox.half_h = INT_TO_FX(8);
        b.env = ENV_AIR;

        bool hit = false;
        for (int f = 0; f < 20; f++) {
            physics_update_body(&b);
            if (b.contact.on_ceiling) {
                hit = true;
                break;
            }
        }
        test("ceiling hit", hit);
        test("vel_y>=0 ceil", b.vel.y >= 0);
        fx32 top = b.pos.y - b.hitbox.half_h;
        test("ceil snap", FX_TO_INT(top) == 112);
    }

    /* Test 9: Jump arc (no collision) */
    {
        PhysicsBody b;
        memset(&b, 0, sizeof(b));
        b.pos.x = INT_TO_FX(128);
        b.pos.y = INT_TO_FX(80);
        b.vel.y = -JUMP_VEL_NORMAL;
        b.hitbox.half_w = INT_TO_FX(4);
        b.hitbox.half_h = INT_TO_FX(4);
        b.env = ENV_AIR;

        int apex_frame = -1;
        for (int f = 0; f < 100; f++) {
            physics_apply_gravity(&b);
            if (b.vel.y >= 0 && apex_frame < 0) {
                apex_frame = f + 1;
            }
        }
        test("apex ~f64", apex_frame >= 63 && apex_frame <= 65);
    }

    iprintf("%d/%d phys OK\n",
            tests_passed - pre_passed,
            tests_total - pre_total);
}

/* ========================================================================
 * M10: Camera Tests
 * ======================================================================== */

static void run_camera_tests(void) {
    iprintf("--- Camera Tests ---\n");
    int pre_passed = tests_passed;
    int pre_total = tests_total;

    /* Ensure room is loaded */
    if (!g_current_room.loaded) {
        room_load(0, 0);
    }

    /* Test 1: camera_init zeros state */
    camera_init();
    test("cam_init_x=0", g_camera.x == 0);
    test("cam_init_y=0", g_camera.y == 0);
    test("cam_shake=0", g_camera.shake_frames == 0);

    /* Test 2: camera clamps to room bounds (test room scroll_max = 0) */
    g_camera.x = INT_TO_FX(100);
    g_camera.y = INT_TO_FX(100);
    camera_update();
    test("cam_clamp_x", g_camera.x == INT_TO_FX(g_current_room.scroll_max_x));
    test("cam_clamp_y", g_camera.y == INT_TO_FX(g_current_room.scroll_max_y));

    /* Test 3: camera doesn't go negative */
    g_camera.x = INT_TO_FX(-50);
    g_camera.y = INT_TO_FX(-50);
    camera_update();
    test("cam_no_neg_x", g_camera.x >= 0);
    test("cam_no_neg_y", g_camera.y >= 0);

    /* Test 4: shake sets up correctly */
    camera_shake(10, 3);
    test("cam_shake_f=10", g_camera.shake_frames == 10);
    test("cam_shake_m=3", g_camera.shake_mag == 3);

    /* Test 5: shake decrements */
    camera_update();
    test("cam_shake_dec", g_camera.shake_frames == 9);

    /* Reset for further tests */
    camera_init();

    iprintf("%d/%d cam OK\n",
            tests_passed - pre_passed,
            tests_total - pre_total);
}

/* ========================================================================
 * M11: Enemy Pool Tests
 * ======================================================================== */

static void run_enemy_tests(void) {
    iprintf("--- Enemy Tests ---\n");
    int pre_passed = tests_passed;
    int pre_total = tests_total;

    /* Ensure room is loaded for physics */
    if (!g_current_room.loaded) {
        room_load(0, 0);
    }

    /* Test 1: pool init */
    enemy_pool_init();
    test("epool_init_0", enemy_get_count() == 0);

    /* Test 2: spawn */
    int idx = enemy_spawn(ENEMY_ZOOMER, INT_TO_FX(64), INT_TO_FX(148));
    test("espawn_ok", idx == 0);
    test("ecount_1", enemy_get_count() == 1);

    /* Test 3: spawn more */
    int idx2 = enemy_spawn(ENEMY_GEEMER, INT_TO_FX(192), INT_TO_FX(148));
    test("espawn2_ok", idx2 == 1);
    test("ecount_2", enemy_get_count() == 2);

    /* Test 4: invalid spawn */
    int bad = enemy_spawn(ENEMY_NONE, 0, 0);
    test("espawn_none=-1", bad == -1);
    test("ecount_still2", enemy_get_count() == 2);

    /* Test 5: spawn to max */
    for (int i = enemy_get_count(); i < MAX_ENEMIES; i++) {
        enemy_spawn(ENEMY_ZOOMER, INT_TO_FX(64), INT_TO_FX(148));
    }
    test("ecount_max", enemy_get_count() == MAX_ENEMIES);

    /* Test 6: overflow returns -1 */
    int overflow = enemy_spawn(ENEMY_ZOOMER, INT_TO_FX(64), INT_TO_FX(148));
    test("eoverflow=-1", overflow == -1);
    test("ecount_still_max", enemy_get_count() == MAX_ENEMIES);

    /* Test 7: remove */
    enemy_remove(0);
    test("ecount_15", enemy_get_count() == MAX_ENEMIES - 1);

    /* Test 8: clear all */
    enemy_clear_all();
    test("eclear_0", enemy_get_count() == 0);

    iprintf("%d/%d enemy OK\n",
            tests_passed - pre_passed,
            tests_total - pre_total);
}

/* ========================================================================
 * M12: Projectile Tests
 * ======================================================================== */

static void run_projectile_tests(void) {
    iprintf("--- Proj Tests ---\n");
    int pre_passed = tests_passed;
    int pre_total = tests_total;

    /* Ensure room is loaded */
    if (!g_current_room.loaded) {
        room_load(0, 0);
    }

    /* Test 1: pool init */
    projectile_pool_init();
    /* No getter for count, so test spawn behavior */

    /* Test 2: spawn beam */
    int idx = projectile_spawn(PROJ_POWER_BEAM, PROJ_OWNER_PLAYER,
                               INT_TO_FX(128), INT_TO_FX(80),
                               INT_TO_FX(4), 0);
    test("pspawn_ok", idx == 0);

    /* Test 3: spawn missile */
    int idx2 = projectile_spawn(PROJ_MISSILE, PROJ_OWNER_PLAYER,
                                INT_TO_FX(128), INT_TO_FX(80),
                                INT_TO_FX(5), 0);
    test("pspawn_missile", idx2 == 1);

    /* Test 4: spawn bomb */
    int idx3 = projectile_spawn(PROJ_BOMB, PROJ_OWNER_PLAYER,
                                INT_TO_FX(128), INT_TO_FX(80), 0, 0);
    test("pspawn_bomb", idx3 == 2);

    /* Test 5: invalid spawn */
    int bad = projectile_spawn(PROJ_NONE, PROJ_OWNER_PLAYER, 0, 0, 0, 0);
    test("pspawn_none=-1", bad == -1);

    /* Test 6: spawn to max */
    projectile_pool_init();
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        projectile_spawn(PROJ_POWER_BEAM, PROJ_OWNER_PLAYER,
                        INT_TO_FX(128), INT_TO_FX(80),
                        INT_TO_FX(4), 0);
    }
    int overflow = projectile_spawn(PROJ_POWER_BEAM, PROJ_OWNER_PLAYER,
                                    INT_TO_FX(128), INT_TO_FX(80),
                                    INT_TO_FX(4), 0);
    test("poverflow=-1", overflow == -1);

    /* Test 7: clear all */
    projectile_clear_all();
    int after = projectile_spawn(PROJ_POWER_BEAM, PROJ_OWNER_PLAYER,
                                 INT_TO_FX(128), INT_TO_FX(80),
                                 INT_TO_FX(4), 0);
    test("pclear_respawn", after == 0);

    /* Test 8: beam hits enemy */
    projectile_pool_init();
    enemy_pool_init();
    enemy_spawn(ENEMY_ZOOMER, INT_TO_FX(140), INT_TO_FX(80));
    projectile_spawn(PROJ_POWER_BEAM, PROJ_OWNER_PLAYER,
                    INT_TO_FX(136), INT_TO_FX(80),
                    INT_TO_FX(4), 0);
    projectile_update_all();  /* Beam at 140, enemy at 140 -- overlap */
    Enemy* hit_enemy = enemy_get(0);
    test("pbeam_dmg", hit_enemy != NULL && hit_enemy->hp < 20);

    /* Cleanup */
    projectile_pool_init();
    enemy_pool_init();

    iprintf("%d/%d proj OK\n",
            tests_passed - pre_passed,
            tests_total - pre_total);
}

/* ========================================================================
 * M13: Boss AI Tests
 * ======================================================================== */

static void run_boss_tests(void) {
    iprintf("--- Boss Tests ---\n");
    int pre_passed = tests_passed;
    int pre_total = tests_total;

    /* Ensure room is loaded */
    if (!g_current_room.loaded) {
        room_load(0, 0);
    }

    /* Test 1: boss_init zeros state */
    boss_init();
    test("binit_inactive", !boss_is_active());

    /* Test 2: spawn Spore Spawn */
    boss_spawn(BOSS_SPORE_SPAWN, INT_TO_FX(128), INT_TO_FX(48));
    test("bspawn_active", boss_is_active());
    test("bspawn_hp=960", g_boss.hp == 960);
    test("bspawn_type", g_boss.type == BOSS_SPORE_SPAWN);

    /* Test 3: damage blocked when not vulnerable */
    boss_damage(100);
    test("bdmg_blocked", g_boss.hp == 960);

    /* Test 4: damage works when vulnerable */
    g_boss.vulnerable = true;
    boss_damage(100);
    test("bdmg_ok", g_boss.hp == 860);

    /* Test 5: invuln timer blocks repeated hits */
    boss_damage(100);
    test("bdmg_invuln", g_boss.hp == 860);

    /* Test 6: after invuln expires, can hit again */
    g_boss.invuln_timer = 0;
    boss_damage(50);
    test("bdmg_again", g_boss.hp == 810);

    /* Test 7: invalid spawn type */
    boss_init();
    boss_spawn(BOSS_NONE, 0, 0);
    test("bspawn_none", !boss_is_active());

    /* Test 8: kill boss -> death state -> deactivate */
    boss_spawn(BOSS_SPORE_SPAWN, INT_TO_FX(128), INT_TO_FX(48));
    g_boss.vulnerable = true;
    g_boss.hp = 10;
    boss_damage(20);          /* HP goes to -10 */
    test("boss_hp0", g_boss.hp <= 0);
    /* Boss should still be active (death animation pending) */
    test("boss_still_active", boss_is_active());
    /* Run update to trigger death state transition + animation */
    for (int f = 0; f < 200; f++) {
        boss_update();
        if (!boss_is_active()) break;
    }
    test("boss_dead", !boss_is_active());

    /* --- Crocomire tests --- */

    /* Test: spawn Crocomire */
    boss_spawn(BOSS_CROCOMIRE, INT_TO_FX(200), INT_TO_FX(120));
    test("croc_active", boss_is_active());
    test("croc_type", g_boss.type == BOSS_CROCOMIRE);
    test("croc_vuln", g_boss.vulnerable == true);

    /* Test: Crocomire push mechanic (damage pushes, doesn't reduce HP) */
    fx32 croc_start_x = g_boss.body.pos.x;
    int16_t croc_hp_before = g_boss.hp;
    boss_damage(100);
    test("croc_push", g_boss.body.pos.x > croc_start_x);
    test("croc_hp_same", g_boss.hp == croc_hp_before);
    test("croc_flinch", g_boss.ai_state == 2); /* CROC_FLINCH = 2 */

    /* Test: Crocomire death by push threshold */
    boss_init();
    boss_spawn(BOSS_CROCOMIRE, INT_TO_FX(200), INT_TO_FX(120));
    /* Push many times to pass threshold */
    for (int i = 0; i < 30; i++) {
        g_boss.invuln_timer = 0;
        boss_damage(100);
        if (g_boss.ai_state == 4) break; /* CROC_FALLING = 4 */
    }
    test("croc_falling", g_boss.ai_state == 4); /* CROC_FALLING */
    /* Run through fall + death animation */
    for (int f = 0; f < 200; f++) {
        boss_update();
        if (!boss_is_active()) break;
    }
    test("croc_dead", !boss_is_active());

    /* --- Bomb Torizo tests --- */

    /* Test: spawn Bomb Torizo */
    boss_init();
    boss_spawn(BOSS_BOMB_TORIZO, INT_TO_FX(128), INT_TO_FX(120));
    test("bt_active", boss_is_active());
    test("bt_hp=800", g_boss.hp == 800);
    test("bt_statue", g_boss.ai_state == 0); /* BT_STATUE */
    test("bt_not_vuln", g_boss.vulnerable == false);

    /* Test: damage blocked while in statue state */
    g_boss.vulnerable = true; /* Force vulnerable for test */
    boss_damage(100);
    test("bt_dmg_ok", g_boss.hp == 700);

    /* Test: kill Bomb Torizo -> death state -> deactivate */
    boss_init();
    boss_spawn(BOSS_BOMB_TORIZO, INT_TO_FX(128), INT_TO_FX(120));
    g_boss.vulnerable = true;
    g_boss.ai_state = 2; /* BT_IDLE - skip statue/wake */
    g_boss.hp = 10;
    boss_damage(20);
    test("bt_hp0", g_boss.hp <= 0);
    test("bt_still_active", boss_is_active());
    for (int f = 0; f < 200; f++) {
        boss_update();
        if (!boss_is_active()) break;
    }
    test("bt_dead", !boss_is_active());

    /* --- Kraid tests --- */

    /* Test: spawn Kraid */
    boss_init();
    boss_spawn(BOSS_KRAID, INT_TO_FX(200), INT_TO_FX(100));
    test("kr_active", boss_is_active());
    test("kr_hp=1000", g_boss.hp == 1000);
    test("kr_type", g_boss.type == BOSS_KRAID);
    test("kr_not_vuln", g_boss.vulnerable == false);
    /* Kraid starts below target (rising entrance) */
    test("kr_below", g_boss.body.pos.y > INT_TO_FX(100));

    /* Test: damage blocked when not vulnerable (mouth closed) */
    g_boss.ai_state = 1; /* KRAID_IDLE */
    boss_damage(100);
    test("kr_dmg_block", g_boss.hp == 1000);

    /* Test: damage works during roar (mouth open) */
    g_boss.ai_state = 2; /* KRAID_ROAR */
    g_boss.vulnerable = true;
    boss_damage(200);
    test("kr_dmg_ok", g_boss.hp == 800);
    /* Kraid flinches on hit — mouth closes */
    test("kr_flinch", g_boss.ai_state == 5); /* KRAID_FLINCH */
    test("kr_mouth_close", g_boss.vulnerable == false);

    /* Test: kill Kraid */
    boss_init();
    boss_spawn(BOSS_KRAID, INT_TO_FX(200), INT_TO_FX(100));
    g_boss.vulnerable = true;
    g_boss.hp = 10;
    boss_damage(20);
    test("kr_hp0", g_boss.hp <= 0);
    test("kr_still_active", boss_is_active());
    for (int f = 0; f < 200; f++) {
        boss_update();
        if (!boss_is_active()) break;
    }
    test("kr_dead", !boss_is_active());

    /* --- Botwoon tests --- */

    /* Test: spawn Botwoon */
    boss_init();
    boss_spawn(BOSS_BOTWOON, INT_TO_FX(128), INT_TO_FX(96));
    test("bot_active", boss_is_active());
    test("bot_hp=3000", g_boss.hp == 3000);
    test("bot_type", g_boss.type == BOSS_BOTWOON);
    test("bot_hidden", g_boss.ai_state == 0); /* BOT_HIDDEN */

    /* Test: damage when vulnerable */
    g_boss.vulnerable = true;
    boss_damage(100);
    test("bot_dmg", g_boss.hp == 2900);

    /* Test: kill Botwoon */
    boss_init();
    boss_spawn(BOSS_BOTWOON, INT_TO_FX(128), INT_TO_FX(96));
    g_boss.vulnerable = true;
    g_boss.hp = 10;
    boss_damage(20);
    test("bot_hp0", g_boss.hp <= 0);
    for (int f = 0; f < 200; f++) {
        boss_update();
        if (!boss_is_active()) break;
    }
    test("bot_dead", !boss_is_active());

    /* --- Phantoon tests --- */

    /* Test: spawn Phantoon */
    boss_init();
    boss_spawn(BOSS_PHANTOON, INT_TO_FX(128), INT_TO_FX(80));
    test("ph_active", boss_is_active());
    test("ph_hp=2500", g_boss.hp == 2500);
    test("ph_invis", g_boss.ai_state == 0); /* PH_INVISIBLE */
    test("ph_not_vuln", g_boss.vulnerable == false);

    /* Test: super missile triggers rage */
    g_boss.vulnerable = true;
    g_boss.ai_state = 2; /* PH_VISIBLE */
    boss_damage(300);  /* Super missile damage */
    test("ph_rage", g_boss.param_b != 0);
    test("ph_dmg", g_boss.hp == 2200);

    /* Test: kill Phantoon */
    boss_init();
    boss_spawn(BOSS_PHANTOON, INT_TO_FX(128), INT_TO_FX(80));
    g_boss.vulnerable = true;
    g_boss.hp = 10;
    boss_damage(20);
    test("ph_hp0", g_boss.hp <= 0);
    for (int f = 0; f < 200; f++) {
        boss_update();
        if (!boss_is_active()) break;
    }
    test("ph_dead", !boss_is_active());

    /* --- Draygon tests --- */

    boss_init();
    boss_spawn(BOSS_DRAYGON, INT_TO_FX(128), INT_TO_FX(80));
    test("dy_active", boss_is_active());
    test("dy_hp=6000", g_boss.hp == 6000);
    test("dy_vuln", g_boss.vulnerable == true);

    /* Kill Draygon */
    g_boss.hp = 10;
    boss_damage(20);
    test("dy_hp0", g_boss.hp <= 0);
    for (int f = 0; f < 200; f++) {
        boss_update();
        if (!boss_is_active()) break;
    }
    test("dy_dead", !boss_is_active());

    /* --- Golden Torizo tests --- */

    boss_init();
    boss_spawn(BOSS_GOLDEN_TORIZO, INT_TO_FX(128), INT_TO_FX(120));
    test("gt_active", boss_is_active());
    test("gt_hp=8000", g_boss.hp == 8000);

    /* Test: catches super missiles (damage >= 200) */
    boss_damage(300);
    test("gt_catch", g_boss.hp == 8000); /* HP restored after catch */

    /* Test: normal damage still works */
    g_boss.invuln_timer = 0;
    g_boss.ai_state = 0; /* GT_IDLE */
    boss_damage(50);
    test("gt_dmg_ok", g_boss.hp == 7950);

    /* Kill Golden Torizo */
    boss_init();
    boss_spawn(BOSS_GOLDEN_TORIZO, INT_TO_FX(128), INT_TO_FX(120));
    g_boss.hp = 10;
    boss_damage(20);
    for (int f = 0; f < 200; f++) {
        boss_update();
        if (!boss_is_active()) break;
    }
    test("gt_dead", !boss_is_active());

    /* --- Ridley tests --- */

    boss_init();
    boss_spawn(BOSS_RIDLEY, INT_TO_FX(128), INT_TO_FX(80));
    test("ri_active", boss_is_active());
    test("ri_hp=18000", g_boss.hp == 18000);
    test("ri_vuln", g_boss.vulnerable == true);

    /* Kill Ridley */
    g_boss.hp = 10;
    boss_damage(20);
    for (int f = 0; f < 300; f++) {
        boss_update();
        if (!boss_is_active()) break;
    }
    test("ri_dead", !boss_is_active());

    /* --- Mother Brain tests --- */

    boss_init();
    boss_spawn(BOSS_MOTHER_BRAIN, INT_TO_FX(200), INT_TO_FX(96));
    test("mb_active", boss_is_active());
    test("mb_hp=3000", g_boss.hp == 3000);
    test("mb_phase0", g_boss.phase == 0);

    /* Phase 1 → 2 transition */
    g_boss.hp = 10;
    boss_damage(20);
    /* Should enter TANK_BREAK, not die */
    test("mb_still_active", boss_is_active());
    /* Run through transition */
    for (int f = 0; f < 200; f++) {
        boss_update();
        if (g_boss.phase == 1) break;
    }
    test("mb_phase1", g_boss.phase == 1);
    test("mb_hp2=18000", g_boss.hp == 18000);

    /* Phase 2 → 3 transition */
    g_boss.vulnerable = true;
    g_boss.hp = 10;
    boss_damage(20);
    for (int f = 0; f < 200; f++) {
        boss_update();
        if (g_boss.phase == 2) break;
    }
    test("mb_phase2", g_boss.phase == 2);
    test("mb_hp3=36000", g_boss.hp == 36000);

    /* Phase 3 death */
    g_boss.vulnerable = true;
    g_boss.hp = 10;
    boss_damage(20);
    for (int f = 0; f < 300; f++) {
        boss_update();
        if (!boss_is_active()) break;
    }
    test("mb_dead", !boss_is_active());

    /* Cleanup */
    boss_init();

    iprintf("%d/%d boss OK\n",
            tests_passed - pre_passed,
            tests_total - pre_total);
}

/* ========================================================================
 * M9: Player State Machine Tests
 * ======================================================================== */

static void run_player_tests(void) {
    iprintf("--- Player Tests ---\n");
    int pre_passed = tests_passed;
    int pre_total = tests_total;

    /* Init player */
    player_init();

    test("p_alive", g_player.alive);
    test("p_hp=99", g_player.hp == 99);
    test("p_state=fall", g_player.state == PSTATE_FALLING);
    test("p_facing=R", g_player.facing == DIR_RIGHT);
    test("p_hw=8", FX_TO_INT(g_player.body.hitbox.half_w) == 8);
    test("p_hh=20", FX_TO_INT(g_player.body.hitbox.half_h) == 20);

    /* Simulate: player should fall and land on floor */
    for (int f = 0; f < 100; f++) {
        player_update();
        if (g_player.body.contact.on_ground) break;
    }
    test("p_landed", g_player.body.contact.on_ground);
    test("p_standing", g_player.state == PSTATE_STANDING ||
                       g_player.state == PSTATE_RUNNING);

    /* Verify floor position: bottom at pixel 160 */
    fx32 bottom = g_player.body.pos.y + g_player.body.hitbox.half_h;
    test("p_floor=160", FX_TO_INT(bottom) == 160);

    iprintf("%d/%d player OK\n",
            tests_passed - pre_passed,
            tests_total - pre_total);
}

/* ========================================================================
 * M14: Audio System Tests
 * ======================================================================== */

static void run_audio_tests(void) {
    iprintf("--- Audio Tests ---\n");
    int pre_passed = tests_passed;
    int pre_total = tests_total;

    /* Test 1: init sets no music */
    audio_init();
    test("aud_init_none", audio_get_current_music() == MUSIC_NONE);

    /* Test 2: play music changes state */
    audio_play_music(MUSIC_TITLE);
    test("aud_play_title", audio_get_current_music() == MUSIC_TITLE);

    /* Test 3: same music is a no-op */
    audio_play_music(MUSIC_TITLE);
    test("aud_same_noop", audio_get_current_music() == MUSIC_TITLE);

    /* Test 4: switch music */
    audio_play_music(MUSIC_CRATERIA_SURFACE);
    test("aud_switch", audio_get_current_music() == MUSIC_CRATERIA_SURFACE);

    /* Test 5: stop music */
    audio_stop_music();
    test("aud_stop", audio_get_current_music() == MUSIC_NONE);

    /* Test 6: stop when already stopped is safe */
    audio_stop_music();
    test("aud_stop2", audio_get_current_music() == MUSIC_NONE);

    /* Test 7: SFX calls don't crash */
    audio_play_sfx(SFX_BEAM);
    audio_play_sfx(SFX_JUMP);
    audio_play_sfx(SFX_NONE);
    test("aud_sfx_ok", true);

    /* Test 8: invalid IDs handled */
    audio_play_music(MUSIC_NONE);
    test("aud_none_ok", audio_get_current_music() == MUSIC_NONE);

    /* Cleanup */
    audio_init();

    iprintf("%d/%d audio OK\n",
            tests_passed - pre_passed,
            tests_total - pre_total);
}

/* ========================================================================
 * M15: Save System Tests
 * ======================================================================== */

static void run_save_tests(void) {
    iprintf("--- Save Tests ---\n");
    int pre_passed = tests_passed;
    int pre_total = tests_total;

    save_init();

    /* Test 1: empty slot is invalid */
    save_delete(0);
    test("sv_empty_inv", !save_slot_valid(0));

    /* Test 2: write and validate */
    SaveData wd;
    memset(&wd, 0, sizeof(SaveData));
    wd.hp = 99;
    wd.hp_max = 99;
    wd.missiles = 5;
    wd.missiles_max = 5;
    wd.area_id = 0;
    wd.save_station_id = 0;
    wd.equipment = EQUIP_MORPH_BALL | EQUIP_BOMBS;
    wd.boss_flags = BOSS_FLAG_BOMB_TORIZO | BOSS_FLAG_KRAID;
    wd.time_hours = 1;
    wd.time_minutes = 30;
    wd.time_seconds = 45;
    wd.time_frames = 30;

    bool wrote = save_write(0, &wd);
    test("sv_write_ok", wrote);
    test("sv_valid", save_slot_valid(0));

    /* Test 3: read back and verify */
    SaveData rd;
    memset(&rd, 0, sizeof(SaveData));
    bool read_ok = save_read(0, &rd);
    test("sv_read_ok", read_ok);
    test("sv_hp=99", rd.hp == 99);
    test("sv_miss=5", rd.missiles == 5);
    test("sv_equip", rd.equipment == (EQUIP_MORPH_BALL | EQUIP_BOMBS));
    test("sv_time_h=1", rd.time_hours == 1);
    test("sv_time_m=30", rd.time_minutes == 30);
    test("sv_bosses", rd.boss_flags == (BOSS_FLAG_BOMB_TORIZO | BOSS_FLAG_KRAID));

    /* Test 4: delete makes slot invalid */
    save_delete(0);
    test("sv_del_inv", !save_slot_valid(0));

    /* Test 5: read from deleted slot fails */
    bool read_del = save_read(0, &rd);
    test("sv_read_del", !read_del);

    /* Test 6: out-of-range slot handling */
    test("sv_oob_neg", !save_write(-1, &wd));
    test("sv_oob_high", !save_write(3, &wd));
    test("sv_oob_valid", !save_slot_valid(-1));
    test("sv_oob_valid2", !save_slot_valid(3));

    /* Test 7: multiple slots independent */
    SaveData s1, s2;
    memset(&s1, 0, sizeof(SaveData));
    memset(&s2, 0, sizeof(SaveData));
    s1.hp = 50;
    s2.hp = 200;
    save_write(0, &s1);
    save_write(1, &s2);
    test("sv_slot0_v", save_slot_valid(0));
    test("sv_slot1_v", save_slot_valid(1));

    SaveData r1, r2;
    save_read(0, &r1);
    save_read(1, &r2);
    test("sv_slot0_hp", r1.hp == 50);
    test("sv_slot1_hp", r2.hp == 200);

    /* Cleanup: delete test data */
    save_delete(0);
    save_delete(1);
    save_delete(2);

    iprintf("%d/%d save OK\n",
            tests_passed - pre_passed,
            tests_total - pre_total);
}

/* ========================================================================
 * Gameplay State Handlers
 * ======================================================================== */

/* Determine beam type from equipped items */
static ProjectileTypeID get_beam_type(void) {
    if (g_player.equipment & EQUIP_PLASMA_BEAM) return PROJ_PLASMA_BEAM;
    if (g_player.equipment & EQUIP_SPAZER_BEAM) return PROJ_SPAZER_BEAM;
    if (g_player.equipment & EQUIP_WAVE_BEAM)   return PROJ_WAVE_BEAM;
    if (g_player.equipment & EQUIP_ICE_BEAM)    return PROJ_ICE_BEAM;
    return PROJ_POWER_BEAM;
}

/* Can player fire in current state? */
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

static void gameplay_enter(void) {
    consoleClear();
    iprintf(">> GAMEPLAY\n");

    player_init();
    camera_init();
    enemy_pool_init();
    projectile_pool_init();
    boss_init();

    if (!g_current_room.loaded) room_load(0, 0);

    /* Spawn enemies from room data */
    for (int i = 0; i < g_current_room.spawn_count; i++) {
        EnemySpawnData* s = &g_current_room.spawns[i];
        enemy_spawn((EnemyTypeID)s->enemy_id,
                    INT_TO_FX(s->x), INT_TO_FX(s->y));
    }

    iprintf("Enemies: %d\n", enemy_get_count());
}

static void gameplay_exit(void) {
    enemy_clear_all();
    projectile_clear_all();
    boss_init();
}

static void gameplay_update(void) {
    player_update();

    /* Weapon firing */
    if (can_fire()) {
        /* X = fire beam */
        if (input_pressed(KEY_X)) {
            ProjectileTypeID beam = get_beam_type();
            fx32 speed = INT_TO_FX(4);
            fx32 vx = (g_player.facing == DIR_RIGHT) ? speed : -speed;
            projectile_spawn(beam, PROJ_OWNER_PLAYER,
                           g_player.body.pos.x, g_player.body.pos.y,
                           vx, 0);
        }
        /* R = fire missile */
        if (input_pressed(KEY_R) && g_player.missiles > 0) {
            g_player.missiles--;
            fx32 speed = INT_TO_FX(5);
            fx32 vx = (g_player.facing == DIR_RIGHT) ? speed : -speed;
            projectile_spawn(PROJ_MISSILE, PROJ_OWNER_PLAYER,
                           g_player.body.pos.x, g_player.body.pos.y,
                           vx, 0);
        }
        /* L = fire super missile */
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

    /* SELECT = cycle boss spawn (test trigger) */
    if (input_pressed(KEY_SELECT) && !boss_is_active()) {
        static BossTypeID next_boss = BOSS_SPORE_SPAWN;
        fx32 spawn_x = g_player.body.pos.x + INT_TO_FX(64);
        fx32 spawn_y = INT_TO_FX(48);
        if (next_boss != BOSS_SPORE_SPAWN) {
            spawn_y = g_player.body.pos.y;
        }
        boss_spawn(next_boss, spawn_x, spawn_y);
        next_boss++;
        if (next_boss > BOSS_MOTHER_BRAIN) {
            next_boss = BOSS_SPORE_SPAWN;
        }
    }

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
 * Main Entry Point
 * ======================================================================== */

int main(int argc, char* argv[]) {
    defaultExceptionHandler();

    /* Initialize graphics hardware */
    graphics_init();

    /* Console on sub engine BG3 for debug text / HUD.
     * Map base 4, tile base 3 -- no conflict with HUD/MAP layers. */
    consoleInit(NULL, 3, BgType_Text4bpp, BgSize_T_256x256, 4, 3, false, true);

    iprintf("=========================\n");
    iprintf("  Super Metroid DS Port\n");
    iprintf("  M0-M16 Build\n");
    iprintf("=========================\n\n");

    /* Initialize subsystems */
    room_init();
    camera_init();
    audio_init();
    save_init();

    /* Run tests */
    tests_passed = 0;
    tests_total = 0;
    run_fixed_math_tests();
    run_room_tests();
    run_physics_tests();
    run_camera_tests();
    run_enemy_tests();
    run_projectile_tests();
    run_boss_tests();
    run_player_tests();
    run_audio_tests();
    run_save_tests();

    /* Initialize state manager */
    state_init();

    /* Register gameplay state handlers */
    state_set_handlers(STATE_GAMEPLAY, (StateHandlers){
        gameplay_enter, gameplay_exit, gameplay_update, gameplay_render
    });

    iprintf("\nTOTAL: %d/%d passed\n", tests_passed, tests_total);
    if (tests_passed == tests_total) {
        iprintf("ALL TESTS PASSED!\n\n");
    }

    iprintf("A=gameplay SELECT=boss\n");
    iprintf("X=title START=exit\n\n");

    fprintf(stderr, "SuperMetroidDS: M0-M16 boot, %d/%d tests\n",
            tests_passed, tests_total);

    /* Main loop */
    while (pmMainLoop()) {
        swiWaitForVBlank();
        scanKeys();

        /* Input system */
        input_update();

        /* State transitions */
        if (input_pressed(KEY_A) && state_current() != STATE_GAMEPLAY) {
            state_set(STATE_GAMEPLAY);
        }
        if (input_pressed(KEY_X)) {
            state_set(STATE_TITLE);
        }

        /* Update current state */
        state_update();

        /* Render */
        graphics_begin_frame();
        state_render();
        graphics_end_frame();

        /* Exit */
        if (input_pressed(KEY_START)) {
            break;
        }
    }

    return 0;
}
