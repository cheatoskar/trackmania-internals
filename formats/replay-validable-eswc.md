# ESWC replay `validable` flag

Exe: TmNationsESWC.exe 0.1.7.9 (unofficial 1.80, SHA-256 983bd127...). Source: own Ghidra notes. Status: verified (Ghidra) unless noted.

The header attribute `validable="0|1"` of an ESWC replay is computed by the game itself when the header chunk is written. It depends only on data inside the replay (input events, number of ghosts), not on any server, ladder or settings flag. This page documents the format only. It deliberately does not describe how to change a replay's validation status: editing replay files or this flag to make a replay pass as validable is forgery. Do not do it, and do not upload such files.

| Fact | Where | Status |
|---|---|---|
| Flag computation | fn 0x0068AF50, called from the header-chunk writer 0x0068AC00 (format push 0x0068B011) | verified |
| Replay viewer: Validate button enabled by the same data; export-to-validate also needs recorded samples | 0x00753BD0 | verified |
| Mode check 0x0068B440: Validate_Version must equal the running exe; RaceSettings must match the loaded race's settings or "1P-Time" in the viewer, so Net-TimeAttack replays are refused ("Validation of replays recorded in mode %1 not implemented.") | 0x0068B440 | verified |

| Case | validable | Status |
|---|---|---|
| Offline / Solo, RaceSettings "1P-Time" | 1 | verified (files) |
| LAN, alone, "Net-TimeAttack" | 1 | verified |
| LAN with several players | 0 | verified |
| Internet dedicated server | 0 (the file holds one ghost per participant plus the server account) | verified (4-ghost sample) |
| `GetValidationReplay` from the server | one ghost without samples and no GBX header; TMX cannot read it | verified |

A 2006 TMX forum note says online replays are "multiplayer format" and only Solo saves validate; that matches the observations.
