# scan_tas_ubers.py - probes TAS replays with fvlab and lists kept speed gains (uber candidates). Input: replay folder. MIT licence, see LICENSE-MIT.
"""Scan TAS replays for kept speed gains (uber candidates).

Usage: python scripts/tas/scan_tas_ubers.py <replay dir> <out.csv>   (fvlab: env FVLAB_BIN, packs: env TM_PACKS)

For every replay: extract nothing, just probe the whole run with fvlab (FVLAB_OPTIMIZED=1) and report each tick where the
speed 10 ticks later is at least 40 km/h above the speed 2 ticks before (a gain the car keeps, not a one-tick spike),
with position, pitch, roll, contacts and the inputs at that tick. Nearby events are merged (one per 0.3 s).
"""
import csv
import os
import re
import subprocess
import sys

from fvlab_env import fvlab_exe, packs_dir



def run_ms(name):
    m = re.search(r"(\d\d)[_'](\d\d)[_'']+(\d\d)", name)
    if not m:
        return 60000
    return (int(m.group(1)) * 60 + int(m.group(2))) * 1000 + int(m.group(3)) * 10


def main():
    src, out = sys.argv[1], sys.argv[2]
    FVLAB, PACKS = fvlab_exe(), packs_dir()
    env = dict(os.environ, FVLAB_OPTIMIZED="1")
    rows = []
    for name in sorted(os.listdir(src)):
        if not name.lower().endswith(".replay.gbx"):
            continue
        ticks = run_ms(name) // 10 + 50
        try:
            res = subprocess.run([FVLAB, PACKS, os.path.join(src, name), "probe", "0", str(ticks)], env=env,
                                 capture_output=True, text=True, timeout=300)
        except subprocess.TimeoutExpired:
            print(f"{name}: timeout", flush=True)
            continue
        ticks_rows = [l.split(",") for l in res.stdout.splitlines()[1:] if l and not l.lstrip().startswith("q,")]
        ticks_rows = [r for r in ticks_rows if len(r) >= 15]
        if not ticks_rows:
            print(f"{name}: no probe output", flush=True)
            continue
        kmh = [float(r[1]) for r in ticks_rows]
        last_t = -1e9
        found = 0
        for i in range(2, len(kmh) - 10):
            gain = min(kmh[i + 5:i + 11]) - kmh[i - 2]
            if gain >= 40 and kmh[i - 2] >= 100:
                t = float(ticks_rows[i][0])
                if t - last_t < 300:
                    continue
                last_t = t
                found += 1
                r = ticks_rows[i]
                rows.append({"replay": name, "t": t / 1000, "kmh_before": kmh[i - 2], "kmh_peak": max(kmh[i:i + 11]),
                             "kmh_kept": min(kmh[i + 5:i + 11]), "x": r[2], "y": r[3], "z": r[4], "pitch": r[5],
                             "roll": r[6], "contacts": r[10], "gas": r[11], "brake": r[12], "steer": r[13]})
        print(f"{name}: {len(kmh)} ticks, {found} events", flush=True)
    with open(out, "w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=list(rows[0].keys()) if rows else ["replay"])
        w.writeheader()
        w.writerows(rows)


if __name__ == "__main__":
    main()
