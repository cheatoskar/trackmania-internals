# Running TMF dedicated servers in practice

Operating notes from three public servers (TMNF, TMUF, and a TMN ESWC server on the 2006 build) that ran for about three months on one small Linux cloud VM, driven by the controller [100-TMX-Server](https://github.com/cheatoskar/100-TMX-Server). The XML-RPC side is in [xmlrpc-dedicated.md](xmlrpc-dedicated.md), the 2006 server in [old-2006-server.md](old-2006-server.md), player UI in [manialinks.md](manialinks.md). Nothing here is about the master server's login protocol (see the scope note in [masterserver.md](masterserver.md)).

Source keys: `SS` = [ServerSetup.cs](https://github.com/cheatoskar/100-TMX-Server/blob/main/RandomizerAnywhere/ServerSetup.cs), `RG` = [RandomizerGame.cs](https://github.com/cheatoskar/100-TMX-Server/blob/main/RandomizerAnywhere/RandomizerGame.cs), `TR` = [TmxRules.cs](https://github.com/cheatoskar/100-TMX-Server/blob/main/RandomizerAnywhere/TmxRules.cs), `README` = [100-TMX-Server README](https://github.com/cheatoskar/100-TMX-Server/blob/main/README.md), `deploy` = [deploy/deploy-when-empty.sh](https://github.com/cheatoskar/100-TMX-Server/blob/main/deploy/deploy-when-empty.sh) and [deploy/game-service-hardening.conf](https://github.com/cheatoskar/100-TMX-Server/blob/main/deploy/game-service-hardening.conf), `c:<hash>` = commit message in 100-TMX-Server (`https://github.com/cheatoskar/100-TMX-Server/commit/<hash>`), `ANALYSE` = own analysis of code and three months of production logs (2026-10-16), `own notes` = the author's operating notes. Status as in the [README](../README.md#confidence-levels).

## Server account and config

| Fact | Source | Status |
|---|---|---|
| A server that should appear on the internet needs its own server account (login, password; TMF also a validation key) created on the official site / in the game. Never use your player account for it | SS; own notes | verified |
| Do not log in to the game with the server's account while the server runs: the master server refuses the server with "already connected (from a different ip)" | own notes (2006 server) | verified |
| A wrong account password makes the dedicated exit at once; under systemd with `Restart=` that is an endless restart loop. Stop the unit, fix the config, then start | own notes | verified |
| The TMUF server's account was made from a United player account. Whether a Nations account works for a United server was not tested | own notes | unverified |
| Config file `GameData/Config/dedicated_cfg.txt` (keys in [xmlrpc-dedicated.md](xmlrpc-dedicated.md#config-and-command-line)). `<packmask>`: `nations` for a TMNF server; empty for a TMUF server, which then only TMUF owners can join and only they see | SS; README | verified |
| `<use_changing_validation_seed>False`: with the changing seed, press-forward lines drifted differently per player and block-mixed starts clipped under the start block during the countdown, unlike offline. Server-saved ghost replays do not need it | c:[d17c509](https://github.com/cheatoskar/100-TMX-Server/commit/d17c509); SS | verified (observed), mechanism inferred |
| `<connection_uploadrate>` ships as 512 kbit/s, about 64 KB/s for ALL players together; every player downloads each new map from the server, so joins and map changes crawl with several players. 10240 was used | ANALYSE; c:[b40f357](https://github.com/cheatoskar/100-TMX-Server/commit/b40f357) | inferred (effect not measured) |
| `<xmlrpc_allowremote>False` and the XML-RPC port never opened in any firewall: controllers commonly keep the default `SuperAdmin` credentials | SS; ANALYSE | verified |
| Settings changed over XML-RPC (`SetGameMode`, laps, `SetForceShowAllOpponents`) only apply after the next `ChallengeRestart`/`NextChallenge` | [IDEAS.md](https://github.com/cheatoskar/100-TMX-Server/blob/main/IDEAS.md) | verified |
| Keep the in-list name (with `$` codes) apart from any name used for folders, URLs or a web page: codes in a folder name break download links and show as raw `$0BF` on the web | c:[ea577ab](https://github.com/cheatoskar/100-TMX-Server/commit/ea577ab) | verified |

## Ports and firewall

| Fact | Source | Status |
|---|---|---|
| Per server: game port (default 2350) and P2P port (default 3450), both TCP and UDP, open to the internet; XML-RPC (default 5000) local only; plus whatever web port the controller uses for replays or a status page | README; own notes | verified |
| Several servers on one host: each needs its own game, P2P and XML-RPC port (here 2350/3450/5000, 2351/3451/5001, 2352/3452/5002) and its own server account | README | verified (live) |
| Keep LAN servers inside the client's discovery range: TMF scans 2350-2359, ESWC 2350-2369, TM Sunrise/Original only 2350 (the client's "Port Broadcast Length" option widens it) | randomizer-anywhere setup text (`CANNOT_SEE_SERVER.txt`, generated at start) | verified |
| A local test server next to a running client: the client already holds UDP 2350, so give the test server 2351 | own notes | verified |
| On a cloud VM there are two firewalls: the provider's security list / security group AND the VM's own (iptables). A port open in only one is still closed | own notes; README (TMUF/ESWC setup) | verified |
| At home: forward game and P2P ports (TCP and UDP) on the router by hand. UPnP was never used or tested here | - | recommendation |

## "I cannot see the server"

| Cause | Check / fix | Source | Status |
|---|---|---|---|
| LAN: the server bound to the wrong network interface (several adapters: VPN, virtual switches, link-local `169.254.x.x`) | set `/bindip=<the LAN adapter's address>`; the controller writes a list of the machine's IPv4 addresses to try | randomizer-anywhere setup | verified |
| LAN: game port outside the discovery range | see Ports | randomizer-anywhere setup | verified |
| Started with `/lan` (or a controller's `Lan = true`): no master-server registration, so not in the internet list | remove it for an internet server | SS | verified |
| 2006 server: stays LAN-only unless started with `/internet` | add `/internet` | SS | verified |
| TMUF server with an empty packmask: invisible to TMNF players by design | `nations` packmask if TMNF players should join | SS | verified |
| Master-server registration failed: the dedicated stops; on a Windows PC in internet mode it died 10-15 s after boot (LAN mode worked; cause not investigated) | read the reason with `GetLastConnectionErrorMessage` (2006 server; the console log only says "Server stopped"); check account and password | own notes | verified |
| A LAN server is not joinable over the internet: the client has no address field in the LAN menu | run an internet server | own notes (ESWC) | verified |
| Players see the server but get kicked or crash on join | the running map uses TMUnlimiter or custom blocks: vanilla clients cannot load it. See maps below | RG; own notes | verified (live) |

## Hosting

| Fact | Source | Status |
|---|---|---|
| The TMF server zip contains a native Linux build; all three servers ran on Linux x86_64 (1 GB RAM + 1 GB swap, 2 vCPU). An ARM host was not tried | own notes | verified |
| Load at 9-11 players: dedicated 4-10 % CPU, load about 0.1; controller 93-135 MB RAM, peak 319 MB of 954 MB | own notes; ANALYSE | verified |
| On a shared cloud VM, watch CPU steal time (`/proc/stat`): time the host gave to other guests shows as lag, not as your own CPU use | c:[abfe7b6](https://github.com/cheatoskar/100-TMX-Server/commit/abfe7b6) | verified |
| Two servers on one host: find "your" dedicated by the PID you started, not by process name (by name, two controllers computed nonsense CPU numbers from each other's process) | c:[abfe7b6](https://github.com/cheatoskar/100-TMX-Server/commit/abfe7b6) | verified (live bug) |
| The Linux TMF dedicated forks into the background (parent PID 1): the starting process only notices its death through the XML-RPC socket. Run it under systemd with `KillMode=control-group` so a stop also stops it. The 2006 server has `/nodaemon` for this | ANALYSE; SS | verified |
| Run each server as its own unprivileged user with systemd sandboxing, not as root (the controller's web server faces the internet) | README (ESWC unit); ANALYSE | verified |
| systemd: `RestartPreventExitStatus=78` for config errors (otherwise a broken config restarts forever), `LimitCORE=0` (core dumps filled the disk on crash loops), a start limit | deploy; c:[dd55542](https://github.com/cheatoskar/100-TMX-Server/commit/dd55542) | verified |
| The Windows build of the 2006 server cannot go online on current Windows (WinINet error 12157, TLS 1.0 master server); the Linux build can | [old-2006-server.md](old-2006-server.md) | verified |
| Wine was only used for the game client (needs `d3dx9_31`), never for a server | own notes; [../gotchas.md](../gotchas.md) | verified |
| Download the server zip from a fixed source and pin its SHA-256: it is fetched over plain HTTP and then executed | c:[05cbc5a](https://github.com/cheatoskar/100-TMX-Server/commit/05cbc5a) | verified |

## Restarts and updates without kicking players

A restart of the dedicated drops every player. The working pattern: change nothing while anyone is on.

| Fact | Source | Status |
|---|---|---|
| "Empty" = 0 players in several polls in a row (here 4 x 30 s, 2 minutes), re-checked on the server right before the swap. A player count from a status file only counts if its timestamp is fresh (< 45 s): a dead controller's last status file is still served | deploy; own script (reboot-when-empty) | verified |
| Run the server-side part as its own detached unit/script: a laptop that sleeps or a dropped SSH session must not stop a deploy between "files swapped" and "restart" | c:[dd55542](https://github.com/cheatoskar/100-TMX-Server/commit/dd55542) | verified |
| Stage uploads on persistent disk, not `/tmp`: a reboot during the wait emptied `/tmp` and the old script reported "DEPLOYED" without installing anything | c:[dd55542](https://github.com/cheatoskar/100-TMX-Server/commit/dd55542) | verified |
| Replace files (`cp --remove-destination`), never overwrite in place: the running process has the old DLLs mapped | c:[dd55542](https://github.com/cheatoskar/100-TMX-Server/commit/dd55542) | verified |
| Never ship server-owned data (config, leaderboard, skip lists, map cache, the dedicated itself) with a build | c:[dd55542](https://github.com/cheatoskar/100-TMX-Server/commit/dd55542) | verified |
| "Healthy after restart" needs a status timestamp newer than the restart, twice in a row: the old process's file passed as healthy 6 s after a restart while the new build was still on attempt 1/3. Roll back automatically, then health-check the rollback too | c:[6d43acc](https://github.com/cheatoskar/100-TMX-Server/commit/6d43acc), c:[dd55542](https://github.com/cheatoskar/100-TMX-Server/commit/dd55542) | verified (live) |
| A watchdog that restarts a wedged server must pause during a deploy, or it restarts the service a second time while it boots | c:[6d43acc](https://github.com/cheatoskar/100-TMX-Server/commit/6d43acc) | verified |
| OS updates and reboots: same rule; install with `NEEDRESTART_MODE=l` so apt restarts no services, reboot only if still empty afterwards | own script (reboot-when-empty) | verified |
| Cold boot: the dedicated may not answer the controller's first XML-RPC handshake in time (one 30 s timeout seen); retry instead of exiting | c:[7d1d0f5](https://github.com/cheatoskar/100-TMX-Server/commit/7d1d0f5); own notes 2026-10-24 | verified |
| Cloning a second instance from the first: don't copy `.bak` files. The TMUF controller loaded the TMNF leaderboard backup as its own (its live file was missing) | c:[554469e](https://github.com/cheatoskar/100-TMX-Server/commit/554469e) | verified (live bug) |
| Bash setup scripts with `set -euo pipefail`: `tr < /dev/urandom \| head -c 20` dies of SIGPIPE and silently ends the script. Generate secrets with `openssl rand` | c:[fb2314a](https://github.com/cheatoskar/100-TMX-Server/commit/fb2314a) | verified (live) |

## Maps from TMX

| Fact | Source | Status |
|---|---|---|
| Check every download: HTTP status, the `GBX` magic at the start, a size cap (8 MB). Otherwise an error page gets saved as `.Challenge.Gbx` | c:[b40f357](https://github.com/cheatoskar/100-TMX-Server/commit/b40f357); TR | verified |
| File names: ASCII only (the Content-Disposition name is decoded as Latin-1 by .NET; a stylised name ended as two different byte strings on disk and in the MatchSettings and the dedicated refused to boot with "Track unknown"), cap the length (a long name in a deep folder made `WriteFile` fail with "couldn't write file"), and put the TMX id into the name (`_tmx<id>_`) to identify the running map later | c:[a0adde0](https://github.com/cheatoskar/100-TMX-Server/commit/a0adde0), c:[52becd8](https://github.com/cheatoskar/100-TMX-Server/commit/52becd8), c:[c172b09](https://github.com/cheatoskar/100-TMX-Server/commit/c172b09) | verified (live) |
| `WriteFile` over XML-RPC only below the 512 KB request limit; larger maps go straight into the tracks folder (a 934 KB map took the server down twice) | [xmlrpc-dedicated.md](xmlrpc-dedicated.md#limits-and-timing) | verified (live) |
| Never-validated maps (no author time) are refused by `InsertChallenge` with "Track not complete": put them on a skip list for good | c:[aedfa79](https://github.com/cheatoskar/100-TMX-Server/commit/aedfa79) | verified (live) |
| TMUnlimiter maps lock vanilla players out (they cannot load the map, so nobody can even vote to skip). Filter with `inunlimiter=0`, but that flag is uploader-declared: one Unlimiter map still slipped through and stalled a server for over 2 hours | c:[793a1da](https://github.com/cheatoskar/100-TMX-Server/commit/793a1da); website docs (private) | verified (live) |
| Maps with custom blocks can crash every client; in the logs they show as many joins and 0 checkpoints. Keep a skip list the controller re-reads while running (here every 10 s) and leave the map if it gets listed | c:[79d6648](https://github.com/cheatoskar/100-TMX-Server/commit/79d6648); own notes | verified (live) |
| Switching maps: `InsertChallenge` (after the current one) + `NextChallenge` is the whole operation. A `ChallengeRestart` sent right after can arrive before the switch starts and cancels it (players were stranded on the warm-up map) | c:[655741b](https://github.com/cheatoskar/100-TMX-Server/commit/655741b) | verified (live) |
| "Change in progress" during a map change: retry `NextChallenge`, never `ChallengeRestart` (see [contradictions](../contradictions.md) #11). Two players finishing at once must not trigger two switches (81 such errors in the logs) | c:[89703db](https://github.com/cheatoskar/100-TMX-Server/commit/89703db), c:[72e7a53](https://github.com/cheatoskar/100-TMX-Server/commit/72e7a53); ANALYSE | verified (live) |
| Sending two map-changing calls back to back: the server silently dropped the second | c:[793a1da](https://github.com/cheatoskar/100-TMX-Server/commit/793a1da) | verified |
| The server's selection only grows: when a time limit runs out without your controller advancing, the dedicated rotates through its own selection and replays old maps. Remove the map you came from on each `BeginRace`, and identify the running map from `GetCurrentChallengeInfo().FileName`, not from what you queued | c:[254825d](https://github.com/cheatoskar/100-TMX-Server/commit/254825d), c:[c172b09](https://github.com/cheatoskar/100-TMX-Server/commit/c172b09) | verified (live) |
| `/trackrandom` draws unevenly with replacement (213 draws, 75 distinct maps from a 289-map pool). For small pools list the whole pool and draw each map once per round, keep the played list on disk | c:[e84e2f6](https://github.com/cheatoskar/100-TMX-Server/commit/e84e2f6); [../tmx/api.md](../tmx/api.md) | verified |
| An empty server should not rotate: about 90 map changes a day with nobody on used up the pool and TMX requests | c:[e84e2f6](https://github.com/cheatoskar/100-TMX-Server/commit/e84e2f6) | verified |
| TMX goes down at times: keep a local copy of the pool (weekly refresh, size cap) and fall back to it; a TMX that accepts connections but never answers counts as down. The first map at startup must not depend on TMX (a TMX timeout crash-looped the controller) | c:[4a730a6](https://github.com/cheatoskar/100-TMX-Server/commit/4a730a6), c:[93b6120](https://github.com/cheatoskar/100-TMX-Server/commit/93b6120), c:[b40f357](https://github.com/cheatoskar/100-TMX-Server/commit/b40f357) | verified |
| Bound every TMX request (here 2 retries, 10 s per attempt, 40 s total): an unbounded retry policy kept one request alive for minutes and froze the controller for 19 minutes | c:[af9d412](https://github.com/cheatoskar/100-TMX-Server/commit/af9d412) | verified (live) |
| TMX ids belong to one exchange (tmnf.exchange vs tmuf.exchange): never share caches or skip lists between a TMNF and a TMUF server | c:[1c2b5e1](https://github.com/cheatoskar/100-TMX-Server/commit/1c2b5e1), c:[93b6120](https://github.com/cheatoskar/100-TMX-Server/commit/93b6120) | verified |
| Multilap maps in TimeAttack give no valid replay of the full race; switching the current map to Rounds mode (checked with the map's own `LapRace`/`NbLaps`, not the TMX tag) does | c:[a0adde0](https://github.com/cheatoskar/100-TMX-Server/commit/a0adde0) | unverified (claim in the commit) |

## Players, votes, ladder

| Fact | Source | Status |
|---|---|---|
| The server's anti-cheat bans players for "Time incoherence" on legitimate RPG/trial/stunt finishes. Set the ladder inactive where possible and call `CleanBanList` at start (the ban list lives in memory) | RG; SS; [xmlrpc-dedicated.md](xmlrpc-dedicated.md#behaviour-quirks) | verified (live) |
| Only one vote at a time: check `GetCurrentCallVote` before `CallVote`. The game's own vote text says "type /yes", which a controller's `/yes` command does not see | c:[72e7a53](https://github.com/cheatoskar/100-TMX-Server/commit/72e7a53); ANALYSE | verified |
| A vote opened with one player online passes on its own vote: rate-limit and keep forced maps out of rankings | c:[f4268d1](https://github.com/cheatoskar/100-TMX-Server/commit/f4268d1) | verified |
| `GetPlayerList(2, 0)` silently caps at 2 players: ask for 200 | c:[793a1da](https://github.com/cheatoskar/100-TMX-Server/commit/793a1da) | verified |
| AFK: `ForceSpectator(login, 1)` + `SpectatorReleasePlayerSlot(login)` frees the slot; players typed `/afk` 73 times before the command existed | c:[3b87f11](https://github.com/cheatoskar/100-TMX-Server/commit/3b87f11) | verified |
| Player nicknames are input: `@everyone` in a name pinged a whole Discord via a webhook, `$l` links survive into web pages. Strip codes and links, disable mentions | c:[f4268d1](https://github.com/cheatoskar/100-TMX-Server/commit/f4268d1) | verified |
| An empty admin list in a controller means "everyone is admin" in this controller family: treat a config that fails to parse as fatal (exit 78) instead of running on defaults | c:[b40f357](https://github.com/cheatoskar/100-TMX-Server/commit/b40f357) | verified |
| Count players from the log (connects minus disconnects), not from open TCP connections: the dedicated keeps ghost sockets on the game port | [xmlrpc-dedicated.md](xmlrpc-dedicated.md#behaviour-quirks) | verified |

## Logs and disk

| Fact | Source | Status |
|---|---|---|
| `Logs/GameLog.<login>.txt` (all chat incl. commands and answers) is never rotated; `Logs/ConsoleLog.<pid>.txt` with `/verbose_rpc` is 15-31 MB per run and was never cleaned (255 MB): prune old ones at start (30 kept) | ANALYSE; c:[b40f357](https://github.com/cheatoskar/100-TMX-Server/commit/b40f357) | verified |
| `/verbose_rpc` is the one log that tells "the server never answered" from "the answer got lost in the client" (each request + "XmlRpc request succeeded") | [xmlrpc-dedicated.md](xmlrpc-dedicated.md#logs) | verified |
| Saved replays grow about 0.8 GB a month; downloaded maps are never cleaned up by the dedicated | ANALYSE | verified |
| Write every state file atomically (temp file, flush, rename) and keep a `.bak`: `File.WriteAllTextAsync` truncates first, and a kill mid-write left an empty leaderboard that the next finish saved over | c:[b40f357](https://github.com/cheatoskar/100-TMX-Server/commit/b40f357); ANALYSE | verified |
| A flight recorder (last 150 calls with timings, dumped on a timeout) is what made the analysis of rare hangs possible | c:[219bdcd](https://github.com/cheatoskar/100-TMX-Server/commit/219bdcd); ANALYSE | verified |

## Testing without the live server

Run the controller and a dedicated in LAN mode on your own PC with its own XML-RPC port; send chat commands as SuperAdmin with `ChatSend("/command ...")` and read answers with `GetChatLines`. Details: [xmlrpc-dedicated.md](xmlrpc-dedicated.md#local-test-harness-no-live-server-needed).
