# immref.py <exe> <va-hex>... : list .text instructions whose operands contain the given VA (as imm or disp32),
# with the containing function start guessed from the previous int3/nop padding.
import struct, sys
import capstone

exe = sys.argv[1]
d = open(exe, "rb").read()
pe = struct.unpack_from("<I", d, 0x3c)[0]
nsec = struct.unpack_from("<H", d, pe + 6)[0]
optsz = struct.unpack_from("<H", d, pe + 20)[0]
base = struct.unpack_from("<I", d, pe + 24 + 28)[0]
text = None
for i in range(nsec):
    name, vsize, vaddr, rsize, raw = struct.unpack_from("<8sIIII", d, pe + 24 + optsz + i * 40)
    if name.rstrip(b"\0") == b".text":
        text = (vaddr, max(vsize, rsize), raw)
md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
tva, tsz, traw = text
for a in sys.argv[2:]:
    va = int(a, 16)
    pat = struct.pack("<I", va)
    print("== %08x" % va)
    p = d.find(pat, traw, traw + tsz)
    while p != -1:
        rv = base + tva + p - traw
        # find function start: walk back to padding (cc/90 run) boundary
        q = p
        while q > traw and not (d[q - 1] in (0xCC, 0x90) and d[q - 2] in (0xCC, 0x90)):
            q -= 1
            if p - q > 0x4000:
                break
        fstart = base + tva + q - traw
        # disassemble the instruction containing the reference
        best = None
        for back in range(1, 8):
            so = p - back
            ins = next(md.disasm(d[so:so + 16], base + tva + so - traw), None)
            if ins and ins.address + ins.size >= rv + 4 and ins.address <= rv:
                best = ins
                break
        txt = "%s %s" % (best.mnemonic, best.op_str) if best else "?"
        print("  %08x  fn~%08x  %s" % (rv, fstart, txt))
        p = d.find(pat, p + 1, traw + tsz)
