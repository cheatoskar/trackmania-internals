# list ghosts (time, logins) of ESWC replay bodies
import sys, struct, re
sys.stdout.reconfigure(encoding="utf-8")
for p in sys.argv[1:]:
    d = open(p, "rb").read()
    print("==", p.split("/")[-1])
    starts = [m.start() for m in re.finditer(re.escape(struct.pack("<I", 0x2401b000)), d)]
    for s in starts:
        i5 = d.find(struct.pack("<I", 0x2401b005), s)
        i6 = d.find(struct.pack("<I", 0x2401b006), s)
        i8 = d.find(struct.pack("<I", 0x2401b008), s)
        ib = d.find(struct.pack("<I", 0x2401b00b), s)
        ic = d.find(struct.pack("<I", 0x2401b00c), s)
        t = struct.unpack_from("<i", d, i5 + 12)[0]
        strs = [x.decode("utf-8", "replace") for x in re.findall(rb"[\x20-\x7e\x80-\xff]{3,}", d[i6:i8])]
        cps = d[ib + 12: ic]
        print("  ghost@%06x samples=%d time=%d strs=%s cp=%s" % (s, i5 - s, t, strs, cps.hex(" ")))
