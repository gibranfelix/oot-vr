#!/usr/bin/env python3
"""Make an HTML preview of a scene from the XML files that the game reads.

The preview reads the files in port/soh/assets/custom/scenes/shared/<name>/: the room display list, the vertex buffers,
the textures, and the start position. It shows the vertex colors times the textures, as the game draws them. The fog
and the filter of the textures are approximate. Open the HTML file in a browser.
"""

import argparse
import base64
import json
import os
import sys
import xml.etree.ElementTree as ET

# The eye height of adult Link, in units: Link has a height of 68 units.
EYE_HEIGHT = 60


def read_xml(scene_dir, resource_path):
    return ET.parse(os.path.join(scene_dir, resource_path.rsplit("/", 1)[-1])).getroot()


def find_texture(scene_dir, resource_path):
    base = resource_path.rsplit("/", 1)[-1]
    for file_name in os.listdir(scene_dir):
        if file_name.startswith(base + ".") and file_name.endswith(".png"):
            return os.path.join(scene_dir, file_name)
    raise FileNotFoundError("No PNG file for the texture " + resource_path)


def scene_data(scene_dir, name):
    """Returns the geometry of the room, grouped by texture, and the start position of Link."""
    scene = read_xml(scene_dir, name)
    start = scene.find("SetStartPositionList/StartPositionEntry")
    light = scene.find("SetLightingSettings/LightingSetting")
    room = read_xml(scene_dir, scene.find("SetRoomList/RoomEntry").get("Path"))
    groups = []
    current = None
    texture_size = (32, 32)
    vertices = []
    for polygon in room.find("SetMesh").iter("Polygon"):
        for cmd in read_xml(scene_dir, polygon.get("MeshOpa")):
            if cmd.tag == "SetTextureImage":
                path = find_texture(scene_dir, cmd.get("Path"))
                with open(path, "rb") as f:
                    png = base64.b64encode(f.read()).decode()
                current = {"texture": "data:image/png;base64," + png, "positions": [], "uvs": [], "colors": []}
                groups.append(current)
            elif cmd.tag == "SetTileSize":
                texture_size = ((int(cmd.get("Lrs")) >> 2) + 1, (int(cmd.get("Lrt")) >> 2) + 1)
            elif cmd.tag == "Texture" and cmd.get("On") == "0":
                current = {"texture": None, "positions": [], "uvs": [], "colors": []}
                groups.append(current)
            elif cmd.tag == "LoadVertices":
                vertices = list(read_xml(scene_dir, cmd.get("Path")))
            elif cmd.tag in ("Triangle1", "Triangles2"):
                keys = ["V00", "V01", "V02"] + (["V10", "V11", "V12"] if cmd.tag == "Triangles2" else [])
                for key in keys:
                    v = vertices[int(cmd.get(key))]
                    current["positions"] += [int(v.get("X")), int(v.get("Y")), int(v.get("Z"))]
                    current["uvs"] += [int(v.get("S")) / 32.0 / texture_size[0],
                                       int(v.get("T")) / 32.0 / texture_size[1]]
                    current["colors"] += [int(v.get(c)) / 255.0 for c in "RGB"]
    return {
        "groups": groups,
        "start": [int(start.get("PosX")), int(start.get("PosY")), int(start.get("PosZ"))],
        "startYaw": int(start.get("RotY")),
        "eyeHeight": EYE_HEIGHT,
        "fogColor": [int(light.get("FogColor" + c)) / 255.0 for c in "RGB"],
        "fogFar": int(light.get("FogFar")),
        "fogNear": int(light.get("FogNear")) & 0x3FF,
    }


PAGE = """<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>ARENA scene preview</title>
<style>
  :root { --view-fg: #e8e8ec; --view-muted: #b5b7c4; --view-panel: rgba(17, 19, 24, 0.78); }
  body { margin: 0; font: 14px/1.4 system-ui, sans-serif; }
  #view { position: relative; height: 560px; border-radius: 8px; overflow: hidden; touch-action: none; }
  canvas { display: block; }
  #panel { position: absolute; top: 12px; left: 12px; right: 12px; max-width: 420px; padding: 10px 12px;
           border-radius: 8px; background: var(--view-panel); color: var(--view-fg); }
  #panel h1 { margin: 0 0 4px; font-size: 15px; }
  #panel p { margin: 0 0 8px; color: var(--view-muted); }
  button { font: inherit; padding: 4px 10px; margin-right: 6px; border-radius: 6px; border: 1px solid var(--view-muted);
           background: transparent; color: var(--view-fg); cursor: pointer; }
  button[aria-pressed="true"] { background: var(--view-fg); color: #111318; }
</style>
</head>
<body>
<div id="view">
<div id="panel">
  <h1>__NAME__</h1>
  <p id="help"></p>
  <button id="orbit" aria-pressed="true">Orbit</button><button id="walk" aria-pressed="false">Link's eyes</button>
</div>
</div>
<script src="https://cdnjs.cloudflare.com/ajax/libs/three.js/r128/three.min.js"></script>
<script>
const DATA = __DATA__;
const renderer = new THREE.WebGLRenderer({ antialias: true });
renderer.setPixelRatio(window.devicePixelRatio);
const view = document.getElementById("view");
view.insertBefore(renderer.domElement, view.firstChild);
const scene = new THREE.Scene();
const fogColor = new THREE.Color(DATA.fogColor[0], DATA.fogColor[1], DATA.fogColor[2]);
scene.background = fogColor;
// The game starts the fog at fogNear / 1000 of the way to fogFar. This is an approximation.
scene.fog = new THREE.Fog(fogColor, DATA.fogFar * DATA.fogNear / 1000, DATA.fogFar);
const camera = new THREE.PerspectiveCamera(70, 1, 5, 20000);

let min = [Infinity, Infinity, Infinity], max = [-Infinity, -Infinity, -Infinity];
for (const g of DATA.groups) {
  const geo = new THREE.BufferGeometry();
  geo.setAttribute("position", new THREE.Float32BufferAttribute(g.positions, 3));
  geo.setAttribute("uv", new THREE.Float32BufferAttribute(g.uvs, 2));
  geo.setAttribute("color", new THREE.Float32BufferAttribute(g.colors, 3));
  for (let i = 0; i < g.positions.length; i += 3) for (let k = 0; k < 3; k++) {
    min[k] = Math.min(min[k], g.positions[i + k]); max[k] = Math.max(max[k], g.positions[i + k]);
  }
  const mat = new THREE.MeshBasicMaterial({ vertexColors: true, side: THREE.FrontSide });
  if (g.texture) {
    const tex = new THREE.TextureLoader().load(g.texture, () => draw());
    tex.wrapS = tex.wrapT = THREE.RepeatWrapping;
    tex.flipY = false;
    mat.map = tex;
  }
  scene.add(new THREE.Mesh(geo, mat));
}
const center = new THREE.Vector3((min[0] + max[0]) / 2, (min[1] + max[1]) / 2, (min[2] + max[2]) / 2);
const radius = Math.max(max[0] - min[0], max[2] - min[2]);

// A marker of 68 units where Link starts: the height of adult Link.
const link = new THREE.Mesh(new THREE.CylinderGeometry(12, 12, 68, 16), new THREE.MeshBasicMaterial({ color: 0x3fbf5f }));
link.position.set(DATA.start[0], DATA.start[1] + 34, DATA.start[2]);
scene.add(link);

let mode = "orbit";
let yaw = 0.7, pitch = 0.6, dist = radius * 1.1;
let walkPos = new THREE.Vector3(DATA.start[0], DATA.start[1] + DATA.eyeHeight, DATA.start[2]);
let walkYaw = DATA.startYaw / 32768 * Math.PI, walkPitch = 0;
const help = document.getElementById("help");

function draw() {
  if (mode === "orbit") {
    camera.position.set(center.x + dist * Math.cos(pitch) * Math.sin(yaw), center.y + dist * Math.sin(pitch),
                        center.z + dist * Math.cos(pitch) * Math.cos(yaw));
    camera.lookAt(center);
    link.visible = true;
    help.textContent = "Drag to turn. Scroll to zoom. The green marker is Link (68 units).";
  } else {
    camera.position.copy(walkPos);
    camera.lookAt(walkPos.x + Math.sin(walkYaw) * Math.cos(walkPitch), walkPos.y + Math.sin(walkPitch),
                  walkPos.z + Math.cos(walkYaw) * Math.cos(walkPitch));
    link.visible = false;
    help.textContent = "Drag to look. W A S D or the arrow keys walk. Eye height: " + DATA.eyeHeight + " units.";
  }
  renderer.render(scene, camera);
}
function resize() {
  renderer.setSize(view.clientWidth, view.clientHeight);
  camera.aspect = view.clientWidth / view.clientHeight;
  camera.updateProjectionMatrix();
  draw();
}
function setMode(m) {
  mode = m;
  document.getElementById("orbit").setAttribute("aria-pressed", m === "orbit");
  document.getElementById("walk").setAttribute("aria-pressed", m === "walk");
  draw();
}
document.getElementById("orbit").onclick = () => setMode("orbit");
document.getElementById("walk").onclick = () => setMode("walk");
let dragging = null;
renderer.domElement.addEventListener("pointerdown", e => { dragging = [e.clientX, e.clientY]; renderer.domElement.setPointerCapture(e.pointerId); });
renderer.domElement.addEventListener("pointerup", () => { dragging = null; });
renderer.domElement.addEventListener("pointermove", e => {
  if (!dragging) return;
  const dx = (e.clientX - dragging[0]) * 0.005, dy = (e.clientY - dragging[1]) * 0.005;
  dragging = [e.clientX, e.clientY];
  if (mode === "orbit") { yaw -= dx; pitch = Math.max(0.05, Math.min(1.5, pitch + dy)); }
  else { walkYaw -= dx; walkPitch = Math.max(-1.2, Math.min(1.2, walkPitch - dy)); }
  draw();
});
renderer.domElement.addEventListener("wheel", e => {
  if (mode !== "orbit") return;
  e.preventDefault();
  dist = Math.max(100, Math.min(radius * 4, dist * Math.exp(e.deltaY * 0.001)));
  draw();
}, { passive: false });
window.addEventListener("keydown", e => {
  if (mode !== "walk") return;
  const step = 20;
  const fx = Math.sin(walkYaw), fz = Math.cos(walkYaw);
  if (e.key === "w" || e.key === "ArrowUp") { walkPos.x += fx * step; walkPos.z += fz * step; }
  else if (e.key === "s" || e.key === "ArrowDown") { walkPos.x -= fx * step; walkPos.z -= fz * step; }
  else if (e.key === "a" || e.key === "ArrowLeft") { walkPos.x += fz * step; walkPos.z -= fx * step; }
  else if (e.key === "d" || e.key === "ArrowRight") { walkPos.x -= fz * step; walkPos.z += fx * step; }
  else return;
  draw();
});
window.addEventListener("resize", resize);
resize();
</script>
</body>
</html>
"""


def make_page(scene_dir, name):
    data = scene_data(scene_dir, name)
    return PAGE.replace("__NAME__", name).replace("__DATA__", json.dumps(data))


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("scene_dir", help="The directory of the scene, for example "
                                          "port/soh/assets/custom/scenes/shared/arena_scene")
    parser.add_argument("-o", "--output", required=True, help="The HTML file to write")
    args = parser.parse_args(argv)
    name = os.path.basename(os.path.normpath(args.scene_dir))
    with open(args.output, "w") as f:
        f.write(make_page(args.scene_dir, name))
    print(args.output)
    return 0


if __name__ == "__main__":
    sys.exit(main())
