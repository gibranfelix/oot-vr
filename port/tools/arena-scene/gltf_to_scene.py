#!/usr/bin/env python3
"""Convert a glTF 2.0 file to the XML resources of a Ship of Harkinian scene.

The output is one scene with one room. The files are the scene, the room, the collision header, the display list of
the room, the vertex buffers, and the textures. The XML loaders of the game read these files from soh.o2r
(port/soh/soh/resource/importer/ and libultraship/src/fast/resource/factory/).

The glTF file can have:
- Meshes. Each triangle is drawn and collides. A node with a name that starts with "col_" only collides. A node with a
  name that starts with "vis_" is only drawn.
- Materials with a base color factor and a base color texture (PNG, power-of-two size, 4 KB or smaller in RGBA16).
- One directional light (KHR_lights_punctual). It lights Link and the enemies, and the script bakes it into the vertex
  colors of the room.
- One node with the name "spawn". It gives the start position and the direction of Link.
- Scene extras: "ambient" ([r, g, b], 0 to 1), "fogColor" ([r, g, b], 0 to 1), "fogNear", and "fogFar".

Only the Python standard library is necessary.
"""

import argparse
import base64
import json
import math
import os
import shutil
import struct
import sys
import xml.etree.ElementTree as ET

# One meter of the glTF file is 40 units of the game: adult Link has a height of 68 units.
DEFAULT_UNITS_PER_METER = 40.0
# libultraship accepts 64 vertices in one load (MAX_VERTICES), and the original microcode accepts 32.
MAX_VERTICES_PER_LOAD = 32
# The texture memory of the N64 has 4 KB.
TMEM_BYTES = 4096

CAM_SET_DUNGEON0 = 3
NA_BGM_NO_MUSIC = 0x7F
NATURE_ID_DISABLED = 0xFF
PLAYER_PARAMS_STAND = 0x0FFF


class SceneError(Exception):
    pass


# ---------------------------------------------------------------------------------------------------------------------
# Vector math


def sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def cross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def normalize(v):
    length = math.sqrt(dot(v, v))
    if length == 0.0:
        return None
    return (v[0] / length, v[1] / length, v[2] / length)


def mat_mul(a, b):
    """4x4 matrices, column-major lists of 16 values (the glTF layout)."""
    out = [0.0] * 16
    for col in range(4):
        for row in range(4):
            out[col * 4 + row] = sum(a[k * 4 + row] * b[col * 4 + k] for k in range(4))
    return out


def mat_from_trs(node):
    if "matrix" in node:
        return [float(v) for v in node["matrix"]]
    tx, ty, tz = node.get("translation", [0.0, 0.0, 0.0])
    qx, qy, qz, qw = node.get("rotation", [0.0, 0.0, 0.0, 1.0])
    sx, sy, sz = node.get("scale", [1.0, 1.0, 1.0])
    r = [
        1 - 2 * (qy * qy + qz * qz), 2 * (qx * qy + qz * qw), 2 * (qx * qz - qy * qw),
        2 * (qx * qy - qz * qw), 1 - 2 * (qx * qx + qz * qz), 2 * (qy * qz + qx * qw),
        2 * (qx * qz + qy * qw), 2 * (qy * qz - qx * qw), 1 - 2 * (qx * qx + qy * qy),
    ]
    return [
        r[0] * sx, r[1] * sx, r[2] * sx, 0.0,
        r[3] * sy, r[4] * sy, r[5] * sy, 0.0,
        r[6] * sz, r[7] * sz, r[8] * sz, 0.0,
        tx, ty, tz, 1.0,
    ]


def transform_point(m, p):
    return (
        m[0] * p[0] + m[4] * p[1] + m[8] * p[2] + m[12],
        m[1] * p[0] + m[5] * p[1] + m[9] * p[2] + m[13],
        m[2] * p[0] + m[6] * p[1] + m[10] * p[2] + m[14],
    )


def transform_dir(m, d):
    return (
        m[0] * d[0] + m[4] * d[1] + m[8] * d[2],
        m[1] * d[0] + m[5] * d[1] + m[9] * d[2],
        m[2] * d[0] + m[6] * d[1] + m[10] * d[2],
    )


def mat_determinant3(m):
    return (m[0] * (m[5] * m[10] - m[9] * m[6]) - m[4] * (m[1] * m[10] - m[9] * m[2]) +
            m[8] * (m[1] * m[6] - m[5] * m[2]))


# ---------------------------------------------------------------------------------------------------------------------
# glTF reading

COMPONENT_FORMATS = {5120: "b", 5121: "B", 5122: "h", 5123: "H", 5125: "I", 5126: "f"}
COMPONENT_MAX = {5120: 127.0, 5121: 255.0, 5122: 32767.0, 5123: 65535.0}
TYPE_SIZES = {"SCALAR": 1, "VEC2": 2, "VEC3": 3, "VEC4": 4}


class Gltf:
    def __init__(self, path):
        self.dir = os.path.dirname(os.path.abspath(path))
        with open(path, "rb") as f:
            data = f.read()
        self.glb_bin = None
        if data[:4] == b"glTF":
            self.doc = self._read_glb(data)
        else:
            self.doc = json.loads(data.decode("utf-8"))
        self._buffers = {}

    def _read_glb(self, data):
        _, version, length = struct.unpack_from("<4sII", data, 0)
        if version != 2:
            raise SceneError("Only glTF 2.0 is supported")
        offset = 12
        doc = None
        while offset < length:
            chunk_length, chunk_type = struct.unpack_from("<II", data, offset)
            chunk = data[offset + 8:offset + 8 + chunk_length]
            if chunk_type == 0x4E4F534A:
                doc = json.loads(chunk.decode("utf-8"))
            elif chunk_type == 0x004E4942:
                self.glb_bin = chunk
            offset += 8 + chunk_length
        if doc is None:
            raise SceneError("The GLB file has no JSON chunk")
        return doc

    def _uri_bytes(self, uri):
        if uri.startswith("data:"):
            return base64.b64decode(uri.split(",", 1)[1])
        with open(os.path.join(self.dir, uri), "rb") as f:
            return f.read()

    def buffer(self, index):
        if index not in self._buffers:
            buf = self.doc["buffers"][index]
            self._buffers[index] = self._uri_bytes(buf["uri"]) if "uri" in buf else self.glb_bin
        return self._buffers[index]

    def accessor(self, index):
        acc = self.doc["accessors"][index]
        if "sparse" in acc:
            raise SceneError("Sparse accessors are not supported")
        view = self.doc["bufferViews"][acc["bufferView"]]
        data = self.buffer(view["buffer"])
        fmt = COMPONENT_FORMATS[acc["componentType"]]
        comps = TYPE_SIZES[acc["type"]]
        comp_size = struct.calcsize(fmt)
        stride = view.get("byteStride", comp_size * comps)
        base = view.get("byteOffset", 0) + acc.get("byteOffset", 0)
        normalized = acc.get("normalized", False)
        scale = COMPONENT_MAX.get(acc["componentType"]) if normalized else None
        out = []
        for i in range(acc["count"]):
            values = struct.unpack_from("<" + fmt * comps, data, base + i * stride)
            if scale:
                values = tuple(max(v / scale, -1.0) for v in values)
            out.append(values if comps > 1 else values[0])
        return out

    def image_path(self, texture_index):
        image = self.doc["images"][self.doc["textures"][texture_index]["source"]]
        if "uri" not in image or image["uri"].startswith("data:"):
            raise SceneError("Textures must be external PNG files")
        return os.path.join(self.dir, image["uri"])


def png_size(path):
    with open(path, "rb") as f:
        header = f.read(24)
    if header[:8] != b"\x89PNG\r\n\x1a\n":
        raise SceneError("%s is not a PNG file" % path)
    return struct.unpack(">II", header[16:24])


# ---------------------------------------------------------------------------------------------------------------------
# Scene model


class Material:
    def __init__(self, name, color, texture_path):
        self.name = name
        self.color = color
        self.texture_path = texture_path
        self.width = self.height = 0
        if texture_path:
            self.width, self.height = png_size(texture_path)
            for size in (self.width, self.height):
                if size < 4 or size & (size - 1):
                    raise SceneError("Texture %s: the size must be a power of two" % texture_path)
            if self.width * self.height * 2 > TMEM_BYTES:
                raise SceneError("Texture %s is too large for TMEM in RGBA16" % texture_path)


class Triangle:
    def __init__(self, positions, uvs, colors, material):
        self.positions = positions
        self.uvs = uvs
        self.colors = colors
        self.material = material


class SceneData:
    def __init__(self):
        self.draw = []
        self.collide = []
        self.spawn = (0.0, 0.0, 0.0)
        self.spawn_yaw = 0
        self.light_dir = normalize((1.0, 2.0, 1.0))
        self.light_color = (0.8, 0.8, 0.8)
        self.ambient = (0.4, 0.4, 0.4)
        self.fog_color = (0.0, 0.0, 0.0)
        self.fog_near = 996
        self.fog_far = 12800


def safe_name(name, fallback):
    out = "".join(c if c.isalnum() or c == "_" else "_" for c in (name or ""))
    return out or fallback


def load_scene(path, units_per_meter=DEFAULT_UNITS_PER_METER):
    gltf = Gltf(path)
    doc = gltf.doc
    scene = SceneData()

    materials = []
    for i, mat in enumerate(doc.get("materials", [])):
        pbr = mat.get("pbrMetallicRoughness", {})
        color = tuple(pbr.get("baseColorFactor", [1.0, 1.0, 1.0, 1.0]))
        tex = pbr.get("baseColorTexture")
        texture_path = gltf.image_path(tex["index"]) if tex else None
        materials.append(Material(safe_name(mat.get("name"), "mat%d" % i), color, texture_path))
    default_material = Material("default", (1.0, 1.0, 1.0, 1.0), None)

    scene_index = doc.get("scene", 0)
    root_nodes = doc["scenes"][scene_index]["nodes"] if doc.get("scenes") else range(len(doc.get("nodes", [])))
    extras = doc["scenes"][scene_index].get("extras", {}) if doc.get("scenes") else {}
    if "ambient" in extras:
        scene.ambient = tuple(extras["ambient"])
    if "fogColor" in extras:
        scene.fog_color = tuple(extras["fogColor"])
    scene.fog_near = int(extras.get("fogNear", scene.fog_near))
    scene.fog_far = int(extras.get("fogFar", scene.fog_far))

    lights = doc.get("extensions", {}).get("KHR_lights_punctual", {}).get("lights", [])
    meter = [units_per_meter, 0, 0, 0, 0, units_per_meter, 0, 0, 0, 0, units_per_meter, 0, 0, 0, 0, 1]

    def visit(node_index, parent):
        node = doc["nodes"][node_index]
        world = mat_mul(parent, mat_from_trs(node))
        name = node.get("name", "")
        if name == "spawn":
            scene.spawn = transform_point(world, (0.0, 0.0, 0.0))
            forward = transform_dir(world, (0.0, 0.0, 1.0))
            scene.spawn_yaw = int(round(math.atan2(forward[0], forward[2]) * 0x8000 / math.pi)) & 0xFFFF
        light_ref = node.get("extensions", {}).get("KHR_lights_punctual")
        if light_ref is not None:
            light = lights[light_ref["light"]]
            if light.get("type") == "directional":
                # The light goes along -Z of its node. The game wants the direction to the light.
                to_light = normalize(transform_dir(world, (0.0, 0.0, 1.0)))
                if to_light:
                    scene.light_dir = to_light
                intensity = min(float(light.get("intensity", 1.0)), 1.0)
                scene.light_color = tuple(c * intensity for c in light.get("color", [1.0, 1.0, 1.0]))
        if "mesh" in node:
            draw = not name.startswith("col_")
            collide = not name.startswith("vis_")
            add_mesh(gltf, doc["meshes"][node["mesh"]], world, materials, default_material, scene, draw, collide)
        for child in node.get("children", []):
            visit(child, world)

    for node_index in root_nodes:
        visit(node_index, meter)

    if not scene.draw:
        raise SceneError("The glTF file has no triangles to draw")
    if not scene.collide:
        raise SceneError("The glTF file has no collision triangles")
    return scene


def add_mesh(gltf, mesh, world, materials, default_material, scene, draw, collide):
    flip = mat_determinant3(world) < 0.0
    for prim in mesh["primitives"]:
        if prim.get("mode", 4) != 4:
            raise SceneError("Only triangle lists are supported")
        attrs = prim["attributes"]
        positions = [transform_point(world, p) for p in gltf.accessor(attrs["POSITION"])]
        count = len(positions)
        uvs = gltf.accessor(attrs["TEXCOORD_0"]) if "TEXCOORD_0" in attrs else [(0.0, 0.0)] * count
        if "COLOR_0" in attrs:
            colors = [tuple(c) + (1.0,) * (4 - len(c)) for c in gltf.accessor(attrs["COLOR_0"])]
        else:
            colors = [(1.0, 1.0, 1.0, 1.0)] * count
        indices = gltf.accessor(prim["indices"]) if "indices" in prim else list(range(count))
        material = materials[prim["material"]] if "material" in prim else default_material
        for t in range(0, len(indices) - 2, 3):
            tri = [indices[t], indices[t + 1], indices[t + 2]]
            if flip:
                tri = [tri[0], tri[2], tri[1]]
            triangle = Triangle([positions[i] for i in tri], [uvs[i] for i in tri], [colors[i] for i in tri],
                                material)
            if draw:
                scene.draw.append(triangle)
            if collide:
                scene.collide.append(triangle)


# ---------------------------------------------------------------------------------------------------------------------
# Conversion to the formats of the game


def to_s16(value, what):
    v = int(round(value))
    if v < -32768 or v > 32767:
        raise SceneError("%s %d is out of the signed 16-bit range" % (what, v))
    return v


def to_u8(value):
    return max(0, min(255, int(round(value * 255.0))))


def collision_polygon(a, b, c):
    """The normal (times 32767) and the distance of a collision polygon, as the game reads them in z_bgcheck.c."""
    n = normalize(cross(sub(b, a), sub(c, a)))
    if n is None:
        return None
    normal = tuple(int(round(x * 0x7FFF)) for x in n)
    dist = int(round(-dot(n, a)))
    return normal, dist


def baked_color(scene, triangle, vertex, normal):
    light = max(0.0, dot(normal, scene.light_dir)) if normal else 0.0
    base = triangle.colors[vertex]
    mat = triangle.material.color
    return tuple(
        min(1.0, base[i] * mat[i] * (scene.ambient[i] + scene.light_color[i] * light)) for i in range(3)
    ) + (base[3] * mat[3],)


def tex_coords(triangle):
    """S and T in 10.5 fixed point. Each triangle moves its UVs by whole repeats, so large floors stay in range."""
    mat = triangle.material
    if not mat.texture_path:
        return [(0, 0)] * 3
    du = math.floor(min(uv[0] for uv in triangle.uvs))
    dv = math.floor(min(uv[1] for uv in triangle.uvs))
    return [
        (to_s16((uv[0] - du) * mat.width * 32, "S"), to_s16((uv[1] - dv) * mat.height * 32, "T"))
        for uv in triangle.uvs
    ]


def batches(triangles):
    """Groups the vertices of the triangles in loads of MAX_VERTICES_PER_LOAD vertices or fewer."""
    current_vertices = []
    current_index = {}
    current_tris = []
    for tri_vertices in triangles:
        needed = [v for v in dict.fromkeys(tri_vertices) if v not in current_index]
        if len(current_vertices) + len(needed) > MAX_VERTICES_PER_LOAD:
            yield current_vertices, current_tris
            current_vertices, current_index, current_tris = [], {}, []
            needed = list(dict.fromkeys(tri_vertices))
        for v in needed:
            current_index[v] = len(current_vertices)
            current_vertices.append(v)
        current_tris.append(tuple(current_index[v] for v in tri_vertices))
    if current_tris:
        yield current_vertices, current_tris


def xml_bytes(root):
    ET.indent(root, space="\t")
    return ET.tostring(root, encoding="unicode").encode("utf-8") + b"\n"


def el(parent, tag, **attrs):
    node = ET.SubElement(parent, tag) if parent is not None else ET.Element(tag)
    for key, value in attrs.items():
        node.set(key, str(value))
    return node


def material_commands(dl, mat, texture_resource):
    el(dl, "PipeSync")
    el(dl, "ClearGeometryMode", G_LIGHTING=1, G_TEXTURE_GEN=1)
    el(dl, "SetGeometryMode", G_ZBUFFER=1, G_SHADE=1, G_FOG=1, G_CULL_BACK=1, G_SHADING_SMOOTH=1)
    # Cycle 1: the texture times the vertex color, or only the vertex color. Cycle 2 keeps the result of cycle 1.
    first = ("G_CCMUX_TEXEL0", "G_CCMUX_0", "G_CCMUX_SHADE", "G_CCMUX_0") if texture_resource else \
        ("G_CCMUX_0", "G_CCMUX_0", "G_CCMUX_0", "G_CCMUX_SHADE")
    el(dl, "SetCombineLERP", A0=first[0], B0=first[1], C0=first[2], D0=first[3], Aa0="G_ACMUX_0", Ab0="G_ACMUX_0",
       Ac0="G_ACMUX_0", Ad0="G_ACMUX_1", A1="G_CCMUX_0", B1="G_CCMUX_0", C1="G_CCMUX_0", D1="G_CCMUX_COMBINED",
       Aa1="G_ACMUX_0", Ab1="G_ACMUX_0", Ac1="G_ACMUX_0", Ad1="G_ACMUX_COMBINED")
    el(dl, "SetOtherMode", Cmd="G_SETOTHERMODE_H", Sft=4, Length=20, G_AD_NOISE=1, G_CD_MAGICSQ=1, G_CK_NONE=1,
       G_TC_FILT=1, G_TF_BILERP=1, G_TT_NONE=1, G_TL_TILE=1, G_TD_CLAMP=1, G_TP_PERSP=1, G_CYC_2CYCLE=1,
       G_PM_NPRIMITIVE=1)
    el(dl, "SetOtherMode", Cmd="G_SETOTHERMODE_L", Sft=0, Length=32, G_AC_NONE=1, G_ZS_PIXEL=1,
       G_RM_FOG_SHADE_A=1, G_RM_AA_ZB_OPA_SURF2=1)
    if not texture_resource:
        el(dl, "Texture", S=65535, T=65535, Level=0, Tile=0, On=0)
        return
    w, h = mat.width, mat.height
    words_per_line = max(1, w * 2 // 8)
    el(dl, "Texture", S=65535, T=65535, Level=0, Tile=0, On=1)
    el(dl, "SetTextureImage", Path=texture_resource, Format="G_IM_FMT_RGBA", Size="G_IM_SIZ_16b_LOAD_BLOCK", Width=1)
    el(dl, "SetTile", Format="G_IM_FMT_RGBA", Size="G_IM_SIZ_16b_LOAD_BLOCK", Line=0, TMem=0, Tile=7, Palette=0,
       Cms0="G_TX_WRAP", Cms1="G_TX_NOMIRROR", Cmt0="G_TX_WRAP", Cmt1="G_TX_NOMIRROR", MaskS=0, ShiftS=0, MaskT=0,
       ShiftT=0)
    el(dl, "LoadBlock", Tile=7, Uls=0, Ult=0, Lrs=w * h - 1, Dxt=(2048 + words_per_line - 1) // words_per_line)
    el(dl, "SetTile", Format="G_IM_FMT_RGBA", Size="G_IM_SIZ_16b", Line=words_per_line, TMem=0, Tile=0, Palette=0,
       Cms0="G_TX_WRAP", Cms1="G_TX_NOMIRROR", Cmt0="G_TX_WRAP", Cmt1="G_TX_NOMIRROR",
       MaskS=int(math.log2(w)), ShiftS=0, MaskT=int(math.log2(h)), ShiftT=0)
    el(dl, "SetTileSize", T=0, Uls=0, Ult=0, Lrs=(w - 1) << 2, Lrt=(h - 1) << 2)


def build_files(scene, name, resource_dir):
    """Returns {file name: bytes} and {texture file name: source PNG path}. The files go in resource_dir."""
    prefix = resource_dir.rstrip("/") + "/"
    room_name = name + "_room_0"
    files = {}
    textures = {}

    # Display list of the room.
    dl = el(None, "DisplayList", Version=0)
    by_material = {}
    for tri in scene.draw:
        by_material.setdefault(tri.material, []).append(tri)
    vtx_count = 0
    for mat, tris in by_material.items():
        texture_resource = None
        if mat.texture_path:
            texture_resource = prefix + name + "_tex_" + mat.name
            textures[name + "_tex_" + mat.name + ".rgb5a1.png"] = mat.texture_path
        material_commands(dl, mat, texture_resource)
        keyed = []
        for tri in tris:
            n = normalize(cross(sub(tri.positions[1], tri.positions[0]), sub(tri.positions[2], tri.positions[0])))
            st = tex_coords(tri)
            verts = []
            for i in range(3):
                pos = tuple(to_s16(c, "Position") for c in tri.positions[i])
                color = tuple(to_u8(c) for c in baked_color(scene, tri, i, n))
                verts.append(pos + st[i] + color)
            keyed.append(verts)
        for vertices, tris_idx in batches(keyed):
            vtx_name = "%s_vtx_%d" % (room_name, vtx_count)
            vtx_count += 1
            vtx = el(None, "Vertex", Version=0)
            for v in vertices:
                el(vtx, "Vtx", X=v[0], Y=v[1], Z=v[2], S=v[3], T=v[4], R=v[5], G=v[6], B=v[7], A=v[8])
            files[vtx_name] = xml_bytes(vtx)
            el(dl, "LoadVertices", Path=prefix + vtx_name, VertexBufferIndex=0, VertexOffset=0, Count=len(vertices))
            for i in range(0, len(tris_idx) - 1, 2):
                a, b = tris_idx[i], tris_idx[i + 1]
                el(dl, "Triangles2", V00=a[0], V01=a[1], V02=a[2], Flag0=0, V10=b[0], V11=b[1], V12=b[2], Flag1=0)
            if len(tris_idx) % 2:
                a = tris_idx[-1]
                el(dl, "Triangle1", V00=a[0], V01=a[1], V02=a[2], Flag0=0)
    el(dl, "PipeSync")
    el(dl, "EndDisplayList")
    dl_name = room_name + "_dl"
    files[dl_name] = xml_bytes(dl)

    # Collision header.
    col_vertices = []
    col_index = {}
    polygons = []
    for tri in scene.collide:
        pts = [tuple(to_s16(c, "Position") for c in p) for p in tri.positions]
        poly = collision_polygon(*pts)
        if poly is None:
            continue
        ids = []
        for p in pts:
            if p not in col_index:
                col_index[p] = len(col_vertices)
                col_vertices.append(p)
            ids.append(col_index[p])
        polygons.append((ids, poly))
    if len(col_vertices) > 0x1FFF:
        raise SceneError("The collision has more than 8191 vertices")
    if len(polygons) > 0xFFFF:
        raise SceneError("The collision has more than 65535 polygons")
    mins = [min(v[i] for v in col_vertices) for i in range(3)]
    maxs = [max(v[i] for v in col_vertices) for i in range(3)]
    col = el(None, "CollisionHeader", Version=0, MinBoundsX=mins[0], MinBoundsY=mins[1], MinBoundsZ=mins[2],
             MaxBoundsX=maxs[0], MaxBoundsY=maxs[1], MaxBoundsZ=maxs[2])
    for v in col_vertices:
        el(col, "Vertex", X=v[0], Y=v[1], Z=v[2])
    for ids, (normal, dist) in polygons:
        el(col, "Polygon", Type=0, VertexA=ids[0], VertexB=ids[1], VertexC=ids[2], NormalX=normal[0],
           NormalY=normal[1], NormalZ=normal[2], Dist=to_s16(dist, "Distance"))
    # One surface type for all the polygons: camera data 0, light setting 0, the sound of the ground.
    el(col, "PolygonType", Data1=0, Data2=0)
    el(col, "CameraData", SType=CAM_SET_DUNGEON0, NumData=0, CameraPosDataSeg=0)
    col_name = name + "_col"
    files[col_name] = xml_bytes(col)

    # Room.
    room = el(None, "Room", Version=0)
    el(room, "SetEchoSettings", Echo=0)
    el(room, "SetRoomBehavior", GameplayFlags1=0, GameplayFlags2=0)
    el(room, "SetSkyboxModifier", SkyboxDisabled=1, SunMoonDisabled=1)
    el(room, "SetTimeSettings", Hour=255, Minute=255, TimeIncrement=0)
    mesh = el(room, "SetMesh", Data=0, MeshHeaderType=0, PolyNum=1)
    el(mesh, "Polygon", PolyType=0, MeshOpa=prefix + dl_name, MeshXlu="")
    el(room, "EndMarker")
    files[room_name] = xml_bytes(room)

    # Scene.
    sc = el(None, "Room", Version=0)
    el(sc, "SetSoundSettings", Reverb=0, NatureAmbienceId=NATURE_ID_DISABLED, SeqId=NA_BGM_NO_MUSIC)
    rooms = el(sc, "SetRoomList")
    el(rooms, "RoomEntry", Path=prefix + room_name, VromStart=0, VromEnd=0x1000)
    el(sc, "SetCollisionHeader", FileName=prefix + col_name)
    entrances = el(sc, "SetEntranceList")
    el(entrances, "EntranceEntry", Spawn=0, Room=0)
    el(sc, "SetSpecialObjects", ElfMessage=0, GlobalObject=0)
    starts = el(sc, "SetStartPositionList")
    sx, sy, sz = (to_s16(c, "Spawn") for c in scene.spawn)
    el(starts, "StartPositionEntry", Id=0, PosX=sx, PosY=sy, PosZ=sz, RotX=0,
       RotY=scene.spawn_yaw - 0x10000 if scene.spawn_yaw > 0x7FFF else scene.spawn_yaw, RotZ=0, Params=PLAYER_PARAMS_STAND)
    el(sc, "SetSkyboxSettings", Unknown=0, SkyboxId=0, Weather=0, Indoors=1)
    el(sc, "SetCameraSettings", CameraMovement=0, WorldMapArea=0)
    lighting = el(sc, "SetLightingSettings")
    light_dir = [int(round(c * 127)) for c in scene.light_dir]
    amb = [to_u8(c) for c in scene.ambient]
    lc = [to_u8(c) for c in scene.light_color]
    fog = [to_u8(c) for c in scene.fog_color]
    el(lighting, "LightingSetting", AmbientColorR=amb[0], AmbientColorG=amb[1], AmbientColorB=amb[2],
       Light1DirX=light_dir[0], Light1DirY=light_dir[1], Light1DirZ=light_dir[2], Light1ColorR=lc[0],
       Light1ColorG=lc[1], Light1ColorB=lc[2], Light2DirX=-light_dir[0], Light2DirY=-light_dir[1],
       Light2DirZ=-light_dir[2], Light2ColorR=0, Light2ColorG=0, Light2ColorB=0, FogColorR=fog[0],
       FogColorG=fog[1], FogColorB=fog[2], FogNear=scene.fog_near, FogFar=scene.fog_far)
    el(sc, "EndMarker")
    files[name] = xml_bytes(sc)

    return files, textures


def write_output(files, textures, out_dir):
    if os.path.isdir(out_dir):
        shutil.rmtree(out_dir)
    os.makedirs(out_dir)
    for file_name, data in files.items():
        with open(os.path.join(out_dir, file_name), "wb") as f:
            f.write(data)
    for file_name, source in textures.items():
        shutil.copyfile(source, os.path.join(out_dir, file_name))


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("gltf", help="The .gltf or .glb file")
    parser.add_argument("--name", required=True, help="The name of the scene, for example arena_scene")
    parser.add_argument("--custom-assets", required=True, help="The directory port/soh/assets/custom")
    parser.add_argument("--units-per-meter", type=float, default=DEFAULT_UNITS_PER_METER)
    args = parser.parse_args(argv)

    resource_dir = "scenes/shared/" + args.name
    try:
        scene = load_scene(args.gltf, args.units_per_meter)
        files, textures = build_files(scene, args.name, resource_dir)
    except SceneError as e:
        print("error: %s" % e, file=sys.stderr)
        return 1
    out_dir = os.path.join(args.custom_assets, resource_dir)
    write_output(files, textures, out_dir)
    print("Wrote %d files and %d textures to %s (%d triangles, %d collision triangles)" %
          (len(files), len(textures), out_dir, len(scene.draw), len(scene.collide)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
