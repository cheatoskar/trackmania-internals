# multicontact.py - calls of an FV_TRACE CSV where one wheel collects several contacts (seams, bluebug). Input: trace CSV. MIT licence, see LICENSE-MIT.
"""List calls where a wheel collects several contact samples at once (block seams, "bluebug"), with the
velocity / spin kick of that call. Needs a trace with the per-wheel *_n columns (trace v2).

usage: python scripts/physics/multicontact.py TRACE.csv [--min 3]
"""
import csv
import math
import sys


def fl(r, k):
    v = r.get(k, "")
    return float(v) if v not in ("", None) else 0.0


def main():
    path = sys.argv[1]
    nmin = int(sys.argv[sys.argv.index("--min") + 1]) if "--min" in sys.argv else 3
    rows = list(csv.DictReader(open(path, newline="")))
    print("  tick call    kmh  wheel  n  normal(x,y,z)            dv_y   dw_pitch  dw_roll  steer gas brake")
    count = 0
    for a, b in zip(rows, rows[1:]):
        for w in ("fl", "fr", "rr", "rl"):
            n = int(fl(b, w + "_n"))
            if n >= nmin:
                count += 1
                print("%6d %3d %7.1f   %s %3d  (%6.3f %6.3f %6.3f)  %6.2f  %7.2f  %7.2f  %5.2f %d %d" % (
                    int(b["tick"]), int(b["call"]), fl(a, "kmh"), w.upper(), n,
                    fl(b, w + "_nx"), fl(b, w + "_ny"), fl(b, w + "_nz"),
                    fl(b, "vy") - fl(a, "vy"), fl(b, "wx") - fl(a, "wx"), fl(b, "wz") - fl(a, "wz"),
                    fl(b, "steer"), fl(b, "gas") > 0.1, fl(b, "brake") > 0.1))
    print("events:", count)


if __name__ == "__main__":
    main()
