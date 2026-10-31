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
    floor, top = parts["shade_floor"], parts["shade_top"]
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

    print("\nTop plate screws (M3x8 countersunk down into post-top inserts)")
    shade_xy = {(round(x, 2), round(y, 2)) for x, y in enc.shade_boss_positions()}
    top_clear = xy_set(holes(top, "Z").get(round(enc.SCREW_CLEAR_D, 2), []))
    check_true("top plate clearance holes, 3.4 mm", len(top_clear) == 4)
    check_true("top holes line up with post inserts", top_clear == shade_xy)
    tcsk = top.faces().filter_by(GeomType.CONE)
    flush = all(abs(f.bounding_box().max.Z - (enc.SHADE_ZTOP + enc.SHADE_CAP_T)) < 1e-3
                for f in tcsk)
    check_true("top countersinks 6.6 mm, opening on the top face",
               len(tcsk) == 4 and flush
               and all(abs(c - enc.SCREW_HEAD_D) < 0.05 for c in cone_top_diameters(top)))
    post_ins = holes(floor, "Z").get(round(enc.INSERT_HOLE_D, 2), [])
    check_true("post-top insert holes, 4.2 mm", xy_set(post_ins) == shade_xy,
               f"{len(post_ins)} found")
    for e in post_ins[:1]:
        check("post insert hole depth", e[3], enc.POST_INSERT_DEPTH)
        check("post insert opens at post top", e[2] + e[3] / 2, enc.SHADE_ZTOP)
    check_true("post wall around insert >= 1.5 mm",
               (enc.POST - enc.INSERT_HOLE_D) / 2 >= 1.5,
               f"{(enc.POST - enc.INSERT_HOLE_D) / 2:.1f} mm")
    reach = enc.SHADE_CAP_T + enc.POST_INSERT_DEPTH
    check_true("M3x8 fits: plate + hole depth >= 8", reach >= 8, f"{reach:.1f} mm available")
    check_true("M3x8 reaches through 4 mm insert", 8 - enc.SHADE_CAP_T >= 4,
               f"{8 - enc.SHADE_CAP_T:.1f} mm past plate")

    print("\nFish panels")
    shade_parts = {k: v for k, v in parts.items()
                   if k.startswith("panel_") or k in ("shade_floor", "shade_top", "deck_core")}
    names = list(shade_parts)
    worst = 0.0
    for i, a in enumerate(names):
        for b in names[i + 1:]:
            worst = max(worst, (shade_parts[a] & shade_parts[b]).volume)
    check_true("no shade parts overlap (panels, top, floor+posts, core)", worst < 0.01,
               f"max overlap {worst:.3f} mm3")
    heading = {"front": (1, 0), "right": (0, 1), "back": (-1, 0), "left": (0, -1)}
    for face in enc.FACES:
        flat = enc.panel_flat(face)
        eyes = [f for f in flat.faces().filter_by(GeomType.CYLINDER)
                if f.radius is not None and f.radius < 3
                and abs(f.bounding_box().min.Z - enc.PANEL_T) < 1e-3]
        layout = enc.fish_layout(face)
        check_true(f"{face}: {enc.FISH_COUNTS[face]} fish, each with an eye",
                   len(layout) == enc.FISH_COUNTS[face] and len(eyes) == len(layout),
                   f"{len(layout)} fish, {len(eyes)} eyes")
        depths = {round(e.bounding_box().size.Z, 2) for e in eyes}
        bottoms = {round(e.bounding_box().min.Z, 2) for e in eyes}
        check_true(f"{face}: eyes go down to panel surface, not through",
                   depths == {enc.FISH_H} and bottoms == {enc.PANEL_T},
                   f"depth {sorted(depths)}, bottom at {sorted(bottoms)}")
        check_true(f"{face}: no screw holes on the panel",
                   not flat.faces().filter_by(GeomType.CONE)
                   and len(flat.faces().filter_by(GeomType.CYLINDER)) == len(eyes))
        bb = parts[f"panel_{face}"].bounding_box()
        check_true(f"{face}: panel edges sit in floor and top recesses",
                   abs(bb.min.Z - (enc.SHADE_Z0 - enc.SEAT_DEPTH)) < 1e-3
                   and enc.SHADE_ZTOP < bb.max.Z <= enc.SHADE_ZTOP + enc.SEAT_DEPTH,
                   f"{enc.SHADE_Z0 - bb.min.Z:.1f} mm down, {bb.max.Z - enc.SHADE_ZTOP:.1f} mm up")
        mid = enc.place_panel(enc.Pos(1, 0, 0) * enc.Box(0.1, 0.1, 0.1), face).center()
        org = enc.place_panel(enc.Box(0.1, 0.1, 0.1), face).center()
        hx, hy = round(mid.X - org.X), round(mid.Y - org.Y)
        check_true(f"{face}: fish swim counter-clockwise (seen from above)",
                   (hx, hy) == heading[face], f"heading ({hx},{hy})")
    big = max(max(p.bounding_box().size.X, p.bounding_box().size.Y)
              for p in (floor, top))
    check_true("largest shade part fits a 220 mm bed", big <= 220, f"{big:.0f} mm")

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
