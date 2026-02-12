#!/usr/bin/env python3
"""
build_assets.py - Master asset pipeline for Super Metroid DS port

Pipeline stages:
1. ROM extraction (if needed) - calls rom_extract.py
2. Tile conversion - SNES 4bpp planar to DS 4bpp linear
3. Palette conversion - SNES BGR555 to DS BGR555 (validation + pass-through)
4. Tilemap conversion - SNES metatiles to DS BG map format
5. Output to data/ directory for bin2o embedding

Directory structure:
  assets_raw/        - Extracted ROM data (from rom_extract.py)
    tilesets/        - SNES tile data
    palettes/        - SNES palette data
    tilemaps/        - SNES tilemap data
  data/              - Converted assets ready for DS
    tiles/
    palettes/
    maps/
"""

import argparse
import os
import sys
import subprocess
from pathlib import Path


# Project paths
TOOLS_DIR = Path(__file__).parent
PROJECT_DIR = TOOLS_DIR.parent
ASSETS_RAW_DIR = PROJECT_DIR / "assets_raw"
DATA_DIR = PROJECT_DIR / "data"

# Tool scripts
ROM_EXTRACT_SCRIPT = TOOLS_DIR / "rom_extract.py"
TILE_CONVERTER_SCRIPT = TOOLS_DIR / "tile_converter.py"
PALETTE_CONVERTER_SCRIPT = TOOLS_DIR / "palette_converter.py"
TILEMAP_CONVERTER_SCRIPT = TOOLS_DIR / "tilemap_converter.py"


def run_command(cmd, description):
    """
    Execute a command and handle errors.

    Args:
        cmd: Command list to execute
        description: Human-readable description for error messages
    """
    print(f"[*] {description}")
    try:
        result = subprocess.run(cmd, check=True, capture_output=True, text=True)
        if result.stdout:
            print(result.stdout, end='')
        return True
    except subprocess.CalledProcessError as e:
        print(f"[ERROR] {description} failed:", file=sys.stderr)
        print(e.stderr, file=sys.stderr)
        return False
    except FileNotFoundError as e:
        print(f"[ERROR] Command not found: {e}", file=sys.stderr)
        return False


def extract_rom(rom_path, force=False):
    """
    Run ROM extraction if needed.

    Args:
        rom_path: Path to Super Metroid ROM (optional)
        force: Force re-extraction even if assets_raw exists

    Returns:
        True if extraction succeeded or was skipped, False on error
    """
    if ASSETS_RAW_DIR.exists() and not force:
        print(f"[*] assets_raw/ already exists, skipping ROM extraction (use --force to re-extract)")
        return True

    if not ROM_EXTRACT_SCRIPT.exists():
        print(f"[WARNING] rom_extract.py not found at {ROM_EXTRACT_SCRIPT}, skipping ROM extraction",
              file=sys.stderr)
        return True

    cmd = [sys.executable, str(ROM_EXTRACT_SCRIPT)]
    if rom_path:
        cmd.extend(["--rom", rom_path])
    if force:
        cmd.append("--force")

    return run_command(cmd, "Extracting ROM data")


def convert_tilesets():
    """
    Convert all tilesets from assets_raw/tilesets/ to data/tiles/

    Returns:
        Number of tilesets converted, or -1 on error
    """
    tileset_dir = ASSETS_RAW_DIR / "tilesets"
    output_dir = DATA_DIR / "tiles"

    if not tileset_dir.exists():
        print(f"[WARNING] Tileset directory not found: {tileset_dir}", file=sys.stderr)
        return 0

    output_dir.mkdir(parents=True, exist_ok=True)

    tileset_files = sorted(tileset_dir.glob("*.bin"))
    if not tileset_files:
        print(f"[WARNING] No tileset files (*.bin) found in {tileset_dir}", file=sys.stderr)
        return 0

    converted = 0
    for tileset_file in tileset_files:
        output_file = output_dir / f"{tileset_file.stem}.ds.bin"
        cmd = [
            sys.executable, str(TILE_CONVERTER_SCRIPT),
            str(tileset_file),
            str(output_file)
        ]

        if run_command(cmd, f"Converting tileset: {tileset_file.name}"):
            converted += 1
        else:
            return -1

    return converted


def convert_palettes():
    """
    Convert all palettes from assets_raw/palettes/ to data/palettes/

    Returns:
        Number of palettes converted, or -1 on error
    """
    palette_dir = ASSETS_RAW_DIR / "palettes"
    output_dir = DATA_DIR / "palettes"

    if not palette_dir.exists():
        print(f"[WARNING] Palette directory not found: {palette_dir}", file=sys.stderr)
        return 0

    output_dir.mkdir(parents=True, exist_ok=True)

    palette_files = sorted(palette_dir.glob("*.bin"))
    if not palette_files:
        print(f"[WARNING] No palette files (*.bin) found in {palette_dir}", file=sys.stderr)
        return 0

    converted = 0
    for palette_file in palette_files:
        output_file = output_dir / f"{palette_file.stem}.ds.pal"
        cmd = [
            sys.executable, str(PALETTE_CONVERTER_SCRIPT),
            str(palette_file),
            str(output_file)
        ]

        if run_command(cmd, f"Converting palette: {palette_file.name}"):
            converted += 1
        else:
            return -1

    return converted


def convert_tilemaps():
    """
    Convert all tilemaps from assets_raw/tilemaps/ to data/maps/

    Returns:
        Number of tilemaps converted, or -1 on error
    """
    tilemap_dir = ASSETS_RAW_DIR / "tilemaps"
    output_dir = DATA_DIR / "maps"

    if not tilemap_dir.exists():
        print(f"[WARNING] Tilemap directory not found: {tilemap_dir}", file=sys.stderr)
        return 0

    output_dir.mkdir(parents=True, exist_ok=True)

    tilemap_files = sorted(tilemap_dir.glob("*.bin"))
    if not tilemap_files:
        print(f"[WARNING] No tilemap files (*.bin) found in {tilemap_dir}", file=sys.stderr)
        return 0

    converted = 0
    for tilemap_file in tilemap_files:
        output_file = output_dir / f"{tilemap_file.stem}.ds.map"
        cmd = [
            sys.executable, str(TILEMAP_CONVERTER_SCRIPT),
            str(tilemap_file),
            str(output_file)
        ]

        if run_command(cmd, f"Converting tilemap: {tilemap_file.name}"):
            converted += 1
        else:
            return -1

    return converted


def main():
    parser = argparse.ArgumentParser(
        description="Master asset pipeline for Super Metroid DS port"
    )
    parser.add_argument("--rom-path", help="Path to Super Metroid ROM (for extraction)")
    parser.add_argument("--force", action="store_true",
                        help="Force re-extraction of ROM even if assets_raw exists")

    args = parser.parse_args()

    print("=" * 60)
    print("Super Metroid DS - Asset Conversion Pipeline")
    print("=" * 60)

    # Stage 1: ROM extraction
    if not extract_rom(args.rom_path, args.force):
        print("\n[FAILED] ROM extraction failed", file=sys.stderr)
        sys.exit(1)

    # Stage 2: Tile conversion
    print("\n" + "-" * 60)
    tileset_count = convert_tilesets()
    if tileset_count < 0:
        print("\n[FAILED] Tileset conversion failed", file=sys.stderr)
        sys.exit(1)
    print(f"[OK] Converted {tileset_count} tilesets")

    # Stage 3: Palette conversion
    print("\n" + "-" * 60)
    palette_count = convert_palettes()
    if palette_count < 0:
        print("\n[FAILED] Palette conversion failed", file=sys.stderr)
        sys.exit(1)
    print(f"[OK] Converted {palette_count} palettes")

    # Stage 4: Tilemap conversion
    print("\n" + "-" * 60)
    tilemap_count = convert_tilemaps()
    if tilemap_count < 0:
        print("\n[FAILED] Tilemap conversion failed", file=sys.stderr)
        sys.exit(1)
    print(f"[OK] Converted {tilemap_count} tilemaps")

    # Summary
    print("\n" + "=" * 60)
    print("Asset Conversion Complete")
    print("=" * 60)
    print(f"Tilesets:  {tileset_count}")
    print(f"Palettes:  {palette_count}")
    print(f"Tilemaps:  {tilemap_count}")
    print(f"\nOutput directory: {DATA_DIR}")
    print("\nNext step: Run 'make' to build the DS ROM with embedded assets")


if __name__ == "__main__":
    main()
