# Old 2006 TM dedicated server (Nations ESWC, Sunrise, Original)

`TmDedicatedServer_2006-05-30.zip`, exe `TrackManiaServer(.exe)` in `TmDedicatedServer/`, SHA-256 `28d79550fdd18623c47d3a428af5ce8e150402b8c4c5fa0b45d3efa4e6bf783d` (identical to the Internet Archive copy, checked 2026-11-09). Game chosen with `/game=nations|sunrise|original`. Same XML-RPC/GBXRemote transport as TMF, far fewer methods.

Source keys: `100_server` = own server controller (C#, a fork of BigBang1112/randomizer-anywhere; file names and line numbers refer to it); `100_website` = own website code (not public); `mem:<note>` = own test notes; `nc` = own fork of Nostalgic Completionist (`Program.cs`).

## Command line and config

| Fact | Source | Status |
|---|---|---|
| Stays LAN unless `/internet`; on Linux it forks into the background unless `/nodaemon` (PID/stop lost otherwise) | 100_server ServerSetup.cs; mem:eswc | verified |
| Ignores `/servername` ("Server not started: no ServerName specified"): name must be in `dedicated.cfg` `<server_options><name>` | ServerSetup.cs; mem:eswc | verified |
| Config file `dedicated.cfg`; `<nation>` (must be valid, e.g. GER) instead of `<validation_key>`; lines may carry `// remarks` after the closing tag (a password may itself contain `//`) | ServerSetup.cs; 100_website admin/gbx.ts | verified |
| `<connection_type>` takes presets only (e.g. `DSL_16384_4096`), no kbit/s | ServerSetup.cs | verified |
| Server name in the list: max 25 chars incl. `$` codes, no `$o` | mem:eswc; controller config | verified |
| Windows build fails to go online: WinINet "http error 12157" (master server speaks TLS 1.0 only) -> test online on Linux | mem:eswc | verified |

## Callbacks and polling

| Fact | Source | Status |
|---|---|---|
| Sends BeginRace, EndRace, PlayerConnect/Disconnect, PlayerChat; NO StatusChanged, NO PlayerCheckpoint | RandomizerGame.cs:922-1000 | verified |
| DOES send `TrackMania.PlayerFinish` (an older comment in the same file says it does not). Combined with polling this doubled finishes until a (login,time) de-dupe was added | RandomizerGame.cs:3158-3170; mem:eswc | verified |
| Workable finish detection: poll `GetCurrentRanking(200,0)` every 1 s; a changed BestTime > 0 is a finish; first poll after a map change = baseline only | RandomizerGame.cs:922-1030 | verified |
| `NextChallenge` answers OK during `Synchronization` and then stays on the map: wait for status Play (4) first (up to 15 s) | RandomizerGame.cs:3361-3388 | verified |
| `/verbose_rpc` would write ~10 MB/day while polling -> disabled | controller config | verified |

## Methods (live check 2026-11-09)

| Missing ("Method not defined") | Present |
|---|---|
| SendDisplayManialinkPage(+ToId/ToLogin), SendHideManialinkPage, ForceSpectator, SpectatorReleasePlayerSlot, GetDetailedPlayerInfo, GetCurrentCallVote, SaveBestGhostsReplay, AutoSaveValidationReplays, SetRoundForcedLaps, GetChatCommandList | ChatSend, ChatSendServerMessage(ToLogin), Kick, Ban/UnBan, CleanBanList, BlackList, GetPlayerList, GetPlayerInfo, GetCurrentRanking, NextChallenge, ChallengeRestart, GetCurrentChallengeInfo, GetStatus, Get/SetServerName, GetChatLines, CallVote, GetLastConnectionErrorMessage, GetValidationReplay |

Source: 100_website; mem:eswc. Status: verified. Consequences: no manialinks (no widgets), no `/afk`, chat links (`$l`) show literally, so post plain URLs.

| Other fact | Source | Status |
|---|---|---|
| `SetLadderMode` -> "Ladder mode can't be changed in TrackMania Nations" (always forced) | mem:eswc | verified |
| `GetValidationReplay` returns validation data WITHOUT a GBX header (userdata 0): TMX cannot read it | own ESWC RE notes; mem:eswc | verified |
| Removing the `GetValidationReplay` call did not change validable=0 of client-saved online replays (the cause is the ghost count, see [../formats/replay-validable-eswc.md](../formats/replay-validable-eswc.md)) | mem:eswc | verified |
| Old client used as LAN "server" (Nostalgic Completionist): XML-RPC on the running GAME client (port 5000) - `GetChallengeList(int.MaxValue,0)` + `RemoveChallengeList`, `AddChallengeList`, `SetGameMode 1`, `SetTimeAttackLimit 0`, `SetChatTime 0`, `StartServerLan`. TMS/TMO clients need a manual Refresh in the track browser before `AddChallengeList` sees new files; no callbacks -> watch the replay folder | nc; nc-fork README | verified |
| ESWC `GetCurrentChallengeInfo.FileName`/`MapName` may start with U+FEFF (BOM) when the path has non-ASCII chars: `TrimStart('﻿')` | nc ~line 255 | verified |
| A LAN server over the internet is not joinable: client ignores `/join`, LAN menu has no address field | mem:eswc | verified |

## Master-server login errors

| Symptom | Meaning / fix | Source | Status |
|---|---|---|---|
| ConsoleLog only says "...Server stopped" | call `GetLastConnectionErrorMessage` (SuperAdmin) to get the reason | mem:eswc | verified |
| "bad password" | wrong or nonexistent account. Auto-registration of new Nations server accounts no longer happens: create the server account in the ESWC game first | mem:eswc | verified (contradicts code comments, see contradictions) |
| "already connected (from a different ip)" | the account is logged in elsewhere (e.g. your own game): never play with the server account | mem:eswc | verified |
| Wrong password -> dedicated exits -> systemd restart loop | | mem:tmuf-instance-setup | verified |

ESWC master web service details (TLS 1.0, endpoints) are in [masterserver.md](masterserver.md).
