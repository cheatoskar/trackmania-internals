# touch_probe.py - per tick speed, contacts, pose, heading vs velocity and the triangle under each touching wheel (fvlab probe/scene). Input: replay, inputs CSV. MIT licence, see LICENSE-MIT.
"""Touch analysis: per tick speed, contacts, pitch/roll, heading vs velocity bearing, inputs, and for touching
wheels the nearest collision triangle (height, normal).
argv: replay inputs_csv|- from_ms ticks [scene]   (scene=1 queries triangles near the car on touch ticks)
Bearing: 0 = +z (north), 90 = +x (east)."""
import sys, os, subprocess, math
from fvlab_env import fvlab_exe, packs_dir
EXE, PACKS = fvlab_exe(), packs_dir()  # env FVLAB_BIN, TM_PACKS
rep, inp, t0, n = sys.argv[1], sys.argv[2], int(sys.argv[3]), int(sys.argv[4])
want_scene = len(sys.argv) > 5 and sys.argv[5] == "1"
env = {k: v for k, v in os.environ.items() if not k.startswith("FVLAB_")}
env["FVLAB_OPTIMIZED"] = "1"
if inp != "-": env["FVLAB_INPUTS"] = inp
out = subprocess.run([EXE, PACKS, rep, "probe", str(t0 - 10), str(n + 1)], env=env, capture_output=True, text=True).stdout
rows, qs = [], []
for line in out.splitlines():
    s = line.strip()
    if s.startswith("q,"): qs.append([float(v) for v in s.split(",")[1:5]])
    elif s[:1].isdigit(): rows.append(s.split(","))
def rot(q, v):
    x, y, z, w = q; vx, vy, vz = v
    dot = x*vx + y*vy + z*vz; cx, cy, cz = y*vz - z*vy, z*vx - x*vz, x*vy - y*vx; s = w*w - (x*x + y*y + z*z)
    return (2*dot*x + s*vx + 2*w*cx, 2*dot*y + s*vy + 2*w*cy, 2*dot*z + s*vz + 2*w*cz)
WH = [(0.863, 0.352, 1.782), (-0.863, 0.352, 1.782), (-0.885, 0.352, -1.206), (0.885, 0.352, -1.206)]
NAMES = ["FL", "FR", "RL", "RR"]
def bearing(dx, dz): return math.degrees(math.atan2(dx, dz)) % 360
tri_cache = {}
def tris_near(p):
    key = (round(p[0] / 4), round(p[2] / 4))
    if key not in tri_cache:
        o = subprocess.run([EXE, PACKS, rep, "scene", f"{p[0]:.2f}", f"{p[1]:.2f}", f"{p[2]:.2f}", "25"], env=env,
                           capture_output=True, text=True).stdout
        ts = []
        for l in o.splitlines()[1:]:
            v = [float(a) for a in l.split(",")]
            ts.append((v[0:3], v[3:6], v[6:9]))
        tri_cache[key] = ts
    return tri_cache[key]
def sub(a, b): return [a[i] - b[i] for i in range(3)]
def cross(a, b): return [a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0]]
def dotp(a, b): return sum(a[i]*b[i] for i in range(3))
def closest_tri(p, ts):
    best = None
    for a, b, c in ts:
        nrm = cross(sub(b, a), sub(c, a)); L = math.sqrt(dotp(nrm, nrm))
        if L < 1e-9: continue
        nrm = [x / L for x in nrm]
        d = dotp(sub(p, a), nrm)
        q = [p[i] - d * nrm[i] for i in range(3)]
        # inside test (barycentric via same-side)
        ins = all(dotp(cross(sub(v2, v1), sub(q, v1)), nrm) >= -1e-6 for v1, v2 in ((a, b), (b, c), (c, a)))
        if not ins: continue
        if best is None or abs(d) < abs(best[0]):
            if nrm[1] < 0: nrm = [-x for x in nrm]
            best = (abs(d), q[1], nrm)
    return best
prev = None
for r, q in zip(rows, qs):
    t = int(r[0]); p = [float(r[2]), float(r[3]), float(r[4])]
    if prev is None: prev = p; continue
    v = sub(p, prev); prev = p
    f = rot(q, (0, 0, 1))
    hb, vb = bearing(f[0], f[2]), bearing(v[0], v[2])
    dang = (hb - vb + 180) % 360 - 180
    vy = v[1] * 100 * 3.6
    line = (f"{t} {float(r[1]):6.1f} c{r[10]} pit{float(r[5]):6.1f} rol{float(r[6]):6.1f} head{hb:5.0f} vel{vb:5.0f} "
            f"h-v{dang:5.0f} vy{vy:6.1f} g{r[11]} b{r[12]} s{float(r[13]):+.2f} cp{r[14]} pos({p[0]:.1f},{p[1]:.2f},{p[2]:.1f})")
    if want_scene and "1" in r[10]:
        parts = []
        for i in range(4):
            if r[10][i] != "1": continue
            w = rot(q, WH[i]); wp = [p[j] + w[j] for j in range(3)]
            ct = closest_tri(wp, tris_near(p))
            if ct: parts.append(f"{NAMES[i]}@({wp[0]:.1f},{wp[2]:.1f}) surf y{ct[1]:.2f} n({ct[2][0]:+.2f},{ct[2][1]:+.2f},{ct[2][2]:+.2f})")
            else: parts.append(f"{NAMES[i]}@({wp[0]:.1f},{wp[2]:.1f}) no tri")
        line += " | " + "; ".join(parts)
    print(line)
