# trick_catalog.py - markdown catalogue of tricks from the JSONs of find_tricks.py. Input: find_tricks output folder. MIT licence, see LICENSE-MIT.
"""Builds the trick catalogue (markdown) from the JSONs of find_tricks.py.

usage: python scripts/tas/trick_catalog.py out/tricks [--summary <dir>/replays_summary.json] > catalog.md
"""
import argparse, glob, json, os, statistics as st


def q(vals, digits=0):
    vals = sorted(vals)
    if not vals:
        return "–"
    if len(vals) < 4:
        return " / ".join(f"{v:.{digits}f}" for v in vals)
    qs = st.quantiles(vals, n=4)
    return f"{vals[0]:.{digits}f} · {qs[0]:.{digits}f} · **{qs[1]:.{digits}f}** · {qs[2]:.{digits}f} · {vals[-1]:.{digits}f}"


def ref(code, t):
    return f"{code} {t:.2f}"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("dir")
    ap.add_argument("--summary")
    a = ap.parse_args()
    meta = {}
    if a.summary:
        meta = {s["code"]: s for s in json.load(open(a.summary, encoding="utf-8"))}
    maps = {}
    for f in sorted(glob.glob(os.path.join(a.dir, "*.json"))):
        maps[os.path.basename(f)[:-5]] = json.load(open(f, encoding="utf-8"))

    boosts, ubers, noses, ramms, chains, flicks, coast, entries = [], [], [], [], [], [], [], {"spin kick": [], "landing": [], "hit/bounce": [], "other": []}
    for code, e in maps.items():
        for k in e["kicks"]:
            if k["end"]["kmh"] > 1005 or k["ms"] > 400:
                continue  # crash spike or slow gain, not a kick
            if k["on_nose"]:
                boosts.append((code, k))
            elif k["dkmh"] >= 120:
                ubers.append((code, k))
        for p in e["nose"]:
            noses.append((code, p))
            t0 = p["start"]["t"]
            r = next((r for r in e["ramm"] if r["nose_after_ms"] is not None and abs(r["at"]["t"] + r["nose_after_ms"] / 1000 - t0) < 0.02), None)
            fl = next((f for f in e["flights"] if 0 <= t0 - f["end"]["t"] <= 0.4), None)
            hit = next((h for h in e["hits"] if 0 <= t0 - h["end"]["t"] <= 0.4), None)
            kind = "spin kick" if r else "landing" if fl else "hit/bounce" if hit else "other"
            entries[kind].append((code, p, r or fl or hit))
            if p["boosts"] >= 3:
                chains.append((code, p))
        for r in e["ramm"]:
            if r["nose_after_ms"] is not None:
                ramms.append((code, r))
        for t in e["turns"]:
            if not t.get("on_nose") and t["at"]["kmh"] >= 500 and abs(t["deg"]) >= 20 and t["at"]["kmh"] <= 1005:
                flicks.append((code, t))
        for f in e["flights"]:
            if f["gas"] < 0.7 and f["ms"] >= 500 and f["start"]["kmh"] >= 500:
                coast.append((code, f))

    out = []
    w = out.append
    w("# Trick catalogue\n")
    w("Built by `scripts/tas/find_tricks.py` + `scripts/tas/trick_catalog.py` from the simulation of each replay (all replayed")
    w("exactly). Times in s, speeds in km/h, pitch > 0 = nose down, contacts FL FR RL RR. Quartiles are shown as")
    w("min · 25 % · **median** · 75 % · max. Event finder thresholds are in `find_tricks.py`; crash ticks above")
    w("1005 km/h are left out. Each example is `track time` – look at it with `fvlab … probe`.\n")

    w("## Per track\n")
    w("| track | TAS | noseboosts | nose phases (chains) | spin kick → nose | ubers | flicks | hits |")
    w("|---|---|---|---|---|---|---|---|")
    for code, e in maps.items():
        m = meta.get(code, {})
        nb = sum(1 for c, _ in boosts if c == code)
        ch = sum(1 for c, _ in chains if c == code)
        w(f"| {code} | {m.get('time_ms', e['meta']['end_ms']) / 1000:.2f} {m.get('author', '')} | {nb} | {len(e['nose'])} ({ch}) | "
          f"{sum(1 for c, _ in ramms if c == code)} | {sum(1 for c, _ in ubers if c == code)} | {sum(1 for c, _ in flicks if c == code)} | {len(e['hits'])} |")

    w("\n## How the TAS get into a nose position\n")
    w("Each nose phase (≥ 0.1 s with |pitch| ≥ 65° and a front wheel on the ground) by what came right before it:\n")
    w("| entry | count | entry speed | examples |")
    w("|---|---|---|---|")
    for kind, lst in entries.items():
        ex = ", ".join(ref(c, p["start"]["t"]) for c, p, _ in sorted(lst, key=lambda x: -x[1]["gain"])[:8])
        w(f"| {kind} | {len(lst)} | {q([p['start']['kmh'] for _, p, _ in lst])} | {ex} |")

    w("\n### Spin kick → nose (angular velocity jumps ≥ 2 rad/s in one tick without a speed change, then a nose position)\n")
    w("A spin kick comes from a contact inside the tick: a rammstein (wheel on an edge, as in D-0 3.85), a body or")
    w("wheel hitting an edge, or a landing on one wheel. `probe` cannot tell them apart (no contact normals); check")
    w("single cases with a trace (`FV_TRACE`, tools/fvlab/README.md).\n")
    w(f"{len(ramms)} cases. Speed {q([r['at']['kmh'] for _, r in ramms])}; time from the kick to the nose position "
      f"{q([r['nose_after_ms'] for _, r in ramms])} ms; spin jump {q([r['dspin'] for _, r in ramms], 1)} rad/s.")
    w(f"Inputs in the 0.2 s before the kick: gas {q([r['inputs']['gas'] for _, r in ramms], 2)}, brake {q([r['inputs']['brake'] for _, r in ramms], 2)}, "
      f"|steer| {q([r['inputs']['steer_abs'] for _, r in ramms], 2)}.\n")
    w("| example | km/h | pose before → at the kick | nose after | inputs before |")
    w("|---|---|---|---|---|")
    for c, r in sorted(ramms, key=lambda x: x[1]["nose_after_ms"])[:15]:
        b, at = r["before"], r["at"]
        w(f"| {ref(c, at['t'])} | {at['kmh']} | p {b['pitch']} r {b['roll']} `{b['contacts']}` → `{at['contacts']}` | {r['nose_after_ms']} ms | "
          f"gas {r['inputs']['gas']} brake {r['inputs']['brake']} steer {r['inputs']['steer_abs']} |")

    w("\n## Noseboosts (kick ≥ 25 km/h within 30 ms while on the nose)\n")
    w(f"{len(boosts)} kicks. Speed before {q([k['start']['kmh'] for _, k in boosts])}; gain {q([k['dkmh'] for _, k in boosts])}; "
      f"pitch at the start {q([abs(k['start']['pitch']) for _, k in boosts], 1)}°; roll {q([abs(k['start']['roll']) for _, k in boosts], 1)}°.")
    w(f"Inputs in the 0.3 s before: gas {q([k['inputs']['gas'] for _, k in boosts], 2)}, brake {q([k['inputs']['brake'] for _, k in boosts], 2)}, "
      f"steer sign flips {q([k['inputs']['steer_flips_per_s'] for _, k in boosts])} /s, brake toggles {q([k['inputs']['brake_toggles_per_s'] for _, k in boosts])} /s.")
    lo = sorted(boosts, key=lambda x: x[1]["start"]["kmh"])[:10]
    w("\nLowest entry speeds (the floor of the trick): " + ", ".join(f"{ref(c, k['start']['t'])} ({k['start']['kmh']} → {k['end']['kmh']})" for c, k in lo) + ".")
    big = sorted(boosts, key=lambda x: -x[1]["dkmh"])[:10]
    w("Biggest single kicks: " + ", ".join(f"{ref(c, k['start']['t'])} ({k['start']['kmh']} → {k['end']['kmh']})" for c, k in big) + ".")

    w("\n## Nose phases\n")
    w(f"{len(noses)} phases. Duration {q([p['ms'] for _, p in noses])} ms; entry speed {q([p['start']['kmh'] for _, p in noses])}; "
      f"gain {q([p['gain'] for _, p in noses])}; spin max {q([p['spin_max'] for _, p in noses], 1)} rad/s; heading change {q([abs(p['turn_deg']) for _, p in noses])}°.")
    w(f"Inputs on the nose: gas {q([p['inputs']['gas'] for _, p in noses], 2)}, brake {q([p['inputs']['brake'] for _, p in noses], 2)}, "
      f"steer flips {q([p['inputs']['steer_flips_per_s'] for _, p in noses])} /s, brake toggles {q([p['inputs']['brake_toggles_per_s'] for _, p in noses])} /s.")
    w("Inputs in the 0.5 s before the nose position: gas "
      f"{q([p['before']['gas'] for _, p in noses], 2)}, brake {q([p['before']['brake'] for _, p in noses], 2)}, |steer| {q([p['before']['steer_abs'] for _, p in noses], 2)}.")

    w("\n### Nose chains (≥ 3 boosts in one nose phase)\n")
    w("| example | duration | km/h in → max | boosts | spin max | heading change | gas / brake | steer flips /s |")
    w("|---|---|---|---|---|---|---|---|")
    for c, p in sorted(chains, key=lambda x: -x[1]["ms"])[:20]:
        w(f"| {ref(c, p['start']['t'])}–{p['end']['t']:.2f} | {p['ms']} ms | {p['start']['kmh']} → {p['vmax']} | {p['boosts']} | {p['spin_max']} | "
          f"{p['turn_deg']}° | {p['inputs']['gas']} / {p['inputs']['brake']} | {p['inputs']['steer_flips_per_s']} |")

    w("\n## Ubers and other kicks off the nose (≥ 120 km/h)\n")
    w(f"{len(ubers)} kicks. Speed before {q([k['start']['kmh'] for _, k in ubers])}; gain {q([k['dkmh'] for _, k in ubers])}.\n")
    w("| example | km/h | contacts before → after | pitch / roll before | inputs before |")
    w("|---|---|---|---|---|")
    for c, k in sorted(ubers, key=lambda x: -x[1]["dkmh"])[:15]:
        w(f"| {ref(c, k['start']['t'])} | {k['start']['kmh']} → {k['end']['kmh']} | `{k['start']['contacts']}` → `{k['end']['contacts']}` | "
          f"{k['start']['pitch']} / {k['start']['roll']} | gas {k['inputs']['gas']} brake {k['inputs']['brake']} steer {k['inputs']['steer_abs']} |")

    w("\n## Flicks and sharp turns at ≥ 500 km/h (off the nose, ≥ 20° within 50 ms)\n")
    w(f"{len(flicks)} turns. Turn {q([abs(t['deg']) for _, t in flicks])}°; speed change {q([t['dkmh'] for _, t in flicks])}.\n")
    w("| example | km/h | turn | speed change | contacts during | inputs |")
    w("|---|---|---|---|---|---|")
    for c, t in sorted(flicks, key=lambda x: -abs(x[1]["deg"]))[:15]:
        w(f"| {ref(c, t['at']['t'])} | {t['at']['kmh']} | {t['deg']}° | {t['dkmh']} | {' '.join(t['contacts'])} | "
          f"gas {t['inputs']['gas']} brake {t['inputs']['brake']} steer {t['inputs']['steer_abs']} |")

    w("\n## Flights ≥ 0.5 s at ≥ 500 km/h with gas mostly off\n")
    w(f"{len(coast)} flights (coast drag costs speed, see `docs/bugs/coast-drag.md`; the TAS release gas mostly to")
    w("steer the pose with air brake). Examples: " + ", ".join(f"{ref(c, f['start']['t'])} ({f['start']['kmh']} → {f['end']['kmh']}, gas {f['gas']})" for c, f in coast[:10]) + ".")
    print("\n".join(out))


if __name__ == "__main__":
    main()
