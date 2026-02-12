"""
rom_extract.py - Master ROM extraction script for Super Metroid DS port.

Extracts tilesets, tilemaps, palettes, sprites, rooms, and audio data
from a Super Metroid SNES ROM into individual files.

Usage:
    python tools/rom_extract.py [path_to_rom]
    Default ROM path: roms/

Output: assets_raw/ (gitignored)
"""

import os
import sys
import struct
import glob as glob_mod

# Import sibling modules
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from rom_addresses import *
from lz_decompress import decompress


def find_rom(rom_arg=None):
    """Find the ROM file. Checks argument, then roms/ directory."""
    if rom_arg and os.path.isfile(rom_arg):
        return rom_arg

    # Search roms/ directory
    roms_dir = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'roms')
    if os.path.isdir(roms_dir):
        for ext in ('*.smc', '*.sfc', '*.fig'):
            matches = glob_mod.glob(os.path.join(roms_dir, ext))
            if matches:
                return matches[0]

    return None


def load_rom(path):
    """Load ROM, stripping optional copier header."""
    with open(path, 'rb') as f:
        data = bytearray(f.read())

    # Strip 512-byte copier header if present
    if len(data) % 0x8000 == 512:
        print(f"  Stripping 512-byte copier header")
        data = data[512:]

    print(f"  ROM size: {len(data)} bytes (0x{len(data):06X})")

    if len(data) != SM_ROM_SIZE:
        print(f"  WARNING: Expected {SM_ROM_SIZE} bytes, got {len(data)}")

    return data


def ensure_dir(path):
    """Create directory if it doesn't exist."""
    os.makedirs(path, exist_ok=True)


def read_u8(data, offset):
    return data[offset]

def read_u16(data, offset):
    return data[offset] | (data[offset + 1] << 8)

def read_u24(data, offset):
    return data[offset] | (data[offset + 1] << 8) | (data[offset + 2] << 16)


# ============================================================
# Palette extraction
# ============================================================

def extract_palettes(rom, output_dir):
    """Extract palette data from bank $C2."""
    print("\n--- Extracting Palettes ---")
    pal_dir = os.path.join(output_dir, 'palettes')
    ensure_dir(pal_dir)

    # Palette data starts at bank $C2:$8000
    pal_start = PALETTE_DATA_START

    # Extract blocks of 512 bytes (256 colors * 2 bytes each)
    # We'll extract a generous range and let the converter sort out specifics
    count = 0
    offset = pal_start

    # Extract palette sets -- each area typically has multiple 32-byte (16-color) palettes
    # A full palette set is 256 colors = 512 bytes
    while offset < pal_start + 0x4000 and offset + 512 <= len(rom):
        pal_data = rom[offset:offset + 512]

        # Basic validity check: not all zeros, not all FF
        if pal_data != bytes(512) and pal_data != bytes([0xFF] * 512):
            out_path = os.path.join(pal_dir, f'palette_{count:03d}.bin')
            with open(out_path, 'wb') as f:
                f.write(pal_data)
            count += 1

        offset += 512

    print(f"  Extracted {count} palette sets to {pal_dir}")
    return count


# ============================================================
# Tileset extraction
# ============================================================

def extract_tilesets(rom, output_dir):
    """Extract and decompress tilesets from Banks $B9-$C1."""
    print("\n--- Extracting Tilesets ---")
    ts_dir = os.path.join(output_dir, 'tilesets')
    ensure_dir(ts_dir)

    count = 0
    for name, (bank, addr) in TILESET_BANKS.items():
        offset = snes_to_rom(bank, addr)
        print(f"  {name}: Bank ${bank:02X} -> ROM offset 0x{offset:06X}")

        try:
            decompressed = decompress(rom, offset, max_output=0x20000)
            if len(decompressed) > 0:
                out_path = os.path.join(ts_dir, f'{name}.bin')
                with open(out_path, 'wb') as f:
                    f.write(decompressed)
                print(f"    -> {len(decompressed)} bytes decompressed")
                count += 1
            else:
                print(f"    -> WARNING: Empty decompression result")
        except Exception as e:
            print(f"    -> ERROR: {e}")

    print(f"  Extracted {count} tilesets to {ts_dir}")
    return count


# ============================================================
# Room header extraction
# ============================================================

def extract_rooms(rom, output_dir):
    """Extract room headers and associated data pointers from bank $8F."""
    print("\n--- Extracting Room Data ---")
    room_dir = os.path.join(output_dir, 'rooms')
    ensure_dir(room_dir)

    # Scan bank $8F for room headers
    # Room headers are variable-length structures
    # We'll scan the known area table offsets

    rooms = []
    bank_8f_start = snes_to_rom(0x8F, 0x8000)
    bank_8f_end = snes_to_rom(0x8F, 0xFFFF)

    # Scan for room headers using area tables
    area_names = ['crateria', 'brinstar', 'norfair', 'wrecked_ship',
                  'maridia', 'tourian', 'ceres']

    # The room header list is a sequence of 2-byte pointers in bank $8F
    # Each pointer points to a room header within the same bank
    # We'll extract what we can find

    for area_id, (area_name, table_offset) in enumerate(
            zip(area_names, AREA_ROOM_TABLE.values())):

        area_dir = os.path.join(room_dir, area_name)
        ensure_dir(area_dir)

        area_count = 0

        # Scan forward from the area table offset for room-like data
        offset = table_offset
        max_scan = min(offset + 0x2000, bank_8f_end)

        while offset < max_scan and offset + ROOM_HDR_SIZE <= len(rom):
            # Read room header
            room_idx = read_u8(rom, offset + ROOM_HDR_INDEX)
            area = read_u8(rom, offset + ROOM_HDR_AREA)
            map_x = read_u8(rom, offset + ROOM_HDR_MAP_X)
            map_y = read_u8(rom, offset + ROOM_HDR_MAP_Y)
            width = read_u8(rom, offset + ROOM_HDR_WIDTH)
            height = read_u8(rom, offset + ROOM_HDR_HEIGHT)

            # Validity check: reasonable dimensions
            if width == 0 or width > 15 or height == 0 or height > 15:
                break
            if area > 7:
                break

            door_ptr = read_u16(rom, offset + ROOM_HDR_DOORLIST)

            # Save room header
            room_data = {
                'offset': offset,
                'index': room_idx,
                'area': area,
                'map_x': map_x,
                'map_y': map_y,
                'width': width,
                'height': height,
                'door_ptr': door_ptr,
            }
            rooms.append(room_data)

            # Write room header binary
            header_bytes = rom[offset:offset + ROOM_HDR_SIZE]
            out_path = os.path.join(area_dir,
                                     f'room_{area_count:03d}_0x{offset:06X}.bin')
            with open(out_path, 'wb') as f:
                f.write(header_bytes)

            area_count += 1

            # Room headers have variable trailing data (state entries)
            # Skip past this header -- advance by minimum header size
            # In practice we'd need to parse state entry count
            offset += ROOM_HDR_SIZE

            # Skip state data entries until we hit the next room
            # (This is a simplification -- proper parsing requires following pointers)
            # For now, scan forward to find the next valid-looking header
            scan = offset
            while scan < max_scan - ROOM_HDR_SIZE:
                next_width = read_u8(rom, scan + ROOM_HDR_WIDTH)
                next_height = read_u8(rom, scan + ROOM_HDR_HEIGHT)
                next_area = read_u8(rom, scan + ROOM_HDR_AREA)
                if (1 <= next_width <= 15 and 1 <= next_height <= 15
                        and next_area <= 7):
                    break
                scan += 1
            offset = scan

        if area_count > 0:
            print(f"  {area_name}: {area_count} rooms")

    # Write room index as JSON-like text
    index_path = os.path.join(room_dir, 'room_index.txt')
    with open(index_path, 'w') as f:
        f.write(f"Total rooms found: {len(rooms)}\n\n")
        for r in rooms:
            f.write(f"Room at 0x{r['offset']:06X}: "
                    f"area={r['area']} idx={r['index']} "
                    f"size={r['width']}x{r['height']} "
                    f"map=({r['map_x']},{r['map_y']}) "
                    f"doors=0x{r['door_ptr']:04X}\n")

    print(f"  Total: {len(rooms)} rooms extracted")
    return len(rooms)


# ============================================================
# Sprite data extraction
# ============================================================

def extract_sprites(rom, output_dir):
    """Extract Samus sprite graphics from banks $90-$9F."""
    print("\n--- Extracting Sprite Data ---")
    spr_dir = os.path.join(output_dir, 'sprites')
    ensure_dir(spr_dir)

    # Extract raw banks for Samus sprites ($90-$9F = ROM $080000-$0FFFFF)
    for bank in range(0x90, 0xA0):
        bank_offset = snes_to_rom(bank, 0x8000)
        bank_data = rom[bank_offset:bank_offset + ROM_BANK_SIZE]

        out_path = os.path.join(spr_dir, f'samus_bank_{bank:02X}.bin')
        with open(out_path, 'wb') as f:
            f.write(bank_data)

    print(f"  Extracted 16 Samus sprite banks to {spr_dir}")

    # Extract enemy graphics banks ($AB-$B1, $B7)
    enemy_dir = os.path.join(spr_dir, 'enemies')
    ensure_dir(enemy_dir)

    enemy_banks = list(range(0xAB, 0xB2)) + [0xB7]
    for bank in enemy_banks:
        bank_offset = snes_to_rom(bank, 0x8000)
        bank_data = rom[bank_offset:bank_offset + ROM_BANK_SIZE]

        out_path = os.path.join(enemy_dir, f'enemy_bank_{bank:02X}.bin')
        with open(out_path, 'wb') as f:
            f.write(bank_data)

    print(f"  Extracted {len(enemy_banks)} enemy graphics banks")


# ============================================================
# Audio data extraction
# ============================================================

def extract_audio(rom, output_dir):
    """Extract music/audio data from banks $CF-$DE."""
    print("\n--- Extracting Audio Data ---")
    audio_dir = os.path.join(output_dir, 'audio')
    ensure_dir(audio_dir)

    for bank in range(0xCF, 0xDF):
        bank_offset = snes_to_rom(bank, 0x8000)
        bank_data = rom[bank_offset:bank_offset + ROM_BANK_SIZE]

        out_path = os.path.join(audio_dir, f'music_bank_{bank:02X}.bin')
        with open(out_path, 'wb') as f:
            f.write(bank_data)

    print(f"  Extracted {0xDF - 0xCF} music banks to {audio_dir}")


# ============================================================
# Main
# ============================================================

def main():
    print("=" * 60)
    print("  Super Metroid ROM Extractor")
    print("  Extracts game data for DS port conversion")
    print("=" * 60)

    # Find ROM
    rom_arg = sys.argv[1] if len(sys.argv) > 1 else None
    rom_path = find_rom(rom_arg)

    if not rom_path:
        print("\nERROR: No ROM file found!")
        print("Place a Super Metroid ROM (.smc/.sfc) in the roms/ directory,")
        print("or pass the path as an argument:")
        print("  python tools/rom_extract.py path/to/rom.sfc")
        sys.exit(1)

    print(f"\nROM: {rom_path}")
    rom = load_rom(rom_path)

    # Output directory
    project_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    output_dir = os.path.join(project_dir, 'assets_raw')
    ensure_dir(output_dir)
    print(f"Output: {output_dir}")

    # Extract all data types
    pal_count = extract_palettes(rom, output_dir)
    ts_count = extract_tilesets(rom, output_dir)
    room_count = extract_rooms(rom, output_dir)
    extract_sprites(rom, output_dir)
    extract_audio(rom, output_dir)

    # Summary
    print("\n" + "=" * 60)
    print("  Extraction Summary")
    print("=" * 60)
    print(f"  Palettes:  {pal_count} sets")
    print(f"  Tilesets:  {ts_count} areas")
    print(f"  Rooms:     {room_count} found")
    print(f"  Sprites:   16 Samus banks + 8 enemy banks")
    print(f"  Audio:     16 music banks")
    print(f"\n  All output in: {output_dir}")
    print("  Next step: Run tools/build_assets.py to convert to DS format")


if __name__ == '__main__':
    main()
