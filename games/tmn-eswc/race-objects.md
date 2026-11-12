# TMN ESWC: the editor slot during a solo race, and where the car is

Exe: TmNationsESWC.exe unofficial 1.80, SHA-256 983bd127..., timestamp 5ce7272f, image base 0x00400000.

Established 2026-11-12: static reading of the code and the reflection tables (capstone), and read-only memory reads of a running game (a campaign solo race, map B-7; nothing written). Status: verified = seen in code/data or read in the running game; inferred = reading of the code, not run.

## The short version

- During an ordinary campaign solo race, `CTrackMania.Editor` (+0x2e4) is **not null**: it holds a `CTrackManiaEditorPuzzle` (class id 0x24017000). A state check of the form "editor slot set -> map editor" reports the map editor for every solo race. Check the race first: with `CurrentRace` set to a solo or network race class, the puzzle editor in the slot is part of the race.
- `CTrackManiaPlayer.Mobil` (+0x24) points at the car itself (`CSceneToyVehicleTrackMania`, 0x0a103000), not at a game-side mobil object. Following one more pointer (+0x14, as on Forever: CGameMobil -> SceneMobil) lands on another scene object (class 0x0a003000) whose fields at the input offsets are zero: the reads look plausible ("no input") but never change.

## Objects read in the running game

App global 0x00caee50 -> CTrackMania (0x24001000). Values from one solo race:

| Member | Offset | Class held (id) | Status |
|---|---|---|---|
| Network | +0x100 | 0x2402f000 | verified |
| Challenge | +0x11c | CConstructionChallenge 0x24003000 | verified |
| clip viewer (no member) | +0x128 | null during the race | verified |
| CurrentRace | +0x140 | CTrackManiaRace1P 0x24018000 | verified |
| switcher (no member name used here) | +0x2e0 | 0x24015000 (CTrackManiaSwitcher in the TMNF id list) | verified |
| Editor | +0x2e4 | 0x24017000 (CTrackManiaEditorPuzzle in the TMNF id list; parent chain CTrackManiaEditorFree 0x24016000 -> CTrackManiaEditor 0x24013000) | verified |
| Editor +0x30 | int | 3 during the race (0x006c9ca0 compares it with 3) | verified (value), meaning inferred |

The same slot content was seen at map load for A-3, D-3, B-7 and G-3 (four separate starts): every campaign solo race.

Race -> local player -> car:

| Step | Offset | Result | Status |
|---|---|---|---|
| CTrackManiaRace1P Players {count, items} | +0x20 / +0x24 | 1 player; TMTargetPlayerInfo (+0x10c) is null in solo | verified |
| player | items[0] | CTrackManiaPlayer 0x24031000 | verified |
| player PlayerInfo | +0x34 | CTrackManiaPlayerInfo 0x24036000 | verified |
| player Mobil | +0x24 | **the car**: CSceneToyVehicleTrackMania 0x0a103000 | verified |
| car +0x14 | | another scene object, class 0x0a003000 (not the car) | verified |
| car +0x3c / +0x40 / +0x44 | | 0x06003000 / 0x0a103000 / 0x01001000 objects | verified (ids only) |

## Car input members (reflection table)

Class 0x0a02b000 (the car's input-bearing base; members 0x0a02b002..0x0a02b004), member entries in .data:

| Member | Offset in the car | Range in the entry | Entry | Status |
|---|---|---|---|---|
| InputSteer | +0xc0 | -1 .. 1 | 0x00bffe64 | verified (table) |
| InputGas | +0xb8 | 0 .. 1 | 0x00bffe90 | verified (table) |
| InputBrake | +0xbc | 0 .. 1 | 0x00bffebc | verified (table) |

A second class, 0x0a05f000, has members of the same names at +0x88 (steer), +0x8c (gas), +0x90 (brake): not the car's. Values changing while driving were not observed yet (the car stood still during the reads); the offsets are from the table only.

## String members: wide or 8-bit (reflection type)

The member entry's first dword is the type. Type 0x29 is a wide string {u32 length, wchar_t*}: `CConstructionChallenge.ChallengeName` (+0x124, type 0x29) reads correctly as UTF-16 in the running game. `CSystemFidFile.FileName` (class 0x0b00a000, member 0x0b00a003, entry 0x00c08588, +0x58) has the same type 0x29, so a challenge info's file name (info +0x14 -> CSystemFidFile -> +0x58) is wide too. Read as 8-bit text it yields only the first letter, and no name ever matches a file looked for. Status: verified (table types, ChallengeName read in game); the FileName read itself is inferred from the type.

`CConstructionChallengeInfo` (0x2400b000) members from the table: Password +0x6c (0x29), GoldTime +0x58, Kind +0x60, CopperPrice +0x64, CollectionId2 +0x74, CopperString +0x78 (type 0x25), PlayMode +0x84, LapRace +0x80, BestTime +0xb0.

## Code around the editor slot

| What | Address | Status |
|---|---|---|
| `CTrackMania::SetEditor(editor)`: releases the old one, stores at +0x2e4 | 0x006f3dc0 | verified |
| callers of SetEditor | 0x006c9628 (clear), 0x006c9d36 (clear), 0x006c9f0b (set, from 0x006c9ca0), 0x006e9fc6 (replay editor) | verified |
| editor switch (thiscall on the switcher, arg = new editor; app at this+0x1c): clears or sets the slot, checks the old one against CConstructionEditorReplay 0x2405d000 and the new one against CTrackManiaEditor 0x24013000 | 0x006c9ca0 | verified (code), purpose inferred |
| its callers: 0x006c9c4e (after making a 0x42c-byte object with ctor 0x007233c0), 0x006e57e9 (reflection setter of CTrackMania member 0x24001002 on the switcher at app+0x2e0) | | verified |
| back to the menu: SetEditor(0) and CurrentRace = 0 together | 0x006c95c0 | verified |
