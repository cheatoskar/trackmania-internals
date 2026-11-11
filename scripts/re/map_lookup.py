# map_lookup.py - cross-check documented TMNF addresses against the linker map.
# Looks up VAs in an MSVC linker .map (e.g. TmForever.map for TMNF 4d431494) and prints the containing symbol.
# Usage: python map_lookup.py <TmForever.map> 0x7c69e0 0x9a0990 ...
#        python map_lookup.py <TmForever.map> --name ComputeForces      (substring search on mangled names)
# Note: the map's "0001:xxxxxxxx" column is section-relative; this script uses the "Rva+Base" column (= VA).
import bisect
import re
import sys

LINE = re.compile(r"^\s*([0-9a-fA-F]{4}):([0-9a-fA-F]{8})\s+(\S+)\s+([0-9a-fA-F]{8})\s+(f\s+)?(.*)$")


def load(path):
    syms = []
    with open(path, "r", encoding="latin-1") as f:
        for line in f:
            m = LINE.match(line)
            if not m:
                continue
            va = int(m.group(4), 16)
            if va == 0:
                continue
            syms.append((va, m.group(3), bool(m.group(5)), m.group(6).strip()))
    syms.sort()
    return syms


def main():
    if len(sys.argv) < 3:
        print(__doc__ or "usage: map_lookup.py <map> <va>... | --name <text>")
        return
    syms = load(sys.argv[1])
    if sys.argv[2] == "--name":
        needle = sys.argv[3]
        for va, name, is_fn, obj in syms:
            if needle in name:
                print("0x%08x %s %s" % (va, "f" if is_fn else "d", name))
        return
    vas = [s[0] for s in syms]
    for arg in sys.argv[2:]:
        va = int(arg, 16)
        i = bisect.bisect_right(vas, va) - 1
        if i < 0:
            print("0x%08x  (before first symbol)" % va)
            continue
        sva, name, is_fn, obj = syms[i]
        off = va - sva
        print("0x%08x  %s%s  [%s]" % (va, name, "" if off == 0 else " +0x%x" % off, obj))


if __name__ == "__main__":
    main()
