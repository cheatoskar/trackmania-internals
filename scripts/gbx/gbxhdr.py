# list GBX header user-data chunks
import sys, struct
sys.stdout.reconfigure(encoding="utf-8")
for p in sys.argv[1:]:
    d = open(p, "rb").read()
    print("==", p.split("/")[-1].split("\\")[-1], len(d))
    print(" magic", d[:9], "class %08x" % struct.unpack_from("<I", d, 9))
    ud = struct.unpack_from("<I", d, 13)[0]
    p0 = 17
    if ud:
        n = struct.unpack_from("<I", d, p0)[0]
        q = p0 + 4
        ch = []
        for i in range(n):
            cid, sz = struct.unpack_from("<II", d, q); q += 8
            ch.append((cid, sz))
        print(" userdata", ud, "chunks", [("%08x" % c, s & 0x7fffffff, bool(s >> 31)) for c, s in ch])
        for cid, sz in ch:
            sz &= 0x7fffffff
            print("  %08x:" % cid, d[q:q+sz][:200].hex(" ") if cid & 0xfff != 1 else d[q:q+sz][:300])
            q += sz
    q = 17 + ud
    nn, ext, us, cs = struct.unpack_from("<IIII", d, q)
    print(" numNodes", nn, "ext", ext, "usize", us, "csize", cs, "end", q + 16 + cs, "filelen", len(d))
