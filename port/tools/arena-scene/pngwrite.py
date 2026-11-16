"""Write an RGBA PNG file with the Python standard library."""

import struct
import zlib


def _chunk(kind, data):
    return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data) & 0xFFFFFFFF)


def write_png(path, width, height, pixel):
    """pixel(x, y) returns (r, g, b, a), each 0 to 255."""
    rows = bytearray()
    for y in range(height):
        rows.append(0)
        for x in range(width):
            rows.extend(bytes(pixel(x, y)))
    data = b"\x89PNG\r\n\x1a\n"
    data += _chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0))
    data += _chunk(b"IDAT", zlib.compress(bytes(rows), 9))
    data += _chunk(b"IEND", b"")
    with open(path, "wb") as f:
        f.write(data)
