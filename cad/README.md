# Enclosure CAD

Parametric 3D-printed enclosure: a rectangular "blocky mushroom" with an
opaque electronics base, a rectangular LED core rising from the base deck, and
a translucent shade box over the core. All dimensions live as constants at the
top of `enclosure.py`. `OUTER_SCALE` (currently 1.1) scales the base, shade and
core and where features sit; hole, insert, wall and screw sizes stay fixed.
`python cad/enclosure.py` prints a clearance report after each build.

| Part | Material | Notes |
|---|---|---|
| `base_tub` | opaque PLA | electronics, OLED/encoder/button front, jack + speaker grille rear |
| `deck_core` | opaque PLA | screws onto the tub (M3 countersunk into heat-set inserts); 1 m strip wraps the core |
| `shade_floor` | translucent PLA | 3 mm floor that drops over the core, with four slim 8 mm corner posts (M3 insert in each top) and a 1.2 mm-deep recess for the panel bottoms. Print floor-down. |
| `shade_top` | translucent PLA | flat 3 mm top plate with a matching recess underneath and four flush countersunk holes. |
| `panel_front/right/back/left` | translucent PLA | plain flat 2 mm panels with 1.6 mm raised goldfish (eye is a hole down to the panel). No holes or screws. |

Shade assembly: install 4 M3 inserts in the post tops, stand the four panels in
the floor's recess against the posts (front and back panels cover the side
panels' ends), set the top plate on, and screw it down with 4x M3x8. The only
visible screws are four flush heads on the top surface. Fish all swim
counter-clockwise seen from above (3 front, 2 right, 3 back, 1 left);
`FISH_EYE_R` and `FISH_EYE_POS` shape them. Placement comes from
`fish/layout.json` for any panel it lists, otherwise from a seeded random
layout (`FISH_COUNTS`, `FISH_SEED`, `FISH_SCALE`, `FISH_TILT`).

## Placing fish by hand

`fish/fish_edit_<face>.step` holds each panel lying flat with the plate and every
fish as separate bodies. X is the viewer's right and Y is up the lamp, and the
fish sit on top at +Z.

1. Open one in Fusion. Move/Copy the fish bodies in the XY plane, rotate them
   about Z, scale them uniformly, copy or delete them. Keep each fish a
   separate body on the plate: don't Combine, mirror or stretch.
2. Export it as STEP over the same file: File → Export, Type "STEP Files",
   Location `cadish`, name `fish_edit_<face>`. Fusion's Save only stores
   a Fusion design in its cloud, which the import can't read.
3. Run `python cad/fish_io.py import`. It recovers each fish's position, tilt
   and size into `fish/layout.json` and warns about any fish that enters the
   recesses or the end strips. Then rebuild with `enclosure.py` and run
   `check.py`.

`python cad/fish_io.py export` rewrites the edit files from the current layout.
It skips files that already exist unless you pass `--force`.

## Run

The Python environment lives outside OneDrive at `C:\Users\PC\tools\cad-venv`
(build123d + pyvista).

```sh
# build parts, export STEP files, print strip-wrap numbers
/c/Users/PC/tools/cad-venv/Scripts/python.exe cad/enclosure.py

# measure the built parts: hole sizes, insert/screw fit, mating alignment
/c/Users/PC/tools/cad-venv/Scripts/python.exe cad/check.py

# render preview PNGs
cd cad && /c/Users/PC/tools/cad-venv/Scripts/python.exe render.py
```

## Folders

| Folder | Contents |
|---|---|
| `to_print/` | one STEP per part, ready for the slicer: on the bed at Z = 0, in print orientation, no supports needed. Panels lie flat fish up; the top plate is flipped so its visible face prints on the bed. |
| `assembly/` | `assembly.step`, every part in its assembled position, for Fusion. |
| `fish/` | the hand-placed fish layout: `fish_edit_<face>.step` edit files and `layout.json`. |
| `renders/` | preview PNGs: front, rear, exploded, head-on sides, core with strip, tub inside, shade underside, and the fish outline plot. |

`to_print/`, `assembly/` and `renders/` are regenerated on every build; `fish/`
is an input and is only changed by `fish_io.py`.

## Placeholders to measure

Front cutouts (OLED window, encoder, button), rear speaker grille, plus
mounts for the OLED and speaker.

Perfboard is the kit's 50x70 mm board, set back from the front wall so the
OLED module fits in front of it; it sits on four 6 mm standoffs with M3
insert holes. `PERF_HOLE_PITCH` (65x45 mm) is a guess at the corner-hole
spacing until measured. `OLED_PCB` (61.5x39.5 mm) is a typical 2.42" module
size, used only for the clearance report and previews.

Rear barrel jack is set for a DC-099 5.5x2.5 mm threaded panel jack (11 mm
nominal, 11.2 mm printed clearance); confirm with calipers once it arrives.
