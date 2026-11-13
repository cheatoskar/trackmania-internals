# Cpu/Gpu synchro (`GpuSync`): the game waits for the GPU after every frame

TMNF, TMUF, Nations ESWC, Sunrise and Original (both engine generations) limit how far the CPU may run ahead of the GPU
with a ring of Direct3D 9 event queries. The setting is `CSystemConfigDisplay::GpuSync`, shown as **Cpu/Gpu synchro** in
each game's launcher (advanced display settings). Its default, **Immediate**, makes the CPU wait after every `Present`
until the GPU has finished that very frame: CPU and GPU take turns, and the frame time is the CPU's time plus the GPU's.
That matters for any mod that adds GPU work (post-processing, extra passes): under Immediate all of it lands on the frame
time.

Builds (identify by SHA-256, see the [README](../README.md#coverage)): TMNF 2.11.26 `3847cf9f...`, TMUF 2.11.26
`4b6a7b31...`, Nations ESWC 1.80 community `983bd127...`, Sunrise repack `e59dfc7e...`, Original Online Patch v2
`75e129d7...`. Addresses are link-time VAs (image base 0x00400000). TMNF names from the public `TmForever.map`. Method:
capstone disassembly and call/byte searches over the exes (own `tm-re` scripts), TMNF names cross-checked with the map.

## The mechanism

| fact | value | source | status |
|---|---|---|---|
| functions (TMNF) | `CVisionViewportDx9::SyncGpuInit(count, half, timeout)` 0x0098BAB0 (vtable slot +0x134), `SyncGpuIssueQuery` 0x0098B780, `SyncGpuWaitQuery(doWait)` 0x0098B890 | TmForever.map + disassembly | verified |
| ring | `SyncGpuInit` makes a ring of `count + 1` entries (`count` >= 256: no ring, no wait); TMNF viewport +0x930 the ring (CFastBuffer: count, data; each entry: event query, fallback surface, issued flag, time stamp at +0x10), +0x93C the issue index, +0x940 the wait index, +0x944 the "half" flag, +0x948 the time-out | disassembly of SyncGpuInit / IssueQuery / WaitQuery | verified |
| issue | after `Present` (RenderFrameEnd 0x009A4704 calls Present, 0x009A47FA calls `SyncGpuIssueQuery`): creates the entry's `D3DQUERYTYPE_EVENT` query on first use, `Issue(D3DISSUE_END)`, stamps the time, advances the issue index; without event queries (global flag 0x00D777AC bit 1 clear) a `ColorFill` of a 16x16 surface instead | disassembly | verified |
| wait | waits on the entry at the wait index only when it is issued **and** the issue index has come round to it (the ring is full): with a ring of `count + 1` that is the query issued `count` frames ago; polls `GetData(.., D3DGETDATA_FLUSH)` with `CSystemEngine::Sleep(0, 1)` and `HighFrequencyRun` between polls; logs `[Dx9] Cpu/Gpu Sync not responding after N ms !!` after 200 ms (500 polls) and drops the query; the fallback locks the 16x16 surface | disassembly | verified |
| where it waits | `RenderFrameEnd` calls `SyncGpuWaitQuery(half == 0)` right after the issue (0x009A480D); `RenderFrameBegin` calls `SyncGpuWaitQuery(half == 1)` (0x009A0ACB): with `half` set the wait moves to the next frame's start, after the game's logic, input and physics ran | disassembly | verified |
| setting -> ring | `CHmsViewport::ApplySystemConfig` 0x0052E634: `switch (display->GpuSync)` (+0x8C) to `SyncGpuInit(count, half, display+0x90 time-out)`: 0 None (count -1), 1 "3 Frames" (3, 0), 2 "2 Frames & Half" (2, 1), 3 "2 Frames" (2, 0), 4 "1 Frame & Half" (1, 1), 5 "1 Frame" (1, 0), 6 "Half a frame" (0, 1), 7 "Immediate" (0, 0); jump table 0x0052ECFC; names `|GpuSync|...` at 0x00B30DF0.. | disassembly | verified |
| default | `CSystemConfigDisplay::SetDefaultsDisplay` 0x004380B0 sets GpuSync 7 (Immediate), GpuSyncTimeOut 10000 | disassembly | verified |
| TMUF | the same code: `SyncGpuWaitQuery`'s first 16 bytes (ring at +0x930) and RenderFrameEnd's issue/wait call site (flag at +0x944) are byte-identical to TMNF's | byte comparison | verified |
| Sunrise | ApplySystemConfig's switch at 0x004386C1 (GpuSync at display +0x80, the same mapping, jump table 0x00438D40, `call [vtable+0xF8]`); `SyncGpuIssueQuery` 0x006DCA20, `SyncGpuWaitQuery` 0x006DCB30; viewport +0x87C the ring (count, data +0x880), +0x888 issue index, +0x88C wait index, +0x890 the half flag; RenderFrameEnd issues and waits at 0x006CF6F6 / 0x006CF70B, RenderFrameBegin waits at 0x006C9394 | disassembly | verified |
| Original | `SyncGpuWaitQuery` 0x006DBEC0 with Sunrise's layout (+0x87C ring, +0x888 / +0x88C indices), half flag +0x890 read before the RenderFrameEnd call 0x006CEA5B; RenderFrameBegin's call 0x006C86E4 | disassembly | verified |
| Nations ESWC | `SyncGpuWaitQuery` 0x007F1FD0: ring +0x844 (data +0x848), issue +0x850, wait +0x854, half flag +0x858 (read at 0x007E482B before the RenderFrameEnd call 0x007E483B); RenderFrameBegin's call 0x007DE23A | disassembly | verified |

## Where the setting is stored

| game | file | how | status |
|---|---|---|---|
| TMNF / TMUF | `<Documents>\TrackMania\Config\Default.SystemConfig.Gbx` | binary GBX with a compressed body (`BUCR`): change it in `TmForeverLauncher.exe` (its advanced settings carry the *Cpu/Gpu synchro* combo box; the strings `Cpu/Gpu Synchro` and `|GpuSync|...` are in the launcher) | verified (file format, launcher strings) |
| TMNF chunk | `CSystemConfigDisplay::Chunk` 0x004382C0, chunk 0x0B013003: DoBool +0x98 (DisableShadowBuffer), DoNatural +0x8C (**GpuSync**), DoBool +0x9C (EmulateCursorGDI), DoNatural +0x88 (VertexProcess), DoBool +0xA0 (OptimizePartDyna), DoBool +0xA4 | disassembly | verified |
| ESWC, Sunrise, Original | `<game dir>\Default.SystemConfig.Gbx`, a **text** GBX (`GBX..TUUE`, one value per CRLF line, each chunk: id, `1397442896` ("PIKS" skip marker), byte count, values). Chunk `b013003` holds the same six values; the second is GpuSync (`7` in every install looked at). Changing `7` to `5` keeps the chunk's byte count. | Sunrise `Chunk` handler 0x00475F8C: DoBool +0x8C, DoNatural +0x80 (GpuSync), DoBool +0x90, DoNatural +0x7C (VertexProcess), DoBool +0x94, DoBool; ESWC and Original: same file layout | verified (Sunrise code; ESWC/Original by the identical file layout: inferred) |

## Effect on frame time

| fact | value | source | status |
|---|---|---|---|
| Immediate | after frame N's Present the CPU waits until the GPU has finished frame N; the GPU is then idle while the game runs logic and issues frame N+1 until its first draws reach the GPU: frame time = CPU time + the GPU's time after the last command | follows from the mechanism | inferred |
| 1 Frame | waits for frame N-1: the GPU finishes frame N while the CPU works on frame N+1; frame time close to max(CPU, GPU), one frame more input delay | follows from the mechanism | inferred |
| Half a frame | waits for frame N at the start of frame N+1's rendering: only the game's logic/physics between frames overlaps the GPU | follows from the mechanism | inferred |
| a shader mod under Immediate | Sunrise, GTX 1080 Ti, 2560x1440, TM Vibrant Shaders: 48 FPS (20.86 ms a frame) with ~1.9 ms GPU for the game's 3D scene and ~6.2 ms for the mod's passes; the GPU's Present-to-Present span had 14.7 ms outside those (other game passes and idle). The mod's 6 ms of GPU work lands fully on the frame time under Immediate | `tmvs` perf log line (one 2000-frame window) | verified (measurement) / inferred (cause) |
| what a mod can do | nothing inside the frame: the wait covers everything queued before Present, wherever in the frame the mod issued it. The setting is the lever (launcher or config file); reading it at run time: the viewport's ring count and half flag above (`tmvs` logs it as "Cpu/Gpu synchro") | `tmvs` src/engine.cpp `gpuSync()` | verified |
