# vtalign.py : align the CTrackManiaRace vtables of ESWC and Sunrise slot by slot (sequence alignment on
# instruction-shape similarity), print the Sunrise functions for known ESWC race virtuals with prologue bytes.
import os, re, sys
from difflib import SequenceMatcher
import capstone

sys.argv = [sys.argv[0]]
exec(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "racevt.py")).read().split("\ndef main")[0])

BIG = re.compile(r"0x[0-9a-f]{5,}")


def sig(e, va, n=40):
    out = []
    for ins in e.dis(va, n):
        ops = "" if ins.mnemonic.startswith("j") or ins.mnemonic == "call" else BIG.sub("C", ins.op_str)
        out.append(ins.mnemonic + " " + ops)
        if ins.mnemonic == "ret":
            break
    return out


def sim(a, b):
    if a == b:
        return 1.0
    return SequenceMatcher(None, a, b, autojunk=False).ratio()


known = {0x6d9f50: "OnFinishLine", 0x6d9be0: "OnStartLine", 0x6d9c80: "OnCheckpoint", 0x6d98f0: "RespawnPlayer",
         0x6d9670: "ResetPlayer"}

for name, evt, tvt in [("CTrackManiaRace", 0xaf1b70, 0xa4c7f8), ("CTrackManiaRace1P", 0xaf1390, 0xa4c438)]:
    E, T = Exe("eswc/TmNationsESWC.exe"), Exe("tms/TmSunrise.exe")
    es, ts = slots(E, evt), slots(T, tvt)
    esig = [sig(E, f) for f in es]
    tsig = [sig(T, f) for f in ts]
    n, m = len(es), len(ts)
    GAP = -0.25
    S = [[0.0] * (m + 1) for _ in range(n + 1)]
    B = [[None] * (m + 1) for _ in range(n + 1)]
    for i in range(1, n + 1):
        S[i][0], B[i][0] = S[i - 1][0] + GAP, "u"
    for j in range(1, m + 1):
        S[0][j], B[0][j] = S[0][j - 1] + GAP, "l"
    simc = {}
    for i in range(1, n + 1):
        for j in range(1, m + 1):
            s = sim(esig[i - 1], tsig[j - 1])
            simc[(i - 1, j - 1)] = s
            opts = [(S[i - 1][j - 1] + (s - 0.5) * 2, "d"), (S[i - 1][j] + GAP, "u"), (S[i][j - 1] + GAP, "l")]
            S[i][j], B[i][j] = max(opts)
    pairs = {}
    i, j = n, m
    while i > 0 and j > 0:
        b = B[i][j]
        if b == "d":
            pairs[i - 1] = j - 1
            i, j = i - 1, j - 1
        elif b == "u":
            i -= 1
        else:
            j -= 1
    print("==", name, "ESWC", hex(evt), n, "slots / Sunrise", hex(tvt), m, "slots")
    for k, f in enumerate(es):
        if f in known or k >= 95:
            j = pairs.get(k)
            t = ts[j] if j is not None else None
            print("  ESWC slot %3d %-14s %08x [%s] -> Sunrise slot %s %s [%s] sim %.2f" % (
                k, known.get(f, ""), f, E.prologue(f), j, "%08x" % t if t else "-", T.prologue(t) if t else "",
                simc.get((k, j), 0) if j is not None else 0))
