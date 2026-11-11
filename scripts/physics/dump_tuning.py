# ReadProcessMemory dump of car tuning curves/materials/wheels via RTTI (TMNF+TMUF)
"""Read-only dump of the Stadium car tuning and vehicle materials from a running TmForever.exe.

Start TMNF or TMUF, load any Stadium map until the car is on the track, then run:
    python scripts/dump_tuning.py            (writes re/tuning_dump.json and re/tuning_dump.md)

Finds the objects by their vtable (resolved through MSVC RTTI, so it works on both builds) and
only uses ReadProcessMemory. Layouts come from the TMNF decompile:
  CFuncKeysReal: +0x14 keys {count, ptr} (x in km/h for the tuning curves), +0x20 values, +0x28 interp
  CSceneVehicleMaterial: +0x14..+0x20 the four blendable values used by the car model
"""
import ctypes
import ctypes.wintypes as wt
import json
import os
import struct
import sys

PROCESS_VM_READ = 0x10
PROCESS_QUERY_INFORMATION = 0x400
MEM_COMMIT = 0x1000
READABLE = {0x02, 0x04, 0x08, 0x20, 0x40, 0x80}

k32 = ctypes.WinDLL("kernel32", use_last_error=True)


class MBI(ctypes.Structure):
    _fields_ = [("BaseAddress", ctypes.c_void_p), ("AllocationBase", ctypes.c_void_p),
                ("AllocationProtect", wt.DWORD), ("RegionSize", ctypes.c_size_t),
                ("State", wt.DWORD), ("Protect", wt.DWORD), ("Type", wt.DWORD)]


def find_pid(name="TmForever.exe"):
    import subprocess
    out = subprocess.run(["tasklist", "/FI", f"IMAGENAME eq {name}", "/FO", "CSV", "/NH"],
                         capture_output=True, text=True).stdout
    for line in out.splitlines():
        parts = [p.strip('"') for p in line.split('","')]
        if len(parts) > 1 and parts[0].lower() == name.lower():
            return int(parts[1])
    return None


class Proc:
    def __init__(self, pid):
        self.h = k32.OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, False, pid)
        if not self.h:
            raise OSError(ctypes.get_last_error(), "OpenProcess failed")

    def read(self, addr, n):
        buf = ctypes.create_string_buffer(n)
        got = ctypes.c_size_t()
        if not k32.ReadProcessMemory(self.h, ctypes.c_void_p(addr), buf, n, ctypes.byref(got)):
            return None
        return buf.raw[:got.value]

    def u32(self, a):
        b = self.read(a, 4)
        return None if not b or len(b) < 4 else struct.unpack("<I", b)[0]

    def f32(self, a):
        b = self.read(a, 4)
        return None if not b or len(b) < 4 else struct.unpack("<f", b)[0]

    def regions(self):
        addr, mbi = 0x10000, MBI()
        while addr < 0x7FFF0000:
            if not k32.VirtualQueryEx(self.h, ctypes.c_void_p(addr), ctypes.byref(mbi), ctypes.sizeof(mbi)):
                break
            base, size = mbi.BaseAddress or 0, mbi.RegionSize
            if mbi.State == MEM_COMMIT and (mbi.Protect & 0xFF) in READABLE and not (mbi.Protect & 0x100):
                yield base, size
            addr = base + size


def module_image(p, base=0x400000):
    """Read the main module (TmForever.exe is not relocated, base 0x400000)."""
    hdr = p.read(base, 0x1000)
    e = struct.unpack_from("<I", hdr, 0x3C)[0]
    size = struct.unpack_from("<I", hdr, e + 24 + 56)[0]
    img = bytearray()
    step = 0x10000
    for off in range(0, size, step):
        chunk = p.read(base + off, min(step, size - off)) or b"\0" * min(step, size - off)
        img += chunk
    return base, bytes(img)


def vtable_of(base, img, cls):
    """MSVC RTTI: TypeDescriptor name -> CompleteObjectLocator -> vtable (address after the COL ptr)."""
    name = (".?AV%s@@" % cls).encode()
    i = img.find(name + b"\0")
    if i < 0:
        return None
    td = base + i - 8
    tdb = struct.pack("<I", td)
    vts = []
    j = img.find(tdb)
    while j >= 0:
        col = base + j - 12  # COL: sig, offset, cdOffset, pTypeDescriptor, pClassHierarchy
        sig, offs = struct.unpack_from("<II", img, j - 12)
        if sig == 0 and offs == 0:
            colb = struct.pack("<I", col)
            k = img.find(colb)
            while k >= 0:
                vts.append(base + k + 4)
                k = img.find(colb, k + 1)
        j = img.find(tdb, j + 1)
    return vts[0] if len(vts) == 1 else (vts or None)


def scan_objects(p, vt):
    pat = struct.pack("<I", vt)
    hits = []
    for base, size in p.regions():
        if 0x400000 <= base < 0x1000000:
            continue  # skip the exe image itself
        data = p.read(base, size)
        if not data:
            continue
        i = data.find(pat)
        while i >= 0:
            if i % 4 == 0:
                hits.append(base + i)
            i = data.find(pat, i + 1)
    return hits


def read_curve(p, obj):
    def buf(off):
        n, ptr = p.u32(obj + off), p.u32(obj + off + 4)
        if n is None or ptr is None or n > 256:
            return None
        b = p.read(ptr, 4 * n) if n else b""
        return list(struct.unpack("<%df" % n, b)) if b is not None else None
    keys, vals = buf(0x14), buf(0x20)
    return {"keys": keys, "values": vals, "interp": p.u32(obj + 0x28)}


def main():
    pid = find_pid()
    if not pid:
        sys.exit("TmForever.exe is not running")
    p = Proc(pid)
    base, img = module_image(p)
    vt = {c: vtable_of(base, img, c) for c in
          ("CSceneVehicleCarTuning", "CSceneVehicleMaterial", "CFuncKeysReal", "CSceneVehicleCar")}
    print("vtables:", {k: (hex(v) if isinstance(v, int) else v) for k, v in vt.items()})
    if not all(isinstance(v, int) for v in vt.values()):
        sys.exit("could not resolve all vtables uniquely")

    out = {"pid": pid, "vtables": {k: hex(v) for k, v in vt.items()}, "tunings": [], "materials": []}
    for obj in scan_objects(p, vt["CSceneVehicleCarTuning"]):
        raw = p.read(obj, 0x400)
        if not raw:
            continue
        fields = {}
        for off in range(4, 0x400, 4):
            u = struct.unpack_from("<I", raw, off)[0]
            f = struct.unpack_from("<f", raw, off)[0]
            entry = {"u32": u, "f32": f}
            if 0x10000 < u < 0x7FFF0000 and p.u32(u) == vt["CFuncKeysReal"]:
                entry["curve"] = read_curve(p, u)
            fields[hex(off)] = entry
        out["tunings"].append({"addr": hex(obj), "fields": fields})
    for obj in scan_objects(p, vt["CSceneVehicleMaterial"]):
        vals = [p.f32(obj + o) for o in (0x14, 0x18, 0x1C, 0x20)]
        raw = p.read(obj, 0x40)
        out["materials"].append({"addr": hex(obj), "blend": vals,
                                 "raw": raw.hex() if raw else None})
    # CSceneVehicleCar: +0x2e8 wheels {count, ptr}, stride 0x2fc; +0x6c surface id -> material index;
    # *(+0x68)+0x14 material pointers (see ComputeVehicleGroundMaterialVals)
    out["cars"] = []
    for car in scan_objects(p, vt["CSceneVehicleCar"]):
        n, ptr = p.u32(car + 0x2e8), p.u32(car + 0x2ec)
        if n is None or not 0 < n <= 8:
            continue
        wheels = []
        for i in range(n):
            w = ptr + 0x2fc * i
            wheels.append({"i": i, "u0": p.u32(w), "steer_flag": p.u32(w + 4),
                           "local_pos": [p.f32(w + o) for o in (0xa8, 0xac, 0xb0)],
                           "contact": p.u32(w + 0x124), "surface_id": (p.u32(w + 0x128) or 0) & 0xFFFF})
        remap = []
        rn, rp = p.u32(car + 0x6c), p.u32(car + 0x70)
        mats = p.u32(car + 0x68)
        if rn and rn < 64 and mats:
            mn, mp = p.u32(mats + 0x14), p.u32(mats + 0x18)
            for sid in range(rn):
                idx = p.u32(rp + 4 * sid)
                m = p.u32(mp + 4 * idx) if idx is not None and mn and idx < mn else None
                remap.append({"surface_id": sid, "mat_index": idx, "mat": hex(m) if m else None,
                              "blend": [p.f32(m + o) for o in (0x14, 0x18, 0x1C, 0x20)] if m else None})
        # dyna params: car+0x28 CHmsItem, item+0x34 corpus buffer, corpus+0x58 CHmsDyna,
        # dyna+0x108 params (PhysicsStep2: substeps = round(dt*(|v|+|w|)/params[12]) + 1)
        dyna_info = None
        item = p.u32(car + 0x28)
        if item:
            cn, cp = p.u32(item + 0x34), p.u32(item + 0x38)
            corpus = p.u32(cp) if cn and cp else None
            dyna = p.u32(corpus + 0x58) if corpus else None
            params = p.u32(dyna + 0x108) if dyna else None
            if params:
                dyna_info = {"corpus": hex(corpus), "dyna": hex(dyna), "substep_flag_33c": p.u32(dyna + 0x33c),
                             "dynamic_type_340": p.u32(dyna + 0x340),
                             "params": [p.f32(params + 4 * k) for k in range(24)],
                             "corpus_flags": hex(p.u32(p.u32(corpus + 0x48) + 0x18) or 0)}
        out["cars"].append({"addr": hex(car), "tuning_list": hex(p.u32(car + 100) or 0),
                            "max_linear_speed": p.f32(car + 0x2e0), "flags_2f4": p.u32(car + 0x2f4),
                            "dyna": dyna_info, "wheels": wheels, "surface_remap": remap})
    cars = out["cars"]

    here = os.path.dirname(os.path.abspath(__file__))
    dst = os.path.join(here, "..", "re", "tuning_dump.json")
    with open(dst, "w", encoding="utf-8") as f:
        json.dump(out, f, indent=1)
    print("tunings:", len(out["tunings"]), "materials:", len(out["materials"]), "cars:", len(cars))
    print("written", os.path.normpath(dst))


if __name__ == "__main__":
    main()
