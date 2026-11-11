# callers.py <exe> <target-va-hex>... : every direct `call target` in .text with the 4 preceding instructions.
import struct, sys
import capstone

d = open(sys.argv[1], "rb").read()
pe = struct.unpack_from("<I", d, 0x3c)[0]
nsec = struct.unpack_from("<H", d, pe + 6)[0]
optsz = struct.unpack_from("<H", d, pe + 20)[0]
base = struct.unpack_from("<I", d, pe + 24 + 28)[0]
for i in range(nsec):
    name, vsize, vaddr, rsize, raw = struct.unpack_from("<8sIIII", d, pe + 24 + optsz + i * 40)
    if name.rstrip(b"\0") == b".text":
        tva, tsz, traw = vaddr, max(vsize, rsize), raw
md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
for a in sys.argv[2:]:
    t = int(a, 16)
    print("== callers of %08x" % t)
    for o in range(traw, traw + tsz - 5):
        if d[o] != 0xE8:
            continue
        va = base + tva + o - traw
        if (va + 5 + struct.unpack_from("<i", d, o + 1)[0]) & 0xFFFFFFFF != t:
            continue
        # disassemble a window before the call and keep the last 4 instructions that end exactly at the call
        for back in range(24, 8, -1):
            ins = list(md.disasm(d[o - back:o + 5], va - back))
            if ins and ins[-1].address == va:
                print("  " + " | ".join("%08x %s %s" % (x.address, x.mnemonic, x.op_str) for x in ins[-5:]))
                break
