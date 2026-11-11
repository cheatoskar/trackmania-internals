# analyze_trace.py - calls per tick, burnout phases and biggest per-call speed gains of an FV_TRACE CSV. Input: trace CSV. MIT licence, see LICENSE-MIT.
"""Summarise a ForeverValidator per-call trace (FV_TRACE CSV, see tools/fvlab/README.md).

usage: python scripts/physics/analyze_trace.py TRACE.csv [--events N]
Prints calls per tick, time in each burnout phase, the biggest per-call speed gains and what phase /
contacts / speeds they had, so donut and burnout effects can be tied to speed jumps.
"""
import csv
import sys
from collections import Counter, defaultdict

PHASE = {0: "normal", 1: "burnout", 2: "donut", 3: "exit"}


def load(path):
    with open(path, newline="") as f:
        rows = list(csv.DictReader(f))
    for r in rows:
        for k, v in r.items():
            if v == "":
                r[k] = None
            else:
                try:
                    r[k] = float(v)
                except ValueError:
                    pass
    return rows


def main():
    path = sys.argv[1]
    nev = int(sys.argv[sys.argv.index("--events") + 1]) if "--events" in sys.argv else 15
    rows = load(path)
    if not rows:
        print("empty trace")
        return
    calls = Counter(int(r["tick"]) for r in rows)
    hist = Counter(calls.values())
    print("rows", len(rows), "ticks", len(calls), "calls/tick histogram", dict(sorted(hist.items())))
    ph = Counter(int(r["phase"]) for r in rows)
    print("phase counts (calls):", {PHASE[k]: v for k, v in sorted(ph.items())})
    print("max kmh %.1f" % max(r["kmh"] for r in rows))

    # speed gain per call: kmh at the start of the next call minus this call's start
    gains = []
    for a, b in zip(rows, rows[1:]):
        gains.append((b["kmh"] - a["kmh"], a, b))
    gains.sort(key=lambda g: -g[0])
    print("\nbiggest per-call gains (kmh_next - kmh):")
    print(" gain   tick call  kmh    vz_local  vx_local phase  dist_c  r_t   g b steer body wheels(FL FR RR RL contact/material)")
    for g, a, b in gains[:nev]:
        wh = " ".join("%d/%d" % (a[w + "_c"] or 0, a[w + "_m"] or 0) for w in ("fl", "fr", "rr", "rl"))
        print("%6.1f %6d %3d %7.1f %8.2f %8.2f %-7s %6.2f %5.2f %d %d %5.2f %d %s" % (
            g, a["tick"], a["call"], a["kmh"], a["vz"], a["vx"],
            PHASE[int(a["phase_before"])] + ">" + str(int(a["phase"])), a["dist_center"], a["target_r"],
            a["gas"] > 0.1, a["brake"] > 0.1, a["steer"], a["body_contacts"], wh))

    # gain grouped by phase
    by = defaultdict(list)
    for g, a, b in gains:
        by[int(a["phase"])].append(g)
    print("\nmean gain per call by phase:", {PHASE[k]: round(sum(v) / len(v), 3) for k, v in sorted(by.items())})


if __name__ == "__main__":
    main()
