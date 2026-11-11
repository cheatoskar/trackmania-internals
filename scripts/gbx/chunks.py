# scan a decompressed body for chunk/class ids
import sys, struct
sys.stdout.reconfigure(encoding="utf-8")
d = open(sys.argv[1], "rb").read()
want = {0x2403f, 0x2401b, 0x0303f, 0x2407e}
n = int(sys.argv[2]) if len(sys.argv) > 2 else 40
for i in range(0, len(d) - 4):
    v = struct.unpack_from("<I", d, i)[0]
    if (v >> 12) in want and (v & 0xfff) < 0x30:
        nxt = d[i+4:i+4+n]
        print("%06x %08x  %s  %r" % (i, v, nxt.hex(" "), bytes(c if 32 <= c < 127 else 46 for c in nxt).decode()))
