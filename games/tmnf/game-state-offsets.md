# TMNF: game-state offsets, race/replay functions, menu and ManiaCode functions

Exe: TMNF `TmForever.exe` 2.11.26, PE timestamp 4d431494. RVAs are relative to the module base; VAs assume base 0x00400000. Function names come from `TmForever.map`; a sample (0x0062FBE0, 0x005F5110, 0x00480820, 0x004FC0D0, 0x005E7530, 0x00935400) was re-checked against the map for this repo.

Caution: the plugin's own build key is `4d431494-00a62000`, i.e. SizeOfImage 0xa62000 as seen at runtime, while the file on disk says 0xa2b000 (both measured). The plugin reads the loaded image; whether TMLoader changes the header in memory is unverified.

Source keys (short names for own project sources, not part of this repo): `S` = server controller of a TMNF/TMUF randomizer server project; `W` = its website; `P` = 100 TMX Bingo plugin (own TMLoader mod); `H` = own launcher project; `A`/`T` = ESWC autosave tool; `R` = own RE tools (ESWC); `N` = a GBX tool fork; `M` = own RE notes. "Twinkie" = third-party MIT TMNF mod whose published offsets the plugin reused.

## In-game state offsets (read by 100 TMX Bingo plugin)

| Address / path | Meaning | How established | Source | Status |
|---|---|---|---|---|
| exe+0x972EB8 | `CGameApp*` global (`app`) | Twinkie TwinkTrackmania.cpp, validated live at attach (UID must look like a map UID) | P\src\game.cpp:198; W\docs\game-link.md | verified (works on this build) |
| exe+0x5357D0 | `CMwId::GetName(unsigned* id, CFastString* out)` `__thiscall` (Twinkie writes 0x3357D0+0x200000 "might be modloader related"; plugin says it is simply the function RVA; a profile calling 0x3357D0 landed in `CGameCtnMediaBlockText::SetStyleColor`). Prologue must be `6A FF 68 ... 64 A1 00 00 00 00` (push -1, push handler, mov eax,fs:[0]) else not called | plugin analysis | P\src\game.cpp:100-130,190-200 | verified |
| app+0x198 | `CGameCtnChallenge*`, null in menus | Twinkie | P\game.cpp:200 | verified |
| app+0x454 | race object, null outside a race | Twinkie | P\game.cpp:201 | verified |
| app+0x12C | network object; `->+0x2FC` = player-info buffer (size at +0x2FC, items at +0x300), entry 0 = local player on and off a server | plugin RE | P\game.cpp:634,904-907,962-967 | verified |
| challenge+0xDC (220) | map UID as id index (not string) -> GetName | Twinkie | P\game.cpp:202 | verified |
| challenge+0x108 | map name CFastStringInt (wchar) | Twinkie | P\game.cpp:203 | verified |
| challenge+0x60 | checkpoint buffer (its size = count) | Twinkie | P\game.cpp:210 | verified |
| race+0x330 | local player-info "nod" -> +0x238 player -> +0x1C = PlayerInfo (loop closes: info+0x238 is its player, player+0x1C is the same info); fallback race+0x28 -> +0x0 -> +0x1C | Twinkie/brokenphilip + CE verified | P\game.cpp:203-207,618-626,901-967 | verified |
| race+0x24 / race+0x28 | player count / player array pointer (count must be 1..256) | | P\game.cpp:871-872,929 | verified |
| PlayerInfo+0x2A8 prev race time, +0x2B0 race_time (ms), +0x2B4 best time, +0x2CC NbCompleted, +0x2E4 current checkpoint (SetCurrentCheckpoint writes it), +0x314 race_state (0 before start, 1 running, 2 finished), +0x330 lap CP count, +0x334 CP count, +0x33C race_finished, +0x340 display speed (km/h), +0x344 finish not passed | TMInterface `PlayerInfoStruct` + ModTMNF reflected layout, "tested live: pause leaves 1, finish flips to 2 same tick, on record and non-record runs" (0.9.2, 2026-09-23). `0x2C8` was MinRespawns (not checkpoints). | P\game.cpp:810-823; P\docs\finish-marker.md | verified |
| PlayerInfo vtable | must lie in [exeBase, exeBase+0x2000000) | sanity | P\game.cpp:917 | verified |
| PlayerInfo+0x2BC | "lap time" (restarts every lap); `g_cachedTimeOffset` default 0x2BC (older path) | | P\game.cpp:294,595-600 | inferred (superseded by 0x2B0) |
| older candidate chains (heuristic / CE rows) | base+0x966E1C -> +0x6C -> ... +0x24 ; base+0x4DAE74 -> +0x12C -> +0x18 -> +0x24 ; app+0x12C -> +0x1AC/+0x23C -> +0x78/+0x20 -> +0x2C/+0x24 ; race+0x44/+0x620 chains | Cheat Engine rows, "not stable across maps" | P\game.cpp:618-720,1397-1412; P\finish-marker.md ("race+0x44 -> +0x28 -> +0x24 once race+0xB8 -> +0x24 -> +0x20") | unverified (explicitly flaky; calibrated chain e.g. `0x454/0x28/0x0/0x1C/0x2B0` used as shipped chain) |
| calibrated time chain | `0x454/0x28/0x0/0x1C/0x2B0` (app->race->player->0->sub->race_time) | CE verified | P\game.cpp:339-340 | verified |
| D3D9 vtable indices | IDirect3DDevice9: Reset 16, Present 17, EndScene 42; IDirect3D9: CreateDevice 16, CreateDeviceEx 20 | standard d3d9 | P\hook.cpp:44-46,256-257 | verified |

## TMNF function addresses from Ghidra map (TmForever.exe 0x4d431494, with .map)

| Address | Name | Source | Status |
|---|---|---|---|
| 0x0062FBE0 / 0x0062FBF0 | CGameCtnMenus::DialogInGameMenu_OnSaveReplay / _OnSavePrevReplay | T\re\tmnf_funcs.csv | verified |
| 0x005F5110 | CGameCtnApp::SaveReplay | tmnf_funcs.csv | verified |
| 0x004806E0 | CTrackManiaRace::StopReplayRecordAndKeepCopy | tmnf_funcs.csv | verified |
| 0x00480820 | CTrackManiaRace::OnFinishLine | tmnf_funcs.csv | verified |
| 0x00472300 | CTrackManiaRace1P::OnFinishLine | tmnf_funcs.csv | verified |
| 0x004655B0 | CTrackManiaRaceNetTimeAttack::OnFinishLine | tmnf_funcs.csv | verified |
| 0x004B6B20 | CTrackManiaRaceNet::OnFinishLine | tmnf_funcs.csv | verified |
| 0x00483440 | CTrackManiaRace::DelayedFinishLine (also calls KeepCopy at 0x48351E) | T\re\tmnf\c, q4 | verified |
| 0x0064EA80 | CGameCtnNetwork::AutoSaveReplays | tmnf_funcs.csv | verified |
| 0x004CC9C0 | CTrackManiaMenus::DialogInGameMenu | tmnf_funcs.csv | verified |
| 0x005004D0 | CTrackManiaIPCCallbacks::PlayerFinish | tmnf_funcs.csv | verified |
| 0x004E3560 (DoIngameDialog), 0x00491080 (ChallengeMainLoop), 0x00497830 (UpdateGame), 0x004E4A00 (MainLoop_RoundPlay), 0x00482760 (Validate), 0x004824A0 (SwitchFromRace), 0x004831F0 (ResetPlayer), 0x0071B860 (SF_17) | CTrackMania* / CGameCtn* function names | T\re\q4.txt (xref output names) | verified |

## Menu / ManiaCode / challenge-loading functions (Ghidra decompiles for the legacy randomizer plan)

Source keys (short names for own project sources, not part of this repo): `tmvs` = TM Vibrant Shaders mod source; `tmvs-mp` = its ManiaPlanet port; `tmvs[X]` = feature branch X of it; `tmvs-notes` = its notes; `phys` = own physics RE notes; `tmnfc` = TMNF-C (third-party MIT reimplementation, github.com/adonis-singh/TMNF-C); `dz` = own DeltaZero project; `xr` = TrackManiaForeverOpenXR (upstream jiink/TrackManiaForeverOpenXR); `tm-re` = own RE tools. Status: verified = seen in code/data or measured; inferred; unverified = prose only.

| TMNF VA | name | source | status |
|---|---|---|---|
| 004fc0d0 | DoMenus | tm-re\rmc\out\tmnf_launchmc.c / gh_last.log | verified (decompile exists) |
| 004fb950 | LaunchManiaCodeAction | tmnf_launchmc.c | verified |
| 004d9040 / 004d9180 / 004d9410 | MenuChooseChallenge_OnRefresh / _OnOk / MenuPlayChallenge_Solo | tmnf_menuplay.c | verified |
| 004bb8b0 | StartChallenge | tmnf_menuplay.c | verified |
| 005f1290 / 005fafe0 | OnTrackReceivedForManiaCodePlayTrack (two classes) | tmnf_onrecv.c | verified |
| 005faa50 | ProcessManiaCodePlayTrack | tmnf_playtrack.c | verified |
| 005e7530 | CmdLineFile_Open | tmnf_open.c | verified |
| 005e9f00 | AddNewChallenge | tmnf_playtrack.c | verified |
| 005e4830 / 005e43c0 | ScanDiskForReplays / GetWritableDir | tmnf_*.c | verified |
| 005ceef0 / 0059e110 / 0042f320 / 0041d440 | Build / CGameApp ctor / UpdateTree / FindOrAddFidAt | tmnf_*.c | verified |
| unnamed FUN_ at 00707820, 00707ca0, 00707df0, 006de510, 00703990, 00703b00, 006e5c80, 006a3f90, 006a43f0, 006deb90, 006e61c0, 006a0480, 0069fba0, 0070eb80, 00702a60, 0062e1c0, 0063f260, 006ddba0 | decompiled but unnamed | rmc\out | verified (existence only; meaning not documented) |

(rmc\out files are TMNF: prefix `tmnf_`, symbols from the map. status inferred for the exe being TMNF.)
