# racevt.py : vtables of CTrackManiaRace / CTrackManiaRace1P in ESWC and Sunrise, found from the class info
# static object (init code) -> MwGetClassInfo (`mov eax, ci; ret`) -> vtable slot that points to it.
# Prints ESWC slot numbers of known race functions and the Sunrise functions at the same slots with prologues.
import json, struct, sys
import capstone

md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)


class Exe:
    def __init__(self, path):
        self.d = open(path, "rb").read()
        pe = struct.unpack_from("<I", self.d, 0x3c)[0]
        n = struct.unpack_from("<H", self.d, pe + 6)[0]
        opt = struct.unpack_from("<H", self.d, pe + 20)[0]
        self.secs = [struct.unpack_from("<8sIIII", self.d, pe + 24 + opt + i * 40) for i in range(n)]

    def v2o(self, v):
        for nm, vs, va, rs, raw in self.secs:
            if 0x400000 + va <= v < 0x400000 + va + rs:
                return raw + v - 0x400000 - va

    def o2v(self, o):
        for nm, vs, va, rs, raw in self.secs:
            if raw <= o < raw + rs:
                return 0x400000 + va + o - raw

    def u32(self, v):
        return struct.unpack_from("<I", self.d, self.v2o(v))[0]

    def is_code(self, v):
        for nm, vs, va, rs, raw in self.secs:
            if nm.rstrip(b"\0") == b".text" and 0x400000 + va <= v < 0x400000 + va + vs:
                return True
        return False

    def dis(self, a, n):
        o = self.v2o(a)
        return list(md.disasm(self.d[o:o + n * 12], a))[:n]

    def prologue(self, a, n=8):
        o = self.v2o(a)
        return self.d[o:o + n].hex(" ")


def ci_object(e, init_va):
    # the init code passes the static CMwClassInfo object (mov ecx, imm / push imm)
    for i in e.dis(init_va, 30):
        if i.mnemonic in ("mov", "push") and i.op_str.startswith(("ecx, 0x", "0x")):
            v = int(i.op_str.split("0x")[-1], 16)
            if 0x800000 < v < 0x1000000 and not e.is_code(v):
                return v
    return None


def vtable_of(e, ci):
    pat = b"\xb8" + struct.pack("<I", ci) + b"\xc3"
    fns = []
    o = e.d.find(pat)
    while o != -1:
        fns.append(e.o2v(o))
        o = e.d.find(pat, o + 1)
    res = []
    for f in fns:
        p = struct.pack("<I", f)
        o = e.d.find(p)
        while o != -1:
            v = e.o2v(o)
            if v and not e.is_code(v):
                # vtable start: walk back while previous entries are code pointers
                start = v
                while e.is_code(e.u32(start - 4)):
                    start -= 4
                res.append((f, v, start))
            o = e.d.find(p, o + 1)
    return res


def slots(e, start, count=140):
    out = []
    for i in range(count):
        f = e.u32(start + 4 * i)
        if not e.is_code(f):
            break
        out.append(f)
    return out


def main():
    games = {
        "eswc": ("eswc/TmNationsESWC.exe", "rmc/out/eswc_classes.json"),
        "tms": ("tms/TmSunrise.exe", "rmc/out/tms_classes.json"),
    }
    known = {0x6d9f50: "OnFinishLine", 0x6d9be0: "OnStartLine", 0x6d9c80: "OnCheckpoint", 0x6d98f0: "RespawnPlayer",
             0x6d9670: "ResetPlayer", 0x6db380: "ButtonQuitOnClick"}
    tables = {}
    for g, (exe, cls) in games.items():
        e = Exe(exe)
        c = json.load(open(cls))
        for key, x in c.items():
            if x["name"] not in ("CTrackManiaRace", "CTrackManiaRace1P"):
                continue
            ci = ci_object(e, int(x["init_va"], 16))
            vts = vtable_of(e, ci) if ci else []
            print(g, x["name"], "classinfo", hex(ci) if ci else None, "vtables", [(hex(f), hex(v), hex(s)) for f, v, s in vts])
            for f, v, s in vts:
                tables[(g, x["name"])] = (e, s, slots(e, s))
    for name in ("CTrackManiaRace", "CTrackManiaRace1P"):
        if ("eswc", name) not in tables or ("tms", name) not in tables:
            continue
        ee, es, eslots = tables[("eswc", name)]
        te, ts, tslots = tables[("tms", name)]
        print("==", name, "ESWC vtable", hex(es), len(eslots), "slots; Sunrise vtable", hex(ts), len(tslots), "slots")
        for i, f in enumerate(eslots):
            if f in known:
                t = tslots[i] if i < len(tslots) else None
                print("  slot %3d %-18s ESWC %08x [%s]  Sunrise %s [%s]" % (
                    i, known[f], f, ee.prologue(f), "%08x" % t if t else "-", te.prologue(t) if t else ""))


main()
