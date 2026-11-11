# pe_info.py - prints identification data for game exes.
# Usage: python pe_info.py <exe> [<exe> ...]
# Output per file: size, SHA-256, PE TimeDateStamp, machine, ImageBase, SizeOfImage, AddressOfEntryPoint,
# DllCharacteristics (ASLR flag), section names.
import hashlib
import struct
import sys
import time


def pe_info(path):
    with open(path, "rb") as f:
        data = f.read()
    sha = hashlib.sha256(data).hexdigest()
    if data[:2] != b"MZ":
        return {"path": path, "size": len(data), "sha256": sha, "error": "no MZ header"}
    e_lfanew = struct.unpack_from("<I", data, 0x3C)[0]
    if data[e_lfanew:e_lfanew + 4] != b"PE\0\0":
        return {"path": path, "size": len(data), "sha256": sha, "error": "no PE signature"}
    coff = e_lfanew + 4
    machine, nsect, timestamp, _, _, opt_size, _ = struct.unpack_from("<HHIIIHH", data, coff)
    opt = coff + 20
    magic = struct.unpack_from("<H", data, opt)[0]
    entry = struct.unpack_from("<I", data, opt + 16)[0]
    if magic == 0x20B:  # PE32+
        image_base = struct.unpack_from("<Q", data, opt + 24)[0]
    else:
        image_base = struct.unpack_from("<I", data, opt + 28)[0]
    size_of_image = struct.unpack_from("<I", data, opt + 56)[0]
    dll_chars = struct.unpack_from("<H", data, opt + 70)[0]
    sect = opt + opt_size
    names = []
    for i in range(nsect):
        raw = data[sect + 40 * i: sect + 40 * i + 8]
        names.append(raw.rstrip(b"\0").decode("latin-1"))
    return {
        "path": path,
        "size": len(data),
        "sha256": sha,
        "timestamp": "%08x (%s UTC)" % (timestamp, time.strftime("%Y-%m-%d %H:%M:%S", time.gmtime(timestamp))),
        "machine": "%04x" % machine,
        "pe32plus": magic == 0x20B,
        "image_base": "0x%x" % image_base,
        "size_of_image": "0x%x" % size_of_image,
        "entry_rva": "0x%x" % entry,
        "aslr": bool(dll_chars & 0x40),
        "sections": names,
    }


if __name__ == "__main__":
    for p in sys.argv[1:]:
        info = pe_info(p)
        for k, v in info.items():
            print("%-14s %s" % (k, v))
        print()
