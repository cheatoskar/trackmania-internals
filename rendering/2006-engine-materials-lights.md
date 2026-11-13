# The 2006 engine (Nations ESWC, Sunrise, Original): material per draw, delayed draws, views into textures, point lights

What a Direct3D 9 proxy needs to tell the material of every draw and to find the game's lamps in TrackMania Nations ESWC, TrackMania Sunrise and TrackMania Original. Exes (all addresses are link-time VAs, image base 0x00400000, no `.reloc`):

| Game | Exe | SHA-256 (start) | Identification |
|---|---|---|---|
| Nations ESWC | `TmNationsESWC.exe` unofficial 1.80 | 983bd127 | [../games/tmn-eswc/addresses.md](../games/tmn-eswc/addresses.md) |
| Sunrise | `TmSunrise.exe` Windows 10 community repack | e59dfc7e | [../games/tm-sunrise/README.md](../games/tm-sunrise/README.md) |
| Original | `TMORIGINAL.EXE` Online Patch v2 | 75e129d7 | [../games/tm-original/README.md](../games/tm-original/README.md) |

How established (2026-11-12 / 13): static analysis with Python + capstone (linear sweep of `.text`, call/xref search) and the reflection tables ([../scripts/re/classinfo.py](../scripts/re/classinfo.py)); where marked, a runtime census of an own D3D9 proxy that hooks the two draw functions and logs, once per shader and map, the shader's class id, file and textures (all three games, 2026-11-12/13). Status words as in [../CONTRIBUTING.md](../CONTRIBUTING.md): **verified** = seen in code in all three exes (or in code and at run time), **inferred** = one method or one game only.

## 1. The two draw functions

Every 3D draw goes through one of these, with the `CPlugShader` it draws with. Found by structure (no symbols in these exes), see the ESWC notes in [../games/tmn-eswc/engine-hooks.md](../games/tmn-eswc/engine-hooks.md).

| Function | ESWC | Sunrise | Original | Form | Status |
|---|---|---|---|---|---|
| `CDx9ShaderKeeper::UpdateAndRender(shader, corpus, tree, visualKeeper)` | 0x00807D10 `81 EC 00 02 00 00` | 0x006EFF50 `81 EC B8 01 00 00` | 0x006EF1E0 `81 C4 48 FE FF FF` | thiscall, `ret 0x10` | verified (code; hooked at run time in all three) |
| `CDx9ShaderKeeper::ShadeVisualForCurrentPass(SDelayedRender&)` | 0x00804ED0 `81 EC F8 00 00 00` | 0x006ECDD0 `81 EC F8 00 00 00` | 0x006EC060 `81 C4 08 FF FF FF` | thiscall, `ret 4` | verified (code; hooked at run time in all three) |
| `CPlugTree.Shader` (the tree's own shader) | +0x94 | +0x98 | +0x98 | reflection | verified |

Original's compiler wrote `sub esp, imm` as `add esp, -imm`: the prologue bytes differ from Sunrise's although the functions are the same.

At run time (census, one map each): the class at `delayed+0x10` was `CPlugShaderApply` (0x09026000) for every ShadeVisualForCurrentPass call (verified, runtime class id). On most race maps that path draws several times as many visuals as UpdateAndRender (3x to 14x in the census; a few maps less), and the census of UpdateAndRender alone never saw a ground material: the ground is drawn on the delayed path (inferred).

## 2. The material of a draw: the shader copy's textures

| Fact | Value | Status |
|---|---|---|
| Every block material draws with **its own copy** of a shared shader file (`...PC3.Shader.Gbx?318`, the `?n` a copy number) or with a copy that has no file at all; the shader file alone does not tell grass from road | census: hundreds of distinct `CPlugShaderApply` per map, many `(no file)` | verified (runtime) |
| `CPlugShaderApply` texture list: `+0xB0` count, `+0xB4` `CPlugBitmapApply*` array (reflection `BitmapApply` is the buffer object at +0xAC; the archive handler reads the count into +0xB0 and allocates count*4 bytes into +0xB4: ESWC 0x004FA60A, Sunrise 0x004C292A, Original 0x004C290A) | | verified (code + runtime) |
| Each entry `+0x1C` = the `CPlugBitmap` (reflection `CPlugBitmapSampler.Bitmap` 28) | | verified (runtime) |
| A nod's file: `nod+0x08` = `CSystemFidFile*`, its `FileName` (`CFastStringInt`) at fid+0x58, parent folder fid+0x18 | | verified (runtime) |
| The textures name the material: a material's `Material.Gbx` (loose files under `<Environment>\Media\Material\` in these games) lists its textures in the GBX header's reference table in plain text; the set of texture file names of a draw's shader copy matches exactly one material's set (a road over grass carries the grass textures *and* its own: the larger set wins) | 515 materials of the three games | verified (files; matched against the census) |
| Textures that say nothing about the material (environment cubes, Fresnel ramps, the cloud shadow `Clouds`, `Default*`) are swapped per mood (`Day\Clouds` for `DefaultClouds`) and must be ignored when matching | | verified (runtime) |

## 3. The delayed draw (`SDelayedRender`) and who fills it

`CVisionViewportDx9::RenderVisual(tree)` decides the shader once (the tree's own, or a replacement the viewport holds for it) and both hands it to UpdateAndRender and stores it in the delayed record, an array of 0xC0-byte entries (Sunrise/Original: array at viewport+0x620, count +0x61C; ESWC: array +0x5D8, count +0x5D4):

| Record offset | Meaning | Status |
|---|---|---|
| +0x00 | the corpus (viewport's current corpus) | verified (code) |
| +0x04 | the `CPlugTree` | verified (code) |
| +0x08 | the visual keeper (UpdateAndRender's 4th argument) | verified (code) |
| +0x0C | a viewport state word | inferred |
| +0x10 | **the same shader UpdateAndRender got as its 1st argument** | verified (code) |
| +0x14..+0xB7 | a 0xA4-byte copy of the viewport's current render state | verified (code) |

Store sites (record +0x10 / the UpdateAndRender call with the same register or stack slot): ESWC 0x007E5693 / 0x007E5513, Sunrise 0x006D06BD / 0x006D054A, Original 0x006CFA0D / 0x006CF89A. The pass at Sunrise 0x006EF5E0 reads a record's first texture from its +0x10 shader (`MwIsKindOf(CPlugShaderApply)`, `+0xB0`, `+0xB4`, `[0]+0x1C`; 0x006EF6C7..0x006EF6E7) to choose its own pass shader. So **the shader at delayed+0x10 is the material's own copy with its textures**, the same object the direct path draws with. Status: verified (code, all three).

Callers of ShadeVisualForCurrentPass (Sunrise; ESWC and Original have the same set, 6 or 7 callers):

| Caller (Sunrise) | What it passes at +0x10 | Status |
|---|---|---|
| 0x006CC130 (the sorted pass) | the record unchanged; the keeper (`this`) is `shader+0x14` | verified (code) |
| 0x006CCBAB, 0x006DC7BE, 0x006EF802 | **an engine pass shader** put into +0x10 for this call (taken from a per-viewport table, e.g. viewport+0x129C), the material's shader put back afterwards (0x006CCBA8 / 0x006CCBBB, 0x006DC7AE, 0x006EF7F3 / 0x006EF812); same pattern at ESWC 0x007F1A5E and Original 0x006DBB1E | verified (code, Sunrise); inferred (ESWC, Original: same instruction shape) |
| 0x0075330E, 0x00756339 | records of other structures | inferred |

Consequence for a per-draw material tag: on those three callers the tree (`delayed+0x04`) still names the material through its own `CPlugTree.Shader`.

## 4. Views into textures (Sunrise)

| Fact | Status |
|---|---|
| Sunrise renders a second, 512x512 view into a texture **every frame**, with a depth-stencil surface of its own, before the main camera. A proxy that tells the main camera by "the render target matches the readable depth buffer" takes it for a second main camera (its depth buffer becomes readable too); telling cameras with their own depth buffer smaller than the screen apart as views fixes that | verified (runtime log of an own proxy: render targets rebuilt twice a frame until the rule was added) |
| What the view shows (water reflection or another render-to-texture effect) | inferred |

## 5. Point lights (lamps, car lights)

The 2006 engine lights a visual with **up to two point lights** of its own, chosen per draw in ShadeVisualForCurrentPass (keeper flag bit 0x200 set):

| Fact | ESWC | Sunrise | Original | Status |
|---|---|---|---|---|
| global: the viewport drawing now | 0x00CB1470 | 0x00BA9860 | 0x00BA4AB0 | verified (code) |
| that viewport's zone (`CHmsZone`) | +0x30C | +0x328 | +0x328 | verified (code) |
| zone's elements by kind: `zone+0x64`: +0x04 table of `{first, count}` per kind (8 bytes each), +0x10 the element array; kind **3** = the corpus lights (selector Sunrise 0x006F3BC0(3, 1), ESWC 0x0080B910) | | | | verified (code) |
| per draw: the lights whose GxLight is on are sorted by distance / range, the two nearest are kept in a global list (count, then items) | 0x00CE4D10 | 0x00BDA1D0 | 0x00BD53B0 | verified (code) |
| the two get `LightEnable` (device vtable +0xD4) on indices from viewport+0x824 and their colour, range and position go to the shader constants (Sunrise 0x006ED121..0x006ED36F) | | | | verified (code, Sunrise); inferred (others) |

Classes and members (reflection, identical in the three games unless noted):

| Class (id) | Members | Status |
|---|---|---|
| `CHmsCorpusLight` (0x0600F000) | Location +0x18 (`GmIso4`: 3x3 rotation, position at +0x3C); Light +0x48; vtable slot +0x88 = `lea eax, [ecx+0x18]` (the location; Sunrise vtable 0x00A2E038) | verified |
| `CHmsLight` (0x0600C000) | UpdateType +0x9C, MainGxLight +0xA0, BitmapFlare +0x78, BitmapSprite +0x7C, BitmapProjector +0x80, CorpusLights +0x68, ItemContainer +0x98 (the item the light hangs on, e.g. a car) | verified (reflection) |
| `GxLight` (no reflection) | +0x20 bit 0 = on (lights off are skipped); +0x30 intensity (compared with a minimum); +0x34/+0x38/+0x3C colour; +0x64 range (the distance is divided by it) | inferred (from the use in ShadeVisualForCurrentPass; ESWC's notes have the base colour at +0x24 and the final one at +0x34) |
| `CHmsZone` (0x06004000) | CorpusLights +0x64 (reflection name of the bucketed element container above) | verified |
| `CPlugLight` (0x0901D000) | Light +0x24, FuncLight +0x14, BitmapFlare +0x18, BitmapProjector +0x1C; flags/actions NightOnly, ReflectByGround, DuplicateGxLight, MakePoint, MakeBall, MakeFrustrum, MakeSpot | verified (reflection) |
| `CPlugTreeLight` (0x09062000) | PlugLight +0xAC / Light +0xB0 (ESWC), +0xB0 / +0xB4 (Sunrise, Original) | verified (reflection) |
| `CConstructionDecorationMood` (0x24050000) | IsNight (no field: a getter), BackgroundIsLocallyLighted +0x44, SolidLightAreSkinned +0x48 | verified (reflection) |
| `CScene3d` (0x0A003000) | Lights +0x9C | verified (reflection) |

Not known yet (needs a run): which lamps of a night map are such point lights and which are only baked into the vertex prelight, and whether the car's head lights are `CHmsLight`s with a projector (their ItemContainer would be the car's item, `CSceneMobil.Item` +0x3C).
