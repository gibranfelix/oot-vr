"""Write glTF buffers and RGBA PNG files with the Python standard library."""

import base64
import struct
import zlib


class GltfBuffer:
    """One glTF buffer with its buffer views and accessors."""

    def __init__(self):
        self.blob = bytearray()
        self.views = []
        self.accessors = []

    def add(self, values, fmt, kind, comp_type, target=None):
        """Adds an accessor. fmt is the struct format of one value, for example "fff". Returns the accessor index."""
        offset = len(self.blob)
        for v in values:
            self.blob.extend(struct.pack("<" + fmt, *(v if isinstance(v, tuple) else (v,))))
        while len(self.blob) % 4:
            self.blob.append(0)
        view = {"buffer": 0, "byteOffset": offset, "byteLength": len(self.blob) - offset}
        if target:
            view["target"] = target
        self.views.append(view)
        accessor = {"bufferView": len(self.views) - 1, "componentType": comp_type, "count": len(values), "type": kind}
        if kind == "VEC3":
            accessor["min"] = [min(v[i] for v in values) for i in range(3)]
            accessor["max"] = [max(v[i] for v in values) for i in range(3)]
        self.accessors.append(accessor)
        return len(self.accessors) - 1

    def fill(self, doc):
        """Puts the buffer, the buffer views, and the accessors in the glTF document."""
        doc["buffers"] = [{"byteLength": len(self.blob), "uri": "data:application/octet-stream;base64," +
                           base64.b64encode(bytes(self.blob)).decode()}]
        doc["bufferViews"] = self.views
        doc["accessors"] = self.accessors
        return doc


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
