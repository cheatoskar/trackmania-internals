# Frame loop, timer, Present and GPU sync (TMNF and the 2006 engine)

How one frame is made in TMNF and in the 2006-engine games (Nations ESWC, Sunrise, Original): which thread does
what, which clock the game uses, where `Present` is called and how the engine waits for the GPU, and when shaders
and textures are created. Useful for frame pacing work, profilers and anything that hooks `Present`.

Builds (identify yours with [../scripts/re/pe_info.py](../scripts/re/pe_info.py); see the [README](../README.md#coverage)):
TMNF `4d431494` (`3847cf9f...`), ESWC unofficial 1.80 `5ce7272f` (`983bd127...`), Sunrise community repack
`5cc8f0cf` (`e59dfc7e...`), Original Online Patch v2 `5d337477` (`75e129d7...`). Addresses are link-time VAs
(image base 0x00400000).

Method: Ghidra 12.1.2 headless with the public TMNF map names (decompiled), capstone sweeps of the 2006 exes
(import call sites, string references, device-vtable calls) compared with the TMNF structure. Static only, nothing
run. `verified` = seen in that exe's code; `inferred` = same structure as verified TMNF code, or a consequence.

## Thread and main loop

| Fact | TMNF | ESWC | Sunrise | Original | Status |
|---|---|---|---|---|---|
| One thread runs window messages, game logic, physics and rendering. `WinMain` loops `PeekMessage(PM_REMOVE)`; with no message it calls `CGbxApp::MainLoop`. | `WinMainInternal` 0x00529C00, `MainLoop` 0x0052A8B0 | loop 0x00426390, MainLoop 0x00425320 | loop 0x0043DFF6, MainLoop 0x00411630 | loop 0x0043E1D0, MainLoop 0x00411660 | verified (TMNF decompiled; 2006 loops disassembled, MainLoop has the same call shape) |
| Sleep in the loop | `Sleep(7)` minimised, `Sleep(20)` when the window is not focused, none in front | `Sleep(0)` after every frame, no background sleep: the loop is rewritten in this build (the `PeekMessage` call moved into a stub at 0x00426448) | as TMNF | no sleep at all (rewritten like ESWC, without the `Sleep(0)`) | verified |
| `CMwCmdBufferCore::Run` (TMNF 0x00922DB0): ticks the timer, runs the fixed-period "scheme" command buffers in a catch-up loop (at most 1000 steps per frame; the 100 Hz simulation is one of them), then the "async" buffers once per frame (rendering). | yes | — | — | — | verified TMNF, inferred 2006 |
| "High frequency" commands (input, audio, video) are not a thread: `CMwCmdBufferCore::HighFrequencyRun` (0x00922B00, throttled by QPC) is called from inside long work on the same thread: mip-map generation, texture upload, `PrecalcLighting`, the GPU wait. | yes | — | — | — | verified TMNF (xrefs) |
| Other threads: network check, sea scene, Bink video, OpenAL (TMNF); network and Bink (2006). | | | | | verified (`CreateThread` sites) |

So the thread that calls `Present` is the game thread. Thread-level scheduling (CPU sets, MMCSS, priority) on that
thread covers render, physics and logic at once.

## Timer

| Fact | Status |
|---|---|
| Game clock `CMwTimer`: QueryPerformanceCounter (`GetCurrentCounter0` -> `CMwProfiler::GetTimeStamp`); it switches to `timeGetTime` (`GetCurrentCounter1` 0x00939E00) only if a 90 s calibration fails (QPC vs `timeGetTime` vs `GetTickCount`, 5 % tolerance; `CMwTimer::Tick` 0x0093A670, `CMwTimer_CalibrateEnd_ShouldSwitchOff` 0x00434100, log line `[Timer] SystemTime = ..., PerfCounter = ...`). The same calibration exists in ESWC (0x0062A490). | verified TMNF, ESWC |
| `CMwTimerAdapter::Resync` (0x0093A840) moves the adapter's "human" time half way towards `timeGetTime()` every frame, so its granularity follows the system timer resolution. | verified TMNF; effect inferred |
| `MwSystemTimerInit` calls `timeGetDevCaps` + `timeBeginPeriod(wPeriodMin)` (normally 1 ms): TMNF 0x00939D60 and Sunrise 0x00486D70 (Sunrise logs `[MwTimer] Bad multimedia timer performance` if the minimum is above 5 ms). | verified |
| **ESWC 0x0049BD80 and Original 0x00486DC0 never call it**: in both community builds `push ecx; call [timeBeginPeriod]` is replaced by `xor eax,eax; mov ecx,<IAT slot>` (same length), and the same for `timeEndPeriod`. The game believes the call succeeded. | verified |
| Consequence: on ESWC and Original, unless another process asks for a finer timer, `timeGetTime` steps in about 15.6 ms and `Sleep(n)` rounds up to it. A `timeBeginPeriod(1)` from outside restores what the original code did. | inferred |

## Present and the GPU

| Fact | TMNF | ESWC | Sunrise | Original | Status |
|---|---|---|---|---|---|
| `CVisionViewportDx9::RenderFrameEnd`: `EndScene` (slot 42), then `Present(NULL, NULL, NULL, NULL)` (slot 17); `D3DERR_DEVICELOST` -> release and `OnDeviceLost` | 0x009A4704 (`call edx`) in 0x009A4070 | 0x007E4632 | 0x006CF4D2 | 0x006CE822 | verified |
| The 2006 engine times `Present` with QPC and logs `[Dx9] D3dDevice->Present() has taken N ms !` above 50 ms. | — | yes | yes | yes | verified |
| **GPU sync right after Present**: `SyncGpuIssueQuery` issues an event query into a ring (`CreateQuery(D3DQUERYTYPE_EVENT)`, `Issue(END)`; without queries: `ColorFill` into a 16x16 surface and lock it), `SyncGpuWaitQuery` polls the entry from N frames ago with `GetData(..., D3DGETDATA_FLUSH)`, `Sleep(0)`/`SleepEx(0)` and a high-frequency run, and logs `[Dx9] Cpu/Gpu Sync not responding after N ms` after 200 ms. Ring size from `SyncGpuInit(count, ...)`; the wait is switched by a viewport flag (setting `CSystemConfigDisplay.GpuSync`, +0x8C on TMNF). | issue 0x0098B780, wait 0x0098B890 | 0x007F1EC0 / 0x007F1FD0 | 0x006DCA20 / 0x006DCB30 | 0x006DBDB0 / 0x006DBEC0 | verified (TMNF decompiled; ESWC and Sunrise disassembled; Original by the same call order after `Present`) |
| With the sync on, the CPU never runs more than N frames ahead of the GPU: lower input delay, less CPU/GPU overlap. An external render-ahead limiter duplicates it. | | | | | inferred |

## Shaders and textures created during play

| Fact | Status |
|---|---|
| TMNF compiles HLSL at run time with `d3dx9_30!D3DXCompileShader` (single call site in `CompileInternal` 0x008DEAF0; asm shaders via `D3DXAssembleShader` from `VertexShaderCompileAndCheck` / `PixelShaderCompileAndCheck`). `CPlugGpuCompileCache::CacheOrCompileHR` (0x008DEFF0) does not cache in this build: it always compiles. | verified |
| The compile happens when a shader is **bound for the first time**: `CDx9ShaderKeeper::Undirty` (virtual, 0x00970A60, +0xB3C) and `CDx9PixelShader::UndirtyAndSetPixelShader` call `CDx9PixelShader::Compile` (0x00962B60). Nothing compiles all materials of a map up front. | verified (call chain); hitch size not measured |
| 2006 engine: D3DX is linked statically (no `d3dx9_*.dll` import; D3DX error strings, HLSL compiler messages and the profile table at ESWC 0x0073CAC8). The shader's compile method (ESWC 0x00581220, Sunrise 0x00703620, Original 0x007028A0; logs `Compilation of EntryPoint "...` and `Shader compilation error` when every profile failed) calls the linked `D3DXCompileShader` (ESWC 0x0087AA6B, Sunrise 0x008E3292, Original 0x008DFD8F). ESWC 0x00580A40 is the step after it (samplers, semantics, constants from the bytecode), not the compile itself. Details: [shader-compile-2006.md](shader-compile-2006.md). | verified (ESWC/Sunrise decompiled, Original compared); bind-time path inferred |
| Textures: `CDx9TextureKeeper::Undirty` -> `UpLoadTexture` uploads on first use; `CVisionViewportDx9::TextureMemoryMipFree` drops large mip levels when the video-memory budget is exceeded, and they are uploaded again later. | verified (names, TMNF); hitches inferred |
| Map load runs on the game thread: `CSystemArchiveNod::LoadResource`, lightmap precalc (`PrecalcLighting`, which keeps input/audio alive through high-frequency runs); scene resources load when a corpus enters the scene (`OnEnterScene` -> `CHmsViewport::LoadResourceCorpus`) and zones every `RenderFrameBegin` (`LoadResourceZone`). | verified (call sites), order inferred |

A shader warm-up from outside the engine is therefore not possible at the Direct3D level (`CreatePixelShader`
only receives finished bytecode); it needs a cache in the compile path or engine calls during loading. For the
2006 engine's compile path (the linked D3DX functions, the include handler and what a cache there must respect) see
[shader-compile-2006.md](shader-compile-2006.md).
