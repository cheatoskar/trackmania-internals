# trackmania-internals

Reverse-engineering notes on the classic TrackMania games for mod and tool developers: exe identification, function addresses, class and struct layouts, engine reflection data, GBX file formats, dedicated server and XML-RPC behaviour, Stadium physics and its bugs, offline TAS research, rendering hooks, loaders and hooking techniques, and TMX API quirks.

Everything here is **knowledge, not game content**: no executables, assets, maps, replays, symbol maps or decompiled code are included (see [DISCLAIMER.md](DISCLAIMER.md)).

## Start here

- **Building a mod?** [docs/building-mods.md](docs/building-mods.md): pick the exact exe, choose a hook route, find and verify addresses, test safely, and how to use an AI coding assistant with Ghidra.
- **Starting a mod with an AI assistant?** Paste the [setup prompt](docs/building-mods.md#setup-prompt) into Claude Code (or another assistant) in an empty folder: it clones this repository next to it, checks your exe, copies a [template](templates/README.md), fills in a `CLAUDE.md` and builds, then stops before installing anything.
- **Want code that compiles?** [templates/](templates/README.md): a TMLoader plugin and a dinput8 proxy with build check, prologue check, log and one harmless example hook.
- **Looking for tools?** [docs/tools.md](docs/tools.md): checked links for GBX parsing, reverse engineering, TAS/physics, loaders, TMX and servers.
- **Generating maps with machine learning?** [docs/map-generation.md](docs/map-generation.md): how Stadium maps are represented for a model, what worked and failed, quality metrics, and how to write a `.Challenge.Gbx` (block flags, password chunk, author time).
- **Running a server?** [server/operations.md](server/operations.md) (setup, visibility, updates, maps) and [server/manialinks.md](server/manialinks.md) (in-game UI from a controller).
- **Need an address?** Check your exe in the [Coverage](#coverage) table, then open that game's page under [Navigation](#navigation).

## Coverage

| Game | Exe | Build covered (identify by PE timestamp / SHA-256, not by version string) | What is covered | Depth |
|---|---|---|---|---|
| TrackMania Nations Forever (TMNF) | `TmForever.exe` 2.11.26 | `4d431494` (2011-01-28), 10,498,048 bytes, SHA-256 `3847cf9f20bfc63914450060ed528c12104f743d96ad23d6e76abd178de8c84f` | engine hooks, game/race state, server browser, physics (Model6, bugs, headless TMNF-C), rendering (incl. VR), reflection, symbols | deep (reference build; a public `TmForever.map` exists for it) |
| TrackMania United Forever (TMUF) | `TmForever.exe` 2.11.26 | `4b9e8add` (2010-03-15), 10,493,952 bytes, SHA-256 `4b6a7b31d86766409e94101f1256cd61dffecb23ea497a40b6617506b2ced2d4` | addresses ported from TMNF with confidence, hook table column, VR mod layouts | medium (no map file; every port is byte-compared) |
| TrackMania Nations ESWC | `TmNationsESWC.exe` 0.1.7.9 | unofficial 1.80 community build `5ce7272f` (2019-05-23), SHA-256 `983bd12724739dcb0f876aeb3a4259154b068940f5167f57ede750ca452d8068` | race end, replay save, input engine, render offsets, reflection, online TLS issue | medium |
| TrackMania Sunrise (Win10 community repack) | `TmSunrise.exe` | `5cc8f0cf`, 7,904,768 bytes, SHA-256 `e59dfc7eaa5314e85b2b0df6f40ef7ba01a23bd5a9525f84407f5dff301df4fe` | race events, play-a-map path, input engine, reflection | light (static only, not run) |
| TrackMania Original (Online Patch v2 community exe) | `TMORIGINAL.EXE` | `5d337477` (2019-07-20), 7,883,264 bytes, SHA-256 `75e129d769f4595ddde54a8ef484162915bc2bf998c1a2139cc963294fe8175e`; GreffMASTER's Online Patch v2 (2023) made from the community build without StarForce | exe identification, imports (dgVoodoo d3d9, m4d.dll), race events, play-a-map path, input port, reflection compared with Sunrise | light (static only, not run) |
| ManiaPlanet / TrackMania 2 | `ManiaPlanet.exe` (x64) | last build 2019-11-19, `5dd42b7c`, SHA-256 `4af0e34a33b858c8e379e916c1e4f09c38f5744e0e8e207ae6648b8543e86b2b` | RVAs, class ids, D3D11 constant buffers, reflection table layout | light |
| TMF dedicated server | `TrackmaniaServer` 2011-02-21 | — | XML-RPC (GBXRemote), limits, quirks, logs, ManiaLinks, running servers in practice (ports, visibility, deploys without kicking players, TMX map handling) | medium (three public servers, three months) |
| 2006 dedicated server (TMN/TMS/TMO) | `TrackManiaServer` 2006-05-30 | zip SHA-256 `28d79550fdd18623c47d3a428af5ce8e150402b8c4c5fa0b45d3efa4e6bf783d` | command line, callbacks, available methods | medium |

TMLoader 2.12.0 runs its own `TmForever.exe` for Nations and United (10,685,290 bytes, a different SHA-256 for the `2.12.0` and `2.12.0-compat` profiles): same PE timestamp `4d431494` as TMNF, and its `.text` section is byte-identical to TMNF 2.11.26 (compared 2026-11-11), so the TMNF addresses apply, rebased. Tell it apart from the standalone TMNF exe by file size.

ESWC builds are not interchangeable: the original 1.80 exe (SHA-256 `054778cc...`) and the original with the Win10 patch (`38121332...`) differ from the unofficial build in thousands of places, so the ESWC addresses only apply to `983bd127...`. TMNF and TMUF share the exe name and version string; tell them apart by PE timestamp or hash. [scripts/re/pe_info.py](scripts/re/pe_info.py) prints all of these for your copy.

## Navigation

| Folder / file | Content |
|---|---|
| [docs/](docs/) | [building-mods.md](docs/building-mods.md) (guide, with or without AI help), [tools.md](docs/tools.md) (tools and libraries), [map-generation.md](docs/map-generation.md) (ML map generation, map writing) |
| [games/tmnf/](games/tmnf/) | [engine-hooks.md](games/tmnf/engine-hooks.md) (render/media/light hook table, TMNF + TMUF columns), [game-state-offsets.md](games/tmnf/game-state-offsets.md) (race/player state, replay/menu functions), [master-server-functions.md](games/tmnf/master-server-functions.md) (server browser, `tmtp://`), [tmnfc-annotations.md](games/tmnf/tmnfc-annotations.md) (physics addresses from TMNF-C) |
| [games/tmuf/](games/tmuf/README.md) | TMUF specifics, porting method, VR-mod layouts |
| [games/tmn-eswc/](games/tmn-eswc/) | [addresses.md](games/tmn-eswc/addresses.md), [engine-hooks.md](games/tmn-eswc/engine-hooks.md), [input.md](games/tmn-eswc/input.md) (DirectInput, bindings; ESWC + Sunrise), [race-objects.md](games/tmn-eswc/race-objects.md) (editor slot in solo races, player -> car, car input members), [audio-display-settings.md](games/tmn-eswc/audio-display-settings.md) (music/sound volume, fullscreen; ESWC + Sunrise + Original), [replay-tail.md](games/tmn-eswc/replay-tail.md) (ghost runs ~3 s past the finish) |
| [games/tm-sunrise/](games/tm-sunrise/README.md) | Sunrise exe, play-a-map path, [race-events.md](games/tm-sunrise/race-events.md), [replay-save.md](games/tm-sunrise/replay-save.md) (replay save without the dialog, Sunrise + Original + ESWC) |
| [games/tm-original/](games/tm-original/README.md) | TM Original exe (Online Patch v2), [addresses.md](games/tm-original/addresses.md), [engine-hooks.md](games/tm-original/engine-hooks.md) (loader, dgVoodoo, D3D9, race hooks) |
| [games/maniaplanet/](games/maniaplanet/engine.md) | ManiaPlanet.exe RVAs, class ids, D3D11 constant buffers |
| [formats/](formats/) | [gbx.md](formats/gbx.md) (container, LZO, replay/ghost/challenge chunks, class ids), [replay-validable-eswc.md](formats/replay-validable-eswc.md) (format facts only) |
| [physics/](physics/tmnf-physics.md) | Stadium physics: tick and substeps, Model6 forces, surfaces, caps, state layout, every known bug mechanic (nosebug, uberbug, bluebug, noseboost, bugslide ...), cross-checks with TMNF-C and ForeverValidator, [tmnf-c.md](physics/tmnf-c.md) (headless TMNF-C: matching against the exe, formats, float model, fidelity on TMX maps, Windows build) |
| [tas/](tas/README.md) | offline TAS research: staged short-window searches, objectives, how searches "cheat" and how to verify hits, tools (ForeverValidator, TMInterface) |
| [rendering/](rendering/tmnf-d3d9.md) | D3D9 pipeline facts, lighting/moon/lightmap cache, ManiaPlanet port notes, [vr-openxr.md](rendering/vr-openxr.md) (VR: D3D9 draw replay to OpenXR, projection, device hand-over between mods), [frame-loop.md](rendering/frame-loop.md) (main loop, timer, Present and GPU sync, first-use shader compiles; TMNF and the 2006 engine), [shader-compile-2006.md](rendering/shader-compile-2006.md) (2006 engine: shader compile method, the D3DX functions linked into the exes, include handler, large-address-aware state of the exes), [gpu-sync.md](rendering/gpu-sync.md) (Cpu/Gpu synchro: the game's wait for the GPU after every frame, all five games, where the setting is stored), [2006-engine-materials-lights.md](rendering/2006-engine-materials-lights.md) (Nations ESWC, Sunrise, Original: material per draw from the shader copy's textures, delayed draws, views into textures, point lights) |
| [modding/](modding/) | [loaders-and-hooks.md](modding/loaders-and-hooks.md) (TMLoader, dinput8/d3d9/d3d11 proxies, inline/IAT/vtable hooks), [reflection.md](modding/reflection.md), [game-control.md](modding/game-control.md), [tmx-rules-for-mods.md](modding/tmx-rules-for-mods.md), [vr-tmfoxr.md](modding/vr-tmfoxr.md) |
| [server/](server/) | [xmlrpc-dedicated.md](server/xmlrpc-dedicated.md), [manialinks.md](server/manialinks.md) (controller UI: send/hide, clicks, layout, escaping), [operations.md](server/operations.md) (running servers: account, ports, "cannot see server", hosting, updates without kicking players, TMX maps, logs), [old-2006-server.md](server/old-2006-server.md), [masterserver.md](server/masterserver.md) (compatibility facts only) |
| [tmx/](tmx/api.md) | TMX endpoints, fields, filters, silent-ignore traps, paging |
| [tools/](tools/) | [ghidra.md](tools/ghidra.md) (headless + MCP setup, timings), [scripts.md](tools/scripts.md) (inventory), [fvlab/](tools/fvlab/README.md) (traces and search tool: patch series on ForeverValidator) |
| [scripts/](scripts/) | helper scripts: `ghidra/`, `re/`, `gbx/`, `physics/` (trace analysis), `tas/` (fvlab searches, map and trick analysis) |
| [templates/](templates/README.md) | mod templates that compile: TMLoader plugin, dinput8 proxy, shared hook/log/build-check code, `CLAUDE.md` template |
| [data/](data/README.md) | engine reflection tables (TMNF, ESWC, Sunrise), the TMNF/TMUF symbols the notes use, Model6 call/constant index |
| [gotchas.md](gotchas.md) | one-line traps with links: Windows/encoding, Ghidra, loader lock, testing, dedicated servers, controller logic, ManiaLinks, TMX and maps, game caches, rendering mods |
| [contradictions.md](contradictions.md) | places where sources disagree, and which side holds |

## Using the addresses

- Addresses are **link-time virtual addresses (VAs)** for the 32-bit exes, with image base `0x00400000`, unless a table says RVA. ManiaPlanet values are RVAs (image base `0x140000000`) unless written as `0x14...`.
- Every address belongs to exactly **one build**. Check the PE timestamp or SHA-256 of the running exe first, and compare the prologue bytes before hooking or calling anything; disable your hooks if they differ.
- **TmForever under TMLoader 2.12.0:** the default `2.12.0` profile relocates `TmForever.exe` (ASLR-style rebasing, it has a `.reloc` section); the `2.12.0-compat` profile does not. Always compute `runtime = module_base - 0x400000 + VA` with `module_base = GetModuleHandle(NULL)` (or the module list) instead of using the VA directly. Fixed-address memory readers only work with the `-compat` profile.
- ESWC (`983bd127`) has no `.reloc` section and always loads at `0x00400000`.
- Struct offsets (`+0x1C`) are relative to the object pointer and are only valid for the named build.
- Engine classes can also be found by name through the engine's own reflection tables, which is more robust across builds: see [modding/reflection.md](modding/reflection.md) and [data/reflection/](data/README.md).

## Confidence levels

| Label | Meaning |
|---|---|
| **verified** | seen in code or data, measured, simulated bit-exact, or re-checked against the exe |
| **inferred** | derived from other facts, or a reasoned guess in the source |
| **unverified** | a claim from prose or a single observation that nobody re-checked |

Sources are given per row: a function or address for static analysis, `[sim]` for simulation, a link for community knowledge, or the name of the project the fact came from (see [sources.md](sources.md)). Disagreements are kept in [contradictions.md](contradictions.md) instead of being silently resolved.

## What is deliberately not here

Game files of any kind (exes, DLLs, `TmForever.map`, assets, shaders, maps, replays, memory dumps, Ghidra projects, decompiled functions); anything about logging in to Nadeo's master servers or impersonating a game client; anything that helps bypass anti-cheat, replay validation, the Competition Patch, StarForce or other DRM; input scripts or replays of unfinished TAS routes; personal data.

## Contributing

Issues and pull requests are welcome. Every address needs the game, the exact build (PE timestamp or hash), the address form (VA/RVA), a source and a status. Details: [CONTRIBUTING.md](CONTRIBUTING.md).

## Licence

- **Text and data** (all Markdown files, `data/`): [Creative Commons Attribution 4.0 International (CC BY 4.0)](LICENSE). Credit "trackmania-internals contributors" with a link to this repository.
- **Code** in `scripts/` and `templates/` (the template sources, build files, `CLAUDE.md.template` and the workflow that builds them): [MIT](LICENSE-MIT), unless a file header names another licence.
- **fvlab patches** in `tools/fvlab/`: a derived work of [ForeverValidator](https://github.com/Skycrafter-dev/ForeverValidator), MIT with the upstream copyright notice kept (see its README).
- The games, their names and all game content belong to their owners and are not covered by either licence.

## Credits

Built on public community work: [TMNF-C](https://github.com/adonis-singh/TMNF-C), [ForeverValidator](https://github.com/Skycrafter-dev/ForeverValidator), [TMInterface](https://donadigo.com/tminterface) and the [Competition Patch](https://donadigo.com/tmcp) by donadigo, [TMLoader](https://tomashu.dev/software/tmloader) by Tomashu, [TrackManiaForeverOpenXR](https://github.com/jiink/TrackManiaForeverOpenXR), [TwinkieForever](https://github.com/TwinkieTweaks/TwinkieForever), [GBX.NET](https://github.com/BigBang1112/gbx-net) and [randomizer-anywhere](https://github.com/BigBang1112/randomizer-anywhere) by BigBang1112, [TrackMania Exchange](https://api2.mania.exchange), and tools [Ghidra](https://github.com/NationalSecurityAgency/ghidra), [GhidraMCP](https://github.com/LaurieWired/GhidraMCP), [Capstone](https://www.capstone-engine.org/) and [pefile](https://github.com/erocarrera/pefile). Full list: [sources.md](sources.md).

Not affiliated with Ubisoft or Nadeo. See [DISCLAIMER.md](DISCLAIMER.md).
