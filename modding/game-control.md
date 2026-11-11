# Game control: start a map file in solo, leave the race (ESWC, Sunrise, TMNF, TMUF)

What a mod needs to start a local `.Challenge.Gbx` in solo from the main menu and to leave a solo race, without
hooks or patches. Implemented in the author's gamectl module (used by the TM Horizon launcher). Status:
**statically derived, in-game test pending** (2026-11-11). Update this page when the harness has run.

## Path

1. Copy the map into a folder the game scans (`Tracks\Challenges\Downloaded\_Horizon\`).
   ESWC/Sunrise: `<exe dir>\GameData\Tracks\...`; Forever: `Documents\TrackMania\Tracks\...` (`/useexedir`: exe dir).
2. `CGameCtnChallengeInfo::RefreshFromDisk(folders, ...)` re-reads the folders, then `SortByName()`.
3. Diff `CGameCtnChallengeInfo::s_ChallengeInfos` ({count, items, capacity}) before/after: exactly one new info.
4. Set the menu like the game's own play path does, the menu loop then loads the race:
   - ESWC/Sunrise (`MenuPlayChallenge_Solo` + `MenuChooseChallenge_OnOk`): chosen-infos buffer = {1, [info]},
     `CTrackMania.ChallengeType` = 3, menu mode, menu result.
   - Forever (`CTrackManiaMenus::LaunchManiaCodeAction` tail, TMNF 0x4fbfe5..0x4fbff0): menus+0x13c = info,
     `CTrackMania::SetChallengeType(3)` (= app+0x418), menus+0x704 = 1.
5. Leave: `CTrackManiaRace::ButtonQuitOnClick` (thiscall on the race, `push 2; call X; ret`).

## Table

| | ESWC 1.80 | Sunrise repack | TMNF | TMUF |
|---|---|---|---|---|
| PE timestamp | 5ce7272f | 5cc8f0cf | 4d431494 | 4b9e8add |
| app global | 0xcaee50 | 0xbcce50 | 0xd68c44 (CGameApp::s_TheGame) | 0xd6a2a4 |
| s_ChallengeInfos | 0xcaed34 | 0xbcd1e8 | 0xd69120 | 0xd6a780 |
| RefreshFromDisk | 0x703990 cdecl(folders*, 0, 0) | 0x85fce0 same | 0x5cf100 cdecl(const folders&, callback* or null) | 0x5cf2a0 |
| folders | *(app+0x1f0) | *(app+0x1b8) | app+0x148 (embedded) | app+0x148 |
| SortByName | 0x703b00 | 0x85fe50 | 0x5cdde0 | 0x5cdf70 |
| game malloc / free | 0x4044b0 / 0x4045b0 | 0x402cc0 / 0x402b90 | not needed | not needed |
| ChallengeType | app+0x2e8 | app+0x2a0 | app+0x418 | app+0x418 |
| MenuManager | app+0x104 | app+0x104 | app+0x194 | app+0x194 |
| menus -> app | +0x374 | +0x2e8 | +0x784 | +0x784 |
| chosen | +0x3a4 buffer | +0x318 buffer | +0x13c info* | +0x13c info* |
| mode | +0x3e4 = 0x13 | +0x354 = 2 | - | - |
| result | +0x2e4 = 0xf | +0x258 = 1 | +0x704 = 1 | +0x704 = 1 |
| current race | app+0x2f0 (`Races`) | app+0x2a8 (`Races`) | app+0x454 (SetRace field) | app+0x454 |
| ButtonQuitOnClick | 0x6db380 | 0x7ed580 | 0x47b290 | 0x47b0e0 |
| registry head | 0xc96100 | 0xb8abec | 0xd73ba8 (also 0xd7348c) | 0xd74aec |

TMUF: functions ported from TMNF with `fnmatch.py` (score 1.00 each), offsets read from the TMUF instructions;
the reflection tables of TMNF and TMUF are identical (623 classes, 0 member/offset differences).

## Reading engine objects without symbols

- Class of a live object: vtable slot with `mov eax, imm32; ret` returning a CMwClassInfo (slot 1 in
  ESWC/Sunrise, slot 2 in TMNF/TMUF; the neighbouring GetMwClassId has the same shape but returns an id).
- Registry head for unknown exes: `mov ecx,[head]; mov [eax+18h],ecx; ... mov [head],eax` in the CMwClassInfo
  registration code (matches all four exes above).
- State: race object class `CTrackManiaRace1P*` = solo, `*Net*` = network, `Editor` member set = editor.

## TM Original

No exe yet. gamectl runs a discovery when started on an unknown exe (experimental switch): logs the class
registry, `CTrackMania` members, menu methods and candidate app globals, so one run gives the table column.

## Relocated exes (TMLoader 2.12.0)

TmForever.exe started by TMLoader 2.12.0 loads at a random base (ASLR); the 2.12.0-compat mode and the plain
exes load at 0x400000. All link addresses in the table move by (module base - 0x400000); struct offsets do not.
gamectl/reflect apply this.
