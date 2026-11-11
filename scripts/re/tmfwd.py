# tmfwd.py : derive game-control addresses for TMNF and TMUF from known functions (symbols / fnmatch).
# Prints the globals and offsets the play/quit path needs, read out of the instructions.
import re, struct, sys
import capstone

md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)


class Exe:
    def __init__(self, path):
        self.d = open(path, "rb").read()
        pe = struct.unpack_from("<I", self.d, 0x3c)[0]
        n = struct.unpack_from("<H", self.d, pe + 6)[0]
        opt = struct.unpack_from("<H", self.d, pe + 20)[0]
        self.ts = struct.unpack_from("<I", self.d, pe + 8)[0]
        self.secs = [struct.unpack_from("<8sIIII", self.d, pe + 24 + opt + i * 40) for i in range(n)]
        for nm, vs, va, rs, raw in self.secs:
            if nm.rstrip(b"\0") == b".text":
                self.text = (va, rs, raw)

    def v2o(self, v):
        for nm, vs, va, rs, raw in self.secs:
            if 0x400000 + va <= v < 0x400000 + va + rs:
                return raw + v - 0x400000 - va

    def dis(self, a, n=60):
        o = self.v2o(a)
        out = []
        for i in md.disasm(self.d[o:o + n * 10], a):
            out.append(i)
            if len(out) >= n:
                break
        return out

    def calls_to(self, target):
        va, rs, raw = self.text
        res = []
        for o in range(raw, raw + rs - 5):
            if self.d[o] == 0xE8 and (0x400000 + va + o - raw + 5 + struct.unpack_from("<i", self.d, o + 1)[0]) & 0xFFFFFFFF == target:
                res.append(0x400000 + va + o - raw)
        return res


def globals_in(ins_list, pat):
    out = []
    for i in ins_list:
        for m in re.finditer(pat, i.op_str):
            out.append((i.address, i.mnemonic, i.op_str))
    return out


for name, path, fns in [
    ("TMNF", "bin/tmnf/TmForever.exe", dict(refresh=0x5cf100, launchmc=0x4fb950, appctor=0x59e110, setrace=0x486130, addnew=0x5e9f00, quitfn=0x5dd960)),
    ("TMUF", "bin/tmuf/TmForever.exe", dict(refresh=0x5cf2a0, launchmc=0x4fb5d0, appctor=0x59e310, setrace=0x485f60, addnew=0x5ea050, quitfn=0x5dda30)),
]:
    e = Exe(path)
    print("==", name, "timestamp %08x" % e.ts)
    # s_TheGame: `mov dword ptr [imm], reg` in the CGameApp ctor
    for a, m, op in globals_in(e.dis(fns["appctor"], 200), r"dword ptr \[0x[0-9a-f]{6,}\], e(si|di|bx|bp|ax|cx)"):
        print("  appctor store", hex(a), m, op)
    # s_ChallengeInfos: globals read in RefreshFromDisk
    seen = set()
    for a, m, op in globals_in(e.dis(fns["refresh"], 40), r"\[0x[0-9a-f]{6,}\]"):
        g = re.search(r"0x[0-9a-f]{6,}", op).group(0)
        if g not in seen:
            seen.add(g)
            print("  refresh global", g, "at", hex(a), m, op)
    # SortByName: last call in AddNewChallenge
    calls = [i for i in e.dis(fns["addnew"], 40) if i.mnemonic == "call"]
    print("  addnew calls", [c.op_str for c in calls])
    # menus offsets in LaunchManiaCodeAction: writes to [reg+0x13c]-like and the result
    for i in e.dis(fns["launchmc"], 260):
        if i.mnemonic == "mov" and re.search(r"dword ptr \[e[a-z]{2} \+ 0x(13c|704|[0-9a-f]{3})\], (1|e[a-z]{2})$", i.op_str) and ("0x13c" in i.op_str or "0x704" in i.op_str or ", 1" in i.op_str):
            print("  launchmc", hex(i.address), i.op_str)
    # race offset in SetRace
    for i in e.dis(fns["setrace"], 8):
        if "cmp" in i.mnemonic:
            print("  setrace", hex(i.address), i.mnemonic, i.op_str)
    # ButtonQuitOnClick: `push 2; call quitfn; ret`
    for c in e.calls_to(fns["quitfn"]):
        o = e.v2o(c)
        if e.d[o - 2:o] == b"\x6a\x02" and e.d[o + 5] == 0xC3:
            print("  ButtonQuitOnClick", hex(c - 2))
