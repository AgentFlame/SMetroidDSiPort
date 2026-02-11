# Super Metroid Enemy Data

## Overview

Super Metroid contains:
- ~50 unique enemy types
- 4 major bosses (Kraid, Phantoon, Draygon, Ridley)
- 1 final boss (Mother Brain, 3 phases)
- ~10 mini-bosses (Torizo, Spore Spawn, Crocomire, Botwoon, etc.)

---

## Enemy Code Location

### AI Banks

| Bank | ROM Offset | Enemies |
|------|------------|---------|
| $A0 | $100000 | Enemy base routines |
| $A1 | $108000 | Enemy population handling |
| $A2 | $110000 | Gunship, elevator platforms |
| $A3 | $118000 | Metroid AI |
| $A4 | $120000 | Crocomire AI |
| $A5 | $128000 | Draygon AI |
| $A6 | $130000 | Spore Spawn AI |
| $A7 | $138000 | Ridley AI |
| $A8 | $140000 | Kraid AI |
| $A9 | $148000 | Phantoon AI |
| $AA | $150000 | Mother Brain AI |
| $B2 | $190000 | Space pirate AI |
| $B3 | $198000 | Botwoon, Torizo AI |

### Enemy Graphics

| Bank | ROM Offset | Content |
|------|------------|---------|
| $AB-$B1 | $158000-$18FFFF | Enemy sprites (7 banks) |
| $B7 | $1B8000 | Additional enemy sprites |

---

## RAM Addresses

### Enemy Data Array

Enemies are stored in RAM at $7E0F78-$7E1777:
- 32 enemy slots maximum
- 40 bytes per enemy

| Offset | Size | Description |
|--------|------|-------------|
| +$00 | 2 bytes | Enemy ID |
| +$02 | 2 bytes | X position |
| +$04 | 2 bytes | Y position |
| +$06 | 2 bytes | X subpixel |
| +$08 | 2 bytes | Y subpixel |
| +$0A | 2 bytes | X velocity |
| +$0C | 2 bytes | Y velocity |
| +$0E | 2 bytes | Current HP |
| +$10 | 2 bytes | Sprite map pointer |
| +$12 | 2 bytes | Instruction pointer |
| +$14 | 2 bytes | Timer |
| +$16 | 2 bytes | Palette |
| +$18 | 2 bytes | VRAM index |
| +$1A | 2 bytes | Layer |
| +$1C | 2 bytes | Properties |
| +$1E-$27 | varies | Enemy-specific data |

### Enemy Management

| Address | Description |
|---------|-------------|
| $7E0E4C | Last enemy index + 1 |
| $7E0E4E | Number of enemies in room |
| $7E0E50 | Number killed in room |

---

## Enemy Data Tables

### Location: Bank $B4 (ROM offset $1A0000)

Contains:
- Enemy set definitions
- Vulnerability tables
- Drop chance tables
- Base stats

### Vulnerability Table Format

| Offset | Description |
|--------|-------------|
| +$00 | Power beam damage |
| +$02 | Ice beam damage |
| +$04 | Wave beam damage |
| +$06 | Spazer damage |
| +$08 | Plasma damage |
| +$0A | Missile damage |
| +$0C | Super missile damage |
| +$0E | Power bomb damage |
| +$10 | Charge beam multiplier |

### Drop Table Format

| Offset | Probability |
|--------|-------------|
| +$00 | Nothing |
| +$02 | Small energy |
| +$04 | Large energy |
| +$06 | Missiles |
| +$08 | Super missiles |
| +$0A | Power bombs |

---

## Standard Enemy Types

### Crawlers

| Enemy | HP | Behavior |
|-------|----|---------|
| Zoomer | 4 | Crawls on surfaces (any angle) |
| Geemer | 8 | Crawls, spikes on top |
| Sciser | 30 | Fast crawler, jumps |
| Sidehopper | 25 | Jumps toward Samus |

### Flyers

| Enemy | HP | Behavior |
|-------|----|---------|
| Waver | 20 | Sine wave flight |
| Rinka | 2 | Circles around point |
| Beetom | 15 | Latches onto Samus |
| Mella | 10 | Floating, slow |

### Shooters

| Enemy | HP | Behavior |
|-------|----|---------|
| Space Pirate | 200 | Shoots, wall-climbs |
| Zebesian | 100 | Shoots, guards |
| Ki-Hunter | 70 | Flies, acid spit |
| Dessgeega | 40 | Jumps, shoots |

### Special

| Enemy | HP | Behavior |
|-------|----|---------|
| Metroid | 150 | Latches, drains energy |
| Baby Metroid | - | Follows Samus (story) |
| Torizo (Bomb) | 800 | Statue comes alive |

---

## Boss Data

### Kraid

| Property | Value |
|----------|-------|
| HP | 1000 |
| Location | Bank $A8 |
| Phases | 1 (rising entrance) |
| Attacks | Fingernails, belly spikes, roar |
| Weakness | Mouth (missiles) |
| Drops | None (Varia suit nearby) |

**AI States:**
1. Rising from floor
2. Idle (mouth closed)
3. Roaring (mouth open - vulnerable)
4. Attacking (fingernails)
5. Flinching (hit reaction)
6. Death

### Phantoon

| Property | Value |
|----------|-------|
| HP | 2500 |
| Location | Bank $A9 |
| Phases | 2 (normal, rage) |
| Attacks | Flame circles, flame waves |
| Weakness | Eye (visible only when attacking) |
| Special | Enrages if hit with super missile |

**AI States:**
1. Invisible
2. Fading in
3. Visible (attacking)
4. Fading out
5. Rage mode (faster, more flames)
6. Death

### Draygon

| Property | Value |
|----------|-------|
| HP | 6000 |
| Location | Bank $A5 |
| Phases | 1 |
| Attacks | Swoop, grab (drains HP), gunk spit |
| Weakness | Head/body (any weapon) |
| Special | Can be killed by turret electricity |

**AI States:**
1. Swimming (left/right)
2. Swooping attack
3. Grabbing Samus
4. Spitting gunk
5. Retreating
6. Death (+ turret kill variant)

### Ridley

| Property | Value |
|----------|-------|
| HP | 18000 |
| Location | Bank $A7 |
| Phases | 1 (aggression scales with HP) |
| Attacks | Tail swipe, fireballs, grab, pogo |
| Weakness | Anywhere (missiles/supers best) |
| Behavior | More aggressive at lower HP |

**AI States:**
1. Flying (pattern varies)
2. Tail swipe
3. Fireball (single)
4. Fireball (spread)
5. Grabbing Samus
6. Pogo attack
7. Death

### Mother Brain

| Property | Value |
|----------|-------|
| HP Phase 1 | 3000 |
| HP Phase 2 | 18000 |
| HP Phase 3 | 36000 |
| Location | Bank $AA |

**Phase 1 - Brain in Tank:**
- Turrets fire projectiles
- Rinkas spawn and circle
- Tank takes damage
- Cannons in floor fire

**Phase 2 - Standing:**
- Rises from tank
- Beam attacks (ring pattern)
- Bomb drops
- Eye laser

**Phase 3 - Final:**
- Hyper beam attack
- Baby Metroid intervention
- Samus receives Hyper Beam
- Final attack pattern

---

## Mini-Bosses

### Spore Spawn

| Property | Value |
|----------|-------|
| HP | 960 |
| Location | Bank $A6 |
| Attacks | Swinging, spore clouds |
| Weakness | Core (opens periodically) |

### Crocomire

| Property | Value |
|----------|-------|
| HP | Unlimited (push back) |
| Location | Bank $A4 |
| Mechanic | Push into lava pit |
| Attacks | Projectile spit, lunge |

### Botwoon

| Property | Value |
|----------|-------|
| HP | 3000 |
| Location | Bank $B3 |
| Attacks | Snake movement, spit |
| Weakness | Head |

### Golden Torizo

| Property | Value |
|----------|-------|
| HP | 8000 |
| Location | Bank $B3 |
| Attacks | Same as Bomb Torizo, faster |
| Special | Can catch super missiles |

---

## DSi Port Implementation

### Enemy Structure

```c
typedef struct {
    uint16_t enemy_id;
    int32_t x, y;           // 16.16 fixed point
    int32_t vx, vy;         // 16.16 fixed point
    int16_t hp;
    uint16_t state;
    uint16_t timer;
    uint16_t frame;
    uint8_t palette;
    uint8_t layer;
    uint16_t flags;

    // AI function pointers
    void (*update)(struct Enemy*);
    void (*render)(struct Enemy*);
    void (*on_hit)(struct Enemy*, int damage);

    // Enemy-specific data
    uint8_t data[16];
} Enemy;
```

### Enemy Manager

```c
#define MAX_ENEMIES 32

Enemy enemies[MAX_ENEMIES];
int enemy_count;

void spawn_enemy(uint16_t id, int x, int y) {
    if (enemy_count >= MAX_ENEMIES) return;

    Enemy* e = &enemies[enemy_count++];
    e->enemy_id = id;
    e->x = TO_FIXED(x);
    e->y = TO_FIXED(y);
    e->hp = get_enemy_hp(id);

    // Set up AI based on enemy type
    setup_enemy_ai(e);
}

void update_enemies(void) {
    for (int i = 0; i < enemy_count; i++) {
        enemies[i].update(&enemies[i]);
    }
}
```

### Collision Detection

```c
bool check_enemy_collision(Enemy* e, int px, int py, int pw, int ph) {
    int ex = FROM_FIXED(e->x);
    int ey = FROM_FIXED(e->y);
    int ew = get_enemy_width(e->enemy_id);
    int eh = get_enemy_height(e->enemy_id);

    return (px < ex + ew && px + pw > ex &&
            py < ey + eh && py + ph > ey);
}
```

---

## Boss Health Reference

| Boss | HP | Recommended Approach |
|------|----|--------------------|
| Bomb Torizo | 800 | Missiles to head |
| Spore Spawn | 960 | Missiles when open |
| Kraid | 1000 | Supers to mouth |
| Phantoon | 2500 | Avoid supers (rage) |
| Botwoon | 3000 | Charge beam to head |
| Crocomire | N/A | Push to lava |
| Draygon | 6000 | Grapple + turrets |
| Golden Torizo | 8000 | Charged plasma |
| Ridley | 18000 | Supers + missiles |
| Mother Brain 1 | 3000 | Missiles to tank |
| Mother Brain 2 | 18000 | Missiles/supers |
| Mother Brain 3 | 36000 | Hyper beam |
