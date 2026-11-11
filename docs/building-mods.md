# Building a mod (with or without AI help)

A practical route from "I want to change something in the game" to a tested mod, for the classic games (TMNF, TMUF, ESWC, Sunrise) and, more briefly, ManiaPlanet/TM2. The addresses live in the per-game tables of this repository; this page is the process around them.

## 1. Pick the game and the exact exe

Addresses belong to one build. TMNF and TMUF share the exe name and version string.

1. Print the identity of your exe: `python scripts/re/pe_info.py <exe>` (PE timestamp, size, SHA-256).
2. Compare it with the [Coverage table in the README](../README.md#coverage). If your build is not listed, the addresses do not apply; you would have to find your own (step 3).
3. Use the matching tables: [TMNF/TMUF](../games/tmnf/engine-hooks.md), [ESWC](../games/tmn-eswc/addresses.md), [Sunrise](../games/tm-sunrise/README.md), [ManiaPlanet](../games/maniaplanet/engine.md).

## 2. Choose a hook route

| Route | Use when | Notes |
|---|---|---|
| TMLoader plugin | TMNF/TMUF, you want the normal ecosystem (profiles, other mods) | DLL must export `_DllMain@12`; start work in a thread, not in `DllMain`. The default profile relocates the exe, so rebase addresses. See [loaders-and-hooks.md](../modding/loaders-and-hooks.md). |
| dinput8 / d3d9 proxy | any 32-bit classic game without a loader; rendering or input mods | a DLL named like the system DLL, placed next to the exe, forwarding every export; loads your code on the first real call. Never import the proxied DLL by name from inside the proxy; load the system copy by full path. |
| ASI loader | quick experiments | an ASI loader masquerading as a game DLL; documented in the same page |
| Openplanet | TM2020 and the ManiaPlanet titles | a scripting platform ([docs](https://openplanet.dev/docs)); try it before any binary hook |
| d3d11 proxy | ManiaPlanet x64 | same pattern as d3d9, 64-bit; light coverage here |

Prefer, in this order: engine reflection by name ([reflection.md](../modding/reflection.md)), vtable slot or IAT patch, then inline hook. Fewer code patches mean fewer conflicts with other mods.

## 3. Find addresses

1. Look in the tables first; every row names build, address form (VA or RVA) and status.
2. For something new, use Ghidra: import a **copy** of the exe and analyse it headless ([tools/ghidra.md](../tools/ghidra.md), scripts in [scripts/ghidra/](../scripts/ghidra/)). Analysis of TMNF took about 8 minutes on a desktop CPU.
3. Search by strings, by reflection names ([data/reflection/](../data/README.md)) and by call sites ([scripts/re/](../scripts/re/)).
4. Verify before you trust: read the first 8-16 bytes of the function in the exe and compare them with what you expect (whole instructions, no relative operands in the part you copy). Write the bytes next to the address in your notes.
5. Relocation: `runtime = module_base - 0x400000 + VA` for 32-bit exes. Never use a VA directly.
6. Another build: do not copy addresses. Port by masked byte match (relocated operands and `call rel32` masked) and compare the result, see [games/tmuf/README.md](../games/tmuf/README.md).

### Ghidra with an MCP server

Optional, but useful for assistants and for fast manual queries. Setup in short (details and pitfalls in [tools/ghidra.md](../tools/ghidra.md)):

- Original GUI plugin: [GhidraMCP](https://github.com/LaurieWired/GhidraMCP) (open the project, start the server from the Tools menu). The [fork with a headless server](https://github.com/bethington/ghidra-mcp) serves a project without the GUI; [scripts/ghidra/mcp-headless.ps1](../scripts/ghidra/mcp-headless.ps1) is a launcher.
- Only one Ghidra process may hold a project; stop the headless server before opening the GUI.
- The server listens on the local machine only. Check it with `curl http://127.0.0.1:8089/check_connection`, then register it in your MCP client (a bridge script ships with the plugin).
- A server started from a background shell may exit at once (stdin closed); start it as a detached process.

## 4. A minimal skeleton

Illustrative only, MIT, 32-bit, MSVC. It checks the build, checks the prologue bytes, then installs a 5-byte inline hook with a trampoline. `kExpectedStamp`, `kTarget` and `kPrologue` are placeholders you fill from the tables; the example does not name a real function.

```cpp
// mod.cpp - build as a 32-bit DLL. MIT.
#include <windows.h>
#include <cstring>

static const DWORD kExpectedStamp = 0;        // PE timestamp of the supported exe (see README)
static const DWORD kTarget        = 0;        // link-time VA of the function
static const BYTE  kPrologue[5]   = {0};      // expected first bytes (whole instructions)

typedef int (__cdecl *Fn)(int);
static Fn g_orig = nullptr;

static int __cdecl Hooked(int a) {
    // your code; call through to the original
    return g_orig ? g_orig(a) : 0;
}

static bool Install() {
    BYTE* base = (BYTE*)GetModuleHandleW(nullptr);
    auto nt = (IMAGE_NT_HEADERS*)(base + ((IMAGE_DOS_HEADER*)base)->e_lfanew);
    if (nt->FileHeader.TimeDateStamp != kExpectedStamp) return false;   // unknown build: do nothing

    BYTE* target = base - 0x400000 + kTarget;                           // rebase
    if (memcmp(target, kPrologue, 5) != 0) return false;                // prologue differs: do nothing

    // trampoline: copied prologue + jump back
    BYTE* tramp = (BYTE*)VirtualAlloc(nullptr, 32, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!tramp) return false;
    memcpy(tramp, target, 5);
    tramp[5] = 0xE9;
    *(DWORD*)(tramp + 6) = (DWORD)((target + 5) - (tramp + 10));
    g_orig = (Fn)tramp;

    DWORD old;
    VirtualProtect(target, 5, PAGE_EXECUTE_READWRITE, &old);
    target[0] = 0xE9;
    *(DWORD*)(target + 1) = (DWORD)((BYTE*)Hooked - (target + 5));
    VirtualProtect(target, 5, old, &old);
    FlushInstructionCache(GetCurrentProcess(), target, 5);
    return true;
}

static DWORD WINAPI Boot(LPVOID) { Install(); return 0; }

// TMLoader needs the export name _DllMain@12 (export it with a .def file or /EXPORT).
extern "C" BOOL WINAPI DllMain(HINSTANCE h, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(h);
        CloseHandle(CreateThread(nullptr, 0, Boot, nullptr, 0, nullptr));  // never work under the loader lock
    }
    return TRUE;
}
```

Notes: the 5-byte copy is only valid if the first instructions end exactly at byte 5 and contain no relative operands; otherwise copy up to the next instruction boundary (and patch with NOP padding). Guard calls into the game with `__try/__except`. A proxy DLL additionally needs the forwarding exports of the DLL it replaces; start `Install` on the first real call (not in `DllMain`).

## 5. Test safely

- Work on a **test copy** of the game folder. Never patch the original, never write into a shared install.
- Do not test experimental hooks online. Keep the profile with the mod separate from the one you play with.
- Log what the mod does, and make "unknown exe" or "prologue mismatch" a logged no-op, never a crash.
- Hooks can crash the game at load; test the plain game first, then add one hook at a time.
- If you plan to publish a mod for the TMX community, read [tmx-rules-for-mods.md](../modding/tmx-rules-for-mods.md) first: driving aids, ghost lines and camera/MediaTracker changes are restricted even offline. Do not touch replay files or their validation state; this repository does not document ways around anti-cheat or replay validation.

## Using an AI coding assistant (e.g. Claude Code)

An assistant is good at the grind: reading decompiled functions, writing hook boilerplate, scripting Ghidra, comparing bytes. It is not an oracle for this game.

### Setup

1. Install Ghidra and import a copy of your exe ([tools/ghidra.md](../tools/ghidra.md)).
2. Start a GhidraMCP server (above) and register it in the assistant's MCP configuration under any name. Check your client's documentation for the current syntax. Tools appear only in sessions started after registration.
3. Open the assistant in your mod's repository, with a checkout of this repository next to it (or added to the session as a second folder).
4. Give it a short instruction file in your repo (for Claude Code: `CLAUDE.md`) that says: which game and exact build (PE timestamp, hash), that addresses are VAs of that build, where the tables are (`README.md`, the game's `engine-hooks.md` or `addresses.md`), and the rules below.

### Good practices

- **One change at a time.** One hook, one test, one commit. Small steps make a crash attributable.
- **Verify every address yourself in Ghidra.** Open the function, read the first bytes, compare with the table and with the exe. Ask the assistant to show the bytes, then check them independently.
- **Make it cite sources.** Every address it proposes must name a table row, a reflection entry or a Ghidra query result; "I think it is at ..." is not enough.
- **Never let it run the game online.** Run the game yourself on the test copy, or let it run only offline test scripts. Do not give it any credentials.
- **Keep secrets out.** No tokens, keys, account names or personal paths in prompts, notes or committed files; check logs before pasting them.
- **Keep exes and assets out of the repo.** Do not let it commit game files, Ghidra projects or decompiled code.
- **Review diffs.** Read what it changed before running it; a mod DLL runs inside the game process.

### Limits

- It **hallucinates addresses, struct offsets and function names**, especially for a build it has not seen, and presents a plausible number as certain. Always check prologue bytes against the exe.
- It does not know which build you have unless you tell it; TMNF/TMUF confusion is easy.
- Decompiler output can be wrong (calling conventions, `thiscall`, stack arguments); a hook built on a misread signature crashes at runtime.
- It cannot judge TMX moderation or anti-cheat questions; follow the written rules.
- Long sessions drift; restate the build and the rules when a session gets long.
