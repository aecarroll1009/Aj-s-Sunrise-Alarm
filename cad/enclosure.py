"""Parametric enclosure for AJ's Sunrise Alarm.

Blocky mushroom layout: an opaque electronics base, a rectangular LED core
rising from the base deck, and a translucent shade box that drops over the
core and rests on the deck. All dimensions are millimetres; the origin is the
bottom-centre of the base, +X is the long side, -Y is the front face.

Parts (each exported as STEP and STL to ``cad/out``):
    base_tub    opaque PLA, holds the electronics and front/rear cutouts.
    deck_core   opaque PLA, screws onto the tub; the core carries the strip.
    shade_floor translucent PLA, floor ring that locates on the core, with
                four slim corner posts carrying inserts in their tops.
    shade_top   translucent PLA, flat top plate screwed down into the posts;
                it clamps the panels against the floor.
    panel_*     translucent PLA, four plain flat side panels with raised
                goldfish, held only by shallow recesses in the floor and top.
                Their STLs are exported lying flat, fish up, ready to print.

Run:
    python cad/enclosure.py
"""

import math
import random
from pathlib import Path

from build123d import (
    Align,
    Box,
    Compound,
    Cone,
    Cylinder,
    Ellipse,
    Part,
    Polygon,
    Pos,
    Rot,
    export_step,
    export_stl,
    extrude,
)

OUT_DIR = Path(__file__).parent / "out"

OUTER_SCALE = 1.1

BASE_W, BASE_D, BASE_H = (OUTER_SCALE * v for v in (140.0, 80.0, 50.0))
BASE_WALL = 3.0
BASE_FLOOR = 3.0
DECK_T = 3.0
TUB_H = BASE_H - DECK_T

SHADE_W, SHADE_D, SHADE_H = (OUTER_SCALE * v for v in (170.0, 110.0, 60.0))
SHADE_FLOOR_T = 3.0
SHADE_CAP_T = 3.0
PANEL_T = 2.0
PANEL_FIT = 0.2
SEAT_DEPTH = 1.2
SEAT_LIP = 1.2
SHADE_WALL = SEAT_LIP + PANEL_FIT + PANEL_T
POST = 8.0
POST_INSERT_DEPTH = 6.0
CORE_FIT = 0.4
SHADE_Z0 = BASE_H + SHADE_FLOOR_T
SHADE_ZTOP = BASE_H + SHADE_H - SHADE_CAP_T
FACES = ("front", "right", "back", "left")

FISH_H = 1.6
FISH_EYE_R = 2.9
FISH_EYE_POS = (6.5, 1.0)
FISH_COUNTS = {"front": 3, "right": 2, "back": 3, "left": 1}
FISH_SEED = 7
FISH_TILT = 25.0
FISH_SCALE = (0.7, 0.9)
FISH_GAP = 3.0

STRIP_LEN = 1000.0
STRIP_W = 10.0
STRIP_MARGIN = 3.0
STRIP_PITCH = 11.0
STRIP_START = 10.0

CORE_W, CORE_D = OUTER_SCALE * 120.0, OUTER_SCALE * 60.0
CORE_H = math.ceil(
    2 * STRIP_MARGIN + STRIP_W + STRIP_PITCH * STRIP_LEN / (2 * (CORE_W + CORE_D))
)
CORE_WALL = 2.0
WIRE_SLOT_W, WIRE_SLOT_H = 10.0, 6.0

BOSS_R = 4.0
BOSS_INSET = BASE_WALL + 3.5
INSERT_HOLE_D = 4.2
INSERT_DEPTH = 6.0
SCREW_CLEAR_D = 3.4
SCREW_HEAD_D = 6.6

OLED_WINDOW = (56.0, 29.0)
OLED_PCB = (61.5, 39.5)
OLED_POS = (OUTER_SCALE * 26.0, BASE_H / 2)
ENCODER_HOLE_D = 8.4
ENCODER_POS = (OUTER_SCALE * -28.0, BASE_H / 2)
BUTTON_HOLE_D = 10.4
BUTTON_POS = (OUTER_SCALE * -50.0, BASE_H / 2)

JACK_HOLE_D = 11.2
JACK_POS = (OUTER_SCALE * 45.0, OUTER_SCALE * 20.0)
GRILLE_POS = (OUTER_SCALE * -40.0, BASE_H / 2)
GRILLE_R = 18.0
GRILLE_HOLE_D = 3.0
GRILLE_PITCH = 5.0

PERF_SIZE = (70.0, 50.0)
PERF_POS = (0.0, 10.0)
PERF_T = 1.6
PERF_HOLE_PITCH = (65.0, 45.0)
STANDOFF_H = 6.0
STANDOFF_R = 4.0

BOTTOM = (Align.CENTER, Align.CENTER, Align.MIN)


def boss_positions() -> list[tuple[float, float]]:
    """Returns the XY centres of the four corner screw bosses."""
    x = BASE_W / 2 - BOSS_INSET
    y = BASE_D / 2 - BOSS_INSET
    return [(sx * x, sy * y) for sx in (-1, 1) for sy in (-1, 1)]


def shade_boss_positions() -> list[tuple[float, float]]:
    """Returns the XY centres of the shade's corner posts."""
    inset = SHADE_WALL + PANEL_FIT + POST / 2
    x = SHADE_W / 2 - inset
    y = SHADE_D / 2 - inset
    return [(sx * x, sy * y) for sx in (-1, 1) for sy in (-1, 1)]


def panel_size(face: str) -> tuple[float, float]:
    """Returns a side panel's (width, height) in its own plane.

    Front and back panels run almost the full shade width and cover the ends
    of the side panels, which fit between them. Every panel reaches
    SEAT_DEPTH down into the floor and up into the top plate.
    """
    if face in ("front", "back"):
        width = SHADE_W - 2 * (SEAT_LIP + PANEL_FIT)
    else:
        width = SHADE_D - 2 * SHADE_WALL - 2 * PANEL_FIT
    return width, SHADE_ZTOP - SHADE_Z0 + 2 * SEAT_DEPTH - PANEL_FIT


def panel_seat(face: str) -> Part:
    """Returns the shallow recess cutter for a panel's top and bottom edges."""
    w, h = panel_size(face)
    cutter = Pos(0, 0, -PANEL_FIT) * Box(
        w + 2 * PANEL_FIT, h + PANEL_FIT, PANEL_T + 2 * PANEL_FIT,
        align=(Align.CENTER, Align.MIN, Align.MIN),
    )
    return place_panel(cutter, face)


def place_panel(part: Part, face: str) -> Part:
    """Moves a flat panel-local part onto its side of the shade.

    Panel-local coordinates: u along +X is the viewer's right when facing the
    panel from outside, v along +Y is up, and +Z points out of the lamp, with
    the panel's inner face on Z = 0.

    Args:
        part: Solid in panel-local coordinates.
        face: front, right, back or left.

    Returns:
        The solid in assembled position.
    """
    spin = {"front": 0, "right": 90, "back": 180, "left": -90}[face]
    off = SHADE_WALL
    z = SHADE_Z0 - SEAT_DEPTH
    pos = {
        "front": (0, -SHADE_D / 2 + off, z),
        "right": (SHADE_W / 2 - off, 0, z),
        "back": (0, SHADE_D / 2 - off, z),
        "left": (-SHADE_W / 2 + off, 0, z),
    }[face]
    return Pos(*pos) * Rot(0, 0, spin) * Rot(90, 0, 0) * part


def fish_profile(s: float) -> Part:
    """Builds one raised cartoon goldfish, heading +X, centred on its body.

    Args:
        s: Size factor; 1.0 is a 38 mm nose-to-tail fish.

    Returns:
        The fish relief, FISH_H tall, with the eye cut through to its base.
    """
    body = Ellipse(14 * s, 9 * s)
    tail = Polygon((-10 * s, 0), (-24 * s, 10 * s), (-19 * s, 0),
                   (-24 * s, -10 * s), align=None)
    fish = extrude(body + tail, FISH_H)
    fish -= Pos(FISH_EYE_POS[0] * s, FISH_EYE_POS[1] * s, 0) * Cylinder(
        FISH_EYE_R * s, FISH_H, align=BOTTOM
    )
    return fish


def fish_layout(face: str) -> list[tuple[float, float, float, float]]:
    """Picks non-overlapping fish placements for one panel.

    Every fish heads toward the panel's u axis (the viewer's right), so going
    around the lamp they all swim the same way. Positions, tilts and sizes
    come from a seeded generator, so the layout is the same on every build.

    Args:
        face: front, right, back or left.

    Returns:
        A list of (u, v, tilt_degrees, size) per fish, in panel coordinates.
    """
    rng = random.Random(FISH_SEED * 100 + FACES.index(face))
    w, h = panel_size(face)
    keep_out = w / 2 - (POST + PANEL_T + 3 if face in ("front", "back") else 3)
    u_lo, u_hi = -keep_out, keep_out
    v_lo, v_hi = SEAT_DEPTH + 2, h - SEAT_DEPTH - 2
    placed = []
    for _ in range(5000):
        if len(placed) == FISH_COUNTS[face]:
            break
        s = rng.uniform(*FISH_SCALE)
        r = 22 * s
        tilt = rng.uniform(-FISH_TILT, FISH_TILT)
        cu = rng.uniform(u_lo + r, u_hi - r)
        cv = rng.uniform(v_lo + r, v_hi - r)
        if all(math.hypot(cu - pu, cv - pv) > r + pr + FISH_GAP
               for pu, pv, pr in ((p[0], p[1], 22 * p[3]) for p in placed)):
            placed.append((cu, cv, tilt, s))
    if len(placed) != FISH_COUNTS[face]:
        raise ValueError(f"could not fit {FISH_COUNTS[face]} fish on {face} panel")
    return [(cu + 5 * s * math.cos(math.radians(t)),
             cv + 5 * s * math.sin(math.radians(t)), t, s) for cu, cv, t, s in placed]


def panel_flat(face: str) -> Part:
    """Builds a side panel lying flat, fish up, in panel-local coordinates."""
    w, h = panel_size(face)
    panel = Box(w, h, PANEL_T, align=(Align.CENTER, Align.MIN, Align.MIN))
    for u, v, tilt, s in fish_layout(face):
        panel += Pos(u, v, PANEL_T) * Rot(0, 0, tilt) * fish_profile(s)
    return panel


def countersink(x: float, y: float, z: float, thickness: float,
                from_below: bool = False) -> Part:
    """Returns a cutter for an M3 countersunk clearance hole.

    Args:
        x: Hole centre X.
        y: Hole centre Y.
        z: Bottom face of the plate.
        thickness: Plate thickness.
        from_below: Whether the screw head sits on the bottom face.

    Returns:
        The clearance hole plus 90 degree countersink as one solid.
    """
    csk_h = (SCREW_HEAD_D - SCREW_CLEAR_D) / 2
    hole = Pos(x, y, z) * Cylinder(SCREW_CLEAR_D / 2, thickness, align=BOTTOM)
    if from_below:
        head = Pos(x, y, z) * Cone(
            SCREW_HEAD_D / 2, SCREW_CLEAR_D / 2, csk_h, align=BOTTOM
        )
    else:
        head = Pos(x, y, z + thickness - csk_h) * Cone(
            SCREW_CLEAR_D / 2, SCREW_HEAD_D / 2, csk_h, align=BOTTOM
        )
    return hole + head


def perf_positions() -> list[tuple[float, float]]:
    """Returns the XY centres of the four perfboard standoffs."""
    x, y = PERF_HOLE_PITCH[0] / 2, PERF_HOLE_PITCH[1] / 2
    cx, cy = PERF_POS
    return [(cx + sx * x, cy + sy * y) for sx in (-1, 1) for sy in (-1, 1)]


def perfboard() -> Part:
    """Returns a stand-in for the perfboard, for previews only."""
    z = BASE_FLOOR + STANDOFF_H
    return Pos(*PERF_POS, z) * Box(*PERF_SIZE, PERF_T, align=BOTTOM)


def oled_board() -> Part:
    """Returns a stand-in for the OLED module behind its window, for previews."""
    y = -BASE_D / 2 + BASE_WALL + 2.0
    return Pos(OLED_POS[0], y, OLED_POS[1]) * Box(OLED_PCB[0], 4.0, OLED_PCB[1])


def strip_wrap() -> dict:
    """Computes how the LED strip spirals around the core.

    The strip starts STRIP_START along the front wall from the front-left
    corner of the core and runs
    counter-clockwise seen from above (front, right, back, left), rising
    linearly so its centreline goes from the lowest to the highest allowed
    height over the full strip length.

    Returns:
        Dict with perimeter, turns, pitch, the gap between adjacent wraps, and
        the head/tail positions as (wall, x_or_y, z).
    """
    perimeter = 2 * (CORE_W + CORE_D)
    turns = STRIP_LEN / perimeter
    z_lo = BASE_H + STRIP_MARGIN + STRIP_W / 2
    z_hi = BASE_H + CORE_H - STRIP_MARGIN - STRIP_W / 2
    pitch = (z_hi - z_lo) / turns
    return {
        "perimeter": perimeter,
        "turns": turns,
        "pitch": pitch,
        "gap": pitch - STRIP_W,
        "head": perimeter_point(STRIP_START, z_lo),
        "tail": perimeter_point(STRIP_START + STRIP_LEN, z_hi),
    }


def perimeter_point(s: float, z: float) -> tuple[str, float, float]:
    """Maps a distance along the core perimeter to a wall and coordinate.

    Args:
        s: Distance along the perimeter from the front-left corner.
        z: Height of the point.

    Returns:
        (wall, coord, z) where wall is front/right/back/left and coord is the
        X (front/back) or Y (left/right) position on that wall.
    """
    s %= 2 * (CORE_W + CORE_D)
    hw, hd = CORE_W / 2, CORE_D / 2
    if s < CORE_W:
        return "front", -hw + s, z
    s -= CORE_W
    if s < CORE_D:
        return "right", -hd + s, z
    s -= CORE_D
    if s < CORE_W:
        return "back", hw - s, z
    s -= CORE_W
    return "left", hd - s, z


def wall_cutter(wall: str, coord: float, z: float, cutter: Part,
                half_w: float, half_d: float) -> Part:
    """Places a Y-axis cutter through the given wall of a centred box.

    Args:
        wall: front, back, left or right.
        coord: Position along the wall (X for front/back, Y for left/right).
        z: Height of the cutter centre.
        cutter: Solid oriented along Y, centred on the origin.
        half_w: Half the box width (X).
        half_d: Half the box depth (Y).

    Returns:
        The cutter moved onto the wall.
    """
    if wall == "front":
        return Pos(coord, -half_d, z) * cutter
    if wall == "back":
        return Pos(coord, half_d, z) * cutter
    if wall == "left":
        return Pos(-half_w, coord, z) * Rot(0, 0, 90) * cutter
    return Pos(half_w, coord, z) * Rot(0, 0, 90) * cutter


def through_y(d: float, length: float = 20.0) -> Part:
    """Returns a round cutter of diameter d along the Y axis."""
    return Rot(90, 0, 0) * Cylinder(d / 2, length)


def base_tub() -> Part:
    """Builds the opaque electronics tub with front controls and rear I/O."""
    tub = Box(BASE_W, BASE_D, TUB_H, align=BOTTOM)
    tub -= Pos(0, 0, BASE_FLOOR) * Box(
        BASE_W - 2 * BASE_WALL, BASE_D - 2 * BASE_WALL, TUB_H, align=BOTTOM
    )

    for x, y in boss_positions():
        tub += Pos(x, y, BASE_FLOOR) * Cylinder(
            BOSS_R, TUB_H - BASE_FLOOR, align=BOTTOM
        )
        tub -= Pos(x, y, TUB_H - INSERT_DEPTH) * Cylinder(
            INSERT_HOLE_D / 2, INSERT_DEPTH, align=BOTTOM
        )

    for x, y in perf_positions():
        tub += Pos(x, y, BASE_FLOOR) * Cylinder(STANDOFF_R, STANDOFF_H, align=BOTTOM)
        tub -= Pos(x, y, BASE_FLOOR) * Cylinder(
            INSERT_HOLE_D / 2, STANDOFF_H, align=BOTTOM
        )

    front_y = -BASE_D / 2
    tub -= Pos(OLED_POS[0], front_y, OLED_POS[1]) * Box(
        OLED_WINDOW[0], 20, OLED_WINDOW[1]
    )
    tub -= Pos(ENCODER_POS[0], front_y, ENCODER_POS[1]) * through_y(ENCODER_HOLE_D)
    tub -= Pos(BUTTON_POS[0], front_y, BUTTON_POS[1]) * through_y(BUTTON_HOLE_D)

    rear_y = BASE_D / 2
    tub -= Pos(JACK_POS[0], rear_y, JACK_POS[1]) * through_y(JACK_HOLE_D)
    n = int(GRILLE_R // GRILLE_PITCH)
    for i in range(-n, n + 1):
        for j in range(-n, n + 1):
            dx, dz = i * GRILLE_PITCH, j * GRILLE_PITCH
            if math.hypot(dx, dz) <= GRILLE_R - GRILLE_HOLE_D / 2:
                tub -= Pos(GRILLE_POS[0] + dx, rear_y, GRILLE_POS[1] + dz) * (
                    through_y(GRILLE_HOLE_D)
                )
    return tub


def deck_core() -> Part:
    """Builds the tub lid with the hollow LED core rising from it."""
    deck = Pos(0, 0, TUB_H) * Box(BASE_W, BASE_D, DECK_T, align=BOTTOM)
    for x, y in boss_positions():
        deck -= countersink(x, y, TUB_H, DECK_T)

    core = Pos(0, 0, BASE_H) * Box(CORE_W, CORE_D, CORE_H, align=BOTTOM)
    part = deck + core
    part -= Pos(0, 0, TUB_H - 1) * Box(
        CORE_W - 2 * CORE_WALL, CORE_D - 2 * CORE_WALL, DECK_T + CORE_H + 2,
        align=BOTTOM,
    )

    wrap = strip_wrap()
    slot = Box(WIRE_SLOT_W, 10, WIRE_SLOT_H)
    for wall, coord, z in (wrap["head"], wrap["tail"]):
        part -= wall_cutter(wall, coord, z, slot, CORE_W / 2, CORE_D / 2)
    return part


def shade_floor() -> Part:
    """Builds the floor with its four corner posts, over the core.

    The panels' bottom edges sit in a shallow recess in the floor's top face;
    each post takes an insert in its top for the top plate's screws.
    """
    floor = Pos(0, 0, BASE_H) * Box(SHADE_W, SHADE_D, SHADE_FLOOR_T, align=BOTTOM)
    floor -= Pos(0, 0, BASE_H) * Box(
        CORE_W + 2 * CORE_FIT, CORE_D + 2 * CORE_FIT, SHADE_FLOOR_T, align=BOTTOM
    )
    for x, y in shade_boss_positions():
        floor += Pos(x, y, SHADE_Z0) * Box(
            POST, POST, SHADE_ZTOP - SHADE_Z0, align=BOTTOM
        )
        floor -= Pos(x, y, SHADE_ZTOP - POST_INSERT_DEPTH) * Cylinder(
            INSERT_HOLE_D / 2, POST_INSERT_DEPTH, align=BOTTOM
        )
    for face in FACES:
        floor -= panel_seat(face)
    return floor


def shade_top() -> Part:
    """Builds the flat top plate that clamps the panels onto the floor.

    Four countersunk M3 screws go down through it into the post tops; the
    panels' top edges sit in a shallow recess in its underside.
    """
    top = Pos(0, 0, SHADE_ZTOP) * Box(SHADE_W, SHADE_D, SHADE_CAP_T, align=BOTTOM)
    for face in FACES:
        top -= panel_seat(face)
    for x, y in shade_boss_positions():
        top -= countersink(x, y, SHADE_ZTOP, SHADE_CAP_T)
    return top


def build() -> dict[str, Part]:
    """Builds every part in its assembled position."""
    parts = {
        "base_tub": base_tub(),
        "deck_core": deck_core(),
        "shade_floor": shade_floor(),
        "shade_top": shade_top(),
    }
    for face in FACES:
        parts[f"panel_{face}"] = place_panel(panel_flat(face), face)
    return parts


def report() -> None:
    """Prints the derived strip-wrap and clearance numbers."""
    wrap = strip_wrap()
    gap_x = (SHADE_W - 2 * SHADE_WALL - CORE_W) / 2
    gap_y = (SHADE_D - 2 * SHADE_WALL - CORE_D) / 2
    gap_top = SHADE_ZTOP - (BASE_H + CORE_H)
    print(f"core perimeter   {wrap['perimeter']:.0f} mm")
    print(f"strip turns      {wrap['turns']:.2f}")
    print(f"helix pitch      {wrap['pitch']:.1f} mm (gap between wraps {wrap['gap']:.1f} mm)")
    print(f"strip head       {wrap['head'][0]} wall, {wrap['head'][1]:.1f}, z={wrap['head'][2]:.1f}")
    print(f"strip tail       {wrap['tail'][0]} wall, {wrap['tail'][1]:.1f}, z={wrap['tail'][2]:.1f}")
    print(f"air gap sides    {gap_x:.1f} x {gap_y:.1f} mm, top {gap_top:.1f} mm")
    print(f"overall          {SHADE_W:.0f} x {SHADE_D:.0f} x {BASE_H + SHADE_H:.0f} mm")
    oled_lo = OLED_POS[1] - OLED_PCB[1] / 2
    oled_hi = OLED_POS[1] + OLED_PCB[1] / 2
    print(f"oled pcb z       {oled_lo:.1f}..{oled_hi:.1f} (floor {BASE_FLOOR:.1f}, deck {TUB_H:.1f})")
    oled_x_hi = OLED_POS[0] + OLED_PCB[0] / 2
    boss_x_lo = BASE_W / 2 - BOSS_INSET - BOSS_R
    print(f"oled pcb x edge  {oled_x_hi:.1f} (corner boss starts {boss_x_lo:.1f})")
    perf_front = PERF_POS[1] - PERF_SIZE[1] / 2
    print(f"perfboard y      {perf_front:.1f}..{PERF_POS[1] + PERF_SIZE[1] / 2:.1f} "
          f"(walls inside {-BASE_D / 2 + BASE_WALL:.1f}..{BASE_D / 2 - BASE_WALL:.1f})")
    print(f"button nut x     {BUTTON_POS[0] - 7.0:.1f} (corner boss starts {-boss_x_lo:.1f})")
    print(f"speaker frame x  {GRILLE_POS[0] - 19.0:.1f} (corner boss starts {-boss_x_lo:.1f})")
    print(f"core top z       {BASE_H + CORE_H:.1f} (shade inside top {SHADE_ZTOP:.1f})")
    for face in FACES:
        w, h = panel_size(face)
        fish = ", ".join(f"({u:.0f},{v:.0f}) {t:+.0f}deg x{s:.2f}"
                         for u, v, t, s in fish_layout(face))
        print(f"panel {face:5s}      {w:.1f} x {h:.1f} mm; fish {fish}")


def main() -> None:
    """Builds all parts and writes STEP/STL files plus an assembly STEP."""
    OUT_DIR.mkdir(exist_ok=True)
    parts = build()
    for name, part in parts.items():
        export_step(part, str(OUT_DIR / f"{name}.step"))
        if name.startswith("panel_"):
            export_stl(panel_flat(name[len("panel_"):]), str(OUT_DIR / f"{name}.stl"))
        else:
            export_stl(part, str(OUT_DIR / f"{name}.stl"))
        bb = part.bounding_box()
        print(f"{name:12s} {bb.size.X:6.1f} x {bb.size.Y:6.1f} x {bb.size.Z:6.1f} mm"
              f"  vol {part.volume / 1000:6.1f} cm3")
    assembly = Compound(label="ajs_sunrise_alarm", children=list(parts.values()))
    export_step(assembly, str(OUT_DIR / "assembly.step"))
    report()


if __name__ == "__main__":
    main()
