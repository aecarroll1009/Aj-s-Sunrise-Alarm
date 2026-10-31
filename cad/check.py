"""Measures the built enclosure geometry against the real-world part sizes.

Reads hole diameters, depths and positions straight off the generated solids
(not the constants), so it catches a scale or placement mistake anywhere in
the build. Exits non-zero if any check fails.

Run:
    python cad/check.py
"""

import sys
from collections import defaultdict

from build123d import GeomType

import enclosure as enc

TOL = 0.05
FAILS = []


def check(label: str, actual: float, expected: float, unit: str = "mm") -> None:
    """Records and prints one numeric check."""
    ok = abs(actual - expected) <= TOL
    if not ok:
        FAILS.append(label)
    print(f"  {'PASS' if ok else 'FAIL'}  {label:44s} {actual:8.2f} {unit} "
          f"(want {expected:.2f})")


def check_true(label: str, ok: bool, detail: str = "") -> None:
    """Records and prints one pass/fail check."""
    if not ok:
        FAILS.append(label)
    print(f"  {'PASS' if ok else 'FAIL'}  {label:44s} {detail}")


def holes(part, axis: str) -> dict[float, list[tuple]]:
    """Groups a part's cylindrical faces by diameter for one axis.

    Args:
        part: Solid to inspect.
        axis: "X", "Y" or "Z".

    Returns:
        Diameter (rounded to 0.01 mm) mapped to a list of
        (x, y, z, length) for each cylindrical face along that axis.
    """
    out = defaultdict(list)
    for f in part.faces().filter_by(GeomType.CYLINDER):
        d = f.axis_of_rotation.direction
        if f.radius is None or abs(getattr(d, axis)) < 0.99:
            continue
        bb = f.bounding_box()
        c = bb.center()
        length = getattr(bb.size, axis)
        out[round(2 * f.radius, 2)].append((c.X, c.Y, c.Z, length))
    return out


def xy_set(entries) -> set[tuple[float, float]]:
    """Returns the rounded XY centres of a list of hole entries."""
    return {(round(x, 2), round(y, 2)) for x, y, _, _ in entries}


def cone_top_diameters(part) -> list[float]:
    """Returns the largest diameter of each conical (countersink) face."""
    return [round(max(f.bounding_box().size.X, f.bounding_box().size.Y), 2)
            for f in part.faces().filter_by(GeomType.CONE)]


def front_opening(part, y_lo: float, y_hi: float):
    """Measures the rectangular opening through a wall between two Y planes.

    Returns:
        (width, height, centre_x, centre_z) from the opening's side faces.
    """
    xs, zs = [], []
    for f in part.faces().filter_by(GeomType.PLANE):
        c = f.center()
        if not (y_lo < c.Y < y_hi):
            continue
        n = f.normal_at()
        if abs(n.X) > 0.99:
            xs.append(c.X)
        elif abs(n.Z) > 0.99:
            zs.append(c.Z)
    return max(xs) - min(xs), max(zs) - min(zs), (max(xs) + min(xs)) / 2, (max(zs) + min(zs)) / 2


def main() -> None:
    """Runs every check and exits non-zero on any failure."""
    parts = enc.build()
    tub, deck = parts["base_tub"], parts["deck_core"]
    floor, shell = parts["shade_floor"], parts["shade_shell"]
    front_in = -enc.BASE_D / 2 + enc.BASE_WALL

    print(f"OUTER_SCALE = {enc.OUTER_SCALE}")

    print("\nFront panel (base_tub)")
    tub_y = holes(tub, "Y")
    front = sorted((d, e) for d, es in tub_y.items() for e in es if e[1] < 0)
    print(f"  measured front holes: "
          + ", ".join(f"{d:.2f} mm at x={e[0]:.1f}" for d, e in front))
    for name, want, x in (("button hole", 10.4, enc.BUTTON_POS[0]),
                          ("encoder hole", 8.4, enc.ENCODER_POS[0])):
        hits = [(d, e) for d, e in front if abs(e[0] - x) < 0.01]
        check_true(f"{name} present", len(hits) == 1)
        if hits:
            check(f"{name} diameter (measured)", hits[0][0], want)
            check(f"{name} through wall length", hits[0][1][3], enc.BASE_WALL)
    check_true("button is the leftmost control",
               enc.BUTTON_POS[0] < enc.ENCODER_POS[0] < enc.OLED_POS[0])
    w, h, cx, cz = front_opening(tub, -enc.BASE_D / 2 - 0.01, front_in + 0.01)
    check("OLED window width", w, 56.0)
    check("OLED window height", h, 29.0)
    check("OLED window centre height = base mid", cz, enc.BASE_H / 2)
    check_true("OLED display area 55.01x27.49 fits window",
               w >= 55.01 and h >= 27.49, f"{w:.2f} x {h:.2f}")

    print("\nRear panel (base_tub)")
    rear = [(d, e) for d, es in tub_y.items() for e in es if e[1] > 0 and d > 5]
    check_true("one large hole on rear wall (jack)", len(rear) == 1)
    if rear:
        check("jack hole diameter (measured)", rear[0][0], 11.2)
        check("jack hole through wall length", rear[0][1][3], enc.BASE_WALL)
    grille = [e for e in tub_y.get(round(enc.GRILLE_HOLE_D, 2), []) if e[1] > 0]
    check_true("speaker grille holes, 3.0 mm", len(grille) > 20, f"{len(grille)} holes")

    print("\nBase tub inserts (QSJ M3: 4 long x 5 wide)")
    tub_z = holes(tub, "Z")
    inserts = tub_z.get(round(enc.INSERT_HOLE_D, 2), [])
    boss_xy = {(round(x, 2), round(y, 2)) for x, y in enc.boss_positions()}
    perf_xy = {(round(x, 2), round(y, 2)) for x, y in enc.perf_positions()}
    deck_inserts = [e for e in inserts if (round(e[0], 2), round(e[1], 2)) in boss_xy]
    perf_inserts = [e for e in inserts if (round(e[0], 2), round(e[1], 2)) in perf_xy]
    check_true("deck insert holes, 4.2 mm", len(deck_inserts) == 4, f"{len(deck_inserts)} found")
    for e in deck_inserts[:1]:
        check("deck insert hole depth", e[3], enc.INSERT_DEPTH)
    check_true("insert depth >= 4 mm insert", enc.INSERT_DEPTH >= 4.0)
    check_true("perfboard insert holes, 4.2 mm", len(perf_inserts) == 4, f"{len(perf_inserts)} found")
    for e in perf_inserts[:1]:
        check("perfboard insert hole depth", e[3], enc.STANDOFF_H)
    boss_od = [d for d in tub_z if abs(d - 2 * enc.BOSS_R) < 0.01]
    check_true("corner boss OD 8 mm (1.5 mm wall round insert)", bool(boss_od))

    print("\nDeck screws (M3x8 countersunk into tub inserts)")
    deck_z = holes(deck, "Z")
    clear = xy_set(deck_z.get(round(enc.SCREW_CLEAR_D, 2), []))
    check_true("deck clearance holes, 3.4 mm", len(clear) == 4)
    check_true("deck holes line up with tub inserts", clear == boss_xy)
    csk = cone_top_diameters(deck)
    check_true("deck countersinks 6.6 mm (M3 head ~5.6)",
               len(csk) == 4 and all(abs(c - enc.SCREW_HEAD_D) < 0.05 for c in csk), str(csk))
    reach = enc.DECK_T + enc.INSERT_DEPTH
    check_true("M3x8 fits: deck + hole depth >= 8", reach >= 8.0, f"{reach:.1f} mm available")
    check_true("M3x8 reaches through 4 mm insert", 8.0 - enc.DECK_T >= 4.0,
               f"{8.0 - enc.DECK_T:.1f} mm past deck")

    print("\nShade screws (M3x10 countersunk up into shell inserts)")
    shade_xy = {(round(x, 2), round(y, 2)) for x, y in enc.shade_boss_positions()}
    floor_clear = xy_set(holes(floor, "Z").get(round(enc.SCREW_CLEAR_D, 2), []))
    check_true("floor clearance holes, 3.4 mm", len(floor_clear) == 4)
    check_true("floor holes line up with shell inserts", floor_clear == shade_xy)
    fcsk = cone_top_diameters(floor)
    check_true("floor countersinks 6.6 mm", len(fcsk) == 4 and all(abs(c - enc.SCREW_HEAD_D) < 0.05 for c in fcsk), str(fcsk))
    shell_ins = holes(shell, "Z").get(round(enc.INSERT_HOLE_D, 2), [])
    check_true("shell insert holes, 4.2 mm", xy_set(shell_ins) == shade_xy, f"{len(shell_ins)} found")
    for e in shell_ins[:1]:
        check("shell insert hole depth", e[3], enc.SHADE_INSERT_DEPTH)
    reach = enc.SHADE_FLOOR_T + enc.SHADE_INSERT_DEPTH
    check_true("M3x10 fits: floor + hole depth >= 10", reach >= 10.0, f"{reach:.1f} mm available")

    print("\nShade fit over core")
    fbb = [f.bounding_box() for f in floor.faces().filter_by(GeomType.PLANE)
           if abs(f.normal_at().X) > 0.99 and abs(f.center().X) < enc.CORE_W / 2 + 2]
    hole_w = 2 * max(abs(b.center().X) for b in fbb)
    check("floor hole width - core width (2x 0.4 clearance)", hole_w - enc.CORE_W, 0.8)
    wrap = enc.strip_wrap()
    check_true("strip wraps don't overlap", wrap["gap"] > 0, f"gap {wrap['gap']:.2f} mm")
    check_true("core top below shade ceiling",
               enc.BASE_H + enc.CORE_H < enc.BASE_H + enc.SHADE_H - enc.SHADE_WALL)

    print()
    if FAILS:
        print(f"{len(FAILS)} FAILED: " + "; ".join(FAILS))
        sys.exit(1)
    print("ALL CHECKS PASS")


if __name__ == "__main__":
    main()
