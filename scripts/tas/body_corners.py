# body_corners.py - car body box corners (guessed extents) rotated by the probe quaternion: highest corner and z range per tick. Input: fvlab probe output on stdin. MIT licence, see LICENSE-MIT.
"""Body box corners (car frame x right, y up, z forward; GUESS extents) rotated by the probe quaternion.
stdin: probe output with q lines (stdout+stderr). argv: from_ms to_ms. Prints per tick: pos, top corner (max y), cp."""
import sys, itertools
qs, rows = [], []
for line in sys.stdin:
    line = line.strip()
    if line.startswith("q,"): qs.append([float(v) for v in line.split(",")[1:5]])
    elif line[:1].isdigit(): rows.append(line.split(","))
def rot(q, v):
    x, y, z, w = q; vx, vy, vz = v
    dot = x*vx + y*vy + z*vz; cx, cy, cz = y*vz - z*vy, z*vx - x*vz, x*vy - y*vx; s = w*w - (x*x + y*y + z*z)
    return (2*dot*x + s*vx + 2*w*cx, 2*dot*y + s*vy + 2*w*cy, 2*dot*z + s*vz + 2*w*cz)
lo, hi = int(sys.argv[1]), int(sys.argv[2])
for q, r in zip(qs, rows):
    t = int(r[0])
    if t < lo or t > hi: continue
    p = [float(r[2]), float(r[3]), float(r[4])]
    cs = [rot(q, c) for c in itertools.product((-0.95, 0.95), (-0.3, 0.8), (-2.0, 2.1))]
    top = max(cs, key=lambda c: c[1])
    zs = [p[2] + c[2] for c in cs]
    print(f"{t} cp{r[14]} pos({p[0]:.2f},{p[1]:.2f},{p[2]:.2f}) top y {p[1]+top[1]:.2f} at x {p[0]+top[0]:.2f} z {p[2]+top[2]:.2f}  body z {min(zs):.2f}..{max(zs):.2f}")
