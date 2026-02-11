# Super Metroid Audio Data

## Overview

Super Metroid uses the SPC700 audio processor with:
- 8 audio channels
- 64 KB dedicated audio RAM
- BRR (Bit Rate Reduction) compressed samples
- Custom sequence format for music

---

## Audio Data Location

| Bank Range | ROM Offset | Description |
|------------|------------|-------------|
| $CF-$DE | $278000-$2F7FFF | Music data (16 banks) |
| $DF | $2F8000-$2FFFFF | Incomplete/unused music |

Total audio data: ~512 KB compressed

---

## SPC700 Audio System

### Specifications

- CPU: SPC700 @ 2.048 MHz
- RAM: 64 KB
- Channels: 8 stereo
- Sample format: BRR (4-bit ADPCM)
- Sample rate: 32 kHz max

### Audio RAM Layout

| Address | Size | Description |
|---------|------|-------------|
| $0000-$00FF | 256 bytes | SPC registers/stack |
| $0100-$01FF | 256 bytes | Music driver code |
| $0200-$7FFF | ~30 KB | Sample data |
| $8000-$FFFF | 32 KB | Music sequence data |

---

## Music Tracks

### Track List

| ID | Name | Area/Event |
|----|------|------------|
| $05 | Title Theme | Title screen |
| $06 | Opening | Ceres Station |
| $09 | Item Room | Item acquisition |
| $0C | Crateria - Underground | Crateria interior |
| $0D | Crateria - Surface | Crateria outside |
| $0F | Brinstar - Green | Green Brinstar |
| $12 | Brinstar - Red | Red Brinstar |
| $15 | Norfair - Ancient | Norfair theme |
| $18 | Norfair - Hot | Lower Norfair |
| $1B | Wrecked Ship - Unpowered | Ship (no power) |
| $1E | Wrecked Ship - Powered | Ship (with power) |
| $21 | Maridia - Exterior | Rocky Maridia |
| $24 | Maridia - Interior | Underwater Maridia |
| $27 | Tourian | Final area |
| $2A | Boss Theme 1 | Standard bosses |
| $2D | Boss Theme 2 | Mini-bosses |
| $30 | Ridley Theme | Ridley battle |
| $33 | Escape | Escape sequence |
| $36 | Ending | Credits |

---

## Sample Data (BRR Format)

### BRR Compression

BRR (Bit Rate Reduction) is 4-bit ADPCM:
- 9 bytes = 16 samples
- Byte 0: Header (filter, range, flags)
- Bytes 1-8: 16 samples (4-bit each)

**Header Byte:**
```
Bits 7-6: Filter (0-3)
Bits 5-4: Range (0-12)
Bit 1: Loop flag
Bit 0: End flag
```

### Sample Categories

| Category | Count | Description |
|----------|-------|-------------|
| Instruments | ~30 | Musical instruments |
| Samus SFX | ~20 | Player sounds |
| Weapon SFX | ~15 | Beam, missile sounds |
| Enemy SFX | ~25 | Enemy sounds |
| Environment | ~10 | Ambient sounds |

---

## Sound Effects

### Samus Sound Effects

| ID | Description |
|----|-------------|
| $01 | Footstep |
| $02 | Jump |
| $03 | Land |
| $04 | Damage taken |
| $05 | Health pickup |
| $06 | Morph ball transform |
| $07 | Unmorph |
| $08 | Wall jump |

### Weapon Sound Effects

| ID | Description |
|----|-------------|
| $10 | Power beam shot |
| $11 | Charge beam fire |
| $12 | Ice beam |
| $13 | Wave beam |
| $14 | Spazer |
| $15 | Plasma beam |
| $16 | Missile launch |
| $17 | Super missile |
| $18 | Bomb place |
| $19 | Bomb explosion |
| $1A | Power bomb explosion |

### Environment Sound Effects

| ID | Description |
|----|-------------|
| $30 | Door open |
| $31 | Door close |
| $32 | Elevator start |
| $33 | Elevator stop |
| $34 | Save station |
| $35 | Energy recharge |
| $36 | Lava bubble |
| $37 | Rain |

---

## Music Sequence Format

### Sequence Commands

| Command | Description |
|---------|-------------|
| $00-$7F | Note (pitch) |
| $80-$8F | Duration modifier |
| $90-$9F | Velocity modifier |
| $A0-$AF | Instrument select |
| $B0-$BF | Effect commands |
| $C0-$CF | Loop commands |
| $D0-$DF | Jump commands |
| $E0-$EF | Special commands |
| $F0-$FF | Control commands |

### Note Format

Notes use MIDI-like pitch values:
- $00 = C-0
- $0C = C-1
- $18 = C-2
- etc.

---

## DSi Audio Implementation

### Approach Options

1. **Streaming**: Pre-render tracks to WAV/OGG, stream during gameplay
2. **MOD/XM**: Convert sequences to tracker format, use libmodplug
3. **Custom**: Implement BRR decoder and sequence player

### Recommended: Maxmod Library

Maxmod is the standard DSi audio library:
```c
#include <maxmod.h>

// Initialize
mmInitDefault("soundbank.bin");

// Play music
mmStart(MOD_BRINSTAR_GREEN, MM_PLAY_LOOP);

// Play SFX
mmEffect(SFX_BEAM_SHOT);
```

### Sample Conversion Pipeline

```bash
# Extract BRR samples
python tools/brr_extract.py rom.sfc samples/

# Convert BRR to WAV
for f in samples/*.brr; do
    brr2wav "$f" "${f%.brr}.wav"
done

# Build maxmod soundbank
mmutil -o soundbank.bin samples/*.wav music/*.mod
```

---

## Audio Timing

### Music Sync Points

| Event | Music Trigger |
|-------|---------------|
| Room enter | Area music start |
| Boss trigger | Boss music transition |
| Boss defeat | Victory fanfare |
| Item collect | Item jingle |
| Escape start | Escape music |

### Cross-fade Timing

- Standard transition: 30 frames fade out, 30 frames fade in
- Boss transition: Immediate cut
- Area transition: 60 frames fade

---

## Priority System

### Channel Priority (DSi)

| Priority | Usage |
|----------|-------|
| High (0-1) | Samus sounds |
| Medium (2-5) | Enemy/weapon sounds |
| Low (6-7) | Ambient/environment |

### SFX Priority Rules

1. Player damage always plays
2. Weapon sounds can stack (up to 3)
3. Enemy sounds can be culled if too many
4. Ambient sounds are lowest priority

---

## Memory Requirements

### Estimated Sizes

| Data | Size (Decompressed) |
|------|---------------------|
| Sample data | ~200 KB |
| Music sequences | ~100 KB |
| **Total** | ~300 KB |

### DSi Audio RAM

DSi has limited audio RAM in ARM7:
- Max soundbank: ~256 KB recommended
- Streaming can reduce memory usage

---

## Conversion Notes

### BRR to WAV

```python
def decode_brr(brr_data):
    samples = []
    prev1, prev2 = 0, 0

    for i in range(0, len(brr_data), 9):
        header = brr_data[i]
        filter_type = (header >> 6) & 3
        range_val = (header >> 4) & 3

        for j in range(1, 9):
            byte = brr_data[i + j]
            for nibble in [byte >> 4, byte & 0x0F]:
                # Sign extend
                if nibble >= 8:
                    nibble -= 16
                # Apply range
                sample = nibble << range_val
                # Apply filter
                if filter_type == 1:
                    sample += prev1 * 15/16
                elif filter_type == 2:
                    sample += prev1 * 61/32 - prev2 * 15/16
                elif filter_type == 3:
                    sample += prev1 * 115/64 - prev2 * 13/16

                prev2 = prev1
                prev1 = sample
                samples.append(int(sample))

    return samples
```

### Sequence to MOD/XM

More complex - requires:
1. Parse SPC sequence format
2. Map instruments to samples
3. Convert tempo/timing
4. Handle effects (vibrato, portamento)

Consider using pre-rendered audio if sequence conversion is too complex.
