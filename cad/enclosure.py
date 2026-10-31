"""Parametric enclosure for AJ's Sunrise Alarm.

Blocky mushroom layout: an opaque electronics base, a rectangular LED core
rising from the base deck, and a translucent shade box that drops over the
core and rests on the deck. All dimensions are millimetres; the origin is the
bottom-centre of the base, +X is the long side, -Y is the front face.

Parts (each exported as STEP and STL to ``cad/out``):
    base_tub    opaque PLA, holds the electronics and front/rear cutouts.
    deck_core   opaque PLA, screws onto the tub; the core carries the strip.
    shade_floor translucent PLA, flat ring that locates on the core.
    shade_shell translucent PLA, open-bottom box screwed up through the floor
                into corner inserts.

Run:
    python cad/enclosure.py
"""

import math
from pathlib import Path

from build123d import (
    Align,
    Box,
    Compound,
    Cone,
    Cylinder,
    Location,
    Part,
    Pos,
    Rot,
    export_step,
    export_stl,
)

OUT_DIR = Path(__file__).parent / "out"

BASE_W, BASE_D, BASE_H = 140.0, 80.0, 60.0
BASE_WALL = 3.0
BASE_FLOOR = 3.0
DECK_T = 3.0
TUB_H = BASE_H - DECK_T

SHADE_W, SHADE_D, SHADE_H = 170.0, 110.0, 80.0
SHADE_WALL = 2.0
SHADE_FLOOR_T = 3.0
SHADE_LIP_H = 2.0
SHADE_FIT = 0.2
CORE_FIT = 0.4
SHADE_BOSS_R = 4.5
SHADE_BOSS_H = 10.0
SHADE_BOSS_INSET = SHADE_WALL + 4.0
SHADE_INSERT_DEPTH = 8.0

CORE_W, CORE_D, CORE_H = 120.0, 60.0, 60.0
CORE_WALL = 2.0

STRIP_LEN = 1000.0
STRIP_W = 10.0
STRIP_MARGIN = 5.0
STRIP_START = 10.0
WIRE_SLOT_W, WIRE_SLOT_H = 10.0, 6.0

BOSS_R = 4.0
BOSS_INSET = BASE_WALL + 3.5
INSERT_HOLE_D = 4.2
INSERT_DEPTH = 6.0
SCREW_CLEAR_D = 3.4
SCREW_HEAD_D = 6.6

OLED_WINDOW = (56.0, 29.0)
OLED_POS = (-25.0, 36.0)
ENCODER_HOLE_D = 8.4
ENCODER_POS = (28.0, 30.0)
BUTTON_HOLE_D = 10.4
BUTTON_POS = (48.0, 30.0)

JACK_HOLE_D = 11.2
JACK_POS = (45.0, 20.0)
GRILLE_POS = (-30.0, 34.0)
GRILLE_R = 18.0
GRILLE_HOLE_D = 3.0
GRILLE_PITCH = 5.0

PERF_SIZE = (90.0, 70.0)
PERF_T = 1.6
PERF_HOLE_PITCH = (85.0, 65.0)
STANDOFF_H = 6.0
STANDOFF_R = 4.0

BOTTOM = (Align.CENTER, Align.CENTER, Align.MIN)


def boss_positions() -> list[tuple[float, float]]:
    """Returns the XY centres of the four corner screw bosses."""
    x = BASE_W / 2 - BOSS_INSET
    y = BASE_D / 2 - BOSS_INSET
    return [(sx * x, sy * y) for sx in (-1, 1) for sy in (-1, 1)]


def shade_boss_positions() -> list[tuple[float, float]]:
    """Returns the XY centres of the shade's corner screw bosses."""
    x = SHADE_W / 2 - SHADE_BOSS_INSET
    y = SHADE_D / 2 - SHADE_BOSS_INSET
    return [(sx * x, sy * y) for sx in (-1, 1) for sy in (-1, 1)]


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
    return [(sx * x, sy * y) for sx in (-1, 1) for sy in (-1, 1)]


def perfboard() -> Part:
    """Returns a stand-in for the perfboard, for previews only."""
    z = BASE_FLOOR + STANDOFF_H
    return Pos(0, 0, z) * Box(*PERF_SIZE, PERF_T, align=BOTTOM)


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
    """Builds the translucent floor ring that drops over the core."""
    floor = Pos(0, 0, BASE_H) * Box(SHADE_W, SHADE_D, SHADE_FLOOR_T, align=BOTTOM)
    lip_w = SHADE_W - 2 * (SHADE_WALL + SHADE_FIT)
    lip_d = SHADE_D - 2 * (SHADE_WALL + SHADE_FIT)
    lip_z = BASE_H + SHADE_FLOOR_T
    floor += Pos(0, 0, lip_z) * Box(lip_w, lip_d, SHADE_LIP_H, align=BOTTOM)
    floor -= Pos(0, 0, lip_z) * Box(
        lip_w - 2 * SHADE_WALL, lip_d - 2 * SHADE_WALL, SHADE_LIP_H, align=BOTTOM
    )
    floor -= Pos(0, 0, BASE_H) * Box(
        CORE_W + 2 * CORE_FIT, CORE_D + 2 * CORE_FIT, SHADE_FLOOR_T, align=BOTTOM
    )
    for x, y in shade_boss_positions():
        floor -= Pos(x, y, lip_z) * Cylinder(
            SHADE_BOSS_R + SHADE_FIT * 2, SHADE_LIP_H, align=BOTTOM
        )
        floor -= countersink(x, y, BASE_H, SHADE_FLOOR_T, from_below=True)
    return floor


def shade_shell() -> Part:
    """Builds the translucent open-bottom shade box."""
    z0 = BASE_H + SHADE_FLOOR_T
    h = SHADE_H - SHADE_FLOOR_T
    shell = Pos(0, 0, z0) * Box(SHADE_W, SHADE_D, h, align=BOTTOM)
    shell -= Pos(0, 0, z0) * Box(
        SHADE_W - 2 * SHADE_WALL, SHADE_D - 2 * SHADE_WALL, h - SHADE_WALL,
        align=BOTTOM,
    )
    for x, y in shade_boss_positions():
        shell += Pos(x, y, z0) * Cylinder(SHADE_BOSS_R, SHADE_BOSS_H, align=BOTTOM)
        shell += Pos(x, y, z0 + SHADE_BOSS_H) * Cone(
            SHADE_BOSS_R, 0, SHADE_BOSS_R, align=BOTTOM
        )
        shell -= Pos(x, y, z0) * Cylinder(
            INSERT_HOLE_D / 2, SHADE_INSERT_DEPTH, align=BOTTOM
        )
    return shell


def build() -> dict[str, Part]:
    """Builds every part in its assembled position."""
    return {
        "base_tub": base_tub(),
        "deck_core": deck_core(),
        "shade_floor": shade_floor(),
        "shade_shell": shade_shell(),
    }


def report() -> None:
    """Prints the derived strip-wrap and clearance numbers."""
    wrap = strip_wrap()
    gap_x = (SHADE_W - 2 * SHADE_WALL - CORE_W) / 2
    gap_y = (SHADE_D - 2 * SHADE_WALL - CORE_D) / 2
    gap_top = BASE_H + SHADE_H - SHADE_WALL - (BASE_H + CORE_H)
    print(f"core perimeter   {wrap['perimeter']:.0f} mm")
    print(f"strip turns      {wrap['turns']:.2f}")
    print(f"helix pitch      {wrap['pitch']:.1f} mm (gap between wraps {wrap['gap']:.1f} mm)")
    print(f"strip head       {wrap['head'][0]} wall, {wrap['head'][1]:.1f}, z={wrap['head'][2]:.1f}")
    print(f"strip tail       {wrap['tail'][0]} wall, {wrap['tail'][1]:.1f}, z={wrap['tail'][2]:.1f}")
    print(f"air gap sides    {gap_x:.1f} x {gap_y:.1f} mm, top {gap_top:.1f} mm")
    print(f"overall          {SHADE_W:.0f} x {SHADE_D:.0f} x {BASE_H + SHADE_H:.0f} mm")


def main() -> None:
    """Builds all parts and writes STEP/STL files plus an assembly STEP."""
    OUT_DIR.mkdir(exist_ok=True)
    parts = build()
    for name, part in parts.items():
        export_step(part, str(OUT_DIR / f"{name}.step"))
        export_stl(part, str(OUT_DIR / f"{name}.stl"))
        bb = part.bounding_box()
        print(f"{name:12s} {bb.size.X:6.1f} x {bb.size.Y:6.1f} x {bb.size.Z:6.1f} mm"
              f"  vol {part.volume / 1000:6.1f} cm3")
    assembly = Compound(label="ajs_sunrise_alarm", children=list(parts.values()))
    export_step(assembly, str(OUT_DIR / "assembly.step"))
    report()


if __name__ == "__main__":
    main()
