# M2: ROM Asset Extraction Pipeline - Instruction File

## Purpose

Extract raw game data from the Super Metroid SNES ROM into individual files for conversion. Python scripts that read the ROM, decompress LC_LZ2 data, and output tilesets, tilemaps, palettes, sprites, rooms, and audio samples.

## Dependencies

- Python 3.x (available on system)
- `docs/rom_memory_map.md` -- bank-by-bank ROM offsets for all data types
- `docs/graphics_data.md` -- LC_LZ2 compression format specification
- `docs/level_data.md` -- room data structure
- ROM file in `roms/` directory (default path)

## Scripts to Create

### tools/rom_addresses.py (Haiku)
Constants file mapping SNES ROM addresses to file offsets.

SNES address to ROM offset conversion:
- Banks $80-$FF (HiROM): file_offset = (bank - 0x80) * 0x8000 + (addr & 0x7FFF)  for LoROM
- Super Metroid is HiROM: file_offset = (bank & 0x3F) * 0x10000 + (addr & 0xFFFF)

Key addresses from docs/rom_memory_map.md:
- Room headers, door lists, state data
- Tileset pointers, tilemap pointers
- Palette pointers
- Enemy population data
- Sprite DMA tables

### tools/lz_decompress.py (Sonnet)
LC_LZ2 decompressor matching the SNES compression format.

Algorithm (from docs/graphics_data.md):
```
1. Read command byte
2. For each bit in command byte (MSB first):
   bit=0: literal byte, copy to output
   bit=1: compressed run
     Read 2 bytes: length (upper 3 bits + 3), offset (lower 13 bits)
     Copy 'length' bytes from output[current_pos - offset - 1]
3. Repeat until decompressed size reached
```

### tools/rom_extract.py (Sonnet)
Master extraction script. Takes ROM path as argument.

```
Usage: python tools/rom_extract.py [path_to_rom]
Default: roms/Super Metroid (JU) [!].smc
Output: assets_raw/
```

Output structure:
```
assets_raw/
├── tilesets/       # Decompressed 4bpp planar tilesets
├── tilemaps/       # Decompressed level tilemaps
├── palettes/       # Raw BGR555 palette data (512 bytes each)
├── sprites/        # Samus + enemy sprite data
├── rooms/          # Room headers + state data
└── audio/          # BRR samples (raw)
```

## Verification

- Crateria tileset decompresses to ~60KB
- Palette files are 512 bytes (256 colors * 2 bytes BGR555)
- Room count ~200 across all areas
- No crashes on any data block

## Constraints

- ROM path defaults to `roms/` but accepts argument
- `roms/` and `assets_raw/` are both gitignored
- Scripts must handle both headered (512-byte) and unheadered ROMs
- No external Python dependencies (stdlib only)
