# Gotchas (Windows, encoding, tooling, servers, TMX, process)

Recurring traps from earlier work. Status: verified = happened and was fixed/confirmed; unverified = reported, not re-checked. Sources named `own notes`, project names (TMVS = TM Vibrant Shaders, Bingo plugin, DeltaZero, tm-physics-re, server controller) refer to the author's own projects; file names are kept for provenance.

## Editing files on Windows

| Trap | Fix | Source | Status |
|---|---|---|---|
| `sed -i` on CRLF files produces mixed line endings | Python with `open(..., newline='')` and `\r\n` in the pattern, or an editor tool | own notes | verified |
| Git Bash eats backslashes in heredocs and `python -c` | Write scripts with backslashes/regex/Windows paths to a file first, then run it | own notes | verified |
| Python print with umlauts/emoji -> `UnicodeEncodeError` (cp1252 console) | set `PYTHONIOENCODING=utf-8` | own notes | verified |
| A cp1252 line slipped into a UTF-8 CHANGELOG during an agent merge | repaired with a one-off re-encode script; check `file`/`git diff` after merges | own notes | verified |
| Git Bash turns `/history` (chat command argument) into a Windows path | `MSYS_NO_PATHCONV=1` | own notes | verified |
| Windows `md5sum` prints `*` before the file name; diffs against Linux output fail | strip the `*` before comparing | own notes | verified |
| Secrets piped from Windows Python into `ssh` arrive with a trailing `\r` | write with `newline='\n'` / strip | own notes | verified |
| `merge` tool "resolve both" duplicated lines in shader name lists | check list-type conflicts by hand | own notes | verified |

## Node / builds

| Trap | Fix | Source | Status |
|---|---|---|---|
| After Ctrl+C a `node.exe` keeps running and locks `node_modules` | `Get-CimInstance Win32_Process -Filter "Name='node.exe'"` before `npm ci` | own notes | verified |
| `npm run`/`npx` put `node_modules/.bin` of every parent folder on PATH; a stray `package.json` in the user folder can inject another Node | compare `npx node -v` with `node -v` | own notes | verified |
| Long build paths break the OpenXR (TMFOXR) build | build in a short dir (e.g. `C:/fxb`) | own notes | verified |
| Running searches lock their exe (fvlab) | copy the built exe to a new name per parallel search | own notes | verified |

## Game testing

| Trap | Fix | Source | Status |
|---|---|---|---|
| Starting TMNF unattended (`/file=<replay>`, `TMLoader.exe run TmForever default "/file=..."`) stalls in login/profile dialogs | do not run the game unattended; ask the player for captures/logs | own notes | verified |
| Several sessions install into the same TMLoader product folder (`%LOCALAPPDATA%\TMLoader\database\TmForever\products\<mod>\<ver>\`); a test may run another session's DLL | check the mod log for build-specific lines and the installed DLL's md5 before trusting feedback | own notes | verified |
| One change per in-game test for lighting/tone changes; stacked changes cannot be told apart | gate behind settings, compare captures offline first | own notes | verified |
| Normal (ASLR) TMLoader profile breaks fixed-address memory readers | use a no-ASLR compat profile for captures (DeltaZero used profile `TMNFC`, 2.12.0-compat); see [README](README.md#using-the-addresses) | own notes; DeltaZero notes | verified |
| Old ESWC Windows dedicated server fails online with "http error 12157" (old TLS stack) | test online behaviour on Linux | own notes | verified |
| Local test server vs running client: client uses UDP 2350, so a local server must take 2351 | set ports in config | own notes | verified |

## Ghidra / RE

| Trap | Fix | Source | Status |
|---|---|---|---|
| Ghidra MCP headless server started from a background shell exits at once (stdin EOF) | start it as a hidden detached process (`Start-Process cmd /c <start.cmd>` calling `support\launch.bat ... GhidraMCPHeadlessServer`) | own notes | verified |
| LaurieWired GhidraMCP stops at Ghidra 11.3.2 | use the bethington fork (GhidraMCP 6.0.0) for Ghidra 12.x | own notes | verified |
| MCP tools only appear in client sessions started after registration | restart the MCP client session | own notes | verified |
| A `findfn.py` TMNF->TMUF porter mentioned in a hand-off was never committed | use `scripts/re/port.py` (byte-pattern port) or Version Tracking | own notes | verified |
| `GetFullName` on engine nods leaks a string per call | cache lookups, cap re-looks | own notes | inferred |
| Game reuses material object addresses after the replay editor reload without firing the map-load hook | key caches on address + a stamp, not address alone | own notes | verified |

## Searches / TAS

| Trap | Fix | Source | Status |
|---|---|---|---|
| Optimisation objectives get "cheated" (fake hits) | probe every hit tick by tick before reporting; in probe output pitch > 0 means nose down | own notes, tm-physics-re | verified |
| Event on the first tick of a search window was ignored -> fake hits | fixed in fvlab; keep in mind for any windowed search | own notes | verified |
| Nose/boost searches with short windows miss the boost | evaluation and input window >= 1.5 s past the nose landing | own notes | verified |
| TMInterface scripts: watching the full known run each time | start scripts with `0.00 set speed 10`, `set speed 1` 1-2 s before the new part | own notes | verified |
| A rewind inside TMInterface `OnRunStep` is followed by one simulated tick without input | rewind from the Python side, not in the plugin | DeltaZero notes | verified |
| Replay file names with non-ASCII break ForeverValidator runs | ASCII names only | tm-physics-re notes | verified |
| Near-uber exponent also rises when the car nearly stands | set a minimum speed (FVLAB_EXP_MINKMH, e.g. 200) | tm-physics-re docs/tas | verified |
| Ghidra shows x87 `fistp` for the substep count: result depends on FPU rounding mode | trust validated ports (trunc), not the decompiler's reading | tm-physics-re physics-model6.md (Corrections) | verified |

## More RE / mod traps

| Trap | Fix | Source | Status |
|---|---|---|---|
| `analyzeHeadless.bat` (cmd.exe) breaks on `(x86)` in paths | analyse copies of the exes in a plain folder (e.g. `<tools dir>\bin`) | own Ghidra notes | verified |
| PowerShell 5.1 with `$ErrorActionPreference='Stop'` turns Ghidra's stderr WARNING lines into terminating errors | do not set Stop in Ghidra wrapper scripts | scripts/ghidra/analyze.ps1 | verified |
| Only one Ghidra (GUI or headless/MCP server) may hold a project | stop the headless server before opening the GUI | own Ghidra notes | verified |
| `mcp-headless.log` is UTF-16 | read with an encoding-aware tool | own Ghidra notes | verified |
| `TmForever.map` lines `0001:xxxxxxxx` are section-relative (RVA = offset + 0x1000) | use the `Rva+Base` column (= VA); `scripts/re/map_lookup.py` does this. Twinkie's "0x3357D0 + 0x200000" and the plugin's "RVA 0x5357D0" for `CMwId::GetName` are the same address | this repo's check of TmForever.map | verified |
| TMLoader profile 2.12.0 relocates TmForever.exe (ASLR); 2.12.0-compat does not | fixed-address readers need -compat or `rebase(va) = module - 0x400000 + va` | DeltaZero tmnfc_port README; TM Vibrant Shaders engine.cpp | verified |
| Copying addresses between TMNF and TMUF (or ESWC variants) | each exe has its own table; verify prologue bytes before every hook/call | TMVS GOTCHA.md; 100-TMX-Bingo-Plugin game.cpp | verified |
| Only a pointer CHAIN survives restarts, never a heap address | store chains (e.g. `0x454/0x28/0x0/0x1C/0x2B0`), validate the end value | 100-TMX-Bingo-Plugin docs/finish-marker.md | verified |
| Patching a probe D3D9 device's vtable did not catch the game's device (different VP mode -> different vtable) | IAT-patch `Direct3DCreate9` and patch the game's own device | 100-TMX-Bingo-Plugin hook.cpp; TMVS GOTCHA.md | verified |
| A TMLoader mod DLL must export `_DllMain@12`, else the game fails with 0xc000007b | add the export; DllMain only starts a thread | 100-TMX-Bingo-Plugin dllmain.cpp | verified |
| Loader lock: no COM/known-folder lookups, no joining threads, no `waveOut*` in DllMain/DLL_PROCESS_DETACH | defer to a thread | TMVS GOTCHA.md; Bingo plugin | verified |
| TMLoader only picks up new mod versions at startup; one folder per version | log the version from DllMain and check it | Bingo plugin; own notes | verified |
| The game is 32-bit without LARGE_ADDRESS_AWARE: ~2 GB address space (video encoders, big buffers) | budget memory; TMFOXR offers an LAA patch with backup | TMVS engine-video-export.md; TMFOXR | verified |
| `CMwId::GetName`/`GetFullName` allocate a new string per call that is never freed | cache the result | Bingo plugin game.cpp; own notes | verified |
| ps_3_0: 4096 instruction slots, no bitwise ops, `[unroll]` can explode compile time (120 s) | `[loop]`, split passes | TMVS GOTCHA.md | verified |
| An installer that carries a DLL looks like a dropper to Defender | do not embed the DLL | TMVS GOTCHA.md | verified |
| Windows paths are case-insensitive but git keeps old casing (`Docs/` vs `docs/`) | `git rm --cached` + `git add` | TMVS GOTCHA.md | verified |
| CMake `file(WRITE)` writes CRLF and broke `sha256sum -c` | `file(CONFIGURE ... NEWLINE_STYLE UNIX)` | TMVS GOTCHA.md | verified |
| Python text patching with default cp1252 truncated a file to 0 bytes on a failed write | always pass `encoding='utf-8'`, write to temp + rename | DeltaZero notes | verified |
| `File.WriteAllTextAsync` truncates first; a SIGTERM mid-write lost leaderboard/config | temp file + rename (AtomicFile) | server controller analysis | verified |
| MSBuild MSB3027: output DLL locked | stop the running controller/dotnet first | server controller notes | verified |
| Under Wine, TMNF needs d3dx9_31 for the lightmap shader compile and sometimes dies silently on map load | winetricks d3dx9_31; prefer Windows | DeltaZero notes; TMNF-C LOCAL_ASSETS.md | verified |
| Windows 7 needs KB3140245 (TLS 1.2) for HTTPS from a mod | | Bingo plugin README | verified |

## Server / TMX traps (details in server/ and tmx/)

| Trap | Source | Status |
|---|---|---|
| XML-RPC request > 512 KB kills the TMF connection | server controller RemoteClient.cs | verified |
| ManiaAPI.XmlRpc <= 2.9.2 loses fast replies; use >= 2.9.5 | own notes | verified |
| TMX silent-ignore filters (`trackuid`, `trackpackid`, ...) return 200 + newest tracks | own TMX client | verified |
| TMX `routes=0,1,2,3` -> 500; `primarytype` on `/trackrandom` -> 404 | server controller presets | verified |
| TMX dates have no `Z` but are UTC | own TMX notes | verified |
| 2006 server: `/servername` ignored, needs `/internet` and `/nodaemon` | server controller ServerSetup.cs | verified |

## Running dedicated servers

One line each; the linked page has the details and sources.

| Trap | Fix | Details | Status |
|---|---|---|---|
| Logging in to the game with the server's account: the server is refused ("already connected (from a different ip)") | separate server account, never played with | [operations](server/operations.md#server-account-and-config) | verified |
| Wrong server-account password: the dedicated exits and systemd restarts it forever | stop the unit, fix, start; `RestartPreventExitStatus` for config errors | [operations](server/operations.md#server-account-and-config) | verified |
| Port opened in the cloud firewall but not in the VM's own firewall (or the other way round) | open game and P2P port, TCP and UDP, in both | [operations](server/operations.md#ports-and-firewall) | verified |
| LAN server invisible: bound to a VPN/virtual/link-local adapter, or game port outside 2350-2359 (ESWC 2350-2369, TMS/TMO 2350 only) | `/bindip=<LAN address>`, port in range | [operations](server/operations.md#i-cannot-see-the-server) | verified |
| TMUF server with an empty `<packmask>` is invisible to TMNF players | `nations` if TMNF players should join | [operations](server/operations.md#server-account-and-config) | verified |
| Local Windows test in internet mode: registration fails, the dedicated dies after 10-15 s | LAN mode for local tests | [operations](server/operations.md#i-cannot-see-the-server) | verified |
| `connection_uploadrate` default 512 kbit/s is shared by all players, who download every new map from the server | raise it (10240 used) | [operations](server/operations.md#server-account-and-config) | inferred |
| `use_changing_validation_seed` on: press-forward lines drift per player, block-mixed starts clip under the start block | set it False | [operations](server/operations.md#server-account-and-config) | verified |
| The Linux TMF dedicated forks into the background; the parent never sees it die | systemd `KillMode=control-group`, watch the XML-RPC socket | [operations](server/operations.md#hosting) | verified |
| Two servers on one host found by process name: each controller measured the other's process | track the PID you started | [operations](server/operations.md#hosting) | verified |
| Restarting with players on drops all of them | deploy, update and reboot only after several empty polls | [operations](server/operations.md#restarts-and-updates-without-kicking-players) | verified |
| A dead controller's last `status.json` is still served and looks healthy | trust only a fresh timestamp (< 45 s, newer than the restart) | [operations](server/operations.md#restarts-and-updates-without-kicking-players) | verified |
| Deploy staged in `/tmp`: a reboot during the wait emptied it and the script still said "DEPLOYED" | stage on persistent disk, check every step | [operations](server/operations.md#restarts-and-updates-without-kicking-players) | verified |
| Overwriting the DLLs of a running process in place | replace files (`cp --remove-destination`) | [operations](server/operations.md#restarts-and-updates-without-kicking-players) | verified |
| Cloning an instance copied the first one's `.bak` files; the new controller loaded them as its own leaderboard | never copy `.bak`/state files | [operations](server/operations.md#restarts-and-updates-without-kicking-players) | verified |
| `tr < /dev/urandom \| head -c 20` under `set -euo pipefail` ends a setup script silently (SIGPIPE) | `openssl rand` | [operations](server/operations.md#restarts-and-updates-without-kicking-players) | verified |
| Server zip fetched over plain HTTP and executed | pin its SHA-256 | [operations](server/operations.md#hosting) | verified |
| Ladder anti-cheat bans legitimate RPG/trial/stunt finishers ("Time incoherence") | ladder inactive where possible, `CleanBanList` at start | [operations](server/operations.md#players-votes-ladder) | verified |
| `GetPlayerList(2, 0)` returns at most 2 players | ask for 200 | [operations](server/operations.md#players-votes-ladder) | verified |
| A config that fails to parse made the controller run on defaults: empty admin list = everyone is admin | treat config errors as fatal (exit 78) | [operations](server/operations.md#players-votes-ladder) | verified |
| Server name with `$` codes reused as folder name and on the web page | separate plain name and display name; 2006 server: max 25 chars incl. codes, no `$o` | [operations](server/operations.md#server-account-and-config), [old-2006-server](server/old-2006-server.md) | verified |
| ConsoleLogs with `/verbose_rpc` (15-31 MB per run) and GameLog are never rotated | prune at start | [operations](server/operations.md#logs-and-disk) | verified |
| Player counts from open TCP connections are too high (ghost sockets) | count connects minus disconnects in the log | [xmlrpc-dedicated](server/xmlrpc-dedicated.md#behaviour-quirks) | verified |

## Server controller logic (XML-RPC)

| Trap | Fix | Details | Status |
|---|---|---|---|
| One slow callback handler freezes all callbacks (chat, checkpoints, finishes) while requests still work, so a status page looks healthy | network work off the callback loop; bound every call (10 s) and every handler | [xmlrpc-dedicated](server/xmlrpc-dedicated.md#limits-and-timing) | verified |
| An unhandled exception in one handler ends the dispatch loop for good | try/catch around every handler | [manialinks](server/manialinks.md#what-broke-and-the-fix) | verified |
| Unbounded HTTP retries to TMX kept one request alive for minutes: controller deaf for 19 minutes | 2 retries, 10 s each, 40 s total | [operations](server/operations.md#maps-from-tmx) | verified |
| `ChallengeRestart` right after `NextChallenge` cancels the switch | `InsertChallenge` + `NextChallenge` only | [operations](server/operations.md#maps-from-tmx) | verified |
| Retrying `ChallengeRestart` on "Change in progress" reloads the old map | retry only `NextChallenge` | [operations](server/operations.md#maps-from-tmx) | verified |
| Two finishes at once -> two map switches (81x "Change in progress") | one switch per map, claimed by the first finish | [operations](server/operations.md#maps-from-tmx) | verified |
| Two map-changing calls back to back: the second is dropped silently | one call, then wait for `BeginRace` | [operations](server/operations.md#maps-from-tmx) | verified |
| `BeginRace` never fires for a map that loaded before anyone connected | `GetCurrentChallengeInfo` on demand | [manialinks](server/manialinks.md#size-refresh-load) | verified |
| The server's selection only grows, so it rotates back to played maps when a time limit ends | remove the previous map on `BeginRace`; identify the running map by its file name | [operations](server/operations.md#maps-from-tmx) | verified |
| `NbCheckpoints` and the checkpoint callback count the finish line | normalise once, clamp | [xmlrpc-dedicated](server/xmlrpc-dedicated.md#behaviour-quirks) | verified |
| Idle players on an unlimited time limit send no callbacks: "silence = dead stream" reconnected every 5 minutes | use an independent witness (ranking, map name from a poll) | [contradictions](contradictions.md) #27 | verified |
| One vote at a time; the game's vote text says "type /yes", which a controller's `/yes` never sees | check `GetCurrentCallVote` first | [operations](server/operations.md#players-votes-ladder) | verified |
| `GetChatCommandList` does not exist on TMF | keep your own command list | [xmlrpc-dedicated](server/xmlrpc-dedicated.md#behaviour-quirks) | verified |
| 2006 server: does send `PlayerFinish`; together with a ranking poll every finish counted twice | de-dupe by (login, time) per map | [old-2006-server](server/old-2006-server.md#callbacks-and-polling) | verified |
| 2006 server: `NextChallenge` answers OK during Synchronization and then stays on the map | wait for status Play (4) | [old-2006-server](server/old-2006-server.md#callbacks-and-polling) | verified |
| Player nicknames in Discord/web output: `@everyone` pinged a whole server, `$l` links came along | strip codes and links, disable mentions | [operations](server/operations.md#players-votes-ladder) | verified |
| Non-atomic writes (`File.WriteAllTextAsync`) lost the leaderboard on a kill | temp file + rename, keep `.bak` | [operations](server/operations.md#logs-and-disk) | verified |

## ManiaLinks

| Trap | Fix | Details | Status |
|---|---|---|---|
| Decimal comma from the host locale broke `posn`/`sizen` | invariant number formatting | [manialinks](server/manialinks.md#encoding-and-escaping-traps) | verified |
| `SendHideManialinkPageToId` with the page's string id -> "Value of type STRING supplied where type INT was expected" | hide one page by sending an empty page with the same id | [manialinks](server/manialinks.md#sending-and-hiding) | verified |
| `SendHideManialinkPage` hides every page of every player, always-on panels included | empty page with the same id | [manialinks](server/manialinks.md#sending-and-hiding) | verified |
| Players who connect later do not get pages broadcast before | resend the current widgets on `PlayerConnect` | [manialinks](server/manialinks.md#sending-and-hiding) | verified |
| `...ToLogin` to a player who just left throws "Login unknown" | catch it; never let it abort game logic | [manialinks](server/manialinks.md#sending-and-hiding) | verified |
| Action ids of two generated lists 10 apart: the 11th entry of one was read as a click in the other | id ranges 100 apart | [manialinks](server/manialinks.md#clicks-actions-and-answers) | verified |
| Text cut by character count overflows panels (wide lookalike and CJK glyphs) | cut by estimated width, never inside a `$` code | [manialinks](server/manialinks.md#layout) | verified |
| Coordinates outside about +-64 / +-48, or `pos`/`size` instead of `posn`/`sizen`: the element is just not visible | stay inside the area, use `posn`/`sizen` | [manialinks](server/manialinks.md#layout) | verified |
| The 2006 server (ESWC/TMS/TMO) has no ManiaLinks; `$l` chat links show as text | feature-detect with `system.listMethods`, chat-only UI, plain URLs | [manialinks](server/manialinks.md#which-servers-have-manialinks) | verified |

## TMX and maps

| Trap | Fix | Details | Status |
|---|---|---|---|
| `difficulty=4` (Lunatic) -> 404 on every call | Lunatic is 3 (0 Beginner .. 3 Lunatic) | [tmx/api](tmx/api.md#fields) | verified |
| Author fields `Username`, `Author`, `AuthorLogin` -> 400 | `Uploader.Name` (undocumented dot path) | [tmx/api](tmx/api.md#fields) | verified |
| Result field `Environment` is 1-based, the `environment=` filter 0-based | subtract 1 | [tmx/api](tmx/api.md#fields) | verified |
| TMX `Routes`/Multilap tag disagrees with the map's `LapRace`/`NbLaps` | read the map | [tmx/api](tmx/api.md#filters-and-silent-ignore-traps) | verified |
| `/trackrandom` draws unevenly with replacement | list the pool, draw each map once per round | [operations](server/operations.md#maps-from-tmx) | verified |
| .NET decodes the Content-Disposition file name as Latin-1: two different byte strings on disk and in MatchSettings -> "Track unknown" at boot | ASCII-only file names | [formats/gbx](formats/gbx.md#file-names-and-encoding-traps) | verified |
| An error page saved as `.Challenge.Gbx` | check HTTP status, `GBX` magic, size cap | [operations](server/operations.md#maps-from-tmx) | verified |
| "Track not complete" from `InsertChallenge` = map never validated | skip it for good | [operations](server/operations.md#maps-from-tmx) | verified |
| `inunlimiter=0` is uploader-declared: an Unlimiter map got through and locked vanilla players out for over 2 hours | skip list the controller re-reads while running | [operations](server/operations.md#maps-from-tmx) | verified |
| Custom-block maps crash every client (logs: many joins, 0 checkpoints) | same skip list | [operations](server/operations.md#maps-from-tmx) | verified |
| TMX ids are per exchange (tmnf vs tmuf) | never share caches or skip lists between exchanges | [operations](server/operations.md#maps-from-tmx) | verified |
| Online ESWC replays as the game saves them are `validable=0` (one ghost per participant plus the server account); "Save previous replay" never gives a TMX-valid file. A file reduced to the local ghost by a copy tool says 1 (the flag follows the ghost count, `Net-TimeAttack` stays in it). Never edit the flag | drive and save offline in Solo | [formats/replay-validable-eswc](formats/replay-validable-eswc.md), [replay-tail](games/tmn-eswc/replay-tail.md#why-these-three-files-say-validable1) | verified |
| Online ESWC replays: the ghost's samples run about 3 s past the finish while the input list ends at the race time; the recording stops only with the previous-race copy on restart | do not read the last 3 s as driver input; use header `best` / `EventsDuration` | [replay-tail](games/tmn-eswc/replay-tail.md) | verified (3 files) |
| The 2006 server's `GetValidationReplay` returns data without a GBX header: TMX cannot read it | - | [old-2006-server](server/old-2006-server.md#methods-live-check-2026-11-09) | verified |
| GBX.NET 2.4.3 `ParseHeaderNode` never fills EventsDuration; `Time` can be null | read the full replay | [formats/gbx](formats/gbx.md#replay-cgamectnreplayrecord) | verified |
| Driving aids (minimap highlights, ideal/brake line, pausing outside the replay editor) are against TMX rules even offline | check before building | [tmx-rules-for-mods](modding/tmx-rules-for-mods.md) | verified |

## Game client, caches, rendering mods

| Trap | Fix | Details | Status |
|---|---|---|---|
| TMNF/TMUF write their file index cache (FidCache) only on a clean exit; after a crash, a kill or a TMNF<->TMUF switch the next start rescans everything | exit cleanly; expect a slow first start | [game-control](modding/game-control.md#starting-the-game-from-outside-and-its-caches) | verified |
| ESWC and Sunrise exes have no `/file=` switch | copy the map into a scanned folder, then start or drive the menu | [game-control](modding/game-control.md#starting-the-game-from-outside-and-its-caches) | verified |
| The lightmap cache (`ProgramData\TrackMania\Cache\*.LightMap.zip`) is keyed without the light: a mod that changes the light gets the old bake | bypass the cache while active | [rendering](rendering/tmnf-d3d9.md#findings-from-test-rounds-own-notes) | verified |
| Guessing lamps from the 8-bit image overexposes night maps (floodlit areas are flat 255) | take the engine's own light draws plus a float target | [rendering](rendering/tmnf-d3d9.md#findings-from-test-rounds-own-notes) | verified |
| Car position from screen-space depth samples jumps and flickers | engine's interpolated vehicle location | [rendering](rendering/tmnf-d3d9.md#findings-from-test-rounds-own-notes) | verified |
| Parallel shader branches claimed the same pixel-shader constant register (`c47`) | register table, check on every merge | [rendering](rendering/tmnf-d3d9.md#findings-from-test-rounds-own-notes) | verified |
| Process-loopback audio capture silent after Stop/Reset/Start | fresh capture client per run | [rendering](rendering/tmnf-d3d9.md#findings-from-test-rounds-own-notes) | verified |
| Static `d3d9.dll` import in a mod DLL -> 0xc000007b on TMUF | load the system d3d9 by full path at runtime | [loaders-and-hooks](modding/loaders-and-hooks.md#from-the-renderingphysics-side-tm-vibrant-shaders-tmfoxr-tmnf-c-tminterface) | verified (cause inferred) |
| ManiaPlanet's back buffer is sRGB: an overlay drawn into it comes out too bright | own UNORM layer | [rendering](rendering/tmnf-d3d9.md#maniaplanet-port-notes-own-notes) | verified |
| Proxy DLLs for games installed under `Program Files` need admin rights to copy | install through a setup that asks for elevation, or install the game elsewhere | [loaders-and-hooks](modding/loaders-and-hooks.md#from-the-renderingphysics-side-tm-vibrant-shaders-tmfoxr-tmnf-c-tminterface) | verified |
| Nations ESWC (`983bd127`) and Original (Online Patch v2) never call `timeBeginPeriod`: the call is patched out, so the timer stays at ~15.6 ms unless something else raises it | call `timeBeginPeriod(1)` from outside if timing matters | [frame-loop](rendering/frame-loop.md#timer) | verified |
| The engine waits for the GPU itself after every `Present` (event-query ring, `GpuSync`) | do not add a second render-ahead limiter; measure the wait inside `Present` + the sync | [frame-loop](rendering/frame-loop.md#present-and-the-gpu) | verified |
| Shaders are compiled when first bound, not at map load | expect first-lap hitches; warm-up needs the compile path, not D3D9 | [frame-loop](rendering/frame-loop.md#shaders-and-textures-created-during-play) | verified (path), inferred (hitch) |
| TMNF-C's offline track builder can order blocks differently from the game: a map that should reproduce does not | use the track dumped from the running game | [tas/tools](tas/tools.md#tmnf-c-client-free-physics) | verified |
