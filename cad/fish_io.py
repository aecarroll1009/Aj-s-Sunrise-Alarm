"""Round-trips the goldfish layout through Fusion (or any CAD) as STEP files.

``export`` writes one flat panel per file to ``cad/fish``: the plate plus each
fish as its own body, in panel-local coordinates (X to the viewer's right, Y
up the lamp, fish on top at +Z). Move, rotate, uniformly scale, copy or delete
the fish bodies, keep them as separate bodies sitting on the plate, and save
over the same file. ``import`` reads the files back, recovers each fish's
position, tilt and size, and writes ``cad/fish/layout.json``, which
enclosure.py builds from.

Run:
    python cad/fish_io.py export [--force]
    python cad/fish_io.py import
"""

import argparse
import json
import math

from build123d import CenterOf, Compound, GeomType, Pos, export_step, import_step

import enclosure as enc

FIT_TOL = 0.02
POS_TOL = 0.05


def edit_path(face: str):
    """Returns the editable STEP path for one panel."""
    return enc.FISH_DIR / f"fish_edit_{face}.step"


def export(force: bool) -> None:
    """Writes each panel's plate and fish as separate bodies.

    Args:
        force: Overwrite edit files that already exist.
    """
    enc.FISH_DIR.mkdir(exist_ok=True)
    for face in enc.FACES:
        path = edit_path(face)
        if path.exists() and not force:
            print(f"{path.name}: exists, left alone (--force to overwrite)")
            continue
        plate = enc.panel_plate(face)
        plate.label = "plate"
        bodies = [plate]
        for i, fish in enumerate(enc.fish_layout(face), 1):
            body = enc.place_fish(*fish)
            body.label = f"fish {i}"
            bodies.append(body)
        export_step(Compound(label=f"panel_{face}", children=bodies), str(path))
        print(f"{path.name}: plate + {len(bodies) - 1} fish")


def fish_pose(solid, ref_centroid) -> tuple[float, float, float, float]:
    """Recovers (u, v, tilt, size) of one fish body standing on the plate.

    The eye circle gives the size and one point; the centre of mass gives the
    other, and together they fix the position and tilt.

    Args:
        solid: The fish body, already shifted into panel-local coordinates.
        ref_centroid: (x, y) centre of mass of an untransformed size-1 fish.

    Returns:
        The pose in fish_layout form.

    Raises:
        ValueError: The body has no eye circle.
    """
    circles = solid.edges().filter_by(GeomType.CIRCLE)
    if not circles:
        raise ValueError("no eye circle found")
    s = circles[0].radius / enc.FISH_EYE_R
    eye_x = sum(c.arc_center.X for c in circles) / len(circles)
    eye_y = sum(c.arc_center.Y for c in circles) / len(circles)
    c = solid.center(CenterOf.MASS)
    ref = math.atan2(enc.FISH_EYE_POS[1] - ref_centroid[1],
                     enc.FISH_EYE_POS[0] - ref_centroid[0])
    tilt = math.degrees(math.atan2(eye_y - c.Y, eye_x - c.X) - ref)
    tilt = (tilt + 180) % 360 - 180
    t = math.radians(tilt)
    ox, oy = s * ref_centroid[0], s * ref_centroid[1]
    u = c.X - (ox * math.cos(t) - oy * math.sin(t))
    v = c.Y - (ox * math.sin(t) + oy * math.cos(t))
    return u, v, tilt, s


def read_panel(face: str) -> list[tuple[float, float, float, float]]:
    """Reads one edited panel file and returns its fish placements.

    Args:
        face: front, right, back or left.

    Returns:
        Fish placements sorted left to right.

    Raises:
        ValueError: The file cannot be matched to a plate plus fish bodies.
    """
    path = edit_path(face)
    solids = sorted(import_step(str(path)).solids(), key=lambda x: -x.volume)
    if len(solids) < 2:
        raise ValueError(f"{path.name}: found {len(solids)} body; keep each fish "
                         "a separate body (don't Combine them with the plate)")
    plate, fish_bodies = solids[0], solids[1:]
    w, h = enc.panel_size(face)
    bb = plate.bounding_box()
    if abs(bb.size.Z - enc.PANEL_T) > POS_TOL or abs(bb.size.X - w) > POS_TOL:
        raise ValueError(f"{path.name}: plate is {bb.size.X:.1f} x {bb.size.Y:.1f} x "
                         f"{bb.size.Z:.1f} mm; expected it flat, {w:.1f} wide along X")
    shift = Pos(-bb.center().X, -bb.min.Y, -bb.min.Z)
    if max(abs(bb.center().X), abs(bb.min.Y), abs(bb.min.Z)) > POS_TOL:
        print(f"  {face}: plate was moved; measuring fish relative to it")
    ref = enc.fish_profile(1.0).center(CenterOf.MASS)
    layout = []
    for i, body in enumerate(fish_bodies, 1):
        body = shift * body
        lift = body.bounding_box().min.Z - enc.PANEL_T
        if abs(lift) > POS_TOL:
            print(f"  {face}: a fish was {lift:+.1f} mm off the plate; set back on it")
        body = Pos(0, 0, -lift) * body
        try:
            pose = fish_pose(body, (ref.X, ref.Y))
        except ValueError as e:
            raise ValueError(f"{path.name}: body {i} ({e})") from None
        height = body.bounding_box().size.Z
        if abs(height - enc.FISH_H) > POS_TOL:
            print(f"  {face}: a fish was {height:.2f} mm thick after scaling; "
                  f"rebuilt at {enc.FISH_H} mm")
        rebuilt = enc.place_fish(*pose, height)
        misfit = ((rebuilt - body).volume + (body - rebuilt).volume) / body.volume
        if misfit > FIT_TOL:
            raise ValueError(
                f"{path.name}: body {i} differs {misfit:.0%} from a moved goldfish; "
                "only move, rotate in the panel plane, and scale uniformly "
                "(no mirroring or stretching)")
        layout.append(pose)
    return sorted(layout)


def warn_placement(face: str, layout) -> None:
    """Prints fish that leave the panel's clear zone or touch each other."""
    u_lo, u_hi, v_lo, v_hi = enc.fish_zone(face)
    bodies = [enc.place_fish(*f) for f in layout]
    for i, body in enumerate(bodies, 1):
        bb = body.bounding_box()
        over = max(u_lo - bb.min.X, bb.max.X - u_hi, v_lo - bb.min.Y, bb.max.Y - v_hi)
        if over > 0:
            print(f"  WARNING {face} fish {i}: {over:.1f} mm outside the clear zone "
                  f"(u {u_lo:.0f}..{u_hi:.0f}, v {v_lo:.1f}..{v_hi:.1f})")
        for j in range(i, len(bodies)):
            if (body & bodies[j]).volume > 0.01:
                print(f"  note {face}: fish {i} and {j + 1} overlap and will merge")


def import_layout() -> None:
    """Reads every edited panel file and writes FISH_LAYOUT_FILE."""
    saved = {}
    if enc.FISH_LAYOUT_FILE.exists():
        saved = json.loads(enc.FISH_LAYOUT_FILE.read_text(encoding="utf-8"))
    for face in enc.FACES:
        if not edit_path(face).exists():
            print(f"{face}: no {edit_path(face).name}, keeping current layout")
            continue
        layout = read_panel(face)
        saved[face] = [{"u": round(u, 2), "v": round(v, 2), "tilt": round(t, 1),
                        "size": round(s, 3)} for u, v, t, s in layout]
        fish = ", ".join(f"({u:.0f},{v:.0f}) {t:+.0f}deg x{s:.2f}" for u, v, t, s in layout)
        print(f"{face}: {len(layout)} fish {fish}")
        warn_placement(face, layout)
    enc.FISH_LAYOUT_FILE.write_text(json.dumps(saved, indent=2) + "\n", encoding="utf-8")
    print(f"wrote {enc.FISH_LAYOUT_FILE}; run enclosure.py to rebuild the panels")


def main() -> None:
    """Parses the command line and runs export or import."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("command", choices=("export", "import"))
    parser.add_argument("--force", action="store_true",
                        help="overwrite existing edit files on export")
    args = parser.parse_args()
    if args.command == "export":
        export(args.force)
    else:
        import_layout()


if __name__ == "__main__":
    main()
