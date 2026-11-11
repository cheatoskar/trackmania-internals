# Mod templates

Two small mods that compile, for the 32-bit classic games. They are starting points: each one identifies the exe, refuses anything it does not know, and installs one harmless example hook. Copy one into your own repository and build from there.

| Template | Game / exe | How it gets loaded |
|---|---|---|
| [tmloader-plugin/](tmloader-plugin/) | TMNF / TMUF through [TMLoader](https://tomashu.dev/software/tmloader) 2.12.0 | TMLoader loads `TemplatePlugin.dll` into its own `TmForever.exe` (product folder + `description.yaml`) |
| [dinput8-proxy/](dinput8-proxy/) | TMN ESWC unofficial 1.80, TM Sunrise Win10 repack, TMNF/TMUF without a loader | a `dinput8.dll` next to the game exe; the game loads it instead of the system DLL, the proxy passes every DirectInput call on |

Both share [common/](common/): the build table, exe identification, two hook helpers, a log, and the example.

**Status:** both compile (MSVC, x86) and pass the self-test in [tests/](tests/) without a game: hook helpers, build lookup, the example hook on a real Direct3D 9 device, both DLLs loading and refusing an unknown exe. The build table and check sites were also checked against the TMNF, TMUF and both TMLoader 2.12.0 exe files (mapped at another base, not run); the ESWC and Sunrise rows come from the tables only. The templates have not been run inside the games yet; the first test in a game is yours, on a test copy.

## What the example does

On load, on its own thread (never inside `DllMain`):

1. Opens a log next to the DLL (`TemplatePlugin.log` or `dinput8-template.log`).
2. Hold **Shift** while the game starts: the mod stays off for that session (logged).
3. Reads the exe's PE timestamp (from memory) and file size (from disk) and looks them up in [common/builds.h](common/builds.h), which is hand-written from the [Coverage table](../README.md#coverage). Unknown exe: logs `UNKNOWN exe` and stops.
4. Reads (never writes) the first bytes of one function the tables list with its prologue, at `module_base - 0x400000 + VA`. Different bytes: logs what it found and stops. This catches a patched exe, a wrong table row, and a forgotten rebase.
5. Installs the example: a vtable hook on `IDirect3DDevice9::Present` (plus `IDirect3D9::CreateDevice`, to catch the game's device when it is created). It counts frames and logs the frame rate every 10 s.

It does not draw, does not read or write game state, and does not touch input, physics, replays or anything that affects driving. The only memory it writes is two Direct3D vtable slots, which belong to `d3d9.dll` (or an overlay wrapping it), not to the game. Keep your own mod read-only on the game as long as you can; see the rules below.

## Build

Needs the Visual Studio Build Tools (or Visual Studio) with "Desktop development with C++". The games are 32-bit: build **x86**.

```bat
templates\build.cmd            rem build both + run the self-test
templates\build.cmd notest     rem build only
```

`build.cmd` uses `cl` from `PATH` if it targets x86 (an "x86 Native Tools Command Prompt"), else the script in the `VCVARS` environment variable (for example `set VCVARS=C:\Program Files (x86)\Microsoft Visual Studio\<version>\BuildTools\VC\Auxiliary\Build\vcvars32.bat`), else the newest installation `vswhere` finds. Output:

```
build\tmloader-plugin\TemplatePlugin.dll
build\dinput8-proxy\dinput8.dll
build\selftest.exe
```

CMake works too (`-A Win32` is required):

```bat
cmake -S templates -B templates\build-cmake -A Win32
cmake --build templates\build-cmake --config Release
```

Check the exports when you change them: `dumpbin /exports TemplatePlugin.dll` must show `_DllMain@12`; the proxy must show the six dinput8 functions (`DirectInput8Create`, `DllCanUnloadNow`, `DllGetClassObject`, `DllRegisterServer`, `DllUnregisterServer`, `GetdfDIJoystick`). The self-test checks both. The workflow [templates.yml](../.github/workflows/templates.yml) builds both ways on GitHub's Windows runners.

## Install for a test

Never install into the folder you play from, never into a shared install, and never go online with an experimental mod. Keep the mod in a setup you only use offline.

### TMLoader plugin (TMNF/TMUF)

TMLoader runs its own copy of `TmForever.exe`; your game folder is not changed. Mods are products in its database:

1. Copy [tmloader-plugin/package/TemplatePlugin](tmloader-plugin/package/TemplatePlugin) to `%LOCALAPPDATA%\TMLoader\database\TmForever\products\TemplatePlugin\` and put `TemplatePlugin.dll` into its `0.1.0\` folder. The product `description.yaml` describes the mod; `0.1.0\description.yaml` names the DLL (`executable:`) and its dependencies. A version counts as installed when its folder holds more than `description.yaml`.
2. Make a **separate profile** for testing (in TMLoader, or a file `database\TmForever\profiles\<name>.yaml`) with only this mod:
   ```yaml
   program:
     id: TmForever
   mods:
     - id: TemplatePlugin
   ```
   Do not add the mod to the profile you play online with. TMLoader picks up new products and versions only when it starts: restart it.
3. Start the game with that profile, stay offline, and look at `TemplatePlugin.log` in the `0.1.0` folder.

The default `2.12.0` profile relocates the exe (the log says `(relocated)`); `2.12.0-compat` does not. The template rebases in both cases.

### dinput8 proxy (ESWC, Sunrise, TMNF/TMUF standalone)

1. Copy the whole game folder to a new folder, for example `TmNationsESWC-test`. Work only there.
2. Put `dinput8.dll` next to the game exe in the copy. If a `dinput8.dll` is already there (another proxy mod, for example ESWC Autosave), rename that one to `dinput8.chain.dll` first: the template forwards every call to it, so it keeps working.
3. Start the exe from the test folder, offline, and look at `dinput8-template.log` next to it.
4. To remove: delete `dinput8.dll` (and rename `dinput8.chain.dll` back).

ESWC 1.80 has no `.reloc` section and always loads at `0x00400000`; TMNF/TMUF and ESWC import `dinput8.dll` directly, Sunrise loads it by name. The Sunrise repack also ships its own `d3d9.dll` wrapper; the example hooks whatever `d3d9.dll` the game uses.

## Did it load?

The log is the proof. A good start looks like this (times, base and vtable addresses will differ):

```
21:04:07.704 template mod starting (TMLoader plugin)
21:04:07.704 exe: PE timestamp 4d431494, 10685290 bytes, loaded at 00d10000 (relocated)
21:04:07.704 build: TMLoader 2.12.0 TmForever (TMNF code)
21:04:07.704 check site CTrackManiaRace::RespawnPlayer at VA 0047c0d0 matches
21:04:07.715 hook: Present hooked in device vtable 07F4859C (probe device)
21:04:07.723 hook: CreateDevice hooked in factory vtable 71822C88 (devices created from now on are covered)
21:04:07.723 example hook installed; the frame rate is logged every 10 s
21:04:09.410 first frame presented
21:04:19.412 143.9 fps (frame 1442)
```

| Log says | Meaning |
|---|---|
| no log at all | the DLL was not loaded: wrong folder or name, TMLoader not restarted, profile without the mod, or (proxy) a 64-bit build |
| `UNKNOWN exe` | timestamp or file size not in `builds.h`: another build, or a modified exe. Do not just add it; identify it first (`python scripts/re/pe_info.py <exe>`) |
| `check site ... does not match` | known exe, but the bytes differ: patched exe, another mod hooked that function, or a wrong table row |
| `Shift held at start` | you held Shift; start again without it |
| `no frame seen after 20 s` | the game created its device before the hook, with a vtable the probe did not cover (some overlays give each device its own vtable). Start the mod earlier, or attach to the game's device another way (`frameCounterAttach`) |

## Make it yours

- Rename the DLL, the product, the log and the window class (`TemplateModProbeWindow`).
- Put your code where `common/mod.cpp` says "your mod starts here", after the identification and the check. Remove the example when you no longer need it.
- Take addresses only from the tables or your own Ghidra check, with the build and the bytes next to them. Use [common/hook.h](common/hook.h): `hookSlot` for vtable/IAT slots, `hookInline` for code, which refuses unless the bytes match exactly.
- Restrict `kBuilds` to the builds you actually tested.
- One hook, one test, one commit.
- Using an AI coding assistant: copy [CLAUDE.md.template](CLAUDE.md.template) into your repository as `CLAUDE.md` and fill it in; the setup prompt is in [docs/building-mods.md](../docs/building-mods.md#using-an-ai-coding-assistant-eg-claude-code).

## Rules that apply to anything you build from this

- **Offline only** while you experiment. Never test hooks online or on a server.
- **Read-only on the Forever games** (TMNF/TMUF) for anything that could reach online play or TMX: no writes to game state, no input changes, no physics changes. Mods must coexist with the [Competition Patch](https://donadigo.com/tmcp), TMUnlimiter and TMInterface, not work around them.
- **TMX rules:** read [modding/tmx-rules-for-mods.md](../modding/tmx-rules-for-mods.md) before publishing. Driving aids, ideal/brake lines, checkpoint/trigger display and camera/MediaTracker changes are restricted.
- Nothing that touches replays for submission, replay validation, anti-cheat or master-server login. This repository does not document ways around them.
- No game files in your repository (exes, DLLs, packs, maps, replays, Ghidra projects, decompiled code).

## Licence

The templates are MIT ([LICENSE-MIT](../LICENSE-MIT)). The forwarding and chain-load code of the dinput8 proxy is adapted from the author's own dinput8 loader (MIT, same author).
