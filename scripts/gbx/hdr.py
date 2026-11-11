# print the XML header of a GBX
import sys, glob, re, os
sys.stdout.reconfigure(encoding="utf-8")
for p in sorted(glob.glob(sys.argv[1])):
    d = open(p, "rb").read()
    m = re.search(rb"<header.*?</header>", d, re.S)
    print(os.path.basename(p), len(d))
    print("   ", m.group(0).decode("utf-8", "replace") if m else None)
