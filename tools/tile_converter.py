# tools/tile_converter.py
#
# This script will be responsible for converting Super Metroid SNES graphics
# data into a DSi-compatible format.
#
# Phase 2.1: Graphics Conversion
# Goal: Convert SNES tile data (8x8 2bpp/4bpp) to DSi tile data (8x8 4bpp)
#
# The Super Metroid SNES ROM contains graphics data in various formats,
# typically 2bpp or 4bpp, using 8x8 pixel tiles. The DSi primarily uses
# 4bpp or 8bpp tiles. For this project, we'll aim for 4bpp DSi tiles.
#
# Key considerations:
# - SNES tiles can be 2bpp or 4bpp. DSi will use 4bpp.
# - SNES palettes (15-bit RGB) need to be converted to DSi palettes (15-bit BGR).
# - Tile mapping and VRAM organization for DSi.
#
# This script will initially focus on a simple tile conversion.
# More advanced features like palette conversion and tilemap generation
# will be added in subsequent steps.

import argparse
import os




def snes_4bpp_to_4bpp_dsi_tile(snes_4bpp_tile_data):
    """
    Converts a single 8x8 SNES 4bpp tile to an 8x8 DSi 4bpp tile.
    Assumes standard SNES 4bpp planar format (32 bytes) as input.
    Outputs standard DSi 4bpp interleaved pixel format (32 bytes).
    """
    if len(snes_4bpp_tile_data) != 32:
        raise ValueError("SNES 4bpp tile data must be 32 bytes.")

    dsi_4bpp_tile_data = bytearray(32)

    for y_pixel in range(8):
        for x_pixel in range(8):
            bit_pos = 7 - x_pixel # SNES bits are MSB to LSB for pixels in a byte

            b0 = (snes_4bpp_tile_data[y_pixel] >> bit_pos) & 1
            b1 = (snes_4bpp_tile_data[y_pixel + 8] >> bit_pos) & 1
            b2 = (snes_4bpp_tile_data[y_pixel + 16] >> bit_pos) & 1
            b3 = (snes_4bpp_tile_data[y_pixel + 24] >> bit_pos) & 1

            dsi_pixel_value = (b3 << 3) | (b2 << 2) | (b1 << 1) | b0

            dsi_byte_idx = (y_pixel * 8 + x_pixel) // 2
            if (y_pixel * 8 + x_pixel) % 2 == 0:  # Even pixel, low nibble
                dsi_4bpp_tile_data[dsi_byte_idx] = (dsi_pixel_value & 0xF)
            else:  # Odd pixel, high nibble
                dsi_4bpp_tile_data[dsi_byte_idx] |= ((dsi_pixel_value & 0xF) << 4)
    
    return dsi_4bpp_tile_data

def convert_tiles_from_file(input_file_path, output_file_path, tile_size=32):
    """
    Reads SNES tile data from an input file and converts it to DSi format.
    Assumes each `tile_size` chunk from input is one SNES tile.
    """
    with open(input_file_path, 'rb') as infile:
        snes_data = infile.read()

    dsi_data = bytearray()

    for i in range(0, len(snes_data), tile_size):
        snes_tile = snes_data[i:i+tile_size]
        if len(snes_tile) == tile_size:
            # Assuming 4bpp SNES for now, as 32 bytes is typical for it.
            dsi_tile = snes_4bpp_to_4bpp_dsi_tile(snes_tile)
            dsi_data.extend(dsi_tile)
        else:
            print(f"Warning: Partial tile at offset {i}, skipping.")

    with open(output_file_path, 'wb') as outfile:
        outfile.write(dsi_data)

def main():
    parser = argparse.ArgumentParser(description="Convert Super Metroid SNES tile data to DSi format.")
    parser.add_argument("input_file", help="Path to the input file containing SNES tile data.")
    parser.add_argument("output_file", help="Path for the output file to store DSi tile data.")
    parser.add_argument("--tile_size", type=int, default=32,
                        help="Size of each SNES tile in bytes (e.g., 16 for 2bpp, 32 for 4bpp).")
    
    args = parser.parse_args()

    convert_tiles_from_file(args.input_file, args.output_file, args.tile_size)
    print(f"Successfully converted tiles from '{args.input_file}' to '{args.output_file}'.")

if __name__ == "__main__":
    main()
