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
| `shade_floor` | translucent PLA | ring that drops over the core and rests on the deck |
| `shade_shell` | translucent PLA | open-bottom box; M3x10 countersunk screws go up through the floor into inserts in four corner bosses |

## Run

The Python environment lives outside OneDrive at `C:\Users\PC\tools\cad-venv`
(build123d + pyvista).

```sh
# build parts, export STEP/STL to out/, print strip-wrap numbers
/c/Users/PC/tools/cad-venv/Scripts/python.exe cad/enclosure.py

# measure the built parts: hole sizes, insert/screw fit, mating alignment
/c/Users/PC/tools/cad-venv/Scripts/python.exe cad/check.py

# render preview PNGs to out/
cd cad && /c/Users/PC/tools/cad-venv/Scripts/python.exe render.py
```

## Output (`out/`)

- `assembly.step` — all parts in position; import into Fusion.
- `<part>.step` / `<part>.stl` — one per part, in assembled position.
- `view_*.png` — front, rear, exploded, and core-with-strip previews.

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
