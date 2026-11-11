# find vtables by slot pattern
import sys, struct, pefile
pe = pefile.PE(sys.argv[1], fast_load=True)
data = open(sys.argv[1], 'rb').read()
base = pe.OPTIONAL_HEADER.ImageBase
def va2off(va):
    rva = va - base
    for s in pe.sections:
        if s.VirtualAddress <= rva < s.VirtualAddress + s.SizeOfRawData:
            return s.PointerToRawData + rva - s.VirtualAddress
text = [s for s in pe.sections if s.Name.startswith(b'.text')][0]
tlo, thi = base + text.VirtualAddress, base + text.VirtualAddress + text.Misc_VirtualSize
rd = [s for s in pe.sections if s.Name.startswith(b'.rdata')][0]
o0 = rd.PointerToRawData
slots = [int(x, 16) for x in sys.argv[2:]]
for o in range(o0, o0 + rd.SizeOfRawData - 0x200, 4):
    ok = True
    tgts = []
    for sl in slots:
        v = struct.unpack_from('<I', data, o + sl)[0]
        if not (tlo <= v < thi):
            ok = False; break
        b = data[va2off(v):va2off(v) + 2]
        if b != b'\xc7\x81':
            ok = False; break
        tgts.append(v)
    if ok:
        # vtable start heuristic: previous dword not code pointer
        va = base + rd.VirtualAddress + (o - o0)
        print('%08X' % va, ' '.join('%08X' % t for t in tgts))
