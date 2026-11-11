# TrackMania Original: addresses and struct members

**Exe for everything below:** `TMORIGINAL.EXE`, Online Patch v2 community exe, 7,883,264 bytes, SHA-256 `75e129d769f4595ddde54a8ef484162915bc2bf998c1a2139cc963294fe8175e`, PE timestamp `5d337477`, image base 0x00400000, no `.reloc`. Addresses are VAs. Identification: [README.md](README.md).

How established (2026-11-11, all static, nothing run in game yet): the Sunrise addresses ([../tm-sunrise/](../tm-sunrise/README.md)) ported to this exe by masked byte patterns (`port`, the method of the Horizon Porting Kit's `match.port_masked`: whole instructions, absolute addresses and rel32 masked, unique hit), by the engine's reflection tables (`refl`: method entry points and member offsets, [scripts/re/classinfo.py](../../scripts/re/classinfo.py)), by vtable slots (`vt`: the slot that holds the matched neighbours), by instruction-by-instruction comparison with Sunrise (`cmp`: same instructions, same offsets), and checked with Ghidra 12.1.2 xrefs (`gx`: callers and data references in a fresh headless analysis). The Horizon Porting Kit's atlas generator (call graph from Sunrise) agrees on the rows marked `atlas`.

Status words: **verified (static)** = two independent static methods agree, or the instructions are compared byte for byte with the Sunrise function; **inferred** = one method only. No row is verified in game yet.

## Globals

| VA | Meaning | Sunrise | Method | Status |
|---|---|---|---|---|
| 0x00bc8084 | `CGameApp::s_TheGame` / app global (`CTrackMania*`, class id 0x24001000), written by the `CConstructionGame` constructor at 0x00813c86 (constructor 0x00813b00, called from 0x007fdd02), cleared in 0x00814190 | 0x00bcce50 | port (ctor), gx (2 writes), atlas | verified (static) |
| 0x00b85e4c | `CMwEngineManager::First`, head of the class registry (registration code 0x00411820) | 0x00b8abec | registry-head pattern, gx, atlas | verified (static) |
| 0x00bc841c | `CGameCtnChallengeInfo::s_ChallengeInfos` `{count, items, capacity}` | 0x00bcd1e8 | cmp (RefreshFromDisk), atlas | verified (static) |
| 0x00b86de8 | input port global (`CInputPortDx8*`), written at 0x0043f355 (`89 15 e8 6d b8 00`) and 0x0043f38b | 0x00b8bb88 | cmp, gx | verified (static) |
| 0x00a16398 / 0x00a1606c / 0x00a162d4 | import slots `Direct3DCreate9` / `DirectInput8Create` / `HttpOpenRequestA` | 0x00a1a3dc / 0x00a1a06c / - | import table | verified |

## Functions

All thiscall unless noted. Prologue = first bytes, identical to the Sunrise function unless noted.

| VA | Name / meaning | Sunrise | Prologue | Method | Status |
|---|---|---|---|---|---|
| 0x007eab20 | `CTrackManiaRace::OnStartLine(player)`, ret 4, race vtable slot 86 | 0x007ec550 | `8b 44 24 04 56 8b 70 34 8b 86 28 02 00 00 85 c0` | port, vt, gx | verified (static) |
| 0x007ead30 | `CTrackManiaRace::OnFinishLine(player)`, ret 4, slot 87; `CTrackManiaRace1P::OnFinishLine` calls it first | 0x007ec7a0 | `8b 44 24 04 53 55 56 33 db 3b c3 57 8b f9` | port, vt, gx | verified (static) |
| 0x007eab80 | `CTrackManiaRace::OnCheckpoint(player)`, ret 4, slot 89 (also reached through the thunk 0x007dc3a0, Race1P slot 89) | 0x007ec5b0 | `83 ec 38 53 55 8b 6c 24 44 56 8b f1` (the next instruction reads +0x2d0, Sunrise +0x2e8) | vt, gx | verified (static) |
| 0x007dc250 | `CTrackManiaRace1P` solo race end, Race1P vtable slot 88 (in three race vtables) | 0x007dd7b0 | `83 ec 08 53 55 56 57 8b f1` (then a call; reads +0x420, Sunrise +0x474) | vt, gx | verified (static) |
| 0x007dc120 | `CTrackManiaRace1P::OnFinishLine(player)`, slot 87 of Race1P | 0x007dd680 | `56 8b 74 24 08 57 56 8b f9` | port, vt, atlas | verified (static) |
| 0x007ea5b0 | `CTrackManiaRace::ResetPlayer`, slot 85 | 0x007ebfb0 | | port, vt, atlas | verified (static) |
| 0x007ea850 | `CTrackManiaRace::RespawnPlayer(player)` (increments the info's +0x1d4) | 0x007ec270 | `83 ec 08 53 8b 5c 24 10 56 57` | port, atlas | inferred |
| 0x00829200 | `CTrackManiaRace::GetLocalPlayerInfo()` (race+0x3c -> game, +0x100 -> network, then a jump) | 0x0082aeb0 | `8b 41 3c 85 c0 74 11 8b 80 00 01 00 00` | port, gx (4 callers), atlas | verified (static) |
| 0x007ebb00 | `CTrackManiaRace::ButtonQuitOnClick()` | 0x007ed580 | | refl, port, atlas | verified (static) |
| 0x008406a0 | `CTrackManiaRace::StopReplayRecordAndKeepCopy()` (network race end) | 0x00842350 | `51 56 8b f1 8b 46 3c 8b 88 30 01 00 00` | port, gx, atlas | verified (static) |
| 0x007ba170 | `CTrackManiaMenus::MenuPlayChallenge_Solo`: SetChallengeType(3), menus+0x354 = 2 | 0x007bb660 | | refl, port, cmp | verified (static) |
| 0x007bad40 | `CTrackManiaMenus::MenuPlayChallenge_OnOk` (chosen infos at menus+0x318, result menus+0x258 = 1) | 0x007bc230 | | refl, port, cmp | verified (static) |
| 0x00823370 | `CTrackMania::SetChallengeType(int)`: `mov [ecx+0x2a0], arg; ret 4` | 0x00824f00 | `8b 44 24 04 89 81 a0 02 00 00 c2 04 00` | cmp, gx | verified (static) |
| 0x0085e0a0 | `CGameCtnChallengeInfo::RefreshFromDisk(folders*, 0, 0)` cdecl; one caller 0x0081967b with app+0x1b8 | 0x0085fce0 | | port, cmp, gx, atlas | verified (static) |
| 0x0085e210 | `CGameCtnChallengeInfo::SortByName()` cdecl | 0x0085fe50 | | port, gx, atlas | verified (static) |
| 0x0085d1a0 | `CGameCtnChallengeInfo::Build` | 0x0085ede0 | | port, atlas | inferred |
| 0x0081aca0 | `CGameCtnApp::SetChallenge` | 0x0081c820 | | port, atlas | inferred |
| 0x00813b00 | `CConstructionGame` constructor (stores the app global) | 0x00815640 | | port, gx | verified (static) |
| 0x00815c50 | `CTrackMania::SaveReplayRecord` (fiber; the reflection entry `SaveReplayRecord` names the string 0x00a1744c it loads); instruction by instruction the same as Sunrise's | 0x008177f0 | | port, atlas, refl + string, cmp | verified (static) |
| 0x004055b0 / 0x00401280 | `CFastStringInt::SetString({ptr, len})` ret 4 / `CFastStringInt::Empty()` | 0x0041a2a0 / 0x00401280 | | cmp (SaveReplayRecord, "My Replays" setup), kit | verified (static) |
| 0x00480c10 / 0x0044ea70 | `FilterFileName(dst, src, 0, 0)` / `FileExists(name, folder)`, both cdecl | 0x00480ae0 / 0x0044e860 | | cmp, body = ESWC | verified (static) |
| 0x007aaab0 / 0x007aab90 | `CGameCtnReplayRecord::GetBestGhostRaceTime(int&, int&)` ret 8 / `GetBestGhostStuntsScore()` | 0x007abd30 / 0x007abe10 | | cmp, body = ESWC | verified (static) |
| 0x0041e2b0 / 0x00421ee0 / 0x0041e3b0 | `CSystemArchiveNod` ctor / `Save(name, nod, folder, 10, 7, 1, 1)` ret 0x1c / dtor | 0x0041e220 / 0x00421e60 / 0x0041e330 | | cmp, body = ESWC, kit (0x00421ee0) | verified (static) |
| 0x00414e60 | `CSystemEngine::UnbindFid(CMwNod*)` ret 4 | 0x00414e00 | | cmp, body = ESWC | verified (static) |
| 0x00864ef0 | `DialogInGameMenu_OnSaveReplay` | 0x00866b30 | | port, atlas | inferred |
| 0x00820410 | `CTrackMania::ScanDiskForReplays()` | 0x00821f30 | | refl, port, atlas | verified (static) |
| 0x00402cb0 / 0x00402bb0 | game `malloc` / `free` (cdecl) | 0x00402cc0 / 0x00402b90 | | port, atlas | verified (static) |
| 0x00706cd0 | creates the Direct3D 9 factory (calls the `Direct3DCreate9` thunk 0x008da482 at 0x00706dca); called from 0x005e31af | - | | gx | verified (static) |
| 0x004cdee0 | `CInputPortDx8` setup: `DirectInput8Create` (thunk 0x009998f0, call 0x004cdf96), passes the device enumeration callback 0x004ce870 (reference at 0x004ce6aa) | - | | gx, port | verified (static) |
| 0x004cf5e0 | `CInputPortDx8::DeviceReadHardwareCurState` | 0x004cf530 | | port, atlas | inferred |
| 0x006c8580 / 0x006ce0a0 | `CVisionViewportDx9::RenderFrameBegin` / `RenderFrameEnd` | 0x006c9230 / 0x006ced50 | | port, atlas | inferred |

## Controller hot-plug (input port), all matched to Sunrise

| Item | VA | Sunrise | Method | Status |
|---|---|---|---|---|
| device enumeration callback (calls add pad) | 0x004ce870 | 0x004ce7c0 | port (17 bytes identical) | verified (static) |
| add pad (CreateDevice, append) | 0x004cdc10 | 0x004cdb60 | port | verified (static) |
| port vtable (+0x90 create binding table 0x00496970, +0x94 add binding 0x00496b50, +0xb0 update 0x004ce890) | 0x00a1fcd8 | 0x00a23d18 | slots of the matched functions, gx (2 constructor writes) | verified (static) |
| pad vtable (+0x00 destructor 0x0069a920, +0x88 kind = 2) | 0x00a3c7f8 | 0x00a40af8 | the same instruction in the matched constructor 0x0069a520 / destructor 0x0069a940 | verified (static) |
| Reserve of 8-byte arrays (called by table creation at 0x00496a15) | 0x008b64d0 | 0x0060a070 | call target in the matched function, same 17-byte prologue | verified (static) |
| table resize site | 0x00496a12 | 0x00496772 | cmp | verified (static) |
| pad data format (`c_dfDIJoystick2` header) | 0x00a6895c | 0x00a6cc9c | unique 20-byte match | verified (static) |

## Members (reflection; same as Sunrise unless noted)

| Class.member | Offset | Status |
|---|---|---|
| `CConstructionGame`: Network / Challenge / Camera / CurrentRace / ReplayRecord / Scene | +0x100 / +0x10c / +0x124 / +0x12c / +0x130 / +0x120 | verified (static, refl) |
| `CTrackMania`: MenuManager / Switcher / Editor / ChallengeType / Races | +0x104 / +0x298 / +0x29c / +0x2a0 / +0x2a8 | verified (static, refl) |
| `CTrackMania` data folders pointer (RefreshFromDisk argument) | +0x1b8 | verified (static, cmp) |
| `CTrackManiaMenus`: back pointer to the app / chosen infos `{count, items, cap}` / play mode / menu result | +0x2e8 / +0x318 / +0x354 (= 2 for solo) / +0x258 (= 1 to play) | verified (static, cmp) |
| `CConstructionChallenge`: ChallengeName / PlayMode / ChallengeParameters | +0x128 / +0x11c / +0xe4 | verified (static, refl) |
| `CConstructionChallengeParameters`: Bronze / Silver / Gold / Author time, AuthorScore | +0x14 / +0x18 / +0x1c / +0x20, +0x34 | verified (static, refl) |
| `CTrackManiaPlayerInfo`: RaceStartTime / RaceTime / RaceBestTime / CurrentTime / RaceState / CurCheckpoint | +0x1bc / +0x1c0 / +0x1c4 / +0x224 / +0x228 / +0x1f8 | verified (static, refl) |
| `CConstructionPlayer.PlayerInfo` / `CConstructionRace.Players` `{count, items}` | +0x34 / +0x20 | verified (static, refl) |
| `CTrackManiaRace`: Game / StartTime / ChatEntry (Sunrise +0x34c) / Fps | +0x3c / +0x2a8 / +0x334 / +0x2bc | verified (static, refl) |
| `CTrackManiaRace1P`: StartTime (Sunrise +0x2c0) / BestRaceGhost (Sunrise +0x46c) / CurrentRaceGhost | +0x2a8 / +0x418 / +0x41c | verified (static, refl) |
| Race classes: `CTrackManiaRace1P`, `...1PCrazy`, `...1PGhosts`, `...1PSpecial`, `...2PTurnBased` (solo); `CTrackManiaRaceNetLaps`, `...NetRounds`, `...NetTimeAttack` (network); editor `CConstructionEditor` / replay editor `CConstructionEditorReplay` (0x2405d000) | class names | verified (static, refl) |
| Race vtables: `CTrackManiaRace` 0x00a484f8, `CTrackManiaRace1P` 0x00a48138 (slot 0 0x007dbe20) | | verified (static, vt) |

Dialog-free replay save (solo): all functions, globals (empty string 0x00ab80e8, engine main 0x00b866f4) and members (app+0x130 ReplayRecord, app+0x1d8 "My Replays", replay +0x58 / +0xac), side by side with Sunrise and ESWC: [../tm-sunrise/replay-save.md](../tm-sunrise/replay-save.md) (verified static, 2026-11-11).

Not found yet: the slot of a replay watched from the menu (ESWC: app+0x128, `CCtnMediaClipViewer`). Respawns are not counted (the RespawnPlayer row is inferred only).
