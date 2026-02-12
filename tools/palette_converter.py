#!/usr/bin/env python3
"""
palette_converter.py - Convert SNES palettes to DS format

Both SNES and DS use BGR555 format (15-bit color, 1 bit unused):
  Bit 0-4:   Blue (5 bits)
  Bit 5-9:   Green (5 bits)
  Bit 10-14: Red (5 bits)
  Bit 15:    Unused

Since the formats are identical, this is a validation and pass-through tool.
Standard palette size: 512 bytes = 256 colors * 2 bytes per color.
"""

import argparse
import sys


def convert_palette(input_path, output_path, expected_size=512):
    """
    Validate and copy SNES palette data to DS format.

    Args:
        input_path: Path to input palette file
        output_path: Path to output palette file
        expected_size: Expected palette size in bytes (default 512 for 256-color palette)
    """
    with open(input_path, 'rb') as f:
        palette_data = f.read()

    # Validate size
    if len(palette_data) != expected_size:
        print(f"Warning: Expected {expected_size} bytes, got {len(palette_data)} bytes",
              file=sys.stderr)

    if len(palette_data) % 2 != 0:
        raise ValueError(f"Palette size must be even (2 bytes per color), got {len(palette_data)} bytes")

    color_count = len(palette_data) // 2

    # Optional validation: check if colors are valid BGR555 (bit 15 should be 0)
    invalid_colors = 0
    for i in range(color_count):
        color = palette_data[i * 2] | (palette_data[i * 2 + 1] << 8)
        if color & 0x8000:  # Bit 15 set
            invalid_colors += 1

    if invalid_colors > 0:
        print(f"Warning: {invalid_colors}/{color_count} colors have bit 15 set (should be 0 for BGR555)",
              file=sys.stderr)

    # Write output (direct copy - formats are identical)
    with open(output_path, 'wb') as f:
        f.write(palette_data)

    print(f"Converted palette with {color_count} colors from '{input_path}' to '{output_path}'")


def main():
    parser = argparse.ArgumentParser(
        description="Convert SNES BGR555 palette to DS BGR555 format (validation + pass-through)"
    )
    parser.add_argument("input_file", help="Input palette file")
    parser.add_argument("output_file", help="Output palette file")
    parser.add_argument("--size", type=int, default=512,
                        help="Expected palette size in bytes (default: 512 for 256 colors)")

    args = parser.parse_args()

    try:
        convert_palette(args.input_file, args.output_file, args.size)
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
