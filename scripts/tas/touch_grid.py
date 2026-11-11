# touch_grid.py - steer grid around a front-wheel touch with gas held, peak and kept speed per variant (fvlab probe). Input: replay, inputs CSV. MIT licence, see LICENSE-MIT.
"""Small grid at the front-wheel touch of an uber set-up: gas held through the touch + one steer value from T on.

usage: python scripts/tas/touch_grid.py REPLAY LINE.csv T_MS END_MS   (fvlab: env FVLAB_BIN, packs: env TM_PACKS)
Removes every gas release in [T - 50, END) (gas stays pressed, as in the reference ubers), then puts one steer
event at T with values -65536..65536 (step 8192) and at T-20 / T+20; prints the peak km/h in [T, END + 300)
and the speed 150 ms after the peak (a real uber holds it).
"""
import csv
import os
import subprocess
import sys
import tempfile
from concurrent.futures import ThreadPoolExecutor

from fvlab_env import fvlab_exe, packs_dir

FV, PACKS = fvlab_exe(), packs_dir()
replay, line, T, END = sys.argv[1], sys.argv[2], int(sys.argv[3]), int(sys.argv[4])
rows = list(csv.reader(open(line, newline="")))
head, body = rows[0], rows[1:]
base = [r for r in body if not (r[1] == "1" and r[3] == "0" and T - 50 <= int(r[0]) < END)]
tmp = tempfile.mkdtemp()


def run(args):
    t, steer = args
    v = [r for r in base if not (r[1] == "4" and int(r[0]) == t)] + [[str(t), "4", "2", "0", str(steer)]]
    v.sort(key=lambda r: int(r[0]))
    p = os.path.join(tmp, "g_%d_%d.csv" % (t, steer))
    with open(p, "w", newline="") as f:
        w = csv.writer(f, lineterminator="\n")
        w.writerow(head)
        w.writerows(v)
    out = subprocess.run([FV, PACKS, replay, "probe", str(T), str((END + 300 - T) // 10)],
                         env=dict(os.environ, FVLAB_OPTIMIZED="1", FVLAB_INPUTS=p), capture_output=True, text=True).stdout
    sp = [(int(l.split(",")[0]), float(l.split(",")[1])) for l in out.splitlines() if l[:1].isdigit()]
    os.remove(p)
    i = max(range(len(sp)), key=lambda k: sp[k][1])
    later = sp[min(i + 15, len(sp) - 1)][1]
    return t, steer, sp[i][0], sp[i][1], later


jobs = [(t, s) for t in (T - 20, T, T + 20) for s in range(-65536, 65537, 8192)]
with ThreadPoolExecutor(max(1, (os.cpu_count() or 2) - 1)) as ex:
    res = sorted(ex.map(run, jobs), key=lambda r: -r[4])
for t, s, tp, pk, later in res[:12]:
    print("steer %6d at %d: peak %.0f km/h at %d, 150 ms later %.0f" % (s, t, pk, tp, later))
