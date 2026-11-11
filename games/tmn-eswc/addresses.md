# TrackMania Nations ESWC: addresses and struct members

**Exe for everything below:** `TmNationsESWC.exe` unofficial 1.80 (2021 community build, no StarForce), in-game version `0.1.7.9`, 8,987,136 bytes, SHA-256 `983bd12724739dcb0f876aeb3a4259154b068940f5167f57ede750ca452d8068`, PE timestamp `5ce7272f`, image base 0x00400000, no `.reloc` section (cannot be rebased). Other builds: see [../../README.md](../../README.md#exe-identification). Addresses are VAs.

How established: Ghidra 12.1.2 static analysis, xref queries (`re/q*.txt` in the source column = own query dumps, not included), and for hooked functions a runtime prologue check in the eswc-autosave mod (it refuses to hook if bytes differ).

Source keys: `as` = eswc-autosave mod `src/autosave.cpp` (line numbers), `online` = eswc-autosave `src/online.cpp`, `T` = own eswc-autosave RE notes, `notes` = own ESWC replay RE notes.

## Functions

| VA | Name / meaning | Source | Status |
|---|---|---|---|
| 0x006D9F50 | `CTrackManiaRace::OnFinishLine(CTrackManiaPlayer*)`, ret 4; prologue `8B 44 24 04 83 EC 1C`; logs "--FINISHED-- t=%d"; hooked | as:49,209; T | verified (runtime prologue) |
| 0x0074AB60 | `RaceNet::StopReplayRecordAndKeepCopy()` (LAN/online): copies app+0x144 into app+0x25C; prologue `51 56 8B F1 8B 46 40`; hooked | as:50,210; T re/q6.txt | verified |
| 0x006C5060 | `CTrackManiaRace1P` race end (solo), stops app+0x144 recording; prologue `83 EC 08 53 55 56 57`; hooked | as:51,211; T re/q10.txt | verified |
| 0x0071C730 | `CTrackManiaRace::GetLocalPlayerInfo()` -> `CTrackManiaPlayerInfo*` | as:52 | verified |
| 0x00405F40 | `CFastStringInt::SetString(const SStringParamInt&)`, ret 4 | as:53 | verified |
| 0x00402530 | `CFastStringInt::Empty()` | as:54 | verified |
| 0x00635870 | `FilterFileName(CFastStringInt& dst, const CFastStringInt& src, int, int)` cdecl | as:55 | verified |
| 0x0063F260 | `FileExists(const CFastStringInt&, CSystemFids* folder)` cdecl | as:56 | verified |
| 0x0068C390 | `CGameCtnReplayRecord::GetBestGhostRaceTime(int&, int&)`, ret 8 | as:57 | verified |
| 0x0068C470 | `CGameCtnReplayRecord::GetBestGhostStuntsScore()` | as:58 | verified |
| 0x0062A850 / 0x0062A950 | `CSystemArchiveNod` ctor / dtor (buffer 0x400 bytes, 16-aligned) | as:59,61 | verified |
| 0x0062E1C0 | `CSystemArchiveNod::Save(name&, nod, folder, 10, 7, 1, 1)` synchronous write, ret 0x1C; game's own autosave calls it at 0x006EBE7A | as:60; T re/q1.txt | verified |
| 0x00623F90 | `CSystemEngine::UnbindFid(CMwNod*)`, ret 4 | as:62 | verified |
| 0x007173B0 | `CTrackMania::ScanDiskForReplays()` | as:63 | verified |
| 0x0070B2C0 | `CGameCtnApp::SaveReplay` fiber (default name `<map>(mm'ss''cc)`, Save-As dialog, `.Replay.Gbx`) | as:3-8; T | verified |
| 0x0073CF10 | net in-game dialog loop; result 0x30 -> SaveReplay(app+0x25C) (call 0x0073D79C) | T | verified |
| 0x006EB540 | solo main loop; result 0x12 / Esc 0x2F -> SaveReplay(app+0x144) | T | verified |
| 0x00771B00 / 0x00771AF0 | `DialogInGameMenu_OnSavePrevReplay` / `_OnSaveReplay` (vtable slot 0x12C) | T re/q1.txt | verified |
| 0x0071B350 | sets race state (+0x25C of player info) | T re/q4.txt | inferred |
| 0x0068AF50 | header `validable` computation (see formats) | notes:3 | verified |
| 0x0068AC00 | header-chunk writer (calls 0x68AF50) | notes:3 | verified |
| 0x00753BD0 | replay viewer button logic | notes:12 | verified |
| 0x007541B0 / 0x007541E0 / 0x00757190 | viewer OnValidate / OnExportToValidate / in-game OnValidateBest | notes | verified |
| 0x0068BB50 | replay merge/load (EventsDuration=0 if > 1 ghost, at 0x0068BF82) | notes:21 | verified |
| 0x00401150 | online init: `game.trackmanianations.com`, `/online_game`, port 443 at 0x00C07368 | online:1-4 | verified |
| 0x004E8710 | HTTP request opener (`HttpOpenRequestA`, secure flags); retry path 0x004E7C6F | online; T re/q15.txt | verified |
| 0x00664210, 0x00689930 | reference "CTrackManiaReplayRecord" / "CConstructionReplayRecord" strings | T re/q9.txt | inferred |

A further 54 functions (e.g. 0x006C4DF0..0x006C6070, 0x00746580..0x007483E0) were studied in decompiled form; the decompilation is not included and they are not named individually here.

## Globals, imports, tables

| VA | Meaning | Source | Status |
|---|---|---|---|
| 0x00BB45D4 | `wchar_t*` used by every empty CFastStringInt | as:65 | verified |
| 0x00C96140 | `CMwEngineMain*`; `->+0x28 ->+0x2C` = `CSystemEngine*` | as:66,350 | verified |
| 0x00ACA340 | IAT `HttpOpenRequestA` | online:30 | verified |
| 0x00ACA338 / 0x00ACA33C | IAT `InternetSetOptionA` / `InternetQueryOptionA` | T re/q14-15 | verified |
| 0x00ACA130 | IAT `GetLastError` | T re/q15 | verified |
| 0x00C16CA0 | member table of the replay record (class 0x2403F) | notes | verified |
| 0x00C171F8 / 0x00C170A4 | strings "Validation of replays recorded in mode %1 not implemented." / "1P-Time" | notes | verified |
| class ids | `CNetFormQuerrySessions` 0x12007000, `CNetFormEnumSessions` 0x12008000, `CNetMasterServerInfo` 0x12019000 | reflection data | verified |
| vtable data | OnFinishLine slot 0x00AF1D0C; solo vtables 0x00AED808, 0x00AEEDD0, 0x00AF1530; net slot 0x00AF4118 | T re/q7,q10 | verified |

## Struct members

| Object | Offset | Meaning | Source | Status |
|---|---|---|---|---|
| CTrackManiaRace | +0x40 | -> CTrackMania (app) | as:69 | verified |
| CTrackManiaRace | +0x140 | object whose name = race settings string | notes | inferred |
| CTrackManiaPlayer | +0x34 | -> CTrackManiaPlayerInfo | as:70 | verified |
| CTrackManiaPlayerInfo | +0x25C | race state 1 racing, 2 finished | as:71 | verified |
| CTrackManiaPlayerInfo | +0x1F4 / +0x1F8 | last race time ms / best time ms (0xFFFFFFFF none) | as:72-73 | verified |
| CTrackMania (app) | +0x144 | current-race replay (solo "Save replay") | as:75 | verified |
| CTrackMania (app) | +0x25C | previous-race replay ("Save previous replay") | as:74 | verified |
| CTrackMania (app) | +0x210 | `CSystemFids*` "My Replays" | as:76 | verified |
| CGameCtnReplayRecord | +0x08 | `CSystemFidFile*` while bound | as:77 | verified |
| CGameCtnReplayRecord | +0x2C | ReplayRaceState | notes | inferred |
| CGameCtnReplayRecord | +0x30 / +0x34 | fid buffer count (1 = from file) / `CSystemFidFile**` | as:78-79 | verified |
| CGameCtnReplayRecord | +0x44 | Duration | notes | verified |
| CGameCtnReplayRecord | +0x48 / +0x4C | ghost count / ghost ptr array | notes | verified |
| CGameCtnReplayRecord | +0x60 | `CGameCtnChallenge*` | as:81; notes | verified |
| CGameCtnReplayRecord | +0x6C / +0x70 | EventsDuration / input events | notes | verified |
| CGameCtnReplayRecord | +0xB4 | != 0 while recording | as:82 | verified |
| CGameCtnReplayRecord | +0xB8, +0xC0, +0xC4, +0xC8, +0xCC | Validate_Version, ExeChecksum, OsKind, CpuKind, RaceSettings | notes | verified |
| CGameCtnReplayRecord | +0xDC | fid of last save (0 = never) | as:83,348 | verified |
| CGameCtnReplayRecord vtable | +0xA0 | stop recording | as:338 | verified |
| CSystemFids vtable | +0xAC | GetFullPath(out CFastStringInt) | as:358 | verified |
| CSystemFidFile | +0x18 | parent `CSystemFids*` | as:80 | verified |
| CGameCtnChallenge | +0x118 | map type (5 = stunts) | as:84 | verified |
| CGameCtnChallenge | +0x124 | name, CFastStringInt | as:85 | verified |
| CFastStringInt | | `{u32 len; wchar_t* ptr}` | as:87 | verified |

Calling convention: game `thiscall` methods are called from C++ as `__fastcall` with a dummy `edx`; every call inside `__try/__except`. Source: as:97.

## Exe variants seen locally (not interchangeable)

| Variant | SHA-256 (start) | Relation | Status |
|---|---|---|---|
| unofficial 1.80 (target of all addresses) | 983bd127 | re-linked build, ~5,600 differing byte runs vs the original | verified (byte diff, this repo's measurement) |
| original 1.80 exe kept in the unofficial patch (`Old_exe`) | 054778cc | same PE timestamp 5ce7272f | verified |
| "patched" install (original + Win10 patch) | 38121332 | 11 small patched runs vs `Old_exe` | verified |

Addresses above are only valid for 983bd127. The eswc-autosave mod checks both SHA-256 and PE timestamp and does nothing otherwise (as:154-174).
