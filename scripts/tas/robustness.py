# robustness.py - knife-edge test: nudges single steer events by one step and counts how often the outcome holds (fvlab probe). Input: replay, inputs CSV. MIT licence, see LICENSE-MIT.
"""Knife-edge test of a line: nudge single steer events by the smallest step and count how often the outcome holds.

usage: python scripts/tas/robustness.py REPLAY LINE.csv FROM_MS TO_MS CHECK_MS NEED_CPS [--step 1] [--workers N]
For every steer event in [FROM, TO) two variants (analog +step / -step) are simulated with `fvlab probe` up to
CHECK_MS; a variant "holds" if the car has NEED_CPS checkpoints at CHECK_MS. A line that holds only for a few
variants sits on a knife edge: the game can end elsewhere even where the simulation is exact everywhere else.
"""
import argparse
import csv
import os
import subprocess
import sys
import tempfile
from concurrent.futures import ThreadPoolExecutor

from fvlab_env import fvlab_exe, packs_dir

FV = PACKS = None  # set in main() from FVLAB_BIN / TM_PACKS


def run(replay, rows, head, check_ms, tmp):
    path = os.path.join(tmp, "v%d.csv" % id(rows))
    with open(path, "w", newline="") as f:
        w = csv.writer(f, lineterminator="\n")
        w.writerow(head)
        w.writerows(rows)
    env = dict(os.environ, FVLAB_OPTIMIZED="1", FVLAB_INPUTS=path)
    t0 = check_ms - 10
    out = subprocess.run([FV, PACKS, replay, "probe", str(t0), "2"], env=env, capture_output=True, text=True).stdout
    os.remove(path)
    for line in out.splitlines():
        p = line.split(",")
        if p[0] == str(check_ms):
            return int(p[-1]), float(p[1]), float(p[3])
    return -1, 0.0, 0.0


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("replay")
    ap.add_argument("line")
    ap.add_argument("t_from", type=int)
    ap.add_argument("t_to", type=int)
    ap.add_argument("check_ms", type=int)
    ap.add_argument("need_cps", type=int)
    ap.add_argument("--step", type=int, default=1)
    ap.add_argument("--workers", type=int, default=max(1, (os.cpu_count() or 2) - 1))
    a = ap.parse_args()
    global FV, PACKS
    FV, PACKS = fvlab_exe(), packs_dir()
    with open(a.line, newline="") as f:
        rows = list(csv.reader(f))
    head, body = rows[0], rows[1:]
    idx = [i for i, r in enumerate(body) if r[1] == "4" and a.t_from <= int(r[0]) < a.t_to]
    variants = []
    for i in idx:
        for d in (a.step, -a.step):
            v = [list(r) for r in body]
            v[i][4] = str(max(-65536, min(65536, int(v[i][4]) + d)))
            variants.append((body[i][0], d, v))
    tmp = tempfile.mkdtemp()
    base = run(a.replay, body, head, a.check_ms, tmp)
    with ThreadPoolExecutor(a.workers) as ex:
        res = list(ex.map(lambda x: run(a.replay, x[2], head, a.check_ms, tmp), variants))
    held = sum(1 for r in res if r[0] >= a.need_cps)
    print("base: cps %d, %.0f km/h, y %.1f at %d ms" % (base[0], base[1], base[2], a.check_ms))
    print("variants: %d, held %d (%.0f%%)" % (len(res), held, 100.0 * held / max(1, len(res))))
    for (t, d, _), r in zip(variants, res):
        if r[0] < a.need_cps:
            print("  steer at %s %+d: cps %d, %.0f km/h, y %.1f" % (t, d, r[0], r[1], r[2]))


if __name__ == "__main__":
    sys.exit(main())
