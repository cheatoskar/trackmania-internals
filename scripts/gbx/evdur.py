# print EventsDuration and validation chunk of replay bodies
import sys, struct, glob, os
sys.stdout.reconfigure(encoding="utf-8")
names = {}
for p in sys.argv[1:]:
    d = open(p, "rb").read()
    i = d.rfind(struct.pack("<I", 0x2403f00d))
    j = d.rfind(struct.pack("<I", 0x2403f00e))
    ev = struct.unpack_from("<I", d, i + 4)[0]
    rs_i = d.rfind(b"-Time")
    k = d.rfind(struct.pack("<I", 0x2403f00f))
    seg = d[k + 12:]
    ln = struct.unpack_from("<I", seg, 0)[0]
    ver = seg[4:4 + ln]
    chk, os_, cpu, rl = struct.unpack_from("<IIII", seg, 4 + ln)
    rs = seg[4 + ln + 16:4 + ln + 16 + rl]
    ghosts = d.count(struct.pack("<I", 0x2401b000))
    print("%-10s ghosts=%d EventsDuration=%d inputsBytes=%d ver=%s chk=%08x os=%d cpu=%d race=%s" % (
        os.path.basename(p), ghosts, ev, j - i - 4, ver.decode(), chk, os_, cpu, rs.decode()))
