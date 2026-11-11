# fvlab inputs CSV -> TMInterface script (with set speed fast/slow)
"""Convert an fvlab inputs CSV (inputs/opt mode) to a TMInterface / ForeverTAS input script.

Time mapping follows ForeverTAS `input_timeline_time.h`: script time = event time - (race start event
time + 10 ms), so the replay's first tick input (10 ms) becomes `0.00`.

usage: python scripts/to_script.py best.csv [--fast 10 --slow-at 11.5] [--until 13.0] > run.txt
  --fast N      start the script with `set speed N` (TMInterface runs console commands from scripts when
                `execute_commands` is on), so the known part plays fast
  --slow-at S   `set speed 1` at script time S (I use 1-2 s before the new part)
  --until S     drop all inputs after script time S (hand-over: I drive on from there)
"""
import argparse
import csv

SWITCH = {1: "up", 3: "down", 5: "left", 6: "right", 9: "enter"}
ap = argparse.ArgumentParser()
ap.add_argument("csv")
ap.add_argument("--fast", type=float)
ap.add_argument("--slow-at", type=float)
ap.add_argument("--until", type=float)
a = ap.parse_args()
rows = [{k: int(v) for k, v in r.items()} for r in csv.DictReader(open(a.csv, newline=""))]
rows.sort(key=lambda r: r["timeMs"])
start = next((r["timeMs"] for r in rows if r["action"] == 7), 0)
origin = start + 10
out = []  # (script ms, order, line)
if a.fast:
    out.append((0, 0, "0.00 set speed %g" % a.fast))
if a.slow_at is not None:
    ms = int(round(a.slow_at * 1000)) // 10 * 10
    out.append((ms, 0, "%d.%02d set speed 1" % (ms // 1000, (ms % 1000) // 10)))
for r in rows:
    t, action, kind, switch, analog = r["timeMs"], r["action"], r["kind"], r["switch"], r["analog"]
    if t < start or action in (7, 8, 0):
        continue
    user = max(0, t - origin)
    if a.until is not None and user > a.until * 1000:
        continue
    stamp = "%d.%02d" % (user // 1000, (user % 1000) // 10)
    if action in (4, 2) and kind == 2:
        out.append((user, 1, "%s %s %d" % (stamp, "steer" if action == 4 else "gas", analog)))
    elif kind == 1 and action in SWITCH:
        pressed = switch != 0
        if action == 9 and not pressed:
            continue
        out.append((user, 1, "%s %s %s" % (stamp, "press" if pressed else "rel", SWITCH[action])))
for _, _, line in sorted(out, key=lambda o: (o[0], o[1])):
    print(line)
