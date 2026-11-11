# find_tricks.py - runs `fvlab probe` over whole replays and lists boosts, nose phases, ramm kicks, hits, flicks, flights, CPs. Input: replays. MIT licence, see LICENSE-MIT.
"""Trick finder: runs `fvlab probe` over whole replays and lists boosts, nose phases, nose chains, ramm kicks,
hits, sharp turns (flicks), long flights and checkpoint events, each with speed, pose and the inputs around it.

usage: python scripts/tas/find_tricks.py REPLAY.Gbx [REPLAY.Gbx ...] [--ms TOTAL_MS] [--out out/tricks] [--jobs 11]
       python scripts/tas/find_tricks.py --summary <dir>/replays_summary.json --dir <dir>/replays
                                     [--codes A,B,C,D] [--out out/tricks] [--jobs 11]
Writes one JSON per replay into --out and prints one line per replay. `trick_catalog.py` turns the JSONs into
the catalogue. The summary JSON is a list of {code, time_ms}; replays are <dir>/<code>.Replay.Gbx.
Contacts in `probe` are 4 bits FL FR RL RR (1 = wheel touches); pitch > 0 = nose down.
"""
import argparse, json, math, os, subprocess, sys
from concurrent.futures import ThreadPoolExecutor

from fvlab_env import fvlab_exe, packs_dir

FVLAB = None  # set in main()
PACKS = None

NOSE_PITCH = 65.0      # |pitch| from which the car counts as standing on its nose
NOSE_GAP = 8           # ticks without front contact allowed inside one nose phase
KICK_KMH = 25.0        # speed gain within 3 ticks that counts as a kick (normal driving gains < 3 km/h per tick)
HIT_KMH = 40.0         # speed loss within 3 ticks that counts as a hit
SPIN_JUMP = 2.0        # rad/s change of the angular velocity in one tick without a big speed change: ramm kick
TURN_DEG = 12.0        # heading change within 5 ticks at >= 300 km/h: sharp turn / flick
AIR_MIN = 50           # ticks without contact for a flight


def probe(replay, ms):
    env = dict(os.environ, FVLAB_OPTIMIZED="1", FV_IGNORE_GHOST="1")
    for k in [k for k in env if k.startswith("FVLAB_") and k != "FVLAB_OPTIMIZED"]:
        del env[k]
    ticks = ms // 10 + 60
    r = subprocess.run([FVLAB, PACKS, replay, "probe", "0", str(ticks)], capture_output=True, text=True, env=env)
    rows = []
    for line in r.stdout.splitlines():
        p = line.split(",")
        if len(p) < 15 or not p[0].strip().lstrip("-").isdigit():
            continue
        t, kmh, x, y, z, pitch, roll, wx, wy, wz = (float(v) for v in p[:10])
        c = p[10].strip()
        rows.append(dict(t=int(t), v=kmh, x=x, y=y, z=z, pitch=pitch, roll=roll, w=(wx, wy, wz),
                         c=c, gas=float(p[11]), brake=float(p[12]), steer=float(p[13]), cps=int(float(p[-1]))))
    return rows


def heading(a, b):
    return math.degrees(math.atan2(b["x"] - a["x"], b["z"] - a["z"]))


def dang(a, b):
    return (b - a + 180) % 360 - 180


def inputs(rows, i0, i1):
    seg = rows[max(0, i0):max(i0 + 1, i1)]
    n = len(seg)
    flips = sum(1 for a, b in zip(seg, seg[1:]) if a["steer"] * b["steer"] < 0)
    btog = sum(1 for a, b in zip(seg, seg[1:]) if (a["brake"] > 0.5) != (b["brake"] > 0.5))
    return dict(gas=round(sum(r["gas"] > 0.5 for r in seg) / n, 2), brake=round(sum(r["brake"] > 0.5 for r in seg) / n, 2),
                steer_abs=round(sum(abs(r["steer"]) for r in seg) / n, 2), steer_flips_per_s=round(flips * 100 / n, 1),
                brake_toggles_per_s=round(btog * 100 / n, 1))


def pose(r):
    return dict(t=r["t"] / 1000, kmh=round(r["v"]), pos=[round(r["x"], 1), round(r["y"], 1), round(r["z"], 1)],
                pitch=round(r["pitch"], 1), roll=round(r["roll"], 1), contacts=r["c"],
                spin=round(math.sqrt(sum(w * w for w in r["w"])), 1))


def front(r):
    return r["c"][:2] != "00"


def analyse(rows):
    ev = {"kicks": [], "nose": [], "ramm": [], "hits": [], "turns": [], "flights": [], "cps": []}
    n = len(rows)
    # kicks and hits: 3-tick speed change, merged into one event per burst
    i = 3
    while i < n:
        d = rows[i]["v"] - rows[i - 3]["v"]
        if d >= KICK_KMH or d <= -HIT_KMH:
            j = i
            sign = 1 if d > 0 else -1
            while j + 1 < n and sign * (rows[j + 1]["v"] - rows[j - 2]["v"]) >= (KICK_KMH if sign > 0 else HIT_KMH) * 0.5:
                j += 1
            a, b = rows[i - 3], rows[j]
            e = dict(start=pose(a), end=pose(b), dkmh=round(b["v"] - a["v"]), ms=b["t"] - a["t"],
                     inputs=inputs(rows, i - 30, j + 1))
            e["on_nose"] = abs(a["pitch"]) >= NOSE_PITCH or abs(b["pitch"]) >= NOSE_PITCH
            (ev["kicks"] if sign > 0 else ev["hits"]).append(e)
            i = j + 3
        else:
            i += 1
    # nose phases
    i = 0
    while i < n:
        r = rows[i]
        if abs(r["pitch"]) >= NOSE_PITCH and front(r):
            j, last = i, i
            while j + 1 < n and abs(rows[j + 1]["pitch"]) >= NOSE_PITCH - 10 and j + 1 - last <= NOSE_GAP:
                j += 1
                if front(rows[j]):
                    last = j
            j = last
            if j - i >= 10:
                seg = rows[i:j + 1]
                boosts = [k for k in ev["kicks"] if seg[0]["t"] <= k["start"]["t"] * 1000 <= seg[-1]["t"] + 100]
                h0 = heading(rows[max(0, i - 3)], rows[i])
                h1 = heading(rows[j], rows[min(n - 1, j + 3)])
                ev["nose"].append(dict(start=pose(seg[0]), end=pose(seg[-1]), ms=seg[-1]["t"] - seg[0]["t"],
                                       vmax=round(max(s["v"] for s in seg)), gain=round(max(s["v"] for s in seg) - seg[0]["v"]),
                                       pitch=[round(min(abs(s["pitch"]) for s in seg), 1), round(max(abs(s["pitch"]) for s in seg), 1)],
                                       roll=[round(min(s["roll"] for s in seg), 1), round(max(s["roll"] for s in seg), 1)],
                                       spin_max=round(max(math.sqrt(sum(w * w for w in s["w"])) for s in seg), 1),
                                       boosts=len(boosts), turn_deg=round(dang(h0, h1)),
                                       inputs=inputs(rows, i, j + 1), before=inputs(rows, i - 50, i)))
            i = j + 1
        else:
            i += 1
    # ramm kicks: angular velocity jump without a big speed change
    for i in range(1, n):
        a, b = rows[i - 1], rows[i]
        dw = math.sqrt(sum((p - q) ** 2 for p, q in zip(a["w"], b["w"])))
        if dw >= SPIN_JUMP and abs(b["v"] - a["v"]) < 15 and b["v"] > 100 and abs(a["pitch"]) < 45:
            nxt = next((p for p in ev["nose"] if 0 <= p["start"]["t"] * 1000 - b["t"] <= 1500), None)
            if ev["ramm"] and b["t"] - ev["ramm"][-1]["at"]["t"] * 1000 < 200:
                continue
            ev["ramm"].append(dict(at=pose(b), before=pose(a), dspin=round(dw, 1), inputs=inputs(rows, i - 20, i + 1),
                                   nose_after_ms=None if not nxt else round(nxt["start"]["t"] * 1000 - b["t"])))
    # sharp turns at speed (flicks, border bounces, nose turns)
    i = 5
    while i < n - 1:
        a, b = rows[i - 5], rows[i]
        if b["v"] >= 300 and abs(dang(heading(rows[i - 6], a), heading(rows[i - 1], b))) >= TURN_DEG:
            d = dang(heading(rows[i - 6], a), heading(rows[i - 1], b))
            ev["turns"].append(dict(at=pose(b), deg=round(d), dkmh=round(b["v"] - a["v"]), on_nose=abs(b["pitch"]) >= NOSE_PITCH - 10, contacts=[r["c"] for r in rows[i - 5:i + 1]],
                                    inputs=inputs(rows, i - 10, i + 1)))
            i += 10
        else:
            i += 1
    # flights
    i = 0
    while i < n:
        if rows[i]["c"] == "0000":
            j = i
            while j + 1 < n and rows[j + 1]["c"] == "0000":
                j += 1
            if j - i >= AIR_MIN:
                seg = rows[i:j + 1]
                ev["flights"].append(dict(start=pose(seg[0]), end=pose(seg[-1]), ms=seg[-1]["t"] - seg[0]["t"],
                                          gas=round(sum(s["gas"] > 0.5 for s in seg) / len(seg), 2),
                                          ymax=round(max(s["y"] for s in seg), 1)))
            i = j + 1
        else:
            i += 1
    # checkpoints
    for i in range(1, n):
        if rows[i]["cps"] > rows[i - 1]["cps"]:
            ev["cps"].append(dict(n=rows[i]["cps"], at=pose(rows[i])))
    return ev


def run_one(job):
    name, path, ms, out = job
    rows = probe(path, ms)
    if not rows:
        return name, None
    ev = analyse(rows)
    ev["meta"] = dict(replay=path.replace("\\", "/"), ticks=len(rows), end_ms=rows[-1]["t"], vmax=round(max(r["v"] for r in rows)))
    with open(os.path.join(out, name + ".json"), "w", encoding="utf-8") as f:
        json.dump(ev, f, indent=1)
    return name, ev


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("replays", nargs="*")
    ap.add_argument("--ms", type=int, default=60000)
    ap.add_argument("--summary")
    ap.add_argument("--dir")
    ap.add_argument("--codes", default="A,B,C,D")
    ap.add_argument("--out", default="out/tricks")
    ap.add_argument("--jobs", type=int, default=max(1, (os.cpu_count() or 2) - 1))
    ap.add_argument("--fvlab", help="fvlab.exe (default: env FVLAB_BIN)")
    ap.add_argument("--packs", help="TMUF Packs folder (default: env TM_PACKS)")
    a = ap.parse_args()
    global FVLAB, PACKS
    FVLAB, PACKS = fvlab_exe(a.fvlab), packs_dir(a.packs)
    os.makedirs(a.out, exist_ok=True)
    jobs = []
    if a.summary:
        for s in json.load(open(a.summary, encoding="utf-8")):
            if s["code"][0] in a.codes.split(","):
                jobs.append((s["code"], os.path.join(a.dir, s["code"] + ".Replay.Gbx"), s["time_ms"], a.out))
    for p in a.replays:
        jobs.append((os.path.basename(p).split(".")[0], p, a.ms, a.out))
    with ThreadPoolExecutor(a.jobs) as ex:
        for name, ev in ex.map(run_one, jobs):
            if ev is None:
                print(name, "probe failed", flush=True)
                continue
            nb = sum(1 for k in ev["kicks"] if k["on_nose"])
            print(f"{name}: vmax {ev['meta']['vmax']}, kicks {len(ev['kicks'])} (nose {nb}), nose phases {len(ev['nose'])}, "
                  f"ramm {len(ev['ramm'])} (->nose {sum(1 for r in ev['ramm'] if r['nose_after_ms'] is not None)}), "
                  f"hits {len(ev['hits'])}, turns {len(ev['turns'])}, flights {len(ev['flights'])}, cps {len(ev['cps'])}", flush=True)


if __name__ == "__main__":
    main()
