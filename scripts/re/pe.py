# PE helper (pefile + capstone): find dword, hex dump, disassemble, string at VA
"""Small PE helpers: python -I pe.py EXE cmd args
cmds: dw VALUE        - find dword occurrences of VALUE (hex)
      dump VA N        - hex dump N bytes at VA
      str VA           - ascii string at VA
"""
import sys, struct, pefile
pe = pefile.PE(sys.argv[1], fast_load=True)
data = open(sys.argv[1], 'rb').read()
base = pe.OPTIONAL_HEADER.ImageBase
def off2va(o):
    for s in pe.sections:
        if s.PointerToRawData <= o < s.PointerToRawData + s.SizeOfRawData:
            return base + s.VirtualAddress + (o - s.PointerToRawData)
def va2off(va):
    rva = va - base
    for s in pe.sections:
        if s.VirtualAddress <= rva < s.VirtualAddress + s.SizeOfRawData:
            return s.PointerToRawData + rva - s.VirtualAddress
cmd = sys.argv[2]
if cmd == 'dw':
    for v in sys.argv[3:]:
        pat = struct.pack('<I', int(v, 16))
        i = data.find(pat)
        while i >= 0:
            print('%s at %08X' % (v, off2va(i)))
            i = data.find(pat, i + 1)
elif cmd == 'dump':
    va, n = int(sys.argv[3], 16), int(sys.argv[4], 0)
    o = va2off(va)
    for k in range(0, n, 16):
        chunk = data[o + k:o + k + 16]
        print('%08X  %s' % (va + k, ' '.join('%02x' % c for c in chunk)))
elif cmd == 'dwords':
    va, n = int(sys.argv[3], 16), int(sys.argv[4], 0)
    o = va2off(va)
    for k in range(0, n * 4, 4):
        v = struct.unpack_from('<I', data, o + k)[0]
        s = ''
        so = va2off(v) if base <= v < base + 0x1000000 else None
        if so is not None:
            raw = data[so:so + 60].split(b'\0')[0]
            if raw and all(32 <= c < 127 for c in raw):
                s = repr(raw.decode())
        print('%08X  %08X  %s' % (va + k, v, s))
elif cmd == 'dis':
    import capstone
    va, n = int(sys.argv[3], 16), int(sys.argv[4], 0)
    o = va2off(va)
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    for ins in md.disasm(data[o:o + n], va):
        print('%08X  %-24s %s %s' % (ins.address, ins.bytes.hex(), ins.mnemonic, ins.op_str))
elif cmd == 'str':
    o = va2off(int(sys.argv[3], 16))
    print(data[o:o + 200].split(b'\0')[0])
