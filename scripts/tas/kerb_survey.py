# kerb_survey.py - road-border stretches of a drive whose kick direction points at a checkpoint. Input: fvlab blocks CSV, probe CSV, map JSON. MIT licence, see LICENSE-MIT.
"""Kerb survey: road-border stretches along a drive whose kick direction points at a checkpoint.

Usage: python scripts/tas/kerb_survey.py <blocks.csv> <probe.csv> <map.json> [min_kmh]

blocks.csv is `fvlab ... blocks` output, probe.csv a `probe` dump of the drive (every tick or every few ticks),
map.json the output of map_analysis.py for the same map (checkpoint and finish centres).
A kerb uber (seen in public TASes) kicks the car roughly across the border it hits, i.e. along the
outward normal of the road side, plus some of the slide direction. For every straight main-road block the drive
crosses at min_kmh or more, the script takes the road axis from the drive's horizontal velocity there (snapped to
x or z) and, for both sides, lists the checkpoints within 35 deg of the outward normal, with distance and height.
"""
import csv
import json
import math
import sys

STRAIGHT = ("StadiumRoadMain", "StadiumRoadMainCheckpoint", "StadiumRoadMainCheckpointLeft",
            "StadiumRoadMainCheckpointRight", "StadiumRoadMainTurbo", "StadiumRoadMainStartLine",
            "StadiumRoadMainFinishLine", "StadiumPlatformToRoad")


def main():
    blocks_path, probe_path, map_path = sys.argv[1], sys.argv[2], sys.argv[3]
    min_kmh = float(sys.argv[4]) if len(sys.argv) > 4 else 200
    m = json.load(open(map_path, encoding="utf-8"))
    CPS = {f"CP{c['id']}": tuple(c["centre"]) for c in m["checkpoints"]}
    CPS["Fin"] = tuple(m["finish"]["centre"])
    blocks = set()
    for b in csv.DictReader(open(blocks_path, encoding="utf-8")):
        if b["block"] in STRAIGHT:
            blocks.add((b["block"], float(b["x"]), float(b["y"]), float(b["z"])))
    rows = [r for r in csv.reader(open(probe_path, encoding="utf-8")) if r and r[0][:1].isdigit() and len(r) >= 15]
    seen = {}
    for i in range(1, len(rows)):
        t, kmh, x, y, z = (float(v) for v in rows[i][:5])
        if kmh < min_kmh:
            continue
        px, pz = float(rows[i - 1][2]), float(rows[i - 1][4])
        vx, vz = x - px, z - pz
        if vx * vx + vz * vz < 1e-6:
            continue
        for name, bx, by, bz in blocks:
            if bx <= x < bx + 32 and bz <= z < bz + 32 and by - 1 <= y <= by + 6:
                axis = "x" if abs(vx) > abs(vz) else "z"
                key = (bx, by, bz, axis)
                if key not in seen:
                    seen[key] = (t, kmh, name)
    for (bx, by, bz, axis), (t, kmh, name) in sorted(seen.items(), key=lambda kv: kv[1][0]):
        normals = [(1, 0), (-1, 0)] if axis == "z" else [(0, 1), (0, -1)]
        hits = []
        for nx, nz in normals:
            cx, cz = bx + 16 + nx * 13, bz + 16 + nz * 13
            for cp, (qx, qy, qz) in CPS.items():
                dx, dz = qx - cx, qz - cz
                d = math.hypot(dx, dz)
                if d < 1:
                    continue
                ang = math.degrees(math.acos(max(-1, min(1, (dx * nx + dz * nz) / d))))
                if ang <= 35:
                    side = {(1, 0): "+x", (-1, 0): "-x", (0, 1): "+z", (0, -1): "-z"}[(nx, nz)]
                    hits.append(f"{side}->{cp} {d:.0f}m {ang:.0f}deg dy{qy - by:+.0f}")
        print(f"t {t:6.2f} {kmh:4.0f} km/h {name:32s} block ({bx:.0f},{by:.0f},{bz:.0f}) axis {axis}: " +
              ("; ".join(hits) if hits else "-"))


if __name__ == "__main__":
    main()
