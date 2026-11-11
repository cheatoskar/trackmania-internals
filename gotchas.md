# Gotchas (Windows, encoding, tooling, process)

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
| Normal (ASLR) TMLoader profile breaks fixed-address memory readers | use a no-ASLR compat profile for captures (DeltaZero used profile `TMNFC`, 2.12.0-compat) | own notes | unverified |
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
