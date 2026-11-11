# ManiaPlanet / TrackMania 2 (ManiaPlanet.exe x64)

Exe: `ManiaPlanet.exe` 31,314,016 bytes, PE timestamp 5dd42b7c (2019-11-19), image base 0x140000000, SHA-256 4af0e34a... (measured for this repo). No RTTI. Addresses are RVAs unless written as 0x14... VAs.

Extra facts from own RE notes (found with Python + capstone on reflection tables, no Ghidra import; status verified as used by maniaplanet/src/maplook.cpp):

| Item | Value |
|---|---|
| Member reflection tables in .data | entry = u32 type, u32 member id (class id prefix), ptr, u64 offset (0xffffffff = getter only), name ptr, desc ptr, flags; node members carry the target class id |
| Class registration pattern | `mov edx, classId; lea r9, "Name"; [rsp+28h] = object size; call 0x1402863c0` |
| CGameCtnApp more members | EditorBase +0x210, GameScene +0x2D8, GameCamera +0x2E8, CurrentPlayground +0x3F8, Switcher +0x7F0 |
| CGameCtnChallenge vtable | RVA 0x16D4258 |
| Wide-string assign arg | rdx = 16-byte aligned {ptr, len, 0} |

Source keys (short names for own project sources, not part of this repo): `tmvs` = [TM Vibrant Shaders](https://github.com/cheatoskar/TM-Vibrant-Shaders) mod source; `tmvs-mp` = its ManiaPlanet port; `tmvs[X]` = feature branch X of it; `tmvs-notes` = its notes; `phys` = own physics RE notes; `tmnfc` = TMNF-C (third-party MIT reimplementation, github.com/adonis-singh/TMNF-C); `dz` = [DeltaZero](https://github.com/cheatoskar/DeltaZero); `xr` = TrackManiaForeverOpenXR (upstream jiink/TrackManiaForeverOpenXR; own fork [TrackManiaForeverOpenXR-TMVS](https://github.com/cheatoskar/TrackManiaForeverOpenXR-TMVS)); `tm-re` = own RE tools. Status: verified = seen in code/data or measured; inferred; unverified = prose only.

## Engine facts

| fact | value | source | status |
|---|---|---|---|
| renderer | deferred HDR; reversed float Z (`CDx11Viewport.ZBuffer_Near1_Far0`, `ZBuffer_Float`); shaders in `Packs\GpuCache_D3D11_SM5.zip` (1098 entries, GBX-wrapped DXBC with RDEF), `..._SM4` variants also exist | engine-options.md | verified (stated measurement) |
| shader DB | 6419 pixel shaders keyed by DXBC checksum -> name + role (tmvs-mp tools\gen_shaderdb.py, shaderdb_table.inc 6990 lines) | maniaplanet\README.md | verified |
| key shader names | G-buffer `Tech3/*_DefWrite`, `Block_TDSN_DefWrite_p`, `Block_DefReadP1_p` (lighting), `DeferredDeCompFaceNormal_p`, `DeferredShadowPssm_p` (48 variants 1-5 splits, ± opacity/sprite map, SM5/SM4; five-split variant checksum 30a64492), `ToneMapGlobalFilmCurve`, `FxBloom_Final`, `TcAutoScale` (2x2 auto exposure), self-illum `TMapSelfIllum`/`TMapAdvert`/`TMapIllum`, additive `Tech3_Block_TAdd`, "Tech3 Block DefReadP1 SelfIllum", `PHLSL/RasterBitmap.PHlsl` (UI) | README.md | verified |
| G-buffer binding names | `TMapDeferredPixelNormalInC`, `TMapDeferredMDiffuse` (= albedo), `TMapDeferredMSpecular`, `TMapDeferredLightMask`, `TMapMotion` (2nd RT R16G16 float = motion vectors) | README.md | verified |
| tone map | Hable filmic per channel: `x=min(hdr,50000)*exposure; f=(x(Ax+BC)+DE)/(x(Ax+B)+DF)-E/F; out=f*OutScale+(f.r+f.g+f.b)*Gray`; mod's rebuild is 1.4/255 off game image | README.md | verified (measurement in doc) |
| soft shadows | engine pass: linear depth t0, two static maps t1,t2 (4096^2, ~1 m/texel), split atlas t3 6400x1280 (five 1280^2 splits, coarsest ~370 m, finest ~11 m), face normal; constants `ShaderP` 11+5*splits registers; writes lit^2 into light mask; mod replaces with PCSS (~1 ms @1440p GTX 1080 Ti vs engine 0.2 ms) | README.md | verified |
| moon lighting formula | Block_DefReadP1_p: diffuse = light*N.L*shadow; glint = light*N.L*shadow*(N.H)^n*n*MSpecular.x^2*SpecScale*w, n=MSpecular.w*SpecMaxExp+1; all x2 x cloud factor | README.md | verified (rebuilt in mod) |
| D3D11 runtime quirk | runtime rewrites a context's vtable when the game enables multithread protection and writes draw slots back on Present: hooks re-checked/re-patched around every Present and before shader/RT sets | README.md | verified |
| hooking pattern | d3d11.dll proxy (src\proxy.cpp, forward.asm, d3d11.def) next to ManiaPlanet.exe, 64-bit only; hooks device/context/swap-chain vtables; first interface draw found by shader checksum -> mod's pass runs before it; fallback at Present | README.md | verified |
| Openplanet | MP4 plugin 1.29.14 on openplanet.dev/download/v4; an Openplanet 1.16.8 class dump `Openplanet4.json` = class dump of 1717 classes with member names; signed vs developer mode (developer mode turns on school mode which blocks online) | engine-options.md | verified (stated) / unverified (online rules) |

## ManiaPlanet.exe addresses / offsets (x64, RVA unless stated; last build 31,314,016 B)

| item | value | meaning | source | status |
|---|---|---|---|---|
| kAppGlobal | RVA 0x1C8B410 | global holding CTrackMania* (set by CGameCtnApp base ctor) | tmvs[tm2-full]\...\game.cpp:13 | verified |
| class ids | CTrackMania 0x24001000 <- CGameManiaPlanet 0x03013000 <- CGameCtnApp 0x030D3000 | app chain | game.cpp:14 | verified |
| CGameCtnApp::RootMap | +0x378 (member 0x030D3005); ::Editor +0x7C8 (0x030D300A) | current map; editor | maplook.cpp:15-17 | verified |
| editor classes | CGameCtnEditorCommon 0x0310E000 (+0x410 Challenge); Free 0x0310F000, Puzzle 0x03110000, Simple 0x03115000; MediaTracker 0x030B1000 | map editor vs replay editor | maplook.cpp:15-33; game.cpp:17 | verified |
| CGameCtnChallenge | id 0x03043000, size 0x748; Comments (0x03043003) +0xE0 wide string {wchar_t*, u32 len}; MapName +0xD0; AuthorLogin +0xF0 | map comments (Author's Shader) | maplook.cpp:24-33; game.cpp:137-139; tm2-overlap.md | verified |
| kWStringAssign | RVA 0x142E50, prologue `48 89 5C 24 10 48 89 6C 24 18 57 48 83 EC 30 0F 28 02` | engine's wide-string assign, used to write comments | maplook.cpp:29,37 | verified |
| CGamePlayground | id 0x0300D000; ReplayRecord +0x1E8, PrevReplayRecord +0x1F0 (CGameCtnReplayRecord 0x03093000; reflection static init RVA 0xD7780); GameTerminals +0xA00 (buffer ptr,count) | replay tracking | game.cpp:18-21,128 | verified |
| terminals / player | CGameTerminal 0x03003000, TM terminal 0x24032000, ControlledPlayer +0x18; CTrackManiaPlayer 0x24031000: RaceState +0x2D0, NbRespawns +0x458, checkpoint times +0x460 (ptr,count), DisplaySpeed +0x3CC; getter CurCheckpointRaceTime at VA 0x140EF9D14 | race state | game.cpp:128-136 | verified |
| GetMwClassId | vtable slot 3: `mov dword [rdx], id; mov rax, rdx; ret` | class check before reading any object | maplook.cpp:34-35 | verified |
| camera | CGameCamera 0x03023000 (+0xF0 SceneCamera), CSceneCamera 0x0A00A000 (+0x48 CHmsCamera), CHmsCamera 0x06001000: FovRect +0x2F0 (min x,y, max x,y), FovRectEnable +0x30C, DrawRect +0x190 (slot +0x28) | zoom / tiles / game camera | gamecamera.cpp:12-20 | verified |
| camera code RVAs | CHmsCamera::SetFovRect RVA 0x1CE940 (`48 8B 02 48 89 81 F0 02 00 00 ...`), SetFovRectEnable 0x1CE960 (`89 91 0C 03 00 00 C3`), frustum check of FovRectEnable at 0x1CEC17 (`41 83 B9 0C 03 00 00 00`) | | gamecamera.cpp:22-28,54-56 | verified |
| block registrations | CGameCtnEditorSimple/Puzzle/Free registrations VA 0x1400ED7BF / 0x...85F / 0x...80F | | maplook.cpp:21 | verified |
| block array in map | offset of block array / block fields NOT known | tm2-overlap.md | verified (as unknown) |
| engine anti-cheat | whether a replaced shader / d3d11.dll proxy is tolerated online: unknown | README.md | unverified |

## Constant buffers read by the mod (SceneV vertex slot 13, SceneP)

| buffer | size | offsets | source | status |
|---|---|---|---|---|
| SceneV (vertex cbuffer slot 13) | 704 B | ShadowLDir 80 (world->sun shadow, 4 rows, row 2 along sun light), LightDirRgb0 224 (sun colour), LightAmbient 240, LightAmbientLinear 256, CameraProjection 432, CameraMinMaxZ 496, WorldToCamera 512, WorldPrCamera (world->clip) 560, EyeInWorld 624 | tmvs-mp\maniaplanet\src\engine_frame.h:12-21 | verified |
| SceneP | 672 B | Fog 0, RasterPos 96, LightAmbientLinear 128, LightDirRgbLinear0 144, LightDirInWorld0 160, LightGenSpec 176, CloudsMin 304, CloudsMax 320, DayTime 464 (GbxP_DayTime: x = map time of day 0..1, 0.5 noon) | engine_frame.h:26-36,96 | verified |
| tone-map cbuffer 0 | bound as b2; read directly (no read-back) | | README.md | verified |
| layouts from | `tools/rdef.py` (reads RDEF reflection from DXBC) | | README.md | verified |

TM2 differences from TMNF (rendering): deferred HDR with own SSR, PSSM shadows, volumetric fog, auto exposure, film curve; real normals/motion available; no world-matrix problem; no long-range height-map shadows, no TAA jitter, no hi-res photo/zoom in the early port (later worktree adds FovRect-based zoom/tiles: tmvs[tm2-full] commit "TM2 Game camera: the game's FovRect always put back when zoom, tiles and ..."). Time of day cannot be changed in TM2 (moon brought down by the mod itself). Online rules for d3d11.dll mods unconfirmed. Source: tmvs-mp\maniaplanet\README.md; engine-options.md. status verified (docs) / unverified (online).
