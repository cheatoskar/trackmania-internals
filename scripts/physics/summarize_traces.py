# summarize_traces.py - one markdown line per FV_TRACE CSV in a folder (max speed, phases, multi-contacts, biggest gain). Input: trace folder. MIT licence, see LICENSE-MIT.
"""One line per trace CSV in a folder: max speed, calls in each burnout phase, multi-contact wheel events,
biggest single-call speed gain and when. Writes <folder>/index.md.

usage: python scripts/physics/summarize_traces.py [TRACE_FOLDER]   (default: traces)
"""
import csv
import glob
import math
import os
import sys

tdir = sys.argv[1] if len(sys.argv) > 1 else "traces"
lines = ["| trace | max km/h | calls normal/burnout/donut/exit | multi-contact (n>=3) | biggest gain per call |",
         "|---|---|---|---|---|"]
for path in sorted(glob.glob(os.path.join(tdir, "*.csv"))):
    rows = list(csv.DictReader(open(path, newline="")))
    if not rows:
        continue
    ph = [0, 0, 0, 0]
    mc = 0
    best = (0.0, None)
    for a, b in zip(rows, rows[1:]):
        ph[int(a["phase"])] += 1
        if "fl_n" in b and any(int(float(b[w + "_n"] or 0)) >= 3 for w in ("fl", "fr", "rr", "rl")):
            mc += 1
        g = float(b["kmh"]) - float(a["kmh"])
        if g > best[0]:
            best = (g, a)
    a = best[1]
    gain = "%.1f km/h at tick %s (phase %s, %.0f km/h)" % (best[0], a["tick"], a["phase"], float(a["kmh"])) if a else "-"
    name = os.path.splitext(os.path.basename(path))[0]
    lines.append("| %s | %.0f | %d/%d/%d/%d | %d | %s |" % (
        name, max(float(r["kmh"]) for r in rows), ph[0], ph[1], ph[2], ph[3], mc, gain))
open(os.path.join(tdir, "index.md"), "w", encoding="utf-8").write("\n".join(lines) + "\n")
print("\n".join(lines))
