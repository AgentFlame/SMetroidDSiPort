# Super Metroid DSi Port - Installation & Development Advice

## Required Installation

### Development Tools

**devkitPro/devkitARM**
- ARM cross-compiler toolchain for Nintendo DSi development
- Required for compiling C/C++ code to ARM binary
- Installation: Use devkitPro installer (Windows) or pacman

**libnds**
- Nintendo DS/DSi development library
- Provides hardware access APIs (graphics, input, audio, etc.)
- Included with devkitPro installation

**grit**
- Graphics Raster Image Transcoder
- Converts PNG images to DSi tile format (4bpp, 8bpp)
- Handles palette generation and tile arrangement
- Included with devkitPro

**mmutil**
- Maxmod utility for audio conversion
- Converts WAV files to maxmod soundbank format
- Handles music and sound effect compilation
- Included with devkitPro

**Python 3.x**
- For asset conversion and extraction scripts
- Will be used to parse ROM data and convert formats
- Recommended packages: struct, PIL/Pillow (for image processing)

**Emulator**
- **DeSmuME**: Best compatibility, excellent debugger, good for development
- **melonDS**: Most accurate, closest to real hardware behavior
- Recommendation: Use DeSmuME for development, melonDS for final testing

### Environment Variables

Set these after devkitPro installation:
- `DEVKITPRO` - Points to devkitPro installation directory
- `DEVKITARM` - Points to ARM toolchain subdirectory

On Windows MSYS:
```bash
export DEVKITPRO=/opt/devkitpro
export DEVKITARM=$DEVKITPRO/devkitARM
```

---

## Reading .sfc Super Nintendo ROM Files

### ROM Structure Overview

SNES ROM files (.sfc, .smc) are raw binary data with a specific internal structure. Super Metroid uses **LoROM** mapping.

**Key Sections:**
- **Header**: Located at offset `0x7FC0` (LoROM format)
  - Contains ROM title, checksums, ROM size, region info
- **Code**: 65C816 assembly throughout various banks
- **Graphics**: CHR sections with 4bpp tile data
- **Palettes**: BGR555 format (15-bit color, 5 bits per channel)
- **Level Data**: Compressed room layouts and tilemaps
- **Audio**: SPC700 samples in BRR (Bit Rate Reduction) format

### Graphics Data Format

**SNES 4bpp Planar Tile Format:**
- Each tile is 8x8 pixels
- 4 bits per pixel (16 colors per tile)
- Data stored in "planar" format (bit planes separated)
- Each bitplane is 2 bytes (8 pixels × 1 bit)
- 4 bitplanes × 2 bytes = 8 bytes per tile

**Conversion Process:**
1. Extract raw tile data from ROM
2. Convert planar format to chunky format (DSi uses chunky)
3. Rearrange into tile sheets
4. Generate palette data separately

**Palettes:**
- SNES uses BGR555: `0bbbbbgggggrrrrr` (15-bit)
- DSi also uses BGR555, but byte order may differ
- Each color is 2 bytes
- Palettes are usually 16 colors (for 4bpp) or 256 colors (for 8bpp)

### Data Extraction Strategy

**Tools to Write:**
1. `tools/rom_extract.py` - Main extraction script
2. `tools/tile_converter.py` - Graphics format conversion
3. `tools/palette_converter.py` - Palette extraction
4. `tools/room_parser.py` - Level data parsing
5. `tools/spc_extractor.py` - Audio extraction

**Python Libraries:**
```python
import struct  # For binary data parsing
from PIL import Image  # For image creation
```

**Example: Reading a 16-bit value from ROM**
```python
with open('rom.sfc', 'rb') as f:
    f.seek(0x7FC0)  # ROM header
    data = f.read(2)
    value = struct.unpack('<H', data)[0]  # Little-endian unsigned short
```

---

## Biggest Challenges Beyond Physics

### 1. Boss AI Complexity (HIGH DIFFICULTY)

**Why It's Hard:**
- Each boss has unique, intricate state machines
- Mother Brain has 3+ distinct phases with different behaviors
- Attack patterns are timing-dependent and frame-perfect
- Requires deep understanding of original 65C816 assembly

**What Makes It Worse:**
- Boss AI isn't just "move and shoot"
- Vulnerability windows are precise
- Phase transitions have specific triggers
- Many bosses have multiple body parts with individual hit detection

**Mitigation:**
- Start with simpler bosses (Spore Spawn, Crocomire)
- Use video references for behavior analysis
- Disassemble and heavily comment boss AI code
- Test each attack pattern individually

### 2. Audio System (MEDIUM-HIGH DIFFICULTY)

**Why It's Hard:**
- SNES uses SPC700 audio chip, completely different from DSi
- BRR (Bit Rate Reduction) is a proprietary compression format
- Music sequences are complex, multi-channel arrangements
- Timing synchronization with gameplay events is critical

**Specific Challenges:**
- Sample rate conversion (32kHz SPC700 → DSi rates)
- Loop points must be preserved
- Sound effect priority system (important sounds shouldn't be cut)
- Dynamic music transitions (e.g., boss music starting)

**Approaches:**
- Option A: Pre-render music as streamed audio (simpler, more space)
- Option B: Use maxmod with converted samples (more authentic, less space)
- Option C: Port SPC sequences directly (most complex, best quality)

### 3. Memory Management (MEDIUM DIFFICULTY)

**Why It's Hard:**
- DSi has 16MB total RAM, sounds like a lot but fills up fast
- Original SNES could swap data from cartridge instantly
- Large rooms, boss sprites, and audio compete for space
- Memory leaks accumulate over long play sessions

**Common Issues:**
- Not freeing resources when changing rooms
- Loading entire assets when only portions are needed
- Duplicate data in memory
- Fragmentation from repeated alloc/free cycles

**Best Practices:**
- Implement streaming for music and large graphics
- Profile memory usage regularly
- Use memory pools for fixed-size objects
- Clear unused room data on transitions

### 4. Assembly Translation (HIGH DIFFICULTY)

**Why It's Hard:**
- Much of Super Metroid's complex logic is raw 65C816 assembly
- Not just physics - enemy AI, special effects, item logic
- Assembly doesn't translate 1:1 to C
- Optimizations and tricks used in original are architecture-specific

**Areas Requiring Disassembly:**
- Collision detection algorithms
- Enemy behavior patterns
- Special weapon effects (e.g., wave beam going through walls)
- Boss vulnerability calculations

**Approach:**
- Use existing Super Metroid disassembly projects as reference
- Comment heavily as you translate
- Verify behavior matches original frame-by-frame
- Keep assembly fragments as comments in C code for reference

### 5. Data Format Reverse Engineering (MEDIUM DIFFICULTY)

**Why It's Hard:**
- SNES uses planar graphics, not chunky (most modern systems use chunky)
- Room data is RLE compressed with custom algorithm
- Palette indexing has quirks (transparency, sprite priorities)
- No official documentation, all reverse-engineered

**Specific Formats:**
- **Tiles**: 4bpp planar requires bit manipulation to decode
- **Sprites**: Multiple sizes (8x8, 16x16, 32x32, 64x64)
- **Tilemaps**: Each tile has palette, flip, and priority flags
- **Collision**: Custom per-tile type system

---

## Obvious Pitfalls to Avoid

### 1. Emulator vs. Hardware Differences

**The Problem:**
- Code that works perfectly in DeSmuME may crash on real DSi hardware
- Timing differences cause audio sync issues
- Memory access patterns that work in emulator may fail on hardware

**Prevention:**
- Test on melonDS (more accurate) regularly
- If possible, test on actual DSi hardware before release
- Don't rely on emulator-specific behaviors
- Follow libnds best practices strictly

### 2. Frame Rate Issues

**The Problem:**
- Original Super Metroid runs at exactly 60 FPS
- If you drop below 60 FPS, physics calculations break
- Easy to add too much per-frame processing

**Prevention:**
- Profile every major system addition
- Target 50-55 FPS to leave headroom
- Use VBlank properly for timing
- Don't do heavy calculations every frame (amortize over multiple frames)

**Warning Signs:**
- Stuttering movement
- Inconsistent jump heights
- Audio desync

### 3. Endianness Confusion

**The Problem:**
- SNES (65C816) is little-endian
- ARM can be big or little-endian (usually little)
- Multi-byte values from ROM need careful handling

**Example Bug:**
```c
// WRONG - may read backwards on some systems
uint16_t value = *(uint16_t*)&rom_data[offset];

// RIGHT - explicit endianness handling
uint16_t value = rom_data[offset] | (rom_data[offset+1] << 8);
```

**Prevention:**
- Always use explicit byte ordering when reading ROM
- Use `struct.unpack('<H', data)` in Python (< means little-endian)
- Test on big-endian system if possible

### 4. Collision Detection Errors

**The Problem:**
- Pixel-perfect collision is critical for Super Metroid feel
- Off-by-one errors cause walking through walls or getting stuck
- Slopes are especially tricky (Super Metroid has many slope angles)

**Common Bugs:**
- Wrong collision box size (even 1 pixel matters)
- Incorrect slope angle calculations
- Not checking all corners of hitbox
- Forgetting one-way platform logic

**Prevention:**
- Use collision visualization in debug mode
- Draw hitboxes and collision tiles on screen
- Test every slope angle thoroughly
- Compare against original SNES frame-by-frame

### 5. Sprite Rendering Issues

**The Problem:**
- Incorrect sprite priority causes visual glitches
- SNES has specific sprite priority rules
- Flickering from too many sprites on one scanline (SNES limitation)

**Common Issues:**
- Samus renders behind background she should be in front of
- Projectiles appear under enemies
- Layering looks wrong in doors/elevators

**Prevention:**
- Study SNES OAM (Object Attribute Memory) priority system
- Implement priority sorting correctly
- Test in visually complex rooms

### 6. Save Data Corruption

**The Problem:**
- Not saving all necessary state variables
- Version incompatibility between builds
- File write failures not handled
- Corrupted saves frustrate players immensely

**Critical Data to Save:**
- All item flags (missiles, energy tanks, etc.)
- Equipment status (suits, beams, boots)
- Room visit flags
- Door unlocks
- Boss defeat flags
- Current health/ammo
- Current location

**Prevention:**
- Use save file version number
- Implement checksums
- Test save/load extensively
- Have fallback for corrupted saves
- Back up saves before writing new data

### 7. Hardcoded Assumptions

**The Problem:**
- Assuming specific room layouts or data
- Not handling edge cases
- Magic numbers everywhere

**Examples:**
```c
// BAD - what does 256 mean?
if (x > 256) { ... }

// GOOD - named constant
#define ROOM_WIDTH_PIXELS 256
if (x > ROOM_WIDTH_PIXELS) { ... }
```

**Prevention:**
- Use #define or const for all magic numbers
- Handle all edge cases (even "impossible" ones)
- Test boundary conditions
- Don't assume data is always valid

### 8. Audio Synchronization Drift

**The Problem:**
- Music getting out of sync with gameplay events
- Boss music starts late or early
- Sound effects cut each other off

**Critical Sync Points:**
- Door transitions (music change)
- Boss appearance (music change)
- Item pickup (fanfare, music pause)
- Death (music stop)
- Escape sequence (urgent music)

**Prevention:**
- Use frame-accurate triggering
- Test all music transitions
- Implement proper audio priority
- Don't assume audio timing is exact

### 9. Not Profiling Performance

**The Problem:**
- Performance issues appear late in development
- Hard to find the bottleneck
- Guessing at optimizations wastes time

**Prevention:**
- Profile from day one
- Measure FPS continuously
- Use debug overlay to show performance metrics
- Optimize the right things (biggest bottlenecks first)

### 10. Ignoring Existing Research

**The Problem:**
- Super Metroid has been extensively reverse-engineered
- Don't waste time re-discovering what's already known

**Resources:**
- Super Metroid disassembly projects (on GitHub)
- SNES development documentation
- Super Metroid ROM map (documented by community)
- Speedrunning community knowledge

---

## Development Workflow Tips

### Start Small
1. Get one room rendering before worrying about all rooms
2. Get basic movement before advanced techniques
3. One enemy type before all enemies
4. Simple test cases before full gameplay

### Test Incrementally
- Don't write 1000 lines then compile
- Test each system as you build it
- Keep last working version backed up
- Commit to git frequently

### Use Debug Tools
- FPS counter always visible during development
- Collision box visualization
- Memory usage display
- Position/velocity display
- Room boundaries visible

### Reference the Original
- Keep SNES version running for comparison
- Record video for frame-by-frame analysis
- Test exact same inputs on both versions
- Match the feeling, not just the numbers

---

## Recommended Development Order

1. **Environment setup** (Day 1)
2. **Basic rendering** - Get one tile on screen (Day 1)
3. **ROM extraction** - Parse tile data (Day 1-2)
4. **One room rendering** - Full background (Day 2)
5. **Samus sprite** - Standing still (Day 2)
6. **Basic movement** - Left/right walking (Day 2-3)
7. **Jump physics** - Simple jump (Day 3)
8. **Collision** - Floor detection (Day 3)
9. **One full room** - Playable with items (Day 4)
10. **Expand gradually** - More rooms, more features

---

## When Things Go Wrong

### The build fails
- Check DEVKITPRO and DEVKITARM environment variables
- Verify Makefile has correct paths
- Check for typos in #include statements
- Make clean and rebuild

### Graphics look wrong
- Verify palette is loaded correctly
- Check tile format conversion (planar vs chunky)
- Verify grit parameters (-ftc -gB4 for 4bpp)
- Check VRAM allocation (no overlap)

### Physics feel wrong
- Compare frame-by-frame with original
- Check all constants are exact
- Verify fixed-point vs float usage
- Test at exactly 60 FPS

### Audio doesn't play
- Check maxmod initialization
- Verify soundbank compiled correctly
- Check volume levels (not muted)
- Verify file paths

### Memory issues
- Check for memory leaks (valgrind if possible)
- Profile memory usage over time
- Verify all malloc() have matching free()
- Check for buffer overruns

---

## Final Advice

**Be Patient:**
This is a large, complex project. Super Metroid is not a simple game. Even with AI assistance, expect challenges and setbacks.

**Focus on Core Feel First:**
If the physics and movement don't feel right, nothing else matters. Get that perfect before worrying about every enemy and item.

**Don't Over-Engineer:**
Make it work first, then make it good. Don't spend days optimizing code that isn't bottlenecking performance.

**Commit Frequently:**
Follow the git guidelines in CLAUDE.md. Each commit should be a logical unit of work. Don't lose progress.

**Ask for Help:**
Use the existing Super Metroid reverse engineering community's work. No need to start from scratch on well-understood problems.

**Test on Real Hardware Eventually:**
Emulators are great for development, but nothing beats real hardware for final verification.

---

*Good luck with your Super Metroid DSi port! This is an ambitious project, but with careful planning and persistent effort, it's achievable.*
