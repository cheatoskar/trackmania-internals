#!/usr/bin/env python3
# map_analysis.py - start/finish/checkpoints of a map from `fvlab blocks`, shortest CP order, driven order, top-down plot. Input: replay (+ trace). MIT licence, see LICENSE-MIT.
"""Map analysis without the game.

Usage (repo root):
  python scripts/tas/map_analysis.py REPLAY.Replay.Gbx [--trace TRACE.csv] [--fvlab EXE] [--packs DIR] [--out maps/<map>/map]

Reads the placed blocks with `fvlab blocks`, finds start / finish / checkpoints / multilap blocks, computes the
shortest checkpoint order (start and finish fixed, any order in between, straight-line 3D distance) and the
order the driven run took (from the trace, call==0 rows, default traces/<name>.csv). Writes
<out>/<name>.json and <out>/<name>.png (default out/maps) (SVG if matplotlib is missing).
Block positions from fvlab are corners; Stadium blocks are 32 x 8 x 32 m, centres = corner + (16, 4, 16).
"""
import argparse, csv, itertools, json, math, os, re, subprocess, sys

from fvlab_env import fvlab_exe, packs_dir

BW, BH = 32.0, 8.0
PASS_M = 24.0  # closest approach within this (3D, to the block centre) counts as passed; a guess
GROUND = re.compile(r"^Stadium(Water|Pool|Grass)")  # not drawn (flat ground fill)


def read_blocks(fvlab, packs, replay):
    out = subprocess.run([fvlab, packs, replay, "blocks"], capture_output=True, text=True, check=True).stdout
    seen, blocks, trig = set(), [], []
    for ln in out.splitlines()[1:]:
        p = ln.strip().split(",")
        if len(p) < 6:
            continue
        name, purpose = p[0], int(p[1])
        pos = tuple(float(v) for v in p[3:6])
        # rotated blocks: the origin is the corner the local axes start from (bxx,bxz,bzx,bzz from fvlab blocks)
        bxx, bxz, bzx, bzz = (float(v) for v in p[6:10]) if len(p) >= 10 else (1.0, 0.0, 0.0, 1.0)
        pos = (pos[0] + BW / 2 * (bxx + bzx), pos[1], pos[2] + BW / 2 * (bxz + bzz))  # footprint centre x/z
        pos = (pos[0] - BW / 2, pos[1], pos[2] - BW / 2)  # back to the min corner of the footprint
        key = (name, purpose, pos)
        if key in seen:
            continue
        seen.add(key)
        if purpose == 1:
            blocks.append({"name": name, "x": pos[0], "y": pos[1], "z": pos[2]})
        elif purpose == 4:
            trig.append({"name": name, "x": pos[0], "y": pos[1], "z": pos[2]})
    return blocks, trig


def centre(b):
    return [b["x"] + BW / 2, b["y"] + BH / 2, b["z"] + BW / 2]


def classify(blocks, trig):
    """Start/finish/checkpoints from block names. Checkpoints = purpose-4 trigger blocks named *Checkpoint*
    plus the *CheckpointRing* blocks (real checkpoints in game: one map counted 16 = 11 + 5 rings, although
    fvlab lists no purpose-4 row for the rings)."""
    def find(pat, lst=blocks):
        return [b for b in lst if re.search(pat, b["name"])]
    start = find(r"StartLine|StartFinish|Start$") or find(r"Start")
    start = [b for b in start if "Slope" not in b["name"] and "Loop" not in b["name"]]
    finish = find(r"FinishLine|StartFinish")
    multi = find(r"Multilap")
    cps = [t for t in trig if re.search(r"Checkpoint", t["name"])]
    if not cps:
        cps = [b for b in find(r"Checkpoint") if "Ring" not in b["name"]]
    rings = [b for b in find(r"CheckpointRing")]
    cps = cps + rings
    return start, finish, multi, cps, []


CLIMB = 0.0  # extra cost per metre of climb (--climb); climbing needs a jump or an uber, falling is free


def leg_cost(a, b):
    return math.dist(a, b) + CLIMB * max(0.0, b[1] - a[1])


def tour(start, finish, cps):
    """Cheapest start -> all cps (any order) -> finish (cost = leg_cost). Held-Karp up to 16 cps, else NN + 2-opt."""
    n = len(cps)
    d = leg_cost
    if n == 0:
        return [], d(start, finish)
    if n <= 16:
        INF = float("inf")
        D = [[d(a, b) for b in cps] for a in cps]
        dp = [[INF] * n for _ in range(1 << n)]
        par = [[-1] * n for _ in range(1 << n)]
        for i in range(n):
            dp[1 << i][i] = d(start, cps[i])
        for m in range(1, 1 << n):
            for i in range(n):
                c = dp[m][i]
                if c == INF:
                    continue
                for j in range(n):
                    if m >> j & 1:
                        continue
                    nm = m | 1 << j
                    v = c + D[i][j]
                    if v < dp[nm][j]:
                        dp[nm][j] = v
                        par[nm][j] = i
        full = (1 << n) - 1
        best, bi = min((dp[full][i] + d(cps[i], finish), i) for i in range(n))
        order, m, i = [], full, bi
        while i != -1:
            order.append(i)
            m, i = m & ~(1 << i), par[m][i]
        return order[::-1], best
    # nearest neighbour + 2-opt
    left, order, cur = set(range(n)), [], start
    while left:
        j = min(left, key=lambda k: d(cur, cps[k]))
        order.append(j); left.remove(j); cur = cps[j]
    pts = lambda o: [start] + [cps[k] for k in o] + [finish]
    length = lambda o: sum(d(a, b) for a, b in zip(pts(o), pts(o)[1:]))
    imp = True
    while imp:
        imp = False
        for i in range(n - 1):
            for j in range(i + 1, n):
                o = order[:i] + order[i:j + 1][::-1] + order[j + 1:]
                if length(o) < length(order) - 1e-9:
                    order, imp = o, True
    return order, length(order)


def read_trace(path):
    pts = []
    with open(path, newline="") as f:
        for r in csv.DictReader(f):
            if r["call"] == "0":
                pts.append((int(r["tick"]), float(r["px"]), float(r["py"]), float(r["pz"])))
    return pts


def driven_order(pts, cps):
    """Order of closest approach in time: for each checkpoint the tick of the trace point nearest to its centre."""
    res = []
    for i, c in enumerate(cps):
        t, dm = min(((p[0], math.dist(p[1:], c)) for p in pts), key=lambda v: v[1])
        res.append((t, i, dm))
    res.sort()
    return res  # (tick, id, min distance); only entries with distance <= PASS_M count as passed


def plot_png(path, data, blocks, trace):
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    from matplotlib.patches import Rectangle
    fig, ax = plt.subplots(figsize=(15, 15), dpi=100)
    draw = sorted((b for b in blocks if not GROUND.match(b["name"])), key=lambda b: b["y"])
    ys = [b["y"] for b in draw] or [0, 1]
    lo, hi = min(ys), max(ys)
    cm = plt.get_cmap("viridis")
    for b in draw:
        c = cm((b["y"] - lo) / (hi - lo + 1e-9))
        ax.add_patch(Rectangle((b["x"], b["z"]), BW, BW, facecolor=c, edgecolor="#666666", lw=0.4, alpha=0.55))
    sm = plt.cm.ScalarMappable(cmap=cm, norm=plt.Normalize(lo, hi))
    fig.colorbar(sm, ax=ax, shrink=0.5, label="block height y (m)")
    S, F, C = data["start"], data["finish"], data["checkpoints"]
    P = lambda v: (v[0], v[2])
    R = data.get("route_from") or S["centre"]
    path_pts = [R] + [C[i]["centre"] for i in data["shortest_order"]] + [F["centre"]]
    ax.plot([P(p)[0] for p in path_pts], [P(p)[1] for p in path_pts], "-", color="red", lw=2, label="shortest order", zorder=5)
    if trace:
        ax.plot([p[1] for p in trace], [p[3] for p in trace], "-", color="#ff9900", lw=1.3, label="driven run", zorder=6)
    for c in C:
        ax.plot(*P(c["centre"]), "o", color="blue", ms=9, zorder=8)
        ax.annotate(f"CP{c['id']}\ny={c['centre'][1]:.0f}", P(c["centre"]), xytext=(5, 5), textcoords="offset points",
                    fontsize=9, color="blue", weight="bold", zorder=9)
    ax.plot(*P(S["centre"]), "s", color="lime", mec="k", ms=12, zorder=8)
    ax.annotate(f"START y={S['centre'][1]:.0f}", P(S["centre"]), xytext=(6, -14), textcoords="offset points", fontsize=10, weight="bold", zorder=9)
    if data.get("route_from"):
        ax.plot(*P(R), "*", color="gold", mec="k", ms=20, zorder=8)
        ax.annotate(f"ROUTE START y={R[1]:.0f}", P(R), xytext=(6, -16), textcoords="offset points", fontsize=10, weight="bold", zorder=9)
    if data.get("custom_route"):
        W = data["custom_route"]
        ax.plot([w["p"][0] for w in W], [w["p"][2] for w in W], "--", color="#d4a000", lw=2.5, label="my planned route", zorder=7)
        for i, w in enumerate(W):
            ax.plot(w["p"][0], w["p"][2], "o", color="#d4a000", mec="k", ms=7, zorder=8)
            ax.annotate(f"{i + 1} {w['label']}", (w["p"][0], w["p"][2]), xytext=(-6, -14), textcoords="offset points",
                        fontsize=8, color="#7a5c00", weight="bold", ha="right", zorder=9)
    ax.plot(*P(F["centre"]), "D", color="white", mec="k", ms=12, zorder=8)
    ax.annotate(f"FINISH y={F['centre'][1]:.0f}", P(F["centre"]), xytext=(6, 8), textcoords="offset points", fontsize=10, weight="bold", zorder=9)
    allx = [b["x"] for b in draw] + [b["x"] + BW for b in draw]
    allz = [b["z"] for b in draw] + [b["z"] + BW for b in draw]
    x0, x1 = math.floor(min(allx) / 64) * 64, math.ceil(max(allx) / 64) * 64
    z0, z1 = math.floor(min(allz) / 64) * 64, math.ceil(max(allz) / 64) * 64
    ax.set_xlim((x1, x0) if data.get("flip_x") else (x0, x1)); ax.set_ylim(z0, z1)
    ax.set_xticks(range(int(x0), int(x1) + 1, 64)); ax.set_yticks(range(int(z0), int(z1) + 1, 64))
    ax.tick_params(labelsize=8, labeltop=True, labelright=True)
    plt.setp(ax.get_xticklabels(), rotation=90)
    ax.grid(True, color="#999999", lw=0.5, ls=":")
    ax.set_aspect("equal")
    ax.set_xlabel("x (m)"); ax.set_ylabel("z (m)")
    ax.set_title(f"{data['name']}: shortest order {data['shortest_length_m']:.0f} m (64 m grid, z up"
                 + (", x to the left = view from the start" if data.get("flip_x") else "") + ")")
    ax.legend(loc="lower right")
    fig.tight_layout()
    fig.savefig(path)


def plot_svg(path, data, blocks, trace):
    sc = 1.0
    xs = [b["x"] for b in blocks]; zs = [b["z"] for b in blocks]
    x0, z0 = min(xs) - 32, min(zs) - 32
    W, H = max(xs) + 64 - x0, max(zs) + 64 - z0
    X = lambda x: (x - x0) * sc
    Z = lambda z: (H - (z - z0)) * sc
    o = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}"><rect width="100%" height="100%" fill="white"/>']
    for g in range(int(x0 // 64 * 64), int(x0 + W) + 64, 64):
        o.append(f'<line x1="{X(g)}" y1="0" x2="{X(g)}" y2="{H}" stroke="#bbb"/><text x="{X(g)}" y="10" font-size="9">{g}</text>')
    for g in range(int(z0 // 64 * 64), int(z0 + H) + 64, 64):
        o.append(f'<line x1="0" y1="{Z(g)}" x2="{W}" y2="{Z(g)}" stroke="#bbb"/><text x="0" y="{Z(g)}" font-size="9">{g}</text>')
    for b in sorted((b for b in blocks if not GROUND.match(b["name"])), key=lambda b: b["y"]):
        h = int(255 - min(b["y"], 240))
        o.append(f'<rect x="{X(b["x"])}" y="{Z(b["z"] + BW)}" width="{BW}" height="{BW}" fill="rgb({h},{h},{h})" fill-opacity="0.6" stroke="#666"/>')
    pp = [data["start"]["centre"]] + [data["checkpoints"][i]["centre"] for i in data["shortest_order"]] + [data["finish"]["centre"]]
    o.append('<polyline fill="none" stroke="red" stroke-width="2" points="%s"/>' % " ".join(f"{X(p[0])},{Z(p[2])}" for p in pp))
    if trace:
        o.append('<polyline fill="none" stroke="orange" stroke-width="1.5" points="%s"/>' % " ".join(f"{X(p[1])},{Z(p[3])}" for p in trace))
    for c in data["checkpoints"]:
        o.append(f'<circle cx="{X(c["centre"][0])}" cy="{Z(c["centre"][2])}" r="6" fill="blue"/><text x="{X(c["centre"][0]) + 7}" y="{Z(c["centre"][2]) - 7}" fill="blue" font-size="12">CP{c["id"]}</text>')
    for k, col in (("start", "lime"), ("finish", "white")):
        c = data[k]["centre"]
        o.append(f'<rect x="{X(c[0]) - 6}" y="{Z(c[2]) - 6}" width="12" height="12" fill="{col}" stroke="black"/><text x="{X(c[0]) + 8}" y="{Z(c[2])}" font-size="12">{k.upper()}</text>')
    o.append("</svg>")
    open(path, "w").write("\n".join(o))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("replay")
    ap.add_argument("--trace")
    ap.add_argument("--fvlab", help="fvlab.exe (default: env FVLAB_BIN)")
    ap.add_argument("--packs", help="TMUF Packs folder (default: env TM_PACKS)")
    ap.add_argument("--out", default="out/maps")
    ap.add_argument("--route", help="a route of your own to draw: 'label@x,y,z;label@x,y,z;...' (drawn dashed, in order)")
    ap.add_argument("--climb", type=float, default=0.0, help="extra cost per metre of climb (e.g. 5)")
    ap.add_argument("--from", dest="route_from", help="x,y,z where the route starts instead of the start block")
    ap.add_argument("--flip-x", action=argparse.BooleanOptionalAction, default=True,
                    help="draw x growing to the left (matches the in-game top view from the start; default on)")
    a = ap.parse_args()
    global CLIMB
    CLIMB = a.climb
    name = re.sub(r"\.Replay\.Gbx$|\.Gbx$", "", os.path.basename(a.replay))
    blocks, trig = read_blocks(fvlab_exe(a.fvlab), packs_dir(a.packs), a.replay)
    start, finish, multi, cps, rings = classify(blocks, trig)
    if not start or (not finish and not multi):
        sys.exit(f"start/finish not found: start={start} finish={finish} multilap={multi}")
    if not finish:
        finish = multi
    S, F = start[0], finish[0]
    cps = sorted(cps, key=lambda b: (b["x"], b["y"], b["z"]))
    cc = [centre(b) for b in cps]
    R = [float(v) for v in a.route_from.split(",")] if a.route_from else None
    order, total = tour(R or centre(S), centre(F), cc)
    tpath = a.trace or os.path.join("traces", name + ".csv")
    trace = read_trace(tpath) if os.path.exists(tpath) else []
    drv = driven_order(trace, cc) if trace and cps else []
    data = {
        "name": name, "block_size_m": [BW, BH, BW], "route_from": R, "flip_x": a.flip_x,
        "custom_route": [{"label": w.split("@")[0], "p": [float(v) for v in w.split("@")[1].split(",")]}
                         for w in (a.route or "").split(";") if "@" in w],
        "start": {"block": S["name"], "corner": [S["x"], S["y"], S["z"]], "centre": centre(S)},
        "finish": {"block": F["name"], "corner": [F["x"], F["y"], F["z"]], "centre": centre(F)},
        "multilap": [{"block": b["name"], "centre": centre(b)} for b in multi],
        "checkpoints": [{"id": i, "block": b["name"], "corner": [b["x"], b["y"], b["z"]], "centre": cc[i]} for i, b in enumerate(cps)],
        "checkpoint_rings_no_trigger": [{"block": b["name"], "centre": centre(b)} for b in rings],
        "shortest_order": order, "shortest_length_m": total,
        "driven_order": [{"id": i, "tick": t, "min_dist_m": round(dm, 1)} for t, i, dm in drv],
        "blocks": blocks,
    }
    path = [R or centre(S)] + [cc[i] for i in order] + [centre(F)]
    legs = [math.dist(p, q) for p, q in zip(path, path[1:])]
    labels = ["START"] + [f"CP{i}" for i in order] + ["FINISH"]
    print(f"{name}: {len(blocks)} blocks, {len(cps)} checkpoints, {len(rings)} ring blocks without trigger")
    print("shortest order: " + "  ".join(f"{l} -{d:.0f}m->" for l, d in zip(labels, legs)) + " FINISH")
    print(f"shortest total {sum(legs):.1f} m, climb {sum(max(0, q[1] - p[1]) for p, q in zip(path, path[1:])):.0f} m"
          + (f" (cost with --climb {CLIMB}: {total:.0f})" if CLIMB else ""))
    if drv:
        hit = [x for x in drv if x[2] <= PASS_M]
        miss = [x for x in drv if x[2] > PASS_M]
        print(f"driven (trace ends tick {trace[-1][0]}): START -> " + " -> ".join(f"CP{i}(t{t},{dm:.0f}m)" for t, i, dm in hit)
              + "  | not reached (closest m): " + ", ".join(f"CP{i}({dm:.0f})" for _, i, dm in sorted(miss, key=lambda x: x[1])))
        for x in data["driven_order"]:
            x["passed"] = x["min_dist_m"] <= PASS_M
    os.makedirs(a.out, exist_ok=True)
    json.dump(data, open(os.path.join(a.out, name + ".json"), "w"), indent=1)
    try:
        plot_png(os.path.join(a.out, name + ".png"), data, blocks, trace)
        print("wrote", os.path.join(a.out, name + ".png"))
    except ImportError:
        plot_svg(os.path.join(a.out, name + ".svg"), data, blocks, trace)
        print("wrote", os.path.join(a.out, name + ".svg"))


if __name__ == "__main__":
    main()
