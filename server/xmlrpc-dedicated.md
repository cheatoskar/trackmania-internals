# TMF dedicated server and XML-RPC (GBXRemote)

Covers the TMF dedicated server (`TrackmaniaServer_2011-02-21`, used for TMNF and TMUF) as driven by a server controller (own fork of BigBang1112/randomizer-anywhere, C#, ManiaAPI.XmlRpc). The 2006 server is in [old-2006-server.md](old-2006-server.md). ManiaLinks (controller UI) are in [manialinks.md](manialinks.md), running servers in practice in [operations.md](operations.md).

Source keys: `100-TMX-Server` and bare `*.cs` names = [100-TMX-Server](https://github.com/cheatoskar/100-TMX-Server) (fork of randomizer-anywhere; line numbers refer to it); `website` = the author's website code (private); `ANALYSE` = own server analysis report; `mem:<note>` = own operating notes.

## Wire protocol

| Fact | Source | Status |
|---|---|---|
| Banner: u32 length + ASCII `GBXRemote 2` | website code (private) src/lib/admin/gbx.ts:12-20 | verified |
| Frame: `u32 LE length, u32 LE handle, XML payload`; client handles start at 0x80000000, wrap 0xFFFFFFFE -> 0x80000001 | gbx.ts:252,331 | verified |
| Callbacks arrive on the same socket with their own handles; match replies by handle | gbx.ts | verified |
| Auth: `Authenticate("SuperAdmin", <pw>)`; keep the port on loopback (`<xmlrpc_allowremote>False`) | 100-TMX-Server [RemoteClient.cs](https://github.com/cheatoskar/100-TMX-Server/blob/main/RandomizerAnywhere/RemoteClient.cs), [ServerSetup.cs](https://github.com/cheatoskar/100-TMX-Server/blob/main/RandomizerAnywhere/ServerSetup.cs) | verified |
| `system.listMethods` at connect lets a client feature-detect (2006 server lacks many methods) | RemoteClient.cs | verified |

## Limits and timing

| Fact | Source | Status |
|---|---|---|
| TMF drops the connection on any request > 512 KB (XAseco's GbxRemote enforces the same). `WriteFile` payload is base64 (x4/3) + envelope: keep data under `512*1024-8` minus ~4 KB envelope; bigger maps: write straight into `GetTracksDirectory` when on the same host. A 934 KB map via `/votemap` took the server down twice (2026-10-10) | RemoteClient.cs ~214-230; ANALYSE | verified |
| ManiaAPI.XmlRpc <= 2.9.2 registered a call's reply slot AFTER sending: a fast reply was thrown away, the call hung, callbacks froze 60 s. Fixed in 2.9.5 (upstream PR #1) | RemoteClient.cs; mem:tmf-logs-for-command-failures | verified |
| ManiaAPI dispatches callbacks from ONE loop: one blocked handler freezes chat commands, widgets, CP HUD while plain request/response still works | RemoteClient.cs:485-491 | verified |
| Controller budget used: 10 s per call, slow-call log at 2 s, handler timeout 60 s, slow handler warning 15 s | RemoteClient.cs | verified |
| After a redial, re-attach handlers BEFORE `EnableCallbacks`; whether chat-command registration is per connection is undocumented, so re-register | RemoteClient.cs:468-618 | inferred |
| Dedicated rejects calls during a map transition ("Change in progress"); controller retries for 10 s with 700 ms gaps | [RandomizerGame.cs](https://github.com/cheatoskar/100-TMX-Server/blob/main/RandomizerAnywhere/RandomizerGame.cs):3389-3407 | verified (code; see contradictions) |
| `SetGameMode`, `SetRoundForcedLaps`, `SetNbLaps`, `SetForceShowAllOpponents` only apply after `ChallengeRestart`/`NextChallenge` | 100-TMX-Server [IDEAS.md](https://github.com/cheatoskar/100-TMX-Server/blob/main/IDEAS.md) | verified |
| `connection_uploadrate` default 512 kbit/s (~64 KB/s total); fork uses 10240 (recommended 10240-20480, "plausible, not measured") | ANALYSE; config.default.toml | inferred |

## Methods and callbacks seen in use

| Kind | Names | Source |
|---|---|---|
| Methods (TMF) | Authenticate, EnableCallbacks, AddChatCommand (multicall), SetServerName, GetVersion (Build `yyyy-MM-dd_HH_mm`), GetPlayerList(200,0), GetPlayerInfo, GetDetailedPlayerInfo, GetCurrentRanking(200,0), GetChallengeInfo(file), GetCurrentChallengeInfo, GetChallengeList, GetStatus, GetTracksDirectory, WriteFile, InsertChallenge, ChooseNextChallenge, RemoveChallenge, NextChallenge, ChallengeRestart, SetGameMode (0 Rounds, 1 TimeAttack), SetTimeAttackLimit, SetRoundForcedLaps (0 = map laps), SetChatTime, CallVote, CancelVote, GetCurrentCallVote, ForceSpectator(login, 0/1/2), SpectatorReleasePlayerSlot, SaveBestGhostsReplay, AutoSaveValidationReplays, CleanBanList, ChatSend, ChatSendToLogin, ChatSendServerMessage(ToLogin), SendDisplayManialinkPage(ToLogin), SendHideManialinkPage, StartServerLan, Kick, Ban/UnBan, BlackList, GetChatLines | RemoteClient.cs, RandomizerGame.cs (verified) |
| Callbacks | TrackMania.BeginRace, EndRace, PlayerCheckpoint, PlayerConnect, PlayerChat, PlayerFinish, StatusChanged, EndRound, PlayerManialinkPageAnswer(uid, login, int answer) | RandomizerGame.cs:535-820 (verified) |
| Status codes | Launching 2, Synchronization 3, Play 4, Finish 5 | TrackManiaStatusCode.cs (verified) |

## Behaviour quirks

| Quirk | Source | Status |
|---|---|---|
| `GetChallengeInfo` on a queued, not yet loaded map may report `NbCheckpoints = -1`; use `GetCurrentChallengeInfo` for the running map | RemoteClient.cs:415-420 | verified |
| `NbCheckpoints` counts the finish line: 2 CP blocks -> 3 | RemoteClient.cs ToChallengeSummary | verified |
| `InsertChallenge` "already added": de-dupe by UID over the whole selection (which only grows); reuse with `ChooseNextChallenge` | RandomizerGame.cs:3413-3465 | verified |
| `InsertChallenge` "Track not complete" = map never validated (no author time); treat as permanently unplayable | RandomizerGame.cs:3540-3600 | verified |
| One vote at a time: second `CallVote` -> "Already waiting for a vote"; check `GetCurrentCallVote` first. TMF's vote text says "type /yes" but `/yes` only reaches controller votes | RandomizerGame.cs:3620-3650; ANALYSE | verified |
| `GetChatCommandList` does not exist on TMF ("Method not defined") | ANALYSE | verified |
| `$l[url]text$l` links work in TMF chat (literal text on the 2006 server) | ANALYSE | verified |
| Manialink coordinates roughly +-64 X, +-48 Y (use posn/sizen); `SendHideManialinkPageToId` wants an INT id (string -> "Value of type STRING supplied where type INT was expected"); click = `action="<int>"` -> `PlayerManialinkPageAnswer` | IDEAS.md | verified |
| `TrackMania.BeginRace` is missed for a map that finished loading before any player connected; query `GetCurrentChallengeInfo` on demand | IDEAS.md | verified |
| Ladder anti-cheat auto-bans legit RPG/trial/stunt finishes for "Time incoherence" (`<incoherence> [login] was banned.`), preceded by "Invalid time: the race of X will be ignored"; ban list is in memory -> `CleanBanList` at start | website docs (private); RandomizerGame.cs:1095-1110 | verified |
| Dedicated keeps ghost sockets on 2350: count connects minus disconnects in ConsoleLog, not TCP connections | website docs (private) | verified |
| Replays: `SaveBestGhostsReplay(login, "<dir>/<file>.Replay.Gbx")` writes under `GameData/Tracks/Replays/`; server-saved replays are what TMX accepts | RandomizerGame.cs SendReplayLinkAsync; website docs (private) | verified |
| Custom-block maps that crash every client show in logs as many joins with 0 checkpoints; handled with a live-reloaded skip list | mem:gameserver-analysis-2026-10-16 | verified |
| `inunlimiter=0` on TMX is uploader-declared; an Unlimiter map slipped through and stalled the server 2h18m (2026-09-02) | website docs (private)/gbx-detection-plan.md | verified |

## Config and command line

| Item | Fact | Source | Status |
|---|---|---|---|
| cfg | `GameData/Config/dedicated_cfg.txt`; keys touched: `<packmask>` (`nations` for TMNF, empty for United = only TMUF owners), `<xmlrpc_port>`, `<server_port>`, `<server_p2p_port>`, `<masterserver_account>` (`<login>`, `<password>`, `<validation_key>`), `<use_changing_validation_seed>False`, `<connection_uploadrate>`, `<xmlrpc_allowremote>`, `autosave_replays`, `autosave_validation_replays` | ServerSetup.cs SetupDedicatedCfgAsync | verified |
| command line | `/game_settings=<path> /servername=<n> /verbose_rpc /comment=<c> /dedicated_cfg=dedicated_cfg.txt /lan /bindip=<ip>` | ServerSetup.cs StartServer | verified |
| MatchSettings | `<gameinfos>` game_mode 1, chat_time 0, finishtimeout 1, timeattack_limit 600000, laps_nblaps 5, ...; `<filter>` is_lan/is_internet 1, random_map_order 0; `<challenge><file>` | ServerSetup.cs | verified |
| Ports used by the fork | TMNF 2350/3450, XML-RPC 5000; TMUF 2351/3451, 5001; ESWC 2352/3452, 5002. Client LAN discovery scans 2350-2359 (TMF), 2350-2369 (ESWC), 2350 only (TMS/TMO) | 100-TMX-Server README; CANNOT_SEE_SERVER.txt | verified |
| A wrong master-server password makes the dedicated exit -> restart loop under systemd | mem:tmuf-instance-setup | verified |
| Local Windows test without `Lan = true`: master-server registration fails and the process dies ~10-15 s after boot | DEPLOY_NOTES.md | verified |

## Logs

| File | Content | Source | Status |
|---|---|---|---|
| `Logs/GameLog.<serverlogin>.txt` | full chat incl. every `/command` and server replies; `[YYYY/MM/DD HH:MM:SS] <chat> [login (nick)] text`, local time, never rotated | mem:tmf-logs-for-command-failures; website code (private) admin/chat.ts | verified |
| `Logs/ConsoleLog.<pid>.txt` | with `/verbose_rpc`: every XML-RPC request + "XmlRpc request succeeded" -> tells "server never answered" from "answer lost in client"; 15-31 MB per run | mem:tmf-logs-for-command-failures | verified |

## Local test harness (no live server needed)

Run the controller with `Lan = true`, own XML-RPC port (e.g. 5010); it downloads and starts a Windows TMF server in LAN mode. Send `ChatSend("/votemap 123")` over XML-RPC as SuperAdmin: the controller treats it like a player's chat command; read replies with `GetChatLines`. Slow TMX can be simulated with `HTTPS_PROXY` pointing to a CONNECT proxy that delays. Source: mem:local-lan-test-harness (verified 2026-10-22).
