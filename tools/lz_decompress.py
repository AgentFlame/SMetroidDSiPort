"""
lz_decompress.py - Super Metroid LC_LZ2 decompressor.

Implements the compression format used for tilesets and level data.
Based on the format documented in docs/graphics_data.md.

Header byte format:
  Bits 7-5: Command (CCC)
  Bits 4-0: Length-1 (LLLLL)

Commands:
  000 (0): Direct copy - L+1 bytes follow literally
  001 (1): Byte fill - 1 byte follows, repeat L+1 times
  010 (2): Word fill - 2 bytes follow, alternate L+1 times
  011 (3): Increasing fill - 1 byte follows, increment L+1 times
  100 (4): Back reference - 2 bytes follow (offset), copy L+1 bytes from output
  111 (7): Extended length - next byte is high bits of length (10-bit total)
  $FF:     End of compressed data
"""

import struct


def decompress(data, offset=0, max_output=0x10000):
    """
    Decompress LC_LZ2 data starting at the given offset.

    Args:
        data: bytes or bytearray of the full ROM or data block
        offset: starting offset of compressed data
        max_output: maximum output size (safety limit)

    Returns:
        bytearray of decompressed data
    """
    output = bytearray()
    pos = offset

    while pos < len(data):
        # Read command byte
        cmd_byte = data[pos]
        pos += 1

        # End marker
        if cmd_byte == 0xFF:
            break

        # Decode command and length
        command = (cmd_byte >> 5) & 0x07
        length = (cmd_byte & 0x1F) + 1

        # Extended length mode (command 7)
        if command == 7:
            # Real command is in bits 4-2 of the original byte
            command = (cmd_byte >> 2) & 0x07
            # Length is 10-bit: bits 1-0 of cmd_byte are high bits,
            # next byte is low 8 bits
            length = ((cmd_byte & 0x03) << 8) + data[pos] + 1
            pos += 1

        if command == 0:
            # Direct copy: copy L bytes from input to output
            chunk = data[pos:pos + length]
            output.extend(chunk)
            pos += length

        elif command == 1:
            # Byte fill: repeat 1 byte L times
            fill_byte = data[pos]
            pos += 1
            output.extend(bytes([fill_byte]) * length)

        elif command == 2:
            # Word fill: alternate 2 bytes L times
            byte_a = data[pos]
            byte_b = data[pos + 1]
            pos += 2
            for i in range(length):
                output.append(byte_a if (i % 2 == 0) else byte_b)

        elif command == 3:
            # Increasing fill: 1 byte, increment each time
            fill_byte = data[pos]
            pos += 1
            for i in range(length):
                output.append((fill_byte + i) & 0xFF)

        elif command == 4:
            # Back reference: 2 bytes = offset into output buffer
            ref_offset = data[pos] | (data[pos + 1] << 8)
            pos += 2
            for i in range(length):
                if ref_offset + i < len(output):
                    output.append(output[ref_offset + i])
                else:
                    output.append(0)  # Safety: pad with zero

        else:
            # Unknown command -- stop
            break

        # Safety check
        if len(output) >= max_output:
            break

    return output


def decompress_from_file(rom_path, offset, max_output=0x10000):
    """Convenience: decompress from a ROM file at the given offset."""
    with open(rom_path, 'rb') as f:
        data = f.read()

    # Handle optional 512-byte copier header
    if len(data) % 0x8000 == 512:
        data = data[512:]

    return decompress(data, offset, max_output)


if __name__ == '__main__':
    import sys

    if len(sys.argv) < 3:
        print("Usage: python lz_decompress.py <rom_path> <hex_offset> [max_size]")
        print("Example: python lz_decompress.py roms/sm.sfc 0x1C8000")
        sys.exit(1)

    rom_path = sys.argv[1]
    offset = int(sys.argv[2], 16)
    max_size = int(sys.argv[3], 16) if len(sys.argv) > 3 else 0x10000

    result = decompress_from_file(rom_path, offset, max_size)
    print(f"Decompressed {len(result)} bytes from offset 0x{offset:06X}")

    # Optionally write output
    out_path = f"decompressed_0x{offset:06X}.bin"
    with open(out_path, 'wb') as f:
        f.write(result)
    print(f"Written to {out_path}")
