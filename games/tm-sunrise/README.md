# TM Sunrise (Win10 community repack)

Exe `TmSunrise.exe`, 7,904,768 bytes, SHA-256 e59dfc7eaa5314e85b2b0df6f40ef7ba01a23bd5a9525f84407f5dff301df4fe, PE timestamp 0x5cc8f0cf, base 0x400000. Loads dinput8.dll and d3d9.dll by name; the repack ships its own d3d9.dll wrapper.

Play-a-map-file path (same as ESWC, other offsets). Status: found statically, not yet tested in game.

| What | Sunrise | ESWC 1.80 |
|---|---|---|
| CGameCtnApp* global (set in ctor 0x815640) | 0xbcce50 | 0xcaee50 |
| s_ChallengeInfos {count, items, cap} | 0xbcd1e8 | 0xcaed34 |
| CGameCtnChallengeInfo::Build(fid, folders) | 0x85ede0 | 0x702a60 |
| RefreshFromDisk(folders, 0, 0) cdecl | 0x85fce0 | 0x703990 |
| SortByName() | 0x85fe50 | 0x703b00 |
| game malloc / free | 0x402cc0 / 0x402b90 | 0x4044b0 / 0x4045b0 |
| app+ data folders | 0x1b8 | 0x1f0 |
| app+ challenge type (SetChallengeType) | 0x2a0 (0x824f00) | 0x2e8 (0x730400) |
| menus+ app | 0x2e8 | 0x374 |
| MenuPlayChallenge_Solo: menus+ mode = | 0x354 = 2 (0x7bb660) | 0x3e4 = 0x13 (0x69f8b0) |
| chosen infos buffer (OnOk) | menus+0x318 (0x7bc230) | menus+0x3a4 (0x6a0480) |
| menu result "play" | menus+0x258 = 1 | menus+0x2e4 = 0xf |

Input engine (DirectInput enumeration, device list, binding tables, device loss): same code as ESWC, both columns in [../tmn-eswc/input.md](../tmn-eswc/input.md).

Music volume, sound volume and fullscreen (config, audio port, start order): [../tmn-eswc/audio-display-settings.md](../tmn-eswc/audio-display-settings.md).

Race events (OnStartLine, OnFinishLine, OnCheckpoint, solo race end): [race-events.md](race-events.md). Saving the race replay without the dialog (Sunrise and Original side by side, with ESWC): [replay-save.md](replay-save.md).

Source: own spike code (legacy-randomizer); matched with scripts/re/fnmatch.py (ESWC→Sunrise 1.00 for Build/RefreshFromDisk/SortByName).
