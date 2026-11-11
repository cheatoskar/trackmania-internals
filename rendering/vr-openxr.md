# VR in TMNF/TMUF: D3D9 to OpenXR (TrackMania Forever OpenXR)

How the VR mod TrackMania Forever OpenXR (TMFOXR) turns the game's Direct3D 9 frame into a stereo OpenXR image, what that means for the camera and projection, how it hands the game device to a second rendering mod (TM Vibrant Shaders), and the pitfalls found on the way. Installation, settings files, cockpit offsets and the LAA patch: [../modding/vr-tmfoxr.md](../modding/vr-tmfoxr.md). The TMUF/TMLoader RVAs it hooks: [../games/tmuf/README.md](../games/tmuf/README.md#layouts-from-the-vr-mod-rvas-add-module-base). The game's D3D9 pipeline in general: [tmnf-d3d9.md](tmnf-d3d9.md).

**Credit:** [TrackManiaForeverOpenXR](https://github.com/jiink/TrackManiaForeverOpenXR) by jiink (MIT). Its README states that the code is LLM-generated. Facts about its internals are described from reading `src/d3d9_proxy.cpp` and `src/vr_bridge.cpp` (upstream `master`, November 2026); no code is copied. The device hand-over to TM Vibrant Shaders was written by [wial663](https://github.com/wial663) and brought upstream with my review fixes as [pull request #27](https://github.com/jiink/TrackManiaForeverOpenXR/pull/27) (merged 2026-11-10); my fork is [TrackManiaForeverOpenXR-TMVS](https://github.com/cheatoskar/TrackManiaForeverOpenXR-TMVS).

Source keys: `xr` = TMFOXR upstream source; `xr-pr27` = the hand-over commits in PR #27; `xr-own` = my later TMFOXR patches (not yet published; each row says what was tested); `tmvs` = [TM Vibrant Shaders](https://github.com/cheatoskar/TM-Vibrant-Shaders) 1.4.3 (CHANGELOG, `src/hook.h`, `docs/settings.md`). Status: **verified** = seen in code or measured in a headset; **inferred**; **unverified** = untested hypothesis.

## Targets and loading

| fact | value | source | status |
|---|---|---|---|
| games | 32-bit Steam TMUF 2.11.26 (`4b9e8add`) as a `d3d9.dll` proxy, and TmForever under TMLoader 2.12.0 / 2.12.0-compat (Nations and United) as a TMLoader product that redirects the game's d3d9 imports | xr README, `d3d9_proxy.cpp` | verified |
| layout detection | two hard-coded layouts ("Steam TMUF 2.11.26", "TrackMania ModLoader 2.12.0/2.12.0-compat"), chosen by checking that a CTrackMania vtable slot points at the expected function; unknown exes run without the camera-3 and culling hooks | xr | verified |
| TMNF standalone | no own layout; since the TMLoader exe's `.text` equals TMNF 2.11.26, the TMLoader RVAs should also be the TMNF `4d431494` RVAs | xr; README of this repo (`.text` compare) | inferred |
| OpenXR | dynamically loaded Win32 `openxr_loader.dll` (Khronos 1.1.62), requests API 1.0, needs `XR_KHR_D3D11_enable`; the 32-bit game reads the runtime from `HKLM\SOFTWARE\WOW6432Node\Khronos\OpenXR\1\ActiveRuntime` (a runtime registered only for 64-bit is not found) | xr README | verified (doc) |
| game settings | windowed only, D3D9 MSAA off, window not larger than the monitor's work area: otherwise a dialog and no VR hooks | xr README | verified (doc) |

## How the stereo image is made

| fact | value | source | status |
|---|---|---|---|
| principle | no second render of the scene by the engine: every **perspective** draw call the game issues is replayed by the proxy, once per eye, into a private packed eye target (width 2 x the runtime's recommended eye width, left half / right half) with its own depth; the original desktop draw is kept or skipped (`MirrorEyeToDesktop`) | xr `DrawIndexedPrimitiveHook` and siblings | verified |
| what is replayed | a draw is replayed when a perspective projection is active and an eye pair exists for the current colour target; the target must have the primary scene's size and format; at most 16 scene targets are cached | xr `CanReplayStereoDraw`, `EnsureStereoEyeColor` | verified |
| perspective vs UI | a `SetTransform(PROJECTION)` with `|_34| > 0.5` is a camera (D3D left/right-handed perspective has +-1 there); orthographic UI projections have 0 | xr `vr_bridge.cpp` | verified |
| fixed-function draws | the eye's view and projection go in through `SetTransform`, restored afterwards | xr `SetFixedFunctionEyePose` | verified |
| shader draws | the game's vertex shaders take the camera from constants, not from `SetTransform`: each new vertex shader is disassembled once (`dp4 oPos.x` / `m4x4` patterns) to find the register of its combined matrix; per draw the head pose is multiplied into that constant for each eye. Up to 256 shaders are analysed and kept referenced, because the game destroys and recreates material shaders between tracks and a new shader may reuse an old pointer | xr `AnalyzeVertexShader` | verified |
| unmapped shaders | a few unusual vertex shaders are not camera-mapped and stay head-locked (Island boulders named in the README) | xr README | verified (doc) |
| screen-space sprites | pre-transformed (`XYZRHW`) additive fixed-function sprites drawn during the 3D pass are not replayed per eye | xr `IsDesktopSpaceAdditiveSprite` | verified |
| UI layer | outside perspective passes, alpha-blended draws (`SRCALPHA` or `ONE` / `INVSRCALPHA`, add) to the primary target are drawn a second time into a UI surface, shown as a world-locked quad 2 m in front of the first head pose; after a 3D pass the game makes an opaque full-screen copy, which is scene presentation, not UI | xr `CanCaptureUiDraw`, README | verified |
| transfer to OpenXR | OpenXR cannot take D3D9 surfaces. With `D3D9On12=1` (default) the game's device is created through D3D9On12 and a D3D11On12 device on the same D3D12 device wraps the eye textures, so they stay on the GPU; otherwise (or when the adapters differ) the eyes are read back to system memory and uploaded to D3D11 swapchains. A legacy D3D9 device cannot use D3D9Ex shared textures | xr `vr_bridge.cpp`, README | verified |
| timing | head pose from OpenXR's predicted display time, so runtime reprojection can steady lower game frame rates; the first valid head pose is the origin (restart to recenter), jumps > 0.5 m count as an origin change unless `RecenterOnTrackingJump=0` | xr README | verified (doc) |

## Camera and projection

| fact | value | source | status |
|---|---|---|---|
| reflected X | TrackMania's camera projection has a **negative horizontal scale** and a downward Y basis in its camera constants. An eye projection must keep the sign of each axis, otherwise winding flips and backface culling turns the world inside out; the head rotation is reflected through Y to keep pitch and fix yaw/roll handedness; the cockpit X offset is negated | xr `EyeProjection`, `HeadViewMatrix`, settings comment | verified |
| eye projection | asymmetric frustum from the runtime's per-eye FOV tangents; near/far mapping taken from the game's projection (`_33`, `_43`), with the cockpit near plane substituted in camera 3 | xr `EyeProjection` | verified |
| stereo baseline | fixed 64 mm: left eye at 0, right eye at -0.064 m in the reflected camera basis (not taken from the runtime's eye poses) | xr | verified |
| far plane recovered | far = `-_43 / (_33 - 1)` from the game's matrix, near = the configured cockpit near clip | xr | verified |
| world scale | `WorldScalePercent` 0.5-100: scales stereo depth and head translation relative to the game, the seat stays anchored | xr README | verified (doc) |
| cockpit camera | camera key 3 enables the seat offset; the proxy keeps the car visible by hooking the CTrackMania vtable slot `VehicleSetVisibility`; a persisted camera 3 is detected from the game's first hide request | xr | verified |
| vehicle for the offsets | upstream picks the profile from the challenge collection's display name (`app+0x198` challenge -> `+0x90` collection -> `+0x48` name), which is wrong on envimix maps (issue #15). The challenge's vehicle ident is at `CGameCtnChallenge+0xE8` (what `GetVehicleIdent` returns; same code in TMNF and Steam TMUF 2.11.26); its `CMwId` name resolves through the engine's name table, whose address can be read from `CMwId::GetString` by byte pattern | xr; xr-own (envimix patch, not yet tested in a headset) | verified (offsets) / unverified (patch) |
| frustum culling | the game culls for its original camera, so scenery vanishes when looking sideways. `FrustumCullingFix` hooks `CHmsViewport::SClippingFrustum::ComputePlaneEqs` call sites (one is the world clip plane) to build a frustum that follows the head, and the visibility branch in `CHmsViewport::RenderTree`: only trees the game rejected are tested against the head frustum, accepted ones stay on the normal path, tree flags are not changed | xr | verified |

## Performance

| fact | value | source | status |
|---|---|---|---|
| draw cost | by default the scene is drawn three times (desktop + two eyes); `MirrorEyeToDesktop=1` (default in the ini) skips the desktop draw and mirrors the left eye through the game's normal post-processing | xr README | verified (doc) |
| resolution | always the runtime's recommended eye size (scale it in the runtime, not in the mod) | xr README | verified (doc) |
| CPU | replay is CPU work on the game's render thread; the log reports replayed draws, primitives and CPU ms per frame | xr | verified |
| GPU transfer | D3D9On12 + D3D11On12 avoids the CPU readback; readback is the fallback and costs frame time | xr | verified |
| shading each eye with TMVS | the effects run once per eye; TMVS's *Quality in VR* (default Balanced) drops volumetric and bounce light and caps quality at Medium | tmvs `docs/settings.md` | verified (doc) |

## Device hand-over between two D3D9 mods (TMFOXR + TM Vibrant Shaders)

| fact | value | source | status |
|---|---|---|---|
| problem | TMVS finds the game device by patching the exe's `Direct3DCreate9` import. TMFOXR replaces that import afterwards and wraps a D3D9On12 factory TMVS never patched, so TMVS never saw the device (no menu, no effects) | xr-pr27 commit message | verified |
| solution | when `TMVibrantShaders.dll` is loaded, TMFOXR calls its export `TMVS_AttachDevice(device, mode)` right after creating the device and **before** writing its own vtable hooks: TMVS hooks the device first, TMFOXR's table hooks sit on top | xr-pr27 | verified |
| extra exports | `TMVS_SetEyeProvider(fn)`: TMVS asks per eye for view, projection (16 floats each, D3D row-major as `SetTransform` takes them) and the eye's rectangle in the packed target, 0 = no stereo frame; `TMVS_SetSceneCallbacks(begin, end)`: TMFOXR restores the game's shader state before TMVS shades and the game's render target plus the desktop mirror after; `TMVS_DrawOverlay`: TMVS draws its menu into the headset UI layer | tmvs `src/hook.h`, `src/hook.cpp`; xr | verified |
| modes | `AttachVibrantShaders` in `TMFOXR.ini`: 0 off; 1 (default) TMVS's own *Effects in VR* setting decides (off: TMVS only watches, draws its menu, changes no targets or depth); 2 TMVS shades the eyes regardless | xr-pr27 | verified |
| depth | the packed stereo depth is created as INTZ when the game's depth is INTZ, so TMVS can read it | xr-pr27 | verified |
| tested | menu and effects in a headset reported by the hand-over's author and by jiink; my fixes after that are listed under pitfalls | tmvs CHANGELOG | verified (user reports) |

## Pitfalls

| pitfall | cause and fix | source | status |
|---|---|---|---|
| distant meshes vanish for a frame in the cockpit view | with a 1-5 cm near plane, clip-space `w - z` of distant geometry is only about the near distance, and float rounding on large meshes (sky dome, grandstands) pushes them past the far plane while the head moves. Raising the near clip to ~0.2 m hides it but cuts other cars' roofs. Scaling the projection's depth row by `1 - 1e-2` gives a margin that grows with distance (`1e-4` fixed Coast's sky, not Stadium's grandstands and lamps) | xr-own (far-clip branch; values from in-game tests named in its commit messages) | verified (cause, code) / inferred (1 % suffices everywhere) |
| a second mod that finds the sky by depth == 1 loses it | after that headroom the sky's depth is the projection's depth at infinity (`_33`), not 1; compare against that value | tmvs CHANGELOG 1.4.3 | verified |
| menu lists drawn unclipped (issue #7) | `IDirect3DDevice9::SetRenderTarget(0, ...)` resets viewport **and** scissor rectangle to the new target. TrackMania calls `SetScissorRect` only when its clip zone changes (`CVisionViewportDx9::RenderDelayed_OverlaySetScissorRect` compares the zone index first), so after a mod's internal target switch the rest of the zone is unclipped. Save and restore the scissor rectangle around every internal switch | xr-own (scissor patch) | verified (code reading) / unverified (headset test) |
| black headset with TMVS *Game HDR* | a float scene target took the camera's image away from the eye targets; TMVS keeps *Game HDR* and *Render scale* off while the VR mod draws the eyes | tmvs CHANGELOG 1.4.3 | verified |
| lamps and the Rain sky flicker in the headset | per-eye temporal history in TMVS; without *TAA in VR* the eyes are shaded without history, the over-time state (clock, exposure, fog colour, rain, spray, water, trail) steps with the left eye and the right eye shows the same values | tmvs CHANGELOG 1.4.3 | verified (user report) |
| effect targets switch size every other frame at a race's start | frames without eyes between frames with eyes; an eye larger than the camera's target. Leave such frames alone for 3 s | tmvs CHANGELOG 1.4.3 | verified |
| "Depth buffer unavailable" warning in watch-only mode | the watching mod gives the game its own depth back on purpose; do not report it as an MSAA problem | tmvs CHANGELOG 1.4.3 | verified |
| freeze at the end of multiplayer rounds (issue #6) | the log ends with `xrEndFrame` `XR_ERROR_RUNTIME_FAILURE`, `DXGI_ERROR_DEVICE_REMOVED` (0x887A0005) in the D3D9On12 transfer and `D3DERR_DEVICELOST` in the UI readback; no `Reset` follows, so the game stops presenting while audio runs on. Cause open; a diagnostic patch logs `GetDeviceRemovedReason` once (HUNG points at a GPU hang or cross-queue wait, INVALID_CALL at API misuse) | upstream issue #6; xr-own | verified (log) / open (cause) |
| Meta Horizon Link closes the game during OpenXR session creation (issue #26) | SteamVR and VDXR run the same build. Hypothesis: the Oculus runtime does not accept the D3D11On12 device; a patch gives runtimes named "Oculus" a plain D3D11 device on the runtime's adapter (CPU readback path) and logs each step between `xrCreateSession` and the first swapchain | upstream issue #26; xr-own | unverified (no Link headset) |
| video memory reported as tiny or negative | TMUF rebuilds the value with signed 32-bit arithmetic, so exactly 2048 MiB becomes negative; TMFOXR clamps `GetAvailableTextureMem` and DirectDraw's `GetAvailableVidMem` just below 2 GiB (`VideoMemoryMB=2048`). Do not stack dgVoodoo's `D3D9.dll` on top: only one proxy can have that name | xr README | verified (doc) |
| defaults disagree | `CockpitNearClip`: 0.05 in the code and README, 0.2 in `TMFOXR.defaults.ini`; `FrustumCullingFix`: off in the code and README, `1` in the defaults ini. The ini wins when it is installed | xr | verified |
| long build paths | the OpenXR build breaks in a deep folder; build in a short path | own ([../gotchas.md](../gotchas.md)) | verified |
