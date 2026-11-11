# classinfo.py <exe> [out.json] : static dump of Nadeo CMwClassInfo param tables (32-bit TM exes).
# __InitClassInfo_X(CMwClassInfo&) stores the class's SMwParamInfo* array at classinfo+0x20 and its count at
# +0x24 (TMNF: `mov [reg+20h], imm32; mov [reg+24h], imm32`; ESWC/Sunrise load the count from a variable).
# We take every `mov dword ptr [reg+20h], imm32` whose imm32 is an array of pointers to SMwParamInfo entries
# and walk the array while the entries look valid. SMwParamInfo (0x28+ bytes): +00 type, +04 member id
# (class id | index), +0c field offset or -1, +10 name. Class names: the string pushed next to the class id
# in the class registration code, when it can be found.
import json, re, struct, sys
from collections import Counter

exe = sys.argv[1]
d = open(exe, "rb").read()
pe = struct.unpack_from("<I", d, 0x3c)[0]
nsec = struct.unpack_from("<H", d, pe + 6)[0]
optsz = struct.unpack_from("<H", d, pe + 20)[0]
base = struct.unpack_from("<I", d, pe + 24 + 28)[0]
secs = []
for i in range(nsec):
    name, vsize, vaddr, rsize, raw = struct.unpack_from("<8sIIII", d, pe + 24 + optsz + i * 40)
    secs.append((name.rstrip(b"\0").decode(), vaddr, rsize, raw))


def v2o(v):
    for n, va, rsz, raw in secs:
        if base + va <= v < base + va + rsz:
            return raw + v - base - va
    return None


def o2v(o):
    for n, va, rsz, raw in secs:
        if raw <= o < raw + rsz:
            return base + va + o - raw
    return None


IDENT = re.compile(rb"^[A-Za-z_][A-Za-z0-9_]{0,95}$")


def ident(v):
    o = v2o(v)
    if o is None:
        return None
    e = d.find(b"\0", o, o + 98)
    if e == -1 or not IDENT.match(d[o:e]):
        return None
    return d[o:e].decode()


def u32(v):
    o = v2o(v)
    return None if o is None or o + 4 > len(d) else struct.unpack_from("<I", d, o)[0]


def param(v):
    o = v2o(v)
    if o is None or o + 0x28 > len(d):
        return None
    w = struct.unpack_from("<10I", d, o)
    nm = ident(w[4])
    if not nm or w[1] == 0 or w[1] == 0xFFFFFFFF:
        return None
    return {"type": w[0], "id": "%08x" % w[1], "offset": None if w[3] == 0xFFFFFFFF else w[3], "name": nm,
            "w": ["%08x" % x for x in w]}


classes = {}
for m in re.finditer(rb"\xc7[\x40-\x47]\x20(....)", d, re.S):
    arr = struct.unpack("<I", m.group(1))[0]
    if v2o(arr) is None:
        continue
    params = []
    i = 0
    while i < 4096:
        p = u32(arr + 4 * i)
        if p is None:
            break
        info = param(p)
        if not info:
            break
        params.append(info)
        i += 1
    if not params:
        continue
    cid = int(params[0]["id"], 16) & 0xFFFFF000
    if any(int(p["id"], 16) & 0xFFFFF000 != cid for p in params):
        # tables can hold only the class's own params; skip mixed hits
        continue
    classes["%08x" % cid] = {"init_va": "%08x" % o2v(m.start()), "params_va": "%08x" % arr, "params": params}

# class names: look for `push imm32(class id)` near a `push imm32(identifier string)`
names = {}
for cid in classes:
    pat = b"\x68" + struct.pack("<I", int(cid, 16))
    for mm in re.finditer(re.escape(pat), d):
        win = d[max(0, mm.start() - 24): mm.start() + 24]
        for k in range(len(win) - 4):
            if win[k] == 0x68:
                s = ident(struct.unpack_from("<I", win, k + 1)[0])
                if s and (s.startswith("C") or s.startswith("S")) and len(s) > 3:
                    names.setdefault(cid, s)
for cid, c in classes.items():
    c["name"] = names.get(cid)

total = sum(len(c["params"]) for c in classes.values())
print("%d classes (%d named), %d params" % (len(classes), sum(1 for c in classes.values() if c["name"]), total))
print("param types:", Counter(p["type"] for c in classes.values() for p in c["params"]).most_common(12))
if len(sys.argv) > 2:
    json.dump(classes, open(sys.argv[2], "w"), indent=1)
