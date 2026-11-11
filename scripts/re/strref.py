# strref.py <exe> <string>... : for each ASCII string, print its VA and every code/data reference
# (imm32 / pointer occurrences of that VA) with a short disassembly around code references.
import struct, sys
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


def off2va(o):
    for n, va, sz, raw in secs:
        if raw <= o < raw + sz:
            return base + va + o - raw
    return None


def va2off(v):
    for n, va, sz, raw in secs:
        if base + va <= v < base + va + sz:
            return raw + v - base - va
    return None


def secname(v):
    for n, va, sz, raw in secs:
        if base + va <= v < base + va + sz:
            return n
    return "?"


md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
for s in sys.argv[2:]:
    needle = s.encode() + b"\0"
    o = d.find(needle)
    while o != -1 and d[o - 1] != 0:
        o = d.find(needle, o + 1)
    if o == -1:
        print("== %s: not found" % s)
        continue
    va = off2va(o)
    print("== %s @ %08x" % (s, va))
    pat = struct.pack("<I", va)
    p = d.find(pat)
    while p != -1:
        rv = off2va(p)
        print("  ref at %08x (%s)" % (rv, secname(rv)))
        if secname(rv) == ".text":
            start = rv - 24
            so = va2off(start)
            for ins in md.disasm(d[so:so + 80], start):
                if ins.address > rv + 40:
                    break
                print("     %08x  %-7s %s" % (ins.address, ins.mnemonic, ins.op_str))
        else:
            # data table: dump neighbouring dwords
            words = struct.unpack_from("<8I", d, p - 12)
            print("     table: " + " ".join("%08x" % w for w in words))
        p = d.find(pat, p + 1)
