# run_report.py - overview of one FV_TRACE CSV: speed bands, phases, clamp hits, biggest gains with context. Input: trace CSV. MIT licence, see LICENSE-MIT.
"""Overview of one per-call trace: speed profile, burnout phases, clamp hits, biggest per-call gains
with context (phase, nose-down, wheel contacts, surfaces, stacking).

usage: python scripts/physics/run_report.py TRACE.csv [top_n]
"""
import csv
import sys
from collections import Counter

path = sys.argv[1]
top = int(sys.argv[2]) if len(sys.argv) > 2 else 25
with open(path, newline="") as f:
    rows = list(csv.DictReader(f))
W = ("fl", "fr", "rr", "rl")
kmh = [float(r["kmh"]) for r in rows]
ticks = sorted({int(r["tick"]) for r in rows})
print("calls", len(rows), "ticks", len(ticks), "max km/h %.1f" % max(kmh))
over = [r for r in rows if float(r["kmh"]) >= 999.0]
print("calls >= 999 km/h:", len(over), " ticks:", len({r["tick"] for r in over}))
ph = Counter((r["phase_before"], r["phase"]) for r in rows)
print("phase transitions (before>after):", dict(ph))
bands = Counter(int(float(r["kmh"]) // 100) * 100 for r in rows)
print("calls per 100 km/h band:", dict(sorted(bands.items())))

gains = []
for a, b in zip(rows, rows[1:]):
    g = float(b["kmh"]) - float(a["kmh"])
    gains.append((g, a, b))
gains.sort(key=lambda x: -x[0])
print("\ntop per-call gains (gain measured to the next call):")
print("  tick call   kmh -> kmh_next  gain  ph  vloc(x,y,z)            contacts mats  n(fl fr rr rl)  gas brk steer")
for g, a, b in gains[:top]:
    c = "".join(a[w + "_c"][:1] for w in W)
    m = "/".join(a[w + "_m"] for w in W)
    n = " ".join(a[w + "_n"] for w in W)
    print("  %5s %2s %6.1f -> %6.1f %+6.1f %s>%s (%6.1f %6.1f %6.1f) %s %s  %s  %s %s %s" % (
        a["tick"], a["call"], float(a["kmh"]), float(b["kmh"]), g, a["phase_before"], a["phase"],
        float(a["vx"]), float(a["vy"]), float(a["vz"]), c, m, n, a["gas"], a["brake"], a["steer"]))

# sustained gain segments: ticks whose speed grows for >= 20 ticks in a row with phase 2 involvement
by_tick = {}
for r in rows:
    by_tick.setdefault(int(r["tick"]), []).append(r)
print("\nsegments with >= 30 km/h net gain over <= 1 s while any call is in the donut (phase 2):")
seg_start = None
last = None
for t in ticks:
    rs = by_tick[t]
    donut = any(r["phase_before"] == "2" or r["phase"] == "2" for r in rs)
    if donut and seg_start is None:
        seg_start = t
    if not donut and seg_start is not None:
        a, b = by_tick[seg_start][0], by_tick[last][-1]
        g = float(b["kmh"]) - float(a["kmh"])
        if g > 30:
            print("  ticks %d-%d (%d ms): %.0f -> %.0f km/h (%+.0f)" % (seg_start, last, last - seg_start + 10,
                                                                    float(a["kmh"]), float(b["kmh"]), g))
        seg_start = None
    last = t
