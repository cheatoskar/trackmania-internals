# find_kicks.py - calls of an FV_TRACE CSV where local velocity or spin jump, with wheel contacts. Input: trace CSV. MIT licence, see LICENSE-MIT.
"""Find per-call velocity kicks in a trace (FV_TRACE CSV): calls where the car-local velocity or the
angular velocity changes by more than a threshold between consecutive calls, with wheel contacts.

usage: python scripts/physics/find_kicks.py TRACE.csv [--top N] [--t0 MS --t1 MS]
"""
import csv
import math
import sys


def f(r, k):
    v = r[k]
    return float(v) if v not in ("", None) else 0.0


def main():
    path = sys.argv[1]
    top = int(sys.argv[sys.argv.index("--top") + 1]) if "--top" in sys.argv else 20
    t0 = int(sys.argv[sys.argv.index("--t0") + 1]) if "--t0" in sys.argv else 0
    t1 = int(sys.argv[sys.argv.index("--t1") + 1]) if "--t1" in sys.argv else 1 << 30
    rows = [r for r in csv.DictReader(open(path, newline="")) if t0 <= int(r["tick"]) <= t1]
    ev = []
    for a, b in zip(rows, rows[1:]):
        dv = math.dist([f(a, "vx"), f(a, "vy"), f(a, "vz")], [f(b, "vx"), f(b, "vy"), f(b, "vz")])
        dw = math.dist([f(a, "wx"), f(a, "wy"), f(a, "wz")], [f(b, "wx"), f(b, "wy"), f(b, "wz")])
        ev.append((dv + 0.5 * dw, dv, dw, a, b))
    ev.sort(key=lambda e: -e[0])
    print("  tick call   kmh   dv(m/s)  dw(rad/s)  v_local(x,y,z)            w(x,y,z)              "
          "contacts FL FR RR RL (c/mat) -> next")
    for s, dv, dw, a, b in ev[:top]:
        def wh(r):
            return " ".join("%d/%d" % (int(f(r, w + "_c")), int(f(r, w + "_m"))) for w in ("fl", "fr", "rr", "rl"))
        print("%6d %3d %7.1f %7.2f %8.2f  (%6.2f %6.2f %6.2f) (%6.2f %6.2f %6.2f)  %s -> %s  body=%s" % (
            int(a["tick"]), int(a["call"]), f(a, "kmh"), dv, dw, f(b, "vx"), f(b, "vy"), f(b, "vz"),
            f(b, "wx"), f(b, "wy"), f(b, "wz"), wh(a), wh(b), b["body_contacts"]))


if __name__ == "__main__":
    main()
