# Super Metroid ROM Memory Map

## Complete Bank-by-Bank Breakdown

This document provides the complete ROM memory map for Super Metroid, organized by functional category.

---

## System & Core Code (Banks $80-$8C)

| Bank | ROM Offset | SNES Address | Description |
|------|------------|--------------|-------------|
| $80 | $000000-$007FFF | $80:8000-$80:FFFF | System routines, decompression, core functions |
| $81 | $008000-$00FFFF | $81:8000-$81:FFFF | SRAM handling, spritemap processing, menus |
| $82 | $010000-$017FFF | $82:8000-$82:FFFF | Top level main game routines, game loop |
| $83 | $018000-$01FFFF | $83:8000-$83:FFFF | FX definitions, door definitions |
| $84 | $020000-$027FFF | $84:8000-$84:FFFF | PLMs (Post Load Modifications - items, doors) |
| $85 | $028000-$02FFFF | $85:8000-$85:FFFF | Message boxes, text display |
| $86 | $030000-$037FFF | $86:8000-$86:FFFF | Enemy projectiles |
| $87 | $038000-$03FFFF | $87:8000-$87:FFFF | Animated tiles |
| $88 | $040000-$047FFF | $88:8000-$88:FFFF | HDMA graphics effects |
| $89 | $048000-$04FFFF | $89:8000-$89:FFFF | Item PLM graphics, FX loader |
| $8A | $050000-$057FFF | $8A:8000-$8A:FFFF | FX tilemaps |
| $8B | $058000-$05FFFF | $8B:8000-$8B:FFFF | Non-gameplay routines (title, credits) |
| $8C | $060000-$067FFF | $8C:8000-$8C:FFFF | Title sequence, intro cutscene |

---

## Menu & Room Data (Banks $8E-$8F)

| Bank | ROM Offset | SNES Address | Description |
|------|------------|--------------|-------------|
| $8E | $070000-$077FFF | $8E:8000-$8E:FFFF | Menu tiles, font data |
| $8F | $078000-$07FFFF | $8F:8000-$8F:FFFF | **Room definitions** (headers, pointers) |

### Bank $8F - Room Data Structure

Each room entry contains:
- Room header (dimensions, scroll type, etc.)
- Pointer to level data (tilemap)
- Pointer to BTS (Behind The Scenery) data
- Pointer to scroll data
- Pointer to PLM list
- Pointer to enemy population
- Pointer to enemy set
- Pointer to FX
- Pointer to door list

---

## Samus Graphics & Animation (Banks $90-$9F)

| Bank | ROM Offset | SNES Address | Description |
|------|------------|--------------|-------------|
| $90 | $080000-$087FFF | $90:8000-$90:FFFF | Samus sprite graphics |
| $91 | $088000-$08FFFF | $91:8000-$91:FFFF | Samus sprite graphics |
| $92 | $090000-$097FFF | $92:8000-$92:FFFF | Samus animations, pose definitions |
| $93 | $098000-$09FFFF | $93:8000-$93:FFFF | Projectile code, beam graphics |
| $94 | $0A0000-$0A7FFF | $94:8000-$94:FFFF | Block collision, grapple drawing |
| $95-$99 | $0A8000-$0CFFFF | $95-$99:8000-FFFF | Cutscene graphics (intro, ending) |
| $9A | $0D0000-$0D7FFF | $9A:8000-$9A:FFFF | Projectile tiles, BG3 tiles |
| $9B | $0D8000-$0DFFFF | $9B:8000-$9B:FFFF | Grapple beam, additional Samus graphics |
| $9C-$9F | $0E0000-$0FFFFF | $9C-$9F:8000-FFFF | Additional Samus graphics, effects |

### Samus Sprite Data Start

**$00DEC00** (ROM offset $DEC00) - Start of Samus sprite graphics

---

## Enemy Code & Data (Banks $A0-$B4)

| Bank | ROM Offset | SNES Address | Description |
|------|------------|--------------|-------------|
| $A0 | $100000-$107FFF | $A0:8000-$A0:FFFF | Enemy base code, common routines |
| $A1 | $108000-$10FFFF | $A1:8000-$A1:FFFF | Enemy population definitions |
| $A2 | $110000-$117FFF | $A2:8000-$A2:FFFF | Gunship, elevator AI |
| $A3 | $118000-$11FFFF | $A3:8000-$A3:FFFF | Metroid AI |
| $A4 | $120000-$127FFF | $A4:8000-$A4:FFFF | Crocomire AI |
| $A5 | $128000-$12FFFF | $A5:8000-$A5:FFFF | Draygon AI |
| $A6 | $130000-$137FFF | $A6:8000-$A6:FFFF | Spore Spawn AI |
| $A7 | $138000-$13FFFF | $A7:8000-$A7:FFFF | Ridley AI |
| $A8 | $140000-$147FFF | $A8:8000-$A8:FFFF | Kraid AI |
| $A9 | $148000-$14FFFF | $A9:8000-$A9:FFFF | Phantoon AI |
| $AA | $150000-$157FFF | $AA:8000-$AA:FFFF | Mother Brain AI |
| $AB-$B1 | $158000-$18FFFF | $AB-$B1:8000-FFFF | Enemy graphics (7 banks) |
| $B2 | $190000-$197FFF | $B2:8000-$B2:FFFF | Space pirate AI |
| $B3 | $198000-$19FFFF | $B3:8000-$B3:FFFF | Botwoon, Torizo AI |
| $B4 | $1A0000-$1A7FFF | $B4:8000-$B4:FFFF | Enemy sets, vulnerabilities, drop tables |

---

## Region Maps & Background Graphics (Banks $B5-$C2)

| Bank | ROM Offset | SNES Address | Description |
|------|------------|--------------|-------------|
| $B5 | $1A8000-$1AFFFF | $B5:8000-$B5:FFFF | Region/area map tiles |
| $B7 | $1B8000-$1BFFFF | $B7:8000-$B7:FFFF | Additional enemy graphics |
| $B9 | $1C8000-$1CFFFF | $B9:8000-$B9:FFFF | CRE (Common Room Elements), background images |
| $BA | $1D0000-$1D7FFF | $BA:8000-$BA:FFFF | Background images, tile graphics |
| $BB-$C1 | $1D8000-$20FFFF | $BB-$C1:8000-FFFF | Tile graphics for all areas |
| $C1-$C2 | $208000-$217FFF | $C1-$C2:8000-FFFF | Tile tables, level assembly data |

---

## Palettes & Level Data (Banks $C2-$CE)

| Bank | ROM Offset | SNES Address | Description |
|------|------------|--------------|-------------|
| $C2 | $210000-$217FFF | $C2:8000-$C2:FFFF | Palette data (start at $210000) |
| $C3-$CE | $218000-$277FFF | $C3-$CE:8000-FFFF | Compressed level data, tilemaps |

### Palette Data Format

Palettes are in SNES BGR555 format:
- 15-bit color: `0BBBBBGG GGGRRRRR`
- 16 colors per palette row
- Multiple palette rows per area

---

## Music Data (Banks $CF-$DF)

| Bank | ROM Offset | SNES Address | Description |
|------|------------|--------------|-------------|
| $CF | $278000-$27FFFF | $CF:8000-$CF:FFFF | Music track data |
| $D0-$DE | $280000-$2F7FFF | $D0-$DE:8000-FFFF | Additional music tracks |
| $DF | $2F8000-$2FFFFF | $DF:8000-$DF:FFFF | Incomplete/unused music |

---

## Key Data Location Quick Reference

| Data Type | Bank | ROM Offset | Notes |
|-----------|------|------------|-------|
| Decompression routine | $80 | $000000 | System bank |
| Room headers | $8F | $078000 | All room definitions |
| Samus sprites | $90-$9F | $080000-$0FFFFF | 6 banks of graphics |
| Samus sprite start | - | $0DEC00 | Documented start |
| Enemy AI | $A0-$B4 | $100000-$1A7FFF | Per-enemy banks |
| Boss AI (Ridley) | $A7 | $138000 | Single bank |
| Boss AI (Kraid) | $A8 | $140000 | Single bank |
| Boss AI (Mother Brain) | $AA | $150000 | Single bank |
| Enemy graphics | $AB-$B1, $B7 | $158000-$18FFFF, $1B8000 | 8 banks total |
| Tilesets | $B9-$C1 | $1C8000-$20FFFF | All area tilesets |
| Palettes | $C2 | $210000 | Start of palette data |
| Level tilemaps | $C3-$CE | $218000-$277FFF | Compressed |
| Music | $CF-$DE | $278000-$2F7FFF | SPC music data |

---

## Notes for DSi Port

### Data Extraction Priority

1. **Bank $8F** - Room definitions (needed to understand level structure)
2. **Banks $C2** - Palettes (needed for graphics display)
3. **Banks $B9-$C1** - Tilesets (background graphics)
4. **Banks $90-$9F** - Samus sprites (player graphics)
5. **Banks $AB-$B1, $B7** - Enemy sprites
6. **Banks $CF-$DE** - Music (optional, can be streamed)

### Compression Handling

Most graphics and level data are compressed. The decompression routine in bank $80 uses an LC_LZ2-style algorithm. You'll need to:
1. Locate compressed data
2. Run through decompression
3. Store decompressed data in appropriate DSi format
