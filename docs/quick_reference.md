# Super Metroid Quick Reference

## Critical Constants for DSi Port

### ROM Properties
- **Size**: 3,145,728 bytes (3 MB)
- **Format**: LoROM (no SMC header)
- **Checksum**: $F8DF

### Address Translation (LoROM)
```
ROM_offset = ((bank & 0x7F) * 0x8000) + (address - 0x8000)
```

---

## Key RAM Addresses

### Player Position (7E prefix = WRAM)
| Address | Description |
|---------|-------------|
| $0AF6 | X position (pixels) |
| $0AF8 | X subpixel |
| $0AFA | Y position (pixels) |
| $0AFC | Y subpixel |

### Player Velocity
| Address | Description |
|---------|-------------|
| $0B2C | Y speed (subpixel) |
| $0B2E | Y speed (pixels) |
| $0B42 | X speed (pixels) |
| $0B44 | X speed (subpixel) |

### Player Stats
| Address | Description |
|---------|-------------|
| $09C2 | Energy (health) |
| $09C6 | Missiles |
| $09CA | Super missiles |
| $09CE | Power bombs |
| $09A2 | Equipped items |
| $09A6 | Equipped beams |

---

## Physics Constants (NTSC)

| Property | Value | Hex (subpixel) |
|----------|-------|----------------|
| Gravity (air) | 0.07168 px/f² | $125C |
| Gravity (water) | 0.02048 px/f² | $053F |
| Terminal velocity | 5.02 px/f | ~$50000 |
| Jump initial | 4.57 px/f | ~$49000 |
| Walk speed | 1.5 px/f | $18000 |
| Run speed | 2.0 px/f | $20000 |

---

## ROM Bank Quick Reference

| Data Type | Banks | ROM Offset |
|-----------|-------|------------|
| System code | $80-$82 | $000000 |
| Room headers | $8F | $078000 |
| Samus sprites | $90-$9F | $080000 |
| Enemy AI | $A0-$B4 | $100000 |
| Enemy graphics | $AB-$B1 | $158000 |
| Tilesets | $B9-$C1 | $1C8000 |
| Palettes | $C2 | $210000 |
| Level data | $C3-$CE | $218000 |
| Music | $CF-$DE | $278000 |

---

## Tile/Collision Types

| Type | Value | Description |
|------|-------|-------------|
| Air | $00 | Pass through |
| Solid | $01 | Block movement |
| Slope | $10-$2F | Angled surface |
| Shot block | $30 | Shoot to destroy |
| Bomb block | $31 | Bomb to destroy |
| Spike | $50 | Damage on contact |
| Water | $60 | Reduced gravity |
| Lava | $70 | Damage + slow |

---

## Screen Dimensions

| Property | Value |
|----------|-------|
| SNES resolution | 256×224 |
| Screen (tiles) | 16×14 (16x16 metatiles) |
| Screen (pixels) | 256×224 |
| DSi top screen | 256×192 |
| DSi bottom screen | 256×192 |

### Split Strategy
- Top 192 pixels → DSi top screen
- Bottom 32 pixels → DSi bottom screen (HUD area)

---

## Compression Format (LC_LZ2)

**Header byte**: `CCCLLLL` (3-bit command, 5-bit length)

| Command | Meaning |
|---------|---------|
| 000 | Direct copy |
| 001 | Byte fill |
| 010 | Word fill |
| 011 | Increasing fill |
| 100 | Back reference |
| 111 | Extended length |
| $FF | End marker |

---

## Boss HP Quick Reference

| Boss | HP | Notes |
|------|----|-------|
| Bomb Torizo | 800 | First boss |
| Spore Spawn | 960 | Core vulnerable |
| Kraid | 1000 | Mouth is weak point |
| Phantoon | 2500 | Avoid super missiles |
| Botwoon | 3000 | Head is weak point |
| Draygon | 6000 | Can use turrets |
| Golden Torizo | 8000 | Catches supers |
| Ridley | 18000 | Aggression scales |
| Mother Brain 1 | 3000 | In glass tank |
| Mother Brain 2 | 18000 | Standing form |
| Mother Brain 3 | 36000 | Hyper beam only |

---

## File Organization

```
docs/
├── rom_analysis.md      - Overview and structure
├── rom_memory_map.md    - Bank-by-bank breakdown
├── physics_data.md      - Physics constants and RAM
├── graphics_data.md     - Sprite/tile formats
├── level_data.md        - Room structure and collision
├── audio_data.md        - Music and SFX data
├── enemy_data.md        - Enemy AI and stats
└── quick_reference.md   - This file
```

---

## Useful External Resources

- [P.JBoy's Disassembly](https://patrickjohnston.org/bank/index.html)
- [Kejardon's RAM Map](https://jathys.zophar.net/supermetroid/kejardon/RAMMap.txt)
- [TASVideos Resources](https://tasvideos.org/GameResources/SNES/SuperMetroid)
- [Speedrun Wiki Physics](https://wiki.supermetroid.run/Physics_Compendium)
- [Metroid Construction](https://metroidconstruction.com)
