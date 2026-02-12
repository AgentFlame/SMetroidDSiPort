#!/usr/bin/env python3
"""
tilemap_converter.py - Convert SNES 16x16 metatile tilemaps to DS BG map format

SNES BG tilemap entry (2 bytes, little-endian):
  Byte 0: Tile number low (bits 7-0)
  Byte 1: bit 7   = V-flip
          bit 6   = H-flip
          bit 5   = Priority
          bits 4-2 = Palette (3 bits)
          bits 1-0 = Tile number high (bits 9-8)

DS BG map entry (2 bytes, 16-bit little-endian):
  Bits 0-9:   Tile index (10 bits)
  Bit 10:     H-flip
  Bit 11:     V-flip
  Bits 12-15: Palette (4 bits)
"""

import argparse
import sys


def snes_tilemap_to_ds_bgmap(snes_data):
    """
    Convert SNES tilemap entries to DS BG map format.

    Args:
        snes_data: Byte array containing SNES tilemap data (2 bytes per entry)

    Returns:
        Byte array containing DS BG map data (2 bytes per entry)
    """
    if len(snes_data) % 2 != 0:
        raise ValueError(f"Tilemap data size must be even, got {len(snes_data)} bytes")

    entry_count = len(snes_data) // 2
    ds_data = bytearray(len(snes_data))

    for i in range(entry_count):
        offset = i * 2

        # Read SNES entry
        byte0 = snes_data[offset]
        byte1 = snes_data[offset + 1]

        # Extract SNES fields
        tile_index = ((byte1 & 0x03) << 8) | byte0  # 10-bit tile number
        h_flip = (byte1 >> 6) & 1                    # Bit 14
        v_flip = (byte1 >> 7) & 1                    # Bit 15
        palette = (byte1 >> 2) & 0x07                # Bits 12-10 (3 bits)
        # priority = (byte1 >> 5) & 1               # Bit 13 (unused by DS)
        if tile_index > 1023:  # DS only supports 10-bit tile index
            print(f"Warning: Tile index {tile_index} at entry {i} exceeds DS limit (1023), truncating",
                  file=sys.stderr)
            tile_index &= 0x3FF

        # Build DS BG map entry
        ds_entry = (palette << 12) | (v_flip << 11) | (h_flip << 10) | tile_index

        # Write as little-endian 16-bit value
        ds_data[offset] = ds_entry & 0xFF
        ds_data[offset + 1] = (ds_entry >> 8) & 0xFF

    return ds_data


def convert_tilemap(input_path, output_path):
    """
    Convert SNES tilemap file to DS BG map format.

    Args:
        input_path: Path to input tilemap file
        output_path: Path to output BG map file
    """
    with open(input_path, 'rb') as f:
        snes_data = f.read()

    ds_data = snes_tilemap_to_ds_bgmap(snes_data)

    with open(output_path, 'wb') as f:
        f.write(ds_data)

    entry_count = len(ds_data) // 2
    print(f"Converted {entry_count} tilemap entries from '{input_path}' to '{output_path}'")


def main():
    parser = argparse.ArgumentParser(
        description="Convert SNES 16x16 metatile tilemap to DS BG map format"
    )
    parser.add_argument("input_file", help="Input tilemap file (SNES format)")
    parser.add_argument("output_file", help="Output BG map file (DS format)")

    args = parser.parse_args()

    try:
        convert_tilemap(args.input_file, args.output_file)
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
