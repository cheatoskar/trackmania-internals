# fnmatch.py <src.exe> <dst.exe> <src-va-hex>... : find functions in dst that look like the given src functions.
# Signature = mnemonic + operand "shape" (registers/sizes kept, constants >= 0x10000 and branch targets dropped)
# of the first N instructions (env N, default 30). Prints the best matches by similarity.
import os, re, struct, sys
import capstone
from difflib import SequenceMatcher

N = int(os.environ.get("N", "30"))


def load(path):
    d = open(path, "rb").read()
    pe = struct.unpack_from("<I", d, 0x3c)[0]
    nsec = struct.unpack_from("<H", d, pe + 6)[0]
    optsz = struct.unpack_from("<H", d, pe + 20)[0]
    base = struct.unpack_from("<I", d, pe + 24 + 28)[0]
    for i in range(nsec):
        name, vsize, vaddr, rsize, raw = struct.unpack_from("<8sIIII", d, pe + 24 + optsz + i * 40)
        if name.rstrip(b"\0") == b".text":
            return d, base, vaddr, max(vsize, rsize), raw


md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
BIG = re.compile(r"0x[0-9a-f]{5,}")


def sig(d, base, tva, traw, va, n=N):
    off = traw + va - base - tva
    out = []
    for ins in md.disasm(d[off:off + n * 15], va):
        ops = ins.op_str
        if ins.mnemonic.startswith("j") or ins.mnemonic == "call":
            ops = ""
        ops = BIG.sub("C", ops)
        out.append(ins.mnemonic + " " + ops)
        if len(out) >= n or ins.mnemonic == "ret":
            break
    return out


sd, sbase, stva, stsz, straw = load(sys.argv[1])
dd, dbase, dtva, dtsz, dtraw = load(sys.argv[2])
targets = [int(a, 16) for a in sys.argv[3:]]
tsigs = {t: sig(sd, sbase, stva, straw, t) for t in targets}

# candidate function starts in dst: 16-aligned addresses preceded by padding, plus call targets
text = dd[dtraw:dtraw + dtsz]
starts = set()
for i in range(16, len(text), 16):
    if text[i - 1] in (0xCC, 0x90) and text[i] not in (0xCC, 0x90):
        starts.add(dbase + dtva + i)
best = {t: [] for t in targets}
for s in starts:
    ds = sig(dd, dbase, dtva, dtraw, s)
    if len(ds) < 4:
        continue
    for t in targets:
        ts = tsigs[t]
        if abs(len(ds) - len(ts)) > 10:
            continue
        r = SequenceMatcher(None, ts, ds, autojunk=False).ratio()
        if r > 0.6:
            best[t].append((r, s))
for t in targets:
    top = sorted(best[t], reverse=True)[:5]
    print("%08x -> " % t + ", ".join("%08x (%.2f)" % (s, r) for r, s in top))
