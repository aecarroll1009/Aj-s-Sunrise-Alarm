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
    "shade_floor": ("#f2c27a", 0.6),
    "shade_top": ("#f2c27a", 0.6),
    "panel_front": ("#ffd9a0", 0.85),
    "panel_right": ("#ffd9a0", 0.85),
    "panel_back": ("#ffd9a0", 0.85),
    "panel_left": ("#ffd9a0", 0.85),
    "perfboard": ("#3fa34d", 1.0),
    "oled_board": ("#2a5fa8", 1.0),
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


def render(path, parts, offsets, camera, title, show_strip=True,
           parallel=False) -> None:
    """Renders one view to a PNG file.

    Args:
        path: Output PNG path.
        parts: Name to build123d part mapping.
        offsets: Per-part translation for exploded views.
        camera: Camera position as a unit-ish direction from the model centre.
        title: Text drawn in the corner.
        show_strip: Whether to draw the LED strip ribbon on the core.
        parallel: Whether to use a parallel (elevation-style) projection.
    """
    pl = pv.Plotter(off_screen=True, window_size=(1000, 1000))
    pl.set_background("white")
    for name, part in parts.items():
        color, opacity = COLORS[name]
        m = mesh(part, offsets.get(name, (0, 0, 0)))
        pl.add_mesh(m, color=color, opacity=opacity, smooth_shading=False)
        edges = m.extract_feature_edges(35, boundary_edges=False,
                                        non_manifold_edges=False)
        if edges.n_points:
            pl.add_mesh(edges, color="#5a4a3a", line_width=1.2)
    if show_strip:
        pl.add_mesh(strip_ribbon(offsets.get("deck_core", (0, 0, 0))),
                    color=STRIP_COLOR)
    pl.camera_position = camera
    if parallel:
        pl.camera.view_angle = 8.0
    pl.reset_camera()
    pl.camera.zoom(1.1)
    pl.add_text(title, font_size=12, color="black")
    pl.enable_depth_peeling()
    pl.screenshot(str(path))
    pl.close()


def fish_layout_plot(path) -> None:
    """Plots each side panel's fish outlines as seen from outside the lamp."""
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    from build123d import GeomType

    fig, axes = plt.subplots(2, 2, figsize=(14, 7))
    top = enc.PANEL_T + enc.FISH_H
    for ax, face in zip(axes.flat, enc.FACES):
        for f in enc.panel_flat(face).faces().filter_by(GeomType.PLANE):
            if abs(f.center().Z - top) > 1e-3:
                continue
            pts = f.outer_wire().positions([i / 200 for i in range(201)])
            ax.fill([p.X for p in pts], [p.Y for p in pts], color="#e8a33d", ec="k", lw=0.8)
            for w in f.inner_wires():
                pts = w.positions([i / 60 for i in range(61)])
                ax.fill([p.X for p in pts], [p.Y for p in pts], color="white", ec="k", lw=0.8)
        w, h = enc.panel_size(face)
        ax.plot([-w / 2, w / 2, w / 2, -w / 2, -w / 2], [0, 0, h, h, 0], "k-", lw=1)
        ax.set_aspect("equal")
        ax.set_title(f"{face} panel, seen from outside (fish swim to the right)")
    fig.tight_layout()
    fig.savefig(path, dpi=90)
    plt.close(fig)


def main() -> None:
    """Renders assembled, rear, exploded and core-only views."""
    enc.OUT_DIR.mkdir(exist_ok=True)
    parts = enc.build()
    front = [(-260, -420, 260), (0, 0, 70), (0, 0, 1)]
    rear = [(260, 420, 260), (0, 0, 70), (0, 0, 1)]
    exploded = {"deck_core": (0, 0, 40), "shade_floor": (0, 0, 110),
                "panel_front": (0, -50, 150), "panel_back": (0, 50, 150),
                "panel_right": (50, 0, 150), "panel_left": (-50, 0, 150),
                "shade_top": (0, 0, 220)}
    core_only = {k: parts[k] for k in ("base_tub", "deck_core")}
    render(enc.OUT_DIR / "view_front.png", parts, {}, front, "Assembled, front")
    render(enc.OUT_DIR / "view_rear.png", parts, {}, rear, "Assembled, rear")
    render(enc.OUT_DIR / "view_exploded.png", parts, exploded, front, "Exploded")
    render(enc.OUT_DIR / "view_core_front.png", core_only, {}, front,
           "Base + core, strip path (front)")
    render(enc.OUT_DIR / "view_core_rear.png", core_only, {}, rear,
           "Base + core, strip path (rear)")
    tub_only = {"base_tub": parts["base_tub"], "perfboard": enc.perfboard(),
                "oled_board": enc.oled_board()}
    render(enc.OUT_DIR / "view_tub_inside.png", tub_only, {},
           [(-120, -200, 260), (0, 0, 20), (0, 0, 1)],
           "Base tub, perfboard on standoffs", show_strip=False)
    shade = {k: parts[k] for k in ("shade_floor", "shade_top")}
    render(enc.OUT_DIR / "view_shade_under.png", shade, {"shade_top": (0, 0, 25)},
           [(-160, -280, -170), (0, 0, 75), (0, 0, 1)],
           "Shade floor + posts, top lifted 25 mm", show_strip=False)
    mid = enc.BASE_H + enc.SHADE_H / 2
    for face, eye in (("front", (0, -600, mid)), ("right", (600, 0, mid)),
                      ("back", (0, 600, mid)), ("left", (-600, 0, mid))):
        render(enc.OUT_DIR / f"view_side_{face}.png", parts, {},
               [eye, (0, 0, mid), (0, 0, 1)], f"{face.capitalize()} (head-on)",
               show_strip=False, parallel=True)
    fish_layout_plot(enc.OUT_DIR / "view_fish_layout.png")


if __name__ == "__main__":
    main()
