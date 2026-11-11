# splice_csv.py - joins two fvlab input CSVs at a time, shifting the second part. Input: two inputs CSVs. MIT licence, see LICENSE-MIT.
# Splice two fvlab input CSVs: events of A before Ts, then events of B from Ts+shift on, moved back by shift
# (the held state of B at the splice is re-applied at Ts). usage: python scripts/tas/splice_csv.py A.csv B.csv Ts_ms shift_ms out.csv
import csv, sys
a, b, ts, shift, out = sys.argv[1], sys.argv[2], int(sys.argv[3]), int(sys.argv[4]), sys.argv[5]
def rows(p):
    with open(p, newline="") as f:
        r = list(csv.reader(f))
    return r[0], [x for x in r[1:] if x]
head, ra = rows(a)
_, rb = rows(b)
STRUCT = {"7", "8", "9", "0"}
res = [x for x in ra if int(x[0]) < ts or x[1] in STRUCT and int(x[0]) < ts]
# the held state at the splice: last analog/switch value of B before Ts+shift must be re-applied at Ts
last = {}
for x in rb:
    t = int(x[0])
    if t < ts + shift and x[1] not in STRUCT:
        last[(x[1], x[2])] = x
for k, x in last.items():
    res.append([str(ts), x[1], x[2], x[3], x[4]])
for x in rb:
    t = int(x[0])
    if t >= ts + shift and x[1] not in STRUCT:
        nt = t - shift
        res.append([str(nt - nt % 10), x[1], x[2], x[3], x[4]])
res.sort(key=lambda x: int(x[0]))
with open(out, "w", newline="") as f:
    w = csv.writer(f, lineterminator="\n")
    w.writerow(head)
    w.writerows(res)
