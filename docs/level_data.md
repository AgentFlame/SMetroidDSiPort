# Super Metroid Level Data

## Overview

Super Metroid's world is organized into:
- 6 major areas + Ceres Station
- ~200 rooms total
- Rooms connect via doors
- Each room has its own tilemap, collision, enemies, and items

---

## Room Data Location

**Bank $8F** (ROM offset $078000-$07FFFF) contains all room headers and pointers.

### Room Header Structure

Each room entry in bank $8F contains:

| Offset | Size | Description |
|--------|------|-------------|
| +$00 | 1 byte | Room index |
| +$01 | 1 byte | Area index |
| +$02 | 1 byte | X position on map |
| +$03 | 1 byte | Y position on map |
| +$04 | 1 byte | Width (screens) |
| +$05 | 1 byte | Height (screens) |
| +$06 | 1 byte | Up scroller |
| +$07 | 1 byte | Down scroller |
| +$08 | 1 byte | Special graphics bitflag |
| +$09 | 2 bytes | Door list pointer |

### Room State Data

Following the header, each room has state data:
- Pointer to level data (tilemap)
- Pointer to BTS (block type/special)
- Pointer to layer 2 scroll data
- Pointer to scroll PLMs
- Pointer to PLM list
- Pointer to enemy population
- Pointer to enemy set
- Pointer to FX1 data
- Pointer to door scroll data

---

## Area Index Values

| Index | Area Name | Room Count |
|-------|-----------|------------|
| 0 | Crateria | ~25 |
| 1 | Brinstar | ~40 |
| 2 | Norfair | ~45 |
| 3 | Wrecked Ship | ~20 |
| 4 | Maridia | ~35 |
| 5 | Tourian | ~15 |
| 6 | Ceres | ~10 |
| 7 | Debug/Test | ~5 |

---

## Level Data Format

### Tilemap Structure

Level tilemaps use 16x16 metatiles. Each screen is 16x14 metatiles (256x224 pixels).

**Metatile Format (2 bytes):**
```
Byte 0: Tile index (low byte)
Byte 1: Attributes
  Bits 7-4: Tile index (high bits)
  Bit 3: V-flip
  Bit 2: H-flip
  Bits 1-0: Palette selection
```

### Level Data Compression

Level tilemaps are compressed (stored in banks $C3-$CE).

Compression format: LC_LZ2 variant (see graphics_data.md)

---

## Collision Data (BTS)

### Block Types

Each 16x16 tile has an associated collision type:

| Type | Value | Description |
|------|-------|-------------|
| Air | $00 | No collision |
| Solid | $01-$0F | Standard solid blocks |
| Slope 45° UR | $10 | Rising right slope |
| Slope 45° UL | $11 | Rising left slope |
| Slope 45° DR | $12 | Falling right slope |
| Slope 45° DL | $13 | Falling left slope |
| Gentle slope | $14-$1F | Shallower angles |
| Steep slope | $20-$2F | Steeper angles |
| Shot block | $30 | Destroyable by shooting |
| Bomb block | $31 | Destroyable by bombs |
| Power bomb block | $32 | Destroyable by power bomb |
| Super missile block | $33 | Destroyable by super missile |
| Speed block | $34 | Destroyable by speed booster |
| Crumble block | $40 | Crumbles after stepping |
| Spike | $50 | Damages Samus |
| Water | $60 | Changes physics |
| Lava | $70 | Damages + changes physics |
| Acid | $78 | More damage |
| Grapple point | $80 | Can attach grapple beam |

### BTS (Behind The Scenery) Data

Additional per-tile data for special behaviors:
- Timer values for crumble blocks
- Damage values for hazards
- Special interaction flags

---

## Scroll Data

### Scroll Regions

Rooms can have multiple scroll regions, each with different behavior:

| Scroll Type | Value | Description |
|-------------|-------|-------------|
| Red | $00 | No scroll (boundary) |
| Blue | $01 | Normal scroll allowed |
| Green | $02 | Always visible |

### Scroll Boundaries

Each screen in a room has scroll limits:
- X min/max (horizontal scroll bounds)
- Y min/max (vertical scroll bounds)

---

## Door Data

### Door Structure

Each door entry contains:

| Offset | Size | Description |
|--------|------|-------------|
| +$00 | 2 bytes | Destination room pointer |
| +$02 | 1 byte | Door bitflag |
| +$03 | 1 byte | Door direction |
| +$04 | 2 bytes | Door X cap (screen) |
| +$06 | 2 bytes | Door Y cap (screen) |
| +$08 | 2 bytes | Door X screen |
| +$0A | 2 bytes | Door Y screen |
| +$0C | 2 bytes | Distance to spawn |
| +$0E | 2 bytes | ASM pointer (door effect) |

### Door Types

| Type | Color | Opens With |
|------|-------|------------|
| Blue | Blue | Any weapon |
| Red | Red | 5 missiles |
| Green | Green | 1 super missile |
| Yellow | Yellow | 1 power bomb |
| Gray | Gray | Defeat enemies |
| Eye | - | Shoot eye sensor |

---

## PLM (Power-up/Load Module) System

### PLM Structure

PLMs are interactive objects in rooms:

| Type | Description |
|------|-------------|
| Items | Energy tanks, missiles, upgrades |
| Blocks | Shootable, bombable blocks |
| Doors | Door tiles and triggers |
| Effects | Scrolling sky, animated tiles |
| Triggers | Boss triggers, events |

### PLM Data Format

| Offset | Size | Description |
|--------|------|-------------|
| +$00 | 2 bytes | PLM ID |
| +$02 | 1 byte | X position (tiles) |
| +$03 | 1 byte | Y position (tiles) |
| +$04 | 2 bytes | PLM argument |

---

## Enemy Population

### Enemy Spawn Data

Each room has an enemy population list:

| Offset | Size | Description |
|--------|------|-------------|
| +$00 | 2 bytes | Enemy ID |
| +$02 | 2 bytes | X position |
| +$04 | 2 bytes | Y position |
| +$06 | 2 bytes | Initial parameter |
| +$08 | 2 bytes | Properties |
| +$0A | 2 bytes | Extra 1 |
| +$0C | 2 bytes | Extra 2 |
| +$0E | 2 bytes | Parameter 1 |
| +$10 | 2 bytes | Parameter 2 |

### Enemy Set

Defines which enemies can spawn in the room (for VRAM allocation).

---

## FX (Effects) Data

### FX Types

| FX | Description |
|----|-------------|
| Rain | Falling rain particles |
| Fog | Atmospheric fog layer |
| Lava | Rising/falling lava level |
| Water | Water surface effects |
| Heat | Heat wave distortion |
| Earthquake | Screen shake |

### FX1 Data Format

| Offset | Size | Description |
|--------|------|-------------|
| +$00 | 2 bytes | Door pointer (trigger) |
| +$02 | 2 bytes | FX Y position |
| +$04 | 2 bytes | FX target Y |
| +$06 | 2 bytes | FX speed |
| +$08 | 1 byte | FX timer |
| +$09 | 1 byte | FX type |
| +$0A | 1 byte | FX BG2 properties |
| +$0B | 1 byte | BG blend settings |
| +$0C | 2 bytes | FX palette FX bitset |

---

## DSi Port Room Structure

### Recommended C Structure

```c
typedef struct {
    uint8_t area_id;
    uint8_t room_id;
    uint8_t map_x, map_y;
    uint8_t width_screens;
    uint8_t height_screens;

    // Tilemap (decompressed)
    uint16_t* tilemap;      // 16x14 metatiles per screen
    uint8_t* collision;     // 1 byte per metatile

    // Scroll data
    uint8_t* scroll_data;   // Per-screen scroll type

    // Objects
    PLM* plm_list;
    uint8_t plm_count;

    // Enemies
    EnemySpawn* enemy_spawns;
    uint8_t enemy_count;

    // Doors
    Door* doors;
    uint8_t door_count;

    // FX
    RoomFX* fx;
} Room;

typedef struct {
    uint16_t plm_id;
    uint8_t x, y;
    uint16_t argument;
} PLM;

typedef struct {
    uint16_t enemy_id;
    int16_t x, y;
    uint16_t params[4];
} EnemySpawn;

typedef struct {
    uint16_t dest_room;
    uint8_t direction;
    uint16_t scroll_x, scroll_y;
    uint16_t spawn_x, spawn_y;
} Door;
```

---

## Room Loading Process

### Step-by-Step Load

1. **Load room header** from bank $8F
2. **Decompress tilemap** from banks $C3-$CE
3. **Load collision data** (BTS)
4. **Load scroll regions**
5. **Load PLM list**
6. **Load enemy population**
7. **Load enemy set** (determines graphics to load)
8. **Load door list**
9. **Load FX data**
10. **Initialize enemies** at spawn positions
11. **Activate PLMs**

### Memory Management

Only one room is fully loaded at a time. Door transitions:
1. Fade out
2. Unload current room
3. Load new room
4. Fade in

---

## Area Room Counts and Notable Rooms

### Crateria
- Landing Site (ship location)
- Bomb Torizo room
- Gauntlet entrance
- Surface (rain effect)

### Brinstar
- Spore Spawn boss room
- Etecoon wall jump room
- Red Brinstar with crumble blocks

### Norfair
- Crocomire boss room
- Ridley's lair entrance
- Hot rooms (damage without Varia)

### Wrecked Ship
- Phantoon boss room
- Power on/off state changes

### Maridia
- Draygon boss room
- Water physics throughout
- Sand pits (quicksand)

### Tourian
- Metroid rooms
- Mother Brain multi-phase fight
- Escape sequence trigger
