# mwmethod.py <exe> <name>... : resolve CMwMemberInfo method entries (name ptr x3, type 2, member id, fn ptr)
# in .data and print the method address, optionally disassembling the first N instructions (env DIS=N).
import os, struct, sys
import capstone

exe = sys.argv[1]
d = open(exe, "rb").read()
pe = struct.unpack_from("<I", d, 0x3c)[0]
nsec = struct.unpack_from("<H", d, pe + 6)[0]
optsz = struct.unpack_from("<H", d, pe + 20)[0]
base = struct.unpack_from("<I", d, pe + 24 + 28)[0]
secs = []
for i in range(nsec):
    name, vsize, vaddr, rsize, raw = struct.unpack_from("<8sIIII", d, pe + 24 + optsz + i * 40)
    secs.append((name.rstrip(b"\0").decode(), vaddr, max(vsize, rsize), raw))
off2va = lambda o: next((base + va + o - raw for n, va, sz, raw in secs if raw <= o < raw + sz), None)
va2off = lambda v: next((raw + v - base - va for n, va, sz, raw in secs if base + va <= v < base + va + sz), None)
md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
ndis = int(os.environ.get("DIS", "0"))

for s in sys.argv[2:]:
    o = d.find(s.encode() + b"\0")
    while o != -1 and d[o - 1] != 0:
        o = d.find(s.encode() + b"\0", o + 1)
    if o == -1:
        print("%-45s not found" % s)
        continue
    va = off2va(o)
    pat = struct.pack("<III", va, va, va)
    p = d.find(pat)
    if p == -1:
        print("%-45s str %08x, no member table entry" % (s, va))
        continue
    typ, mid, fn = struct.unpack_from("<III", d, p + 12)
    print("%-45s str %08x  type %d id %05x  fn %08x" % (s, va, typ, mid, fn))
    if ndis and va2off(fn):
        so = va2off(fn)
        for i, ins in enumerate(md.disasm(d[so:so + 600], fn)):
            if i >= ndis:
                break
            print("     %08x  %-7s %s" % (ins.address, ins.mnemonic, ins.op_str))
