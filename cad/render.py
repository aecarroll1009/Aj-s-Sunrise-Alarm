"""Renders preview PNGs of the enclosure to ``cad/out``.

Run:
    python cad/render.py
"""

import numpy as np
import pyvista as pv

import enclosure as enc

COLORS = {
    "base_tub": ("#3f4550", 1.0),
    "deck_core": ("#d9d9d1", 1.0),
    "shade_floor": ("#ffd28c", 0.35),
    "shade_shell": ("#ffd28c", 0.25),
    "perfboard": ("#3fa34d", 1.0),
}
STRIP_COLOR = "#ff4fa0"


def mesh(part, offset=(0.0, 0.0, 0.0)) -> pv.PolyData:
    """Tessellates a part into a PyVista mesh."""
    verts, tris = part.tessellate(0.1, 0.2)
    v = np.array([[p.X, p.Y, p.Z] for p in verts]) + np.array(offset)
    faces = np.hstack([[3, *t] for t in tris])
    return pv.PolyData(v, faces)


def strip_path(offset=(0.0, 0.0, 0.0)) -> np.ndarray:
    """Returns the LED strip centreline as an (n, 3) polyline."""
    wrap = enc.strip_wrap()
    z_lo, z_hi = wrap["head"][2], wrap["tail"][2]
    hw, hd = enc.CORE_W / 2 + 0.5, enc.CORE_D / 2 + 0.5
    pts = []
    for s in np.linspace(0, enc.STRIP_LEN, 800):
        wall, c, _ = enc.perimeter_point(enc.STRIP_START + s, 0)
        z = z_lo + (z_hi - z_lo) * s / enc.STRIP_LEN
        x, y = {"front": (c, -hd), "back": (c, hd),
                "left": (-hw, c), "right": (hw, c)}[wall]
        pts.append((x, y, z))
    return np.array(pts) + np.array(offset)


def strip_ribbon(offset=(0.0, 0.0, 0.0)) -> pv.PolyData:
    """Returns the LED strip as a vertical ribbon mesh."""
    p = strip_path(offset)
    h = np.array([0, 0, enc.STRIP_W / 2])
    pts = np.vstack([p - h, p + h])
    n = len(p)
    faces = np.hstack([[4, i, i + 1, n + i + 1, n + i] for i in range(n - 1)])
    return pv.PolyData(pts, faces)


def render(path, parts, offsets, camera, title, show_strip=True) -> None:
    """Renders one view to a PNG file.

    Args:
        path: Output PNG path.
        parts: Name to build123d part mapping.
        offsets: Per-part translation for exploded views.
        camera: Camera position as a unit-ish direction from the model centre.
        title: Text drawn in the corner.
        show_strip: Whether to draw the LED strip ribbon on the core.
    """
    pl = pv.Plotter(off_screen=True, window_size=(1000, 1000))
    pl.set_background("white")
    for name, part in parts.items():
        color, opacity = COLORS[name]
        pl.add_mesh(mesh(part, offsets.get(name, (0, 0, 0))), color=color,
                    opacity=opacity, smooth_shading=False)
    if show_strip:
        pl.add_mesh(strip_ribbon(offsets.get("deck_core", (0, 0, 0))),
                    color=STRIP_COLOR)
    pl.camera_position = camera
    pl.reset_camera()
    pl.camera.zoom(1.1)
    pl.add_text(title, font_size=12, color="black")
    pl.enable_depth_peeling()
    pl.screenshot(str(path))
    pl.close()


def main() -> None:
    """Renders assembled, rear, exploded and core-only views."""
    enc.OUT_DIR.mkdir(exist_ok=True)
    parts = enc.build()
    front = [(-260, -420, 260), (0, 0, 70), (0, 0, 1)]
    rear = [(260, 420, 260), (0, 0, 70), (0, 0, 1)]
    exploded = {"deck_core": (0, 0, 40), "shade_floor": (0, 0, 110),
                "shade_shell": (0, 0, 150)}
    core_only = {k: parts[k] for k in ("base_tub", "deck_core")}
    render(enc.OUT_DIR / "view_front.png", parts, {}, front, "Assembled, front")
    render(enc.OUT_DIR / "view_rear.png", parts, {}, rear, "Assembled, rear")
    render(enc.OUT_DIR / "view_exploded.png", parts, exploded, front, "Exploded")
    render(enc.OUT_DIR / "view_core_front.png", core_only, {}, front,
           "Base + core, strip path (front)")
    render(enc.OUT_DIR / "view_core_rear.png", core_only, {}, rear,
           "Base + core, strip path (rear)")
    tub_only = {"base_tub": parts["base_tub"], "perfboard": enc.perfboard()}
    render(enc.OUT_DIR / "view_tub_inside.png", tub_only, {},
           [(-120, -200, 260), (0, 0, 20), (0, 0, 1)],
           "Base tub, perfboard on standoffs", show_strip=False)


if __name__ == "__main__":
    main()
