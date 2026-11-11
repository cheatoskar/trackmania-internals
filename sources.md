# Sources and credits

The notes were compiled in November 2026 from the author's own reverse-engineering work and mod projects, and checked against the game exes (exe hashes, 26 TMNF addresses against `TmForever.map`, 7 TMUF ports byte-compared, 6 ESWC prologues against the 983bd127 exe). Where a table's source column names a file such as `autosave.cpp:49` or `RandomizerGame.cs:3160`, it points into one of the projects below; names without a link are the author's unpublished notes or code, kept so that the provenance stays visible.

## The author's own projects (public)

| Project | Used for |
|---|---|
| [TM Vibrant Shaders](https://github.com/cheatoskar/TM-Vibrant-Shaders) (TMVS) | D3D9/D3D11 proxy, engine hook tables, rendering, ManiaPlanet port |
| [eswc-autosave](https://github.com/cheatoskar/eswc-autosave) | ESWC addresses, inline/IAT hooks, dinput8 loader, TLS 1.0 compatibility fix |
| [100-TMX-Bingo-Plugin](https://github.com/cheatoskar/100-TMX-Bingo-Plugin) | TMNF state offsets, pointer chains, D3D9 overlay hook, TMLoader mod contract |
| [100-TMX-Server](https://github.com/cheatoskar/100-TMX-Server) (fork of randomizer-anywhere) | TMF/2006 dedicated server, XML-RPC, TMX API usage |
| [DeltaZero](https://github.com/cheatoskar/DeltaZero) | ghost/replay parsing, TMInterface integration |
| [TrackManiaForeverOpenXR-TMVS](https://github.com/cheatoskar/TrackManiaForeverOpenXR-TMVS) (fork of TMFOXR) | VR mod notes |
| [TMTrackNN2](https://github.com/cheatoskar/TMTrackNN2) (rebuild of TMTrackNN; not published yet) | map generation, map writing, block flags, dataset quirks (`tmtracknn2` in [docs/map-generation.md](docs/map-generation.md)) |

Unpublished (cited by name only): physics RE notes (`tm-physics-re`, `physics-model6.md`, `bugslide.md`), the TM Horizon launcher, the 100% TMX website code, Ghidra workspace notes, ESWC replay RE notes.

## Community projects and documentation

| Source | Licence | Used for |
|---|---|---|
| [TMNF-C](https://github.com/adonis-singh/TMNF-C) | MIT | TMNF physics addresses and annotations, Ghidra pipeline |
| [ForeverValidator](https://github.com/Skycrafter-dev/ForeverValidator) | MIT | physics cross-checks (substeps, caps, car file values) |
| [TrackManiaForeverOpenXR](https://github.com/jiink/TrackManiaForeverOpenXR) (TMFOXR) | MIT | VR mod, TMUF layouts, LAA patch |
| [TwinkieForever](https://github.com/TwinkieTweaks/TwinkieForever) (Twinkie) | MIT | published TMNF offsets reused by the Bingo plugin |
| [tmnf-plugins](https://github.com/Delorean12DMC/tmnf-plugins) | GPL-3.0 | public copy of `TmForever.map` (TMNF 4d431494); only names/addresses derived from it are used here, the file itself is not included |
| [randomizer-anywhere](https://github.com/BigBang1112/randomizer-anywhere), [nostalgic-completionist](https://github.com/BigBang1112/nostalgic-completionist) | AGPL-3.0 / MIT | server controller base, old-game LAN driving |
| [ManiaAPI.XmlRpc](https://github.com/BigBang1112/maniaapi-net), [GBX.NET](https://github.com/BigBang1112/gbx-net) | see repos | XML-RPC client behaviour, GBX format cross-checks |
| [TMInterface](https://donadigo.com/tminterface), [TMInterfaceClientPython](https://github.com/donadigo/TMInterfaceClientPython) | see sites | offline TAS tooling, script format |
| [Competition Patch](https://donadigo.com/tmcp) | see site | public description only |
| [TMLoader / ModLoader](https://tomashu.dev/software/tmloader) | closed source | mod loading, product/profile layout |
| [TMUnlimiter](https://unlimiter.net/) | closed source | coexistence of hooks |
| [TrackMania Exchange API](https://api2.mania.exchange) | | TMX endpoints and fields |
| [TMTrackNN](https://github.com/donadigo/TMTrackNN), [pygbx](https://github.com/donadigo/pygbx) | GPL-3.0 | map generation approach, block tables, GBX reading (`dona` in docs/map-generation.md) |
| [trackmania-community-tracks-and-telemetry](https://huggingface.co/datasets/jeanmidev/trackmania-community-tracks-and-telemetry) | CC BY 4.0 | dataset facts in docs/map-generation.md |

## Tools

[Ghidra](https://github.com/NationalSecurityAgency/ghidra) 12.1.2, [GhidraMCP](https://github.com/LaurieWired/GhidraMCP) and its fork by bethington, [Capstone](https://www.capstone-engine.org/), [pefile](https://github.com/erocarrera/pefile), Python, x64dbg-style debugging. Parts of the extraction and cross-checking were done with the help of an AI coding assistant (Claude); all facts were curated and spot-checked as described above.
