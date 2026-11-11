# Tools & libraries

A short, checked list of tools around TrackMania modding and reverse engineering. Repository data was checked through the GitHub API on 2026-11-11 (language, licence as reported by GitHub, last push); websites were checked for reachability only. "Last push" is not a quality statement. If a licence is not listed, read the repository before reusing code. Closed-source tools are marked.

Nothing here is endorsed by Nadeo or Ubisoft. Respect each tool's terms and the [TMX rules for mods](../modding/tmx-rules-for-mods.md).

## GBX files

| Tool | Language / licence | Last push | For |
|---|---|---|---|
| [GBX.NET](https://github.com/BigBang1112/gbx-net) by BigBang1112 | C#/.NET; multiple licences per directory, the main parts MIT (see its README) | 2026-11-11 (release 2.5.1, 2026-11-06) | read and write GBX files of the Nadeo games, 500+ classes; the most complete parser |
| [pygbx](https://github.com/donadigo/pygbx) by donadigo | Python; GPL-3.0 | 2024-06-05 | parse maps, replays and ghosts from Python; quiet, but the format did not change for the old games |
| [gbx.md](../formats/gbx.md) and [scripts/gbx/](../scripts/gbx/) in this repo | Markdown + small Python scripts, MIT | - | container, LZO and chunk facts for TMNF/TMUF/ESWC; no full parser |

I could not verify a maintained JavaScript GBX parser, so none is listed.

## Reverse engineering

| Tool | Language / licence | Last push | For |
|---|---|---|---|
| [Ghidra](https://github.com/NationalSecurityAgency/ghidra) | Java; Apache-2.0 | 2026-11-10 | disassembler and decompiler; headless mode is scriptable ([tools/ghidra.md](../tools/ghidra.md)) |
| [GhidraMCP](https://github.com/LaurieWired/GhidraMCP) by LaurieWired | Java; Apache-2.0 | 2026-10-10 | Ghidra plugin exposing a Ghidra project to an MCP client (decompile, rename, xrefs); the original, GUI-based |
| [ghidra-mcp](https://github.com/bethington/ghidra-mcp) by bethington | Java; Apache-2.0 | 2026-11-11 | fork with a headless server and many more endpoints; this is the variant [tools/ghidra.md](../tools/ghidra.md) describes (it worked with Ghidra 12.1.2 when the original did not) |
| [x64dbg](https://github.com/x64dbg/x64dbg) | C++; licence reported as "other" (GPL-3.0 text, check the repo) | 2026-11-09 | debugger for 32- and 64-bit Windows exes; use on offline copies |
| [Cheat Engine](https://github.com/cheat-engine/cheat-engine) | Pascal; licence not detected by GitHub (check the repo) | 2025-04-19 | memory scanner; to find and read object fields in an offline test copy only. Do not use it against online play. |
| [Capstone](https://www.capstone-engine.org/) and [pefile](https://github.com/erocarrera/pefile) | Python bindings | - | disassembly and PE parsing in scripts such as [scripts/re/](../scripts/re/) |

## Physics and TAS (offline research)

| Tool | Language / licence | Last push | For |
|---|---|---|---|
| [TMNF-C](https://github.com/adonis-singh/TMNF-C) by adonis-singh | C; MIT | 2026-09-13 | annotated reconstruction of TMNF physics code and the Ghidra pipeline behind it ([tmnfc-annotations.md](../games/tmnf/tmnfc-annotations.md)) |
| [ForeverValidator](https://github.com/Skycrafter-dev/ForeverValidator) by Skycrafter-dev | C++; MIT | 2026-10-05 | TMUF physics reconstruction that replays a replay bit-exactly |
| [ForeverTAS](https://github.com/Skycrafter-dev/ForeverTAS) by Skycrafter-dev | C++; MIT | 2026-10-05 | GUI bruteforce/TAS client on top of ForeverValidator ([tas/tools.md](../tas/tools.md)) |
| [TMInterface](https://donadigo.com/tminterface) by donadigo | closed source, free | - | input scripts and AngelScript plugins in the real TMNF/TMUF; offline only |
| [TMInterfacePublic](https://github.com/donadigo/TMInterfacePublic) | C++; no licence file | 2021-07-27 | public resources of TMInterface, not its source |
| [TMInterfaceClientPython](https://github.com/donadigo/TMInterfaceClientPython) | Python; GPL-3.0 | 2024-03-25 | read and write game state from an external script |
| [TMInterfaceAngelScript](https://github.com/Shweetz/TMInterfaceAngelScript) by Shweetz | AngelScript; no licence file | 2026-03-20 | community bruteforce plugins, read for ideas |

## Loaders and frameworks

| Tool | Notes |
|---|---|
| [TMLoader](https://tomashu.dev/software/tmloader) by Tomashu | closed source; mod loader for TMNF/TMUF. Plugins are DLLs described by a `description.yaml`; the default profile relocates the exe ([modding/loaders-and-hooks.md](../modding/loaders-and-hooks.md)). `CoreMod` appears as a dependency id in its product database; I found no public description of it. |
| [Competition Patch](https://donadigo.com/tmcp) by donadigo | public description only; a patch for TMNF/TMUF that also runs as a TMLoader plugin. Mods must coexist with it, not work around it. |
| [TMUnlimiter](https://unlimiter.net/) | closed source; extends TMNF/TMUF. Hooks must coexist with it. |
| [Openplanet](https://openplanet.dev) ([docs](https://openplanet.dev/docs), [plugin manager](https://github.com/openplanet-nl/plugin-manager), MIT) | closed-source scripting platform for the modern games (TM2020 and ManiaPlanet titles); plugins in AngelScript. The normal route there, instead of binary hooks. |
| dinput8 / d3d9 / d3d11 proxy | a technique, not a product: a DLL with the system DLL's name next to the exe, forwarding every export. See [building-mods.md](building-mods.md). |

## Maps, replays, TMX

| Resource | For |
|---|---|
| [TrackMania Exchange](https://trackmania.exchange) and its [API](https://api2.mania.exchange) | map/replay listing and download; field and filter quirks in [tmx/api.md](../tmx/api.md) |
| [trackmania-community-tracks-and-telemetry](https://huggingface.co/datasets/jeanmidev/trackmania-community-tracks-and-telemetry) by jeanmidev (Hugging Face, CC BY 4.0) | ~617k TMNF and ~282k TM2020 track layouts with metadata and winning-run traces, collected from the Mania Exchange sites; Parquet, no login. Quirks in [map-generation.md](map-generation.md#data) |
| [ManiaPark](https://www.maniapark.com) | I only verified that the site is reachable; I have no notes on it. Check what it offers before relying on it. |
| [trackmania.io wiki](https://wiki.trackmania.io) | community documentation, reachable; mostly about TM2020 |

## Servers

| Tool | For |
|---|---|
| [Dedicated server docs](https://doc.maniaplanet.com/dedicated-server) (ManiaPlanet) | official documentation of the server and its XML-RPC; the TMF server's behaviour is in [server/xmlrpc-dedicated.md](../server/xmlrpc-dedicated.md) |
| [XASECO](https://www.xaseco.org) | the classic PHP server controller for the Forever generation; the homepage is reachable, I did not verify current maintenance. Treat as legacy. |
| [PyPlanet](https://github.com/PyPlanet/PyPlanet) | Python; GPL-3.0; last push 2026-10-29; server controller for ManiaPlanet/TM2 |
| [EvoSC](https://github.com/EvoEsports/EvoSC) | PHP; licence not detected by GitHub; last push 2026-10-22; server controller for TM2/TM2020 |
| [EvoSC-sharp](https://github.com/EvoEsports/EvoSC-sharp) | C#; GPL-3.0; last push 2026-11-10; the rewrite for current Trackmania |
| [ManiaAPI.XmlRpc](https://github.com/BigBang1112/maniaapi-net) by BigBang1112 | .NET client for the XML-RPC interface |

## Own tools and side projects

| Project | Status | For |
|---|---|---|
| [TMX-Downloader](https://github.com/cheatoskar/TMX-Downloader) | maintained (JavaScript, MIT) | bulk download of tracks from all TMX exchanges, bundled as a zip; respects the TMX API limits |
| [DeltaZero](https://github.com/cheatoskar/DeltaZero) | side project, not continued (Python, MIT) | an early driving-AI experiment for TMNF (imitation learning, headless physics on TMNF-C); useful as a reference for ghost parsing |
| [trackmania-map-mcp](https://github.com/cheatoskar/trackmania-map-mcp) | side project, not continued | a quick experiment with an MCP interface for TMF maps; not a usable tool |

The fvlab patches on ForeverValidator and the search scripts are in this repository: [tools/fvlab/](../tools/fvlab/README.md), [scripts/tas/](../scripts/tas/).

## Not listed

Anything I could not open or could not tie to a maintained repository. Suggestions with a link are welcome ([CONTRIBUTING.md](../CONTRIBUTING.md)).
