# Super Metroid Graphics Data

## Overview

Super Metroid uses the SNES PPU (Picture Processing Unit) with:
- 4 background layers (modes vary)
- 128 hardware sprites (OAM)
- 256 colors on screen (8 palettes of 16 colors each, or 256-color mode)
- Resolution: 256x224 pixels (256x239 with overscan)

---

## Graphics Format

### Tile Format (4bpp)

SNES uses planar 4bpp (4 bits per pixel) format:
- Each 8x8 tile = 32 bytes
- Organized as bitplanes: 0+1 interleaved, then 2+3 interleaved
- 16 colors per tile (1 transparent + 15 visible)

**Byte Layout for one 8x8 tile (32 bytes):**
```
Bytes 0-1:   Row 0, bitplanes 0-1
Bytes 2-3:   Row 1, bitplanes 0-1
...
Bytes 14-15: Row 7, bitplanes 0-1
Bytes 16-17: Row 0, bitplanes 2-3
Bytes 18-19: Row 1, bitplanes 2-3
...
Bytes 30-31: Row 7, bitplanes 2-3
```

### Conversion to DSi Format

DSi uses linear 4bpp or 8bpp. Conversion required:
```c
// SNES planar to DSi linear (4bpp)
void convert_tile_4bpp(uint8_t* snes, uint8_t* dsi) {
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int bit = 7 - x;
            int pixel = 0;
            pixel |= ((snes[y * 2 + 0] >> bit) & 1) << 0;
            pixel |= ((snes[y * 2 + 1] >> bit) & 1) << 1;
            pixel |= ((snes[y * 2 + 16] >> bit) & 1) << 2;
            pixel |= ((snes[y * 2 + 17] >> bit) & 1) << 3;
            // Store in linear format
            int dst_idx = y * 4 + x / 2;
            if (x & 1) dsi[dst_idx] |= pixel << 4;
            else dsi[dst_idx] = pixel;
        }
    }
}
```

---

## Palette Format

### SNES Palette (BGR555)

- 15-bit color: `0BBBBBGG GGGRRRRR`
- Each color = 2 bytes (little-endian)
- Max 256 colors (8 palettes × 32 bytes each)

**Format:**
```
Bit:  15  14-10   9-5    4-0
      0   BBBBB   GGGGG  RRRRR
```

### DSi Palette (BGR555)

DSi also uses BGR555, so palettes transfer directly:
- Same format as SNES
- Can use same byte values
- 256 colors for backgrounds, 256 for sprites

### Palette Data Location

- **ROM Offset**: $210000 (Bank $C2)
- Palettes are stored per-area
- Area change loads new palette set

---

## Sprite Data

### Samus Sprites

**Location**: Banks $90-$9F, starting at ROM offset $080000

Main sprite data start: **$0DEC00**

Samus consists of multiple sprites assembled together:
- Body sprites
- Arm cannon (separate, rotates)
- Gun flash effects
- Morph ball (separate sprite set)

### Sprite Assembly (OAM Layout)

Samus uses 4-8 sprites per frame depending on pose:
```
Standing: 4 sprites (torso, legs split)
Running: 4-6 sprites (animated)
Aiming up: 5 sprites (arm rotated)
Morph ball: 1-2 sprites (small)
```

### Sprite Data Format

Each sprite frame has:
- Tile index (which graphics tile)
- X/Y offset from Samus position
- Flip flags (H/V)
- Palette selection
- Priority (layer)

---

## Tileset Data

### Tileset Locations

| Area | ROM Bank | Description |
|------|----------|-------------|
| CRE (Common) | $B9 | Doors, items, effects |
| Crateria | $BA-$BB | Surface and underground |
| Brinstar | $BB-$BC | Green, red, pink areas |
| Norfair | $BC-$BD | Heated, hot areas |
| Wrecked Ship | $BD-$BE | Powered/unpowered |
| Maridia | $BE-$BF | Sandy, underwater |
| Tourian | $BF-$C0 | Final area |
| Ceres | $C0-$C1 | Opening station |

### Tileset Structure

Each tileset contains:
- 8x8 tiles (compressed)
- 16x16 metatile definitions
- Collision data association
- Palette association

---

## Background Layers

### SNES Layer Usage

Super Metroid uses Mode 1 primarily:
- **BG1**: Main level tilemap (16x16 metatiles)
- **BG2**: Parallax background or foreground effects
- **BG3**: HUD, status display
- **BG4**: Not used in Mode 1

### Layer Properties

| Layer | Tile Size | Priority | Usage |
|-------|-----------|----------|-------|
| BG1 | 16x16 | Main | Level foreground |
| BG2 | 16x16 | Behind BG1 | Scrolling background |
| BG3 | 8x8 | Top | HUD/Text |
| Sprites | 8x8-64x64 | Variable | Characters, enemies |

---

## Compression

### LC_LZ2 Style Compression

Super Metroid uses a variant of LC_LZ2 compression for graphics:

**Header Byte Format:**
```
Bits 7-5: Command (CCC)
Bits 4-0: Length-1 (LLLLL)
```

**Commands:**
| Command | Description |
|---------|-------------|
| 000 | Direct copy (L+1 bytes follow) |
| 001 | Byte fill (repeat 1 byte L+1 times) |
| 010 | Word fill (alternate 2 bytes L+1 times) |
| 011 | Increasing fill (increment byte L+1 times) |
| 100 | Back reference (copy L+1 bytes from output buffer) |
| 111 | Extended length mode (10-bit length follows) |
| $FF | End of compressed data |

### Decompression Routine

Located at bank $80 (ROM offset $0000-$7FFF).

---

## Graphics Memory Requirements

### Estimated Decompressed Sizes

| Data Type | Compressed | Decompressed | Notes |
|-----------|------------|--------------|-------|
| Samus (all frames) | ~100 KB | ~200 KB | All suits, poses |
| Single area tileset | ~20 KB | ~60 KB | Per area |
| Enemy graphics (all) | ~300 KB | ~600 KB | All enemies |
| Boss graphics | ~100 KB | ~200 KB | All bosses |
| Effects/items | ~50 KB | ~100 KB | Common elements |

### DSi VRAM Allocation

DSi has 656 KB of VRAM:
- Main BG: 512 KB
- Sub BG: 128 KB
- OBJ (sprites): 256 KB (shared)

**Recommended allocation:**
```
Tilesets:     128 KB (current area)
Samus:        64 KB (current suit)
Enemies:      64 KB (current room set)
Effects/HUD:  32 KB
Reserve:      32 KB
```

---

## Color/Palette Notes

### Area Color Palettes

| Area | Primary Colors | Notes |
|------|----------------|-------|
| Crateria Surface | Browns, grays | Earth tones |
| Crateria Underground | Purples, blues | Dark atmosphere |
| Brinstar Green | Greens, browns | Jungle feel |
| Brinstar Red | Reds, oranges | Heated areas |
| Norfair | Reds, oranges, yellows | Lava environment |
| Wrecked Ship (off) | Grays, dark blues | Powered down |
| Wrecked Ship (on) | Yellows, browns | Powered up |
| Maridia | Blues, cyans | Underwater |
| Tourian | Grays, cyans | Mechanical |

### Palette Animation

Some palettes cycle for effects:
- Lava glow (Norfair)
- Water shimmer (Maridia)
- Energy tanks (HUD)
- Save station glow

---

## DSi Port Conversion Pipeline

### Step 1: Extract Compressed Data
```bash
# Extract tileset at offset
dd if=rom.sfc bs=1 skip=$((0x1C8000)) count=32768 of=tileset.bin
```

### Step 2: Decompress
```c
decompress_lz(compressed_data, decompressed_buffer);
```

### Step 3: Convert Format
```c
// For each 8x8 tile
for (int i = 0; i < num_tiles; i++) {
    convert_tile_4bpp(&snes_data[i * 32], &dsi_data[i * 32]);
}
```

### Step 4: Convert Palette
```c
// Palettes are already BGR555, copy directly
memcpy(dsi_palette, snes_palette, 512); // 256 colors × 2 bytes
```

### Step 5: Load to DSi VRAM
```c
dmaCopy(dsi_tiles, BG_TILE_RAM(0), tiles_size);
dmaCopy(dsi_palette, BG_PALETTE, 512);
```

---

## Sprite Priority and Ordering

### Priority Levels (SNES)

| Priority | Layer |
|----------|-------|
| 3 (highest) | Always on top |
| 2 | Above most BG |
| 1 | Above BG2 |
| 0 (lowest) | Behind most BG |

### Recommended DSi Sprite Order

1. HUD elements (highest)
2. Samus projectiles
3. Samus
4. Enemy projectiles
5. Enemies
6. Item pickups
7. Background effects (lowest)
