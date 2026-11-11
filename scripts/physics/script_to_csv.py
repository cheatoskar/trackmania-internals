# TMInterface script -> fvlab inputs CSV
"""Convert a TMInterface input script (`0.04 steer 2564`, `0.00 press up`, `1.20 rel down`, `1.0-1.5 press up`)
to an fvlab inputs CSV, optionally spliced in front of an existing CSV (the script replaces everything before
its last event + 10 ms; the CSV's held state at that moment is re-applied).

usage: python scripts/script_to_csv.py script.txt [--onto line.csv] > out.csv
Time mapping is the inverse of to_script.py: CSV time = script time + 10 ms (race start event at 0).
"""
import argparse, csv, sys

KEYS = {"up": 1, "down": 3, "left": 5, "right": 6, "enter": 9}
ap = argparse.ArgumentParser()
ap.add_argument("script")
ap.add_argument("--onto")
a = ap.parse_args()


def ms(s):
    return int(round(float(s) * 1000))


ev = [(0, 7, 1, 1, 0)]
for line in open(a.script, encoding="utf-8", errors="replace"):
    p = line.split()
    if len(p) < 3 or p[1] == "set":
        continue
    t0, t1 = (p[0].split("-") + [None])[:2]
    t0 = ms(t0) + 10
    if p[1] == "steer":
        ev.append((t0, 4, 2, 0, int(p[2])))
        if t1:
            ev.append((ms(t1) + 10, 4, 2, 0, 0))
    elif p[1] == "gas":
        ev.append((t0, 2, 2, 0, int(p[2])))
    elif p[1] in ("press", "rel") and p[2] in KEYS:
        ev.append((t0, KEYS[p[2]], 1, 1 if p[1] == "press" else 0, 0))
        if t1 and p[1] == "press":
            ev.append((ms(t1) + 10, KEYS[p[2]], 1, 0, 0))
cut = max(e[0] for e in ev) + 10
if a.onto:
    rows = [tuple(int(v) for v in (r["timeMs"], r["action"], r["kind"], r["switch"], r["analog"]))
            for r in csv.DictReader(open(a.onto, newline=""))]
    held = {}
    for r in rows:
        if 0 < r[0] < cut and r[1] not in (7, 8, 0):
            held[(r[1], r[2])] = r
    trick_last = {}
    for e in ev:
        trick_last[(e[1], e[2])] = e
    for k, r in held.items():
        # re-apply the line's state at the cut unless the script left the same state
        if k not in trick_last or (trick_last[k][3], trick_last[k][4]) != (r[3], r[4]):
            ev.append((cut, r[1], r[2], r[3], r[4]))
    for k, e in trick_last.items():
        if k not in held and (e[3] or e[4]):
            ev.append((cut, e[1], e[2], 0, 0))  # the line had this input released
    ev += [r for r in rows if r[0] >= cut and r[1] != 7]
ev.sort(key=lambda e: e[0])
w = csv.writer(sys.stdout, lineterminator="\n")
w.writerow(["timeMs", "action", "kind", "switch", "analog"])
w.writerows(ev)
