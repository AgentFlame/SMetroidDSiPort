"""
rom_addresses.py - Super Metroid ROM address constants and conversion utilities.

SNES address space -> ROM file offset conversion.
All addresses sourced from docs/rom_memory_map.md.
"""

# Super Metroid uses LoROM mapping (despite HiROM-like bank layout).
# Community disassembly addresses use banks $80-$FF with addr $8000-$FFFF.
# ROM offset = ((bank & 0x7F) * 0x8000) + (addr - 0x8000)
# Bank $80 = ROM $000000, Bank $81 = ROM $008000, ...
# Each bank = 32KB ($8000 bytes)

ROM_BANK_SIZE = 0x8000  # 32KB per bank

def snes_to_rom(bank, addr):
    """Convert SNES bank:addr to ROM file offset."""
    return ((bank & 0x7F) * ROM_BANK_SIZE) + (addr - 0x8000)

def snes_long_to_rom(long_addr):
    """Convert SNES 24-bit long address to ROM file offset."""
    bank = (long_addr >> 16) & 0xFF
    addr = long_addr & 0xFFFF
    return snes_to_rom(bank, addr)

# Expected unheadered ROM size
SM_ROM_SIZE = 0x300000  # 3 MB (24 Mbit)
SM_HEADER_SIZE = 512    # Optional copier header

# ============================================================
# Key ROM regions by category
# ============================================================

# Room headers (Bank $8F)
ROOM_HEADERS_BANK = 0x8F
ROOM_HEADERS_START = snes_to_rom(0x8F, 0x91F8)  # First room header pointer

# Known room header pointers (area header tables in bank $8F)
# These are pointers to the start of room data for each area
AREA_ROOM_TABLE = {
    0: snes_to_rom(0x8F, 0x91F8),   # Crateria
    1: snes_to_rom(0x8F, 0x92B0),   # Brinstar
    2: snes_to_rom(0x8F, 0x93B8),   # Norfair
    3: snes_to_rom(0x8F, 0x948C),   # Wrecked Ship
    4: snes_to_rom(0x8F, 0x9510),   # Maridia
    5: snes_to_rom(0x8F, 0x95DC),   # Tourian
    6: snes_to_rom(0x8F, 0x962A),   # Ceres
}

# Palette data (Bank $C2)
PALETTE_DATA_START = snes_to_rom(0xC2, 0x8000)

# Tileset data banks
TILESET_BANKS = {
    'cre':           (0xB9, 0x8000),  # Common Room Elements
    'crateria':      (0xBA, 0x8000),
    'brinstar':      (0xBB, 0x8000),
    'norfair':       (0xBC, 0x8000),
    'wrecked_ship':  (0xBD, 0x8000),
    'maridia':       (0xBE, 0x8000),
    'tourian':       (0xBF, 0x8000),
    'ceres':         (0xC0, 0x8000),
}

# Level tilemap data (compressed, Banks $C3-$CE)
LEVEL_DATA_START = snes_to_rom(0xC3, 0x8000)
LEVEL_DATA_END   = snes_to_rom(0xCE, 0xFFFF)

# Samus sprite data
SAMUS_SPRITES_START = 0x080000   # Banks $90-$9F
SAMUS_SPRITES_DMA   = 0x0DEC00  # Documented main sprite start

# Enemy graphics (Banks $AB-$B1, $B7)
ENEMY_GFX_START = snes_to_rom(0xAB, 0x8000)
ENEMY_GFX_END   = snes_to_rom(0xB1, 0xFFFF)
ENEMY_GFX_EXTRA = snes_to_rom(0xB7, 0x8000)

# Music data (Banks $CF-$DE)
MUSIC_DATA_START = snes_to_rom(0xCF, 0x8000)
MUSIC_DATA_END   = snes_to_rom(0xDE, 0xFFFF)

# Door definitions (Bank $83)
DOOR_DATA_BANK = 0x83

# Enemy population (Bank $A1)
ENEMY_POP_BANK = 0xA1

# PLM data (Bank $84)
PLM_DATA_BANK = 0x84

# FX data (Bank $83)
FX_DATA_BANK = 0x83

# ============================================================
# Room header structure offsets
# ============================================================

ROOM_HDR_INDEX       = 0x00  # 1 byte
ROOM_HDR_AREA        = 0x01  # 1 byte
ROOM_HDR_MAP_X       = 0x02  # 1 byte
ROOM_HDR_MAP_Y       = 0x03  # 1 byte
ROOM_HDR_WIDTH       = 0x04  # 1 byte (in screens)
ROOM_HDR_HEIGHT      = 0x05  # 1 byte (in screens)
ROOM_HDR_UP_SCROLL   = 0x06  # 1 byte
ROOM_HDR_DOWN_SCROLL = 0x07  # 1 byte
ROOM_HDR_GFX_FLAGS   = 0x08  # 1 byte
ROOM_HDR_DOORLIST    = 0x09  # 2 bytes (pointer)
ROOM_HDR_SIZE        = 0x0B  # Minimum header size

# ============================================================
# Enemy population entry offsets
# ============================================================

ENEMY_ENTRY_ID       = 0x00  # 2 bytes
ENEMY_ENTRY_X        = 0x02  # 2 bytes
ENEMY_ENTRY_Y        = 0x04  # 2 bytes
ENEMY_ENTRY_PARAM    = 0x06  # 2 bytes
ENEMY_ENTRY_PROPS    = 0x08  # 2 bytes
ENEMY_ENTRY_EXTRA1   = 0x0A  # 2 bytes
ENEMY_ENTRY_EXTRA2   = 0x0C  # 2 bytes
ENEMY_ENTRY_PARAM1   = 0x0E  # 2 bytes
ENEMY_ENTRY_PARAM2   = 0x10  # 2 bytes
ENEMY_ENTRY_SIZE     = 0x12  # 18 bytes per entry

# Door entry offsets
DOOR_ENTRY_DEST      = 0x00  # 2 bytes
DOOR_ENTRY_BITFLAG   = 0x02  # 1 byte
DOOR_ENTRY_DIR       = 0x03  # 1 byte
DOOR_ENTRY_XCAP      = 0x04  # 2 bytes
DOOR_ENTRY_YCAP      = 0x06  # 2 bytes
DOOR_ENTRY_XSCREEN   = 0x08  # 2 bytes
DOOR_ENTRY_YSCREEN   = 0x0A  # 2 bytes
DOOR_ENTRY_DIST      = 0x0C  # 2 bytes
DOOR_ENTRY_ASM       = 0x0E  # 2 bytes
DOOR_ENTRY_SIZE      = 0x10  # 16 bytes per entry


if __name__ == '__main__':
    print("Super Metroid ROM Address Map")
    print("=" * 50)
    print(f"Room headers start: 0x{ROOM_HEADERS_START:06X}")
    print(f"Palette data start: 0x{PALETTE_DATA_START:06X}")
    print(f"Level data range:   0x{LEVEL_DATA_START:06X} - 0x{LEVEL_DATA_END:06X}")
    print(f"Samus sprites:      0x{SAMUS_SPRITES_START:06X}")
    print(f"Enemy graphics:     0x{ENEMY_GFX_START:06X} - 0x{ENEMY_GFX_END:06X}")
    print(f"Music data:         0x{MUSIC_DATA_START:06X} - 0x{MUSIC_DATA_END:06X}")
    print()
    for area_name, (bank, addr) in TILESET_BANKS.items():
        offset = snes_to_rom(bank, addr)
        print(f"Tileset {area_name:15s}: Bank ${bank:02X} -> ROM 0x{offset:06X}")
