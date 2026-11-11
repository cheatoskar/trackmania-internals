# Sunrise and Original: saving the race replay without the dialog

Exes: `TmSunrise.exe` Win10 community repack (SHA-256 `e59dfc7e...`, PE timestamp `5cc8f0cf`, 7,904,768 bytes) and `TMORIGINAL.EXE` Online Patch v2 (SHA-256 `75e129d7...`, PE timestamp `5d337477`, 7,883,264 bytes). Image base 0x00400000, no `.reloc`. Addresses are VAs.

What "Save replay" does on the solo end-race screen: `CTrackMania::SaveReplayRecord` builds the default name `<map name>(mm'ss''cc)` (stunts: `(score)`), shows the Save-As dialog, appends `.Replay.Gbx`, filters the name, checks whether the file exists ("Overwrite?"), then writes the replay record with `CSystemArchiveNod::Save(name, replay, folder, 10, 7, 1, 1)` into "My Replays", unbinds the replay from the new file and rescans the replay list. Running exactly these calls with the default name saves the replay as the game would, `validable` flag included. The ESWC version of all of this is in [../tmn-eswc/addresses.md](../tmn-eswc/addresses.md) and is verified in game (eswc-autosave since 1.0).

Status (2026-11-11): **verified (static)** = two independent methods agree; nothing below was run in the game yet. Used by eswc-autosave 1.2 (`src/games.h`, `src/legacy.cpp`) and the Horizon module "Autosave & Login-Fix" 2.1.

Method keys: `call` = same position and arguments in `SaveReplayRecord` as the verified ESWC function (the three fibers were compared instruction by instruction: Sunrise and Original 607 instructions each, identical except absolute addresses; ESWC 603, similarity 0.97); `body` = function body compared with the verified ESWC function (mnemonic sequence similarity 1.00 unless noted, same `ret n`); `refl` = engine reflection tables ([../../scripts/re/classinfo.py](../../scripts/re/classinfo.py)); `str` = string reference; `vt` = vtable slot; `xr` = direct call sites (count in Sunrise / Original). Sunrise and Original have the same call counts for every function below.

## Functions

| What | Sunrise | Original | ESWC | Methods |
|---|---|---|---|---|
| `CTrackMania::SaveReplayRecord` (fiber; the reflection entry `SaveReplayRecord` names the string `0xa1b48c` / `0xa1744c` / `0xc92d24` that the fiber loads) | 0x008177f0 | 0x00815c50 | 0x0070b2c0 | refl + str, body 0.97 |
| `CTrackManiaRace::OnFinishLine(player)`, ret 4; prologue `8b 44 24 04 53 55 56 33 db 3b c3` (11 bytes, whole instructions, no relative operand) | 0x007ec7a0 | 0x007ead30 | 0x006d9f50 | vt (Race vtable slot 87: `0xa4c954` / `0xa48654`), str (only push of `"--FINISHED-- t=%d"` at 0x007ecb30 / 0x007eb0c0), sets RaceState +0x228 = 2 via SetRaceState 0x00838e00 / 0x00837170 |
| `CTrackManiaRace1P` solo race end (stops the recording of app+0x130), ret; prologue `83 ec 08 53 55 56 57` | 0x007dd7b0 | 0x007dc250 | 0x006c5060 | vt (slot 88 of the vtables of `CTrackManiaRace1P`, `...1PGhosts`, `...1PSpecial` only), body 1.00 (103 instructions) |
| `CTrackManiaRace::GetLocalPlayerInfo()` | 0x0082aeb0 | 0x00829200 | 0x0071c730 | body 1.00 (only race+0x40 -> +0x3c), xr 162 / 162, called 3x in the solo race end |
| `CFastStringInt::SetString(const SStringParamInt& {ptr, len})`, ret 4 | 0x0041a2a0 | 0x004055b0 | 0x00405f40 | call (6 sites), also builds "My Replays"; body 0.79 (Sunrise is a different compile with the same steps; Original's prologue is byte-identical to ESWC's) |
| `CFastStringInt::Empty()` | 0x00401280 | 0x00401280 | 0x00402530 | body 0.84 (reads the empty-string global, frees the block, sets {0, empty}); called on the temp string after "My Replays" |
| `FilterFileName(dst, src, 0, 0)` cdecl | 0x00480ae0 | 0x00480c10 | 0x00635870 | call, body 1.00, xr 9 / 9 |
| `FileExists(const CFastStringInt&, CSystemFids*)` cdecl | 0x0044e860 | 0x0044ea70 | 0x0063f260 | call, body 1.00, xr 14 / 14 |
| `CGameCtnReplayRecord::GetBestGhostRaceTime(int&, int&)`, ret 8 | 0x007abd30 | 0x007aaab0 | 0x0068c390 | call, body 1.00 |
| `CGameCtnReplayRecord::GetBestGhostStuntsScore()` | 0x007abe10 | 0x007aab90 | 0x0068c470 | call (after `PlayMode == 5`), body 1.00 |
| `CSystemArchiveNod` constructor / destructor | 0x0041e220 / 0x0041e330 | 0x0041e2b0 / 0x0041e3b0 | 0x0062a850 / 0x0062a950 | call, body 1.00 |
| `CSystemArchiveNod::Save(name, nod, folder, 10, 7, 1, 1)`, ret 0x1c | 0x00421e60 | 0x00421ee0 | 0x0062e1c0 | call (same seven pushes), body 0.93 / 1.00 |
| `CSystemEngine::UnbindFid(CMwNod*)`, ret 4 | 0x00414e00 | 0x00414e60 | 0x00623f90 | call, body 1.00 |
| `CTrackMania::ScanDiskForReplays()` | 0x00821f30 | 0x00820410 | 0x007173b0 | call, refl (method entry), body 1.00 |
| `CSystemFids` get-or-create subfolder (calls GetFullPath at vtable +0xac, then creates the directory) | 0x00478b30 | 0x00478bd0 | 0x00638eb0 | body 1.00, used to make "My Replays" |
| game `free` (cdecl) | 0x00402b90 | 0x00402bb0 | 0x004045b0 | body 1.00, called by Empty |

## Globals

| What | Sunrise | Original | ESWC | Methods |
|---|---|---|---|---|
| `wchar_t*` of every empty `CFastStringInt` | 0x00abc0e8 | 0x00ab80e8 | 0x00bb45d4 | call (compared in SaveReplayRecord and Empty) |
| `CMwEngineMain*`; `->+0x28 ->+0x2c` = `CSystemEngine*` (UnbindFid's `this`) | 0x00b8b494 | 0x00b866f4 | 0x00c96140 | call (same three loads before UnbindFid) |
| strings `"(%.2d'%.2d''%.2d)"` / `"(%d)"` / `".Replay.Gbx"` / `"My Replays"` | 0x00b51174 / 0x00b51188 / 0x00b3cd14 / 0x00b50ed4 | 0x00b4c7d4 / 0x00b4c7e8 / 0x00b387fc / 0x00b4c534 | 0x00c2fa68 / 0x00c2fa7c / 0x00c17434 / 0x00c2f734 | str |

## Members (the same in Sunrise and Original)

| Object | Offset | Meaning | ESWC | Methods |
|---|---|---|---|---|
| `CTrackManiaRace` | +0x3c | `Game` (the app) | +0x40 | refl, read in the solo race end |
| `CConstructionGame` (the app) | +0x130 | `ReplayRecord`: the run being / just recorded (solo "Save replay") | +0x144 | refl, the solo race end stops its recording |
| `CTrackMania` (the app) | +0x1b8 / +0x1d4 / +0x1d8 | data folders / the parent folder of "My Replays" / **"My Replays"** (`CSystemFids*`) | +0x1f0 / +0x20c / +0x210 | str ("My Replays" is made as a subfolder of +0x1d4 and stored at +0x1d8), call (SaveReplayRecord's default folder) |
| `CConstructionReplayRecord` | +0x58 | `Challenge` | +0x60 | refl, call |
| `CConstructionReplayRecord` | +0x40 | `Ghosts` count | +0x48 | refl, solo race end compares it with 1 |
| `CConstructionReplayRecord` | +0xac | != 0 while recording | +0xb4 | call (SaveReplayRecord stops the recording first if set), StopRecord (record vtable +0x8c: 0x007aa5b0 / 0x007a9330) clears it |
| `CConstructionReplayRecord` | +0xd4 | fid of the last save | +0xdc | call (set after a save; not needed for a solo autosave) |
| `CConstructionReplayRecord` | +0x28 / +0x2c | fid buffer `{count, CSystemFidFile**}` (1 = replay came from a file) | +0x30 / +0x34 | call |
| `CConstructionChallenge` | +0x11c / +0x128 | `PlayMode` (5 = stunts) / `ChallengeName` `{len, wchar_t*}` | +0x118 / +0x124 | refl, call |
| `CTrackManiaPlayerInfo` | +0x228 / +0x1c0 / +0x1c4 | RaceState (2 = finished) / RaceTime / RaceBestTime (min of the runs, 0xffffffff = none) | +0x25c / +0x1f4 / +0x1f8 | refl, OnFinishLine writes them |
| `CSystemFids` vtable | +0xac | `GetFullPath(CFastStringInt& out)` | +0xac | the subfolder function above (same slot, same use) |

## Not covered

- The network "Save previous replay" (`StopReplayRecordAndKeepCopy` Sunrise 0x00842350 / Original 0x008406a0, previous replay app+0x224): not needed for a solo autosave, not checked.
- `CTrackManiaRace1PCrazy` and `CTrackManiaRace2PTurnBased` (hot seat) have their own race end; not hooked.
