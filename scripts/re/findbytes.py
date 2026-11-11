# search a byte pattern in an exe
import sys, re, pefile
pe = pefile.PE(sys.argv[1], fast_load=True)
data = open(sys.argv[1], 'rb').read()
base = pe.OPTIONAL_HEADER.ImageBase
def off2va(o):
    for s in pe.sections:
        if s.PointerToRawData <= o < s.PointerToRawData + s.SizeOfRawData:
            return base + s.VirtualAddress + (o - s.PointerToRawData)
pat = bytes.fromhex(sys.argv[2].replace('?', ''))  # plain hex
rx = re.compile(re.escape(pat)) if '..' not in sys.argv[2] else None
# regex form: use python bytes regex given as argv[3]
if len(sys.argv) > 3:
    rx = re.compile(eval("b'" + sys.argv[3] + "'"), re.S)
for m in rx.finditer(data):
    print('%08X' % off2va(m.start()))
