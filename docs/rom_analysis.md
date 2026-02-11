# Super Metroid ROM Analysis

## Overview

This document provides a comprehensive analysis of the Super Metroid (Japan, USA) ROM for the purpose of porting to Nintendo DSi.

**ROM File**: `Super Metroid (Japan, USA) (En,Ja).sfc`
**Size**: 3,145,728 bytes (3 MB / 24 Megabit)
**Format**: SNES LoROM (no SMC header)
**Checksum**: $F8DF
**Region**: Japan/USA (NTSC)

---

## ROM Header (at offset $7FC0)

| Offset | Size | Value | Description |
|--------|------|-------|-------------|
| $7FC0 | 21 | "Super Metroid       " | Internal game title |
| $7FD5 | 1 | $30 | Map mode (LoROM, FastROM) |
| $7FD6 | 1 | $02 | ROM type (ROM + RAM + SRAM) |
| $7FD7 | 1 | $0C | ROM size (4MB allocation) |
| $7FD8 | 1 | $03 | SRAM size (8 KB) |
| $7FD9 | 1 | $00 | Destination (Japan) |
| $7FDA | 1 | $01 | Fixed value |
| $7FDB | 1 | $00 | Version |
| $7FDC-DD | 2 | $2007 | Complement checksum |
| $7FDE-DF | 2 | $F8DF | Checksum |

---

## Memory Mapping (LoROM)

Super Metroid uses LoROM mapping where:
- Banks $00-$3F and $80-$BF map ROM to addresses $8000-$FFFF (32 KB per bank)
- Banks $40-$6F and $C0-$EF are additional ROM space
- Banks $70-$7D are SRAM (mirrored)
- Bank $7E is work RAM (WRAM)
- Bank $7F is extended WRAM

### Address Translation

**SNES Address → ROM Offset Formula (LoROM):**
```
ROM_offset = ((bank & 0x7F) * 0x8000) + (address - 0x8000)
```

**Example:**
- SNES $80:8000 → ROM offset $00000
- SNES $8F:8000 → ROM offset $78000
- SNES $CF:8000 → ROM offset $278000

---

## Bank Organization Summary

| Banks | ROM Offset | Content |
|-------|------------|---------|
| $80-$82 | $000000-$017FFF | System code, main game routines |
| $83 | $018000-$01FFFF | FX and door definitions |
| $84-$89 | $020000-$04FFFF | PLMs, messages, projectiles, animated tiles, HDMA |
| $8A-$8B | $050000-$05FFFF | FX tilemaps, non-gameplay routines |
| $8C | $060000-$067FFF | Title sequence and intro |
| $8E-$8F | $070000-$07FFFF | Menu tiles, room definitions |
| $90-$9F | $080000-$0FFFFF | Samus sprites, animations, projectiles, cutscenes |
| $A0-$B4 | $100000-$1A7FFF | Enemy AI, boss code, enemy data |
| $B5 | $1A8000-$1AFFFF | Region maps |
| $AB-$B1 | $158000-$18FFFF | Enemy graphics |
| $B7 | $1B8000-$1BFFFF | Additional enemy graphics |
| $B9-$C2 | $1C8000-$217FFF | CRE, background images, tile graphics, tile tables |
| $C2-$CE | $210000-$277FFF | Palettes, level data |
| $CF-$DE | $278000-$2F7FFF | Music data |
| $DF | $2F8000-$2FFFFF | Unused/incomplete music |

---

## Related Documentation

For detailed information on specific data types, see:
- [ROM Memory Map](rom_memory_map.md) - Complete bank-by-bank breakdown
- [Physics Data](physics_data.md) - Physics constants and RAM addresses
- [Graphics Data](graphics_data.md) - Sprite and tile formats
- [Level Data](level_data.md) - Room structure and collision data
- [Audio Data](audio_data.md) - Music and sound effect formats
- [Enemy Data](enemy_data.md) - Enemy AI and data tables

---

## Key Technical Notes for DSi Port

### Compression
Super Metroid uses a custom LZ-style compression similar to LC_LZ2. The decompression routine is located in bank $80. Graphics and level data are compressed.

### Coordinate System
- Screen resolution: 256x224 pixels (8 pixels letterbox on PAL)
- Positions tracked to 1/65536th pixel precision
- Rooms use 16x16 pixel tile grid
- Screen scrolling in blocks (256x256 pixel units)

### Frame Timing
- NTSC: 60 FPS (16.67ms per frame)
- PAL: 50 FPS (20ms per frame)
- Physics values differ between NTSC and PAL versions

### Data Dependencies
The DSi port should load data in this order:
1. Palettes (needed for all graphics)
2. Tilesets (background tiles for current area)
3. Sprite graphics (Samus, enemies, items)
4. Room data (tilemap, collision, scroll bounds)
5. Enemy spawn data
6. Audio (music and sound effects)

---

## References

- [P.JBoy's Bank Logs](https://patrickjohnston.org/bank/index.html) - Complete disassembly
- [Kejardon's RAM Map](https://jathys.zophar.net/supermetroid/kejardon/RAMMap.txt)
- [TASVideos Game Resources](https://tasvideos.org/GameResources/SNES/SuperMetroid)
- [Super Metroid Speedrun Wiki](https://wiki.supermetroid.run/)
