#!/usr/bin/env python3
"""
tile_converter.py - Convert SNES 4bpp planar tiles to DS 4bpp linear format

SNES 4bpp planar format (32 bytes per 8x8 tile):
  Bytes 0-1:   Row 0, bitplanes 0-1 (interleaved pair)
  Bytes 2-3:   Row 1, bitplanes 0-1
  ...
  Bytes 14-15: Row 7, bitplanes 0-1
  Bytes 16-17: Row 0, bitplanes 2-3 (interleaved pair)
  Bytes 18-19: Row 1, bitplanes 2-3
  ...
  Bytes 30-31: Row 7, bitplanes 2-3

DS 4bpp linear format (32 bytes per 8x8 tile):
  Each byte holds 2 pixels (low nibble = even pixel, high nibble = odd pixel)
  4 bytes per row, 32 bytes per tile
"""

import argparse
import sys


def snes_4bpp_to_ds_4bpp_tile(snes_data):
    """
    Convert a single 8x8 SNES 4bpp planar tile to DS 4bpp linear format.

    Args:
        snes_data: 32-byte bytearray/bytes containing SNES tile data

    Returns:
        32-byte bytearray containing DS tile data
    """
    if len(snes_data) != 32:
        raise ValueError(f"SNES 4bpp tile must be 32 bytes, got {len(snes_data)}")

    ds_data = bytearray(32)

    for y in range(8):
        # Read bitplane pairs for this row (CORRECT interleaved format)
        b0 = snes_data[y * 2 + 0]   # Bitplane 0
        b1 = snes_data[y * 2 + 1]   # Bitplane 1
        b2 = snes_data[y * 2 + 16]  # Bitplane 2
        b3 = snes_data[y * 2 + 17]  # Bitplane 3

        # Extract pixels for this row (MSB to LSB = left to right)
        for x in range(8):
            bit_pos = 7 - x  # MSB first

            # Combine bitplanes to form 4-bit pixel value
            pixel = (((b3 >> bit_pos) & 1) << 3) | \
                    (((b2 >> bit_pos) & 1) << 2) | \
                    (((b1 >> bit_pos) & 1) << 1) | \
                    (((b0 >> bit_pos) & 1) << 0)

            # Write to DS linear format (2 pixels per byte)
            ds_byte_idx = y * 4 + x // 2
            if x % 2 == 0:
                # Even pixel: low nibble
                ds_data[ds_byte_idx] = pixel & 0x0F
            else:
                # Odd pixel: high nibble
                ds_data[ds_byte_idx] |= (pixel & 0x0F) << 4

    return ds_data


def convert_tiles(input_path, output_path, tile_size=32):
    """
    Convert a file of SNES tiles to DS format.

    Args:
        input_path: Path to input file containing SNES tile data
        output_path: Path to output file for DS tile data
        tile_size: Size of each tile in bytes (default 32 for 4bpp)
    """
    with open(input_path, 'rb') as f:
        snes_data = f.read()

    if len(snes_data) % tile_size != 0:
        print(f"Warning: Input file size ({len(snes_data)} bytes) is not a multiple of tile_size ({tile_size})",
              file=sys.stderr)

    ds_data = bytearray()
    tile_count = 0

    for offset in range(0, len(snes_data), tile_size):
        snes_tile = snes_data[offset:offset + tile_size]

        if len(snes_tile) == tile_size:
            ds_tile = snes_4bpp_to_ds_4bpp_tile(snes_tile)
            ds_data.extend(ds_tile)
            tile_count += 1
        else:
            print(f"Warning: Partial tile at offset {offset} ({len(snes_tile)} bytes), skipping",
                  file=sys.stderr)

    with open(output_path, 'wb') as f:
        f.write(ds_data)

    print(f"Converted {tile_count} tiles from '{input_path}' to '{output_path}'")


def main():
    parser = argparse.ArgumentParser(
        description="Convert SNES 4bpp planar tiles to DS 4bpp linear format"
    )
    parser.add_argument("input_file", help="Input file containing SNES tile data")
    parser.add_argument("output_file", help="Output file for DS tile data")
    parser.add_argument("--tile_size", type=int, default=32,
                        help="Size of each tile in bytes (default: 32 for 4bpp)")

    args = parser.parse_args()

    try:
        convert_tiles(args.input_file, args.output_file, args.tile_size)
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
