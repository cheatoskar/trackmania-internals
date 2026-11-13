# GBX container, replays, challenges

Source keys (own project sources): `tmxcopy` = [eswc-autosave](https://github.com/cheatoskar/eswc-autosave) tool source ([src/tmxcopy.cpp](https://github.com/cheatoskar/eswc-autosave/blob/main/src/tmxcopy.cpp)); `notes` = own ESWC RE notes; `web` = the author's website code (private) (gbx.ts, replay-header.ts, lzo1x.ts) and its notes; `nc` = [nostalgic-completionist](https://github.com/cheatoskar/nostalgic-completionist) (a GBX tool fork; [Program.cs](https://github.com/cheatoskar/nostalgic-completionist/blob/main/Program.cs)).

## File layout (binary GBX, version 6)

| Offset / part | Content | Source | Status |
|---|---|---|---|
| 0 | `"GBX"` + u16 version 6 | tmxcopy:46 | verified |
| 5 | 4 format bytes, e.g. `BUCR` = Binary, Uncompressed ref table, Compressed body, R | tmxcopy:4 | verified |
| 9 | u32 main class id (ESWC replay file: `0x2407E000`) | tmxcopy | verified |
| 13 | u32 userDataSize; header user data starts at 17 | tmxcopy | verified |
| user data | u32 chunk count, then (u32 id, u32 size, bit 31 = "heavy/skippable") pairs, then payloads; replays/maps hold an XML chunk | tm-re/eswc/scripts/gbxhdr.py | verified |
| after user data | u32 numNodes, u32 numExternalRefs (0 here), u32 uncompressed body size, u32 compressed size, LZO1X body | tmxcopy:4-5 | verified |
| body | chunks `u32 id, payload`; skippable chunks: `u32 id, "PIKS" (0x534B4950), u32 size, payload`; node end marker `FACADE01` (bytes `01 DE CA FA`) | tmxcopy:43 | verified |
| `.map` vs file reference | a map file is `*.Challenge.Gbx` (TMF), replay `*.Replay.Gbx`; downloads must start with `GBX` (else it is an HTML error page) | server code [TmxRules.cs](https://github.com/cheatoskar/100-TMX-Server/blob/main/RandomizerAnywhere/TmxRules.cs) ReadMapBytesAsync | verified |

## LZO

| Fact | Source | Status |
|---|---|---|
| Body compression is LZO1X | tmxcopy; web lzo1x.ts | verified |
| Three independent decompressors exist here: Python, C++ (tmxcopy.cpp), TypeScript (lzo1x.ts, round-trip checked on ~1900 buffers) | | verified |
| A literal-only LZO1X stream is a valid body and the game loads it: head byte `17+n` (n <= 238) or a length-extended run, data, end marker `11 00 00` | tmxcopy:192-196 | verified (in-game test) |
| `lzo-ts`/minilzo are GPL-3.0; the TS decompressor was written to keep a client bundle free of copyleft | web docs | verified |

## Replay (CGameCtnReplayRecord)

| Fact | Source | Status |
|---|---|---|
| Body main node class `0x2403F` (chunks `0x2403F0xx`); file class ESWC `0x2407E000` | tmxcopy:6; notes | verified |
| Ghost class `0x2401B000`; chunk `0x2401B005` = race time (skippable, i32, -1 = unfinished); `0x2401B006..008` hold logins (len-prefixed UTF-8); `0x2401B00B` checkpoints; `0x2401B00C` last | tmxcopy:38-41; tm-re/eswc/scripts/ghosts.py | verified (ESWC) |
| `0x2403F004` = ghost list: `u32 4, u32 10, u32 count, nodes...` | tmxcopy:7-8 | verified (ESWC) |
| `0x2403F00C` = MediaTracker clip node (0xFFFFFFFF none; set = edited replay) | tmxcopy | verified (ESWC) |
| `0x2403F00D` = u32 EventsDuration + input events | notes:9 | verified (ESWC) |
| `0x2403F00E` = race-state node (class `0x2407F000`), last node | notes | verified (ESWC) |
| `0x2403F00F` = Validate_Version string ("TmNations"), ExeChecksum u32, OsKind (8), CpuKind (3), RaceSettings ("1P-Time" or "Net-TimeAttack") | notes; tmxcopy | verified (ESWC) |
| Header XML (TMF/ESWC): `<header type="replay" exever=...><challenge uid=.../><times best=.. respawns=.. stuntscore=.. validable="0|1"/></header>`; no date in a replay | eswc-autosave autosave.cpp:374-403; web replay-header.ts | verified |
| TMNF replay header: `mapInfo.id` = map UID, `time` ms, `playerLogin`, `playerNickname`; parsing the header takes ~8 ms | web replay-header.ts | verified |
| GBX.NET 2.4.3 `ParseHeaderNode` never fills EventsDuration; `Time` can be null on some replays | own notes | verified |
| Validable flag: [replay-validable-eswc.md](replay-validable-eswc.md). Ghost samples run ~3 s past the finish, input list does not: [replay-tail](../games/tmn-eswc/replay-tail.md) | | |
| Autosaves: ESWC `Autosaves/*.gbx` are class 2403F000 ghost files named `.Replay.gbx`, always validable=0, written only in Solo | own notes | verified |

## Challenge / map (CGameCtnChallenge)

Block names per game and environment (counts, flags, exe versions): [block-names.md](block-names.md).

| Fact | Source | Status |
|---|---|---|
| Header XML has `nblaps="0"` for non-lap maps; UID = `mapInfo.id`; `MapUid` 22-27 chars base64url-like | nc; web | verified |
| Body chunk `0x0304301F` = block list; `0x03043021` = MediaTracker (gbx.js force-skips it) | web docs | verified |
| gbx.js can loop forever on a real TMNF map (block loop decrements its own counter): run it in a worker with a timeout | web docs | verified |
| Header CP count (chunk "0x017", probably 0x03043017) can disagree with body blocks ("revalidating fixes it") | web | inferred |
| Block classification: multilap `*StartFinish` first, then `Checkpoint*`, `*Finish*`; start must be `*StartLine` (bare "Start" matches road pieces like `StadiumPlatformLoopStart`: 75 phantom starts on one map) | web gbx.ts classifyBlock | verified |
| Direction 0 N, 1 E, 2 S, 3 W | web gbx.ts | verified |
| Medal estimates used: gold = ceil(AT*1.06 s), silver x1.2, bronze x1.5, rounded up to whole seconds | web medals.ts | inferred (approximation, not the game's code) |
| Default car per environment: StadiumCar, DesertCar, SnowCar, RallyCar, IslandCar, BayCar, CoastCar | web gbx.ts | verified |
| Score kind: Stunts = score, Platform = respawns, Race/Puzzle = ms | web gbx.ts | verified |
| `CGameCtnChallenge` in-memory: TMNF +0xDC UID id, +0x108 name, +0x60 CP buffer; ESWC +0x118 map type (5 = stunts), +0x124 name | see games/ | verified |
| ManiaPlanet `CGameCtnChallenge` class 0x03043000, size 0x748 | games/maniaplanet | verified |
| Body chunk `0x03043017` (skippable, `PIKS`) = u32 count + count x Int3 (x,y,z block coords) of the waypoint blocks (`*Checkpoint*` incl. ring and ground-level variants, not start/finish), in block-list order. Compared on two TMUF-saved maps: entry counts match the `*Checkpoint*` block counts (5 of 5, 13 of 13); positions match in block order (all 5, first 6 of 13 compared) | own analysis of two maps (file parse, Python) | verified |
| A legacy map (header `version="TMc.5"`, class `0x24003000`, body chunk ids `0x24003xxx`) had one `0x24003017` entry that pointed at a non-waypoint block (a clip block) while all 3 real checkpoint blocks were listed too. TMUF crashes when racing this map (the editor still opens it). Removing only the stale entry (count 4 -> 3, nothing else changed, same UID and author) makes the map race in TMUF; fixing only the sign skin paths of the same map (FileRef version 1 `Any\...` -> version 2 `Skins\Any\...`) still crashes. Editing any block helps because the editor recomputes the list | own analysis of one community map; my test of both repaired copies in TMUF | verified |
| Block skin ref in the block list (flag `0x8000` in the block flags): author lookback string, then a node ref; node class `0x2403A000` (old id of the block skin, new `0x03059000`), chunk `0x2403A002` = string text, FileRef, FileRef (second one empty). TMUF-saved maps write FileRef version 2 with the path `Skins\Any\Advertisement\SignLeft.bik` (no checksum, no locator); the legacy map wrote version 1 with the path `Any\Advertisement\SignLeft.bik`, also in the header `<dep file=...>` | own analysis of three maps | verified |
| Legacy header differences (`TMc.5`, `exever="0.1.7.5"`): XML name is URL-encoded (`%240f0...+...`), `authorscore` is not the author time, header chunk `0x24003002` has version 10 instead of 11, and the body has no chunk `0x03043029` (password hash + CRC, 20 bytes `PIKS` in TMUF-saved maps) | own analysis of one legacy and two TMUF maps | verified |
| Body node-ref count: the u32 before the body size in the file header equals the highest node index in the body + 1 (12 in one map, nodes 0..11); a body that decodes exactly to the stored uncompressed size and ends on `0xFACADE01` is not truncated | own analysis | verified |
| GBX LZO body: a literal-only LZO1X stream (first byte `n-3` or `0` + length bytes, then the literals, then `11 00 00`) is a valid body; a pure-Python decompressor was enough to read TMUF/TMNF maps without `python-lzo` | own analysis (round trip with own decompressor only, game not run) | inferred |

## File names and encoding traps

| Fact | Source | Status |
|---|---|---|
| .NET `HttpClient` decodes Content-Disposition `filename` as Latin-1 -> mojibake for stylised names -> written to disk one way and to MatchSettings XML another -> "Track unknown". Replace non-ASCII with `_` | server code TmxRules.cs GetValidFileName | verified |
| Invalid filename chars: `" < > | \0 \x01-\x1f : * ? \ /` | TmxRules.cs; nc | verified |
| ESWC server strings may start with U+FEFF | nc | verified |

## TMNF/TMUF class ids, chunk lists, ghost decoding (DeltaZero, TMVS, TMNF-C)

Source keys (short names for own project sources, not part of this repo): `tmvs` = [TM Vibrant Shaders](https://github.com/cheatoskar/TM-Vibrant-Shaders) mod source; `tmvs-mp` = its ManiaPlanet port; `tmvs[X]` = feature branch X of it; `tmvs-notes` = its notes; `phys` = own physics RE notes; `tmnfc` = TMNF-C (third-party MIT reimplementation, github.com/adonis-singh/TMNF-C); `dz` = [DeltaZero](https://github.com/cheatoskar/DeltaZero); `xr` = TrackManiaForeverOpenXR (upstream jiink/TrackManiaForeverOpenXR; own fork [TrackManiaForeverOpenXR-TMVS](https://github.com/cheatoskar/TrackManiaForeverOpenXR-TMVS)); `tm-re` = own RE tools. Status: verified = seen in code/data or measured; inferred; unverified = prose only.

| item | fact | source | status |
|---|---|---|---|
| GBX class ids (TMNF/TMUF, old->new pairs) | CHALLENGE 0x03043000 (old 0x24003000); COLLECTOR_LIST 0x0301B000 (0x2403C000); CHALLENGE_PARAMS 0x0305B000 (0x2400C000); BLOCK_SKIN 0x03059000; REPLAY_RECORD 0x03093000 (old 0x2403F000); GAME_GHOST 0x0303F005; CTN_GHOST 0x03092000 (old 0x2401B000); CTN_COLLECTOR 0x0301A000; CTN_OBJECT_INFO 0x0301C000; CTN_DECORATION 0x03038000; CTN_COLLECTION 0x03033000; GAME_SKIN 0x03031000; GAME_PLAYER_PROFILE 0x0308C000; WAYPOINT_SPECIAL_PROP 0x0313B000 (0x2E009000); ITEM_MODEL 0x2E002000; MW_NOD 0x01001000. ESWC ghost class 0x2401b000 matches CTN_GHOST_OLD | dz\src\tmdriver\gbx\reader.py:16-34; tmvs[eswc2] engine.cpp | verified |
| chunks parsed | challenge 0x03043002/3/5, 0x0304300D, 0x03043011, 0x0304301F (saved replays rewrite map from memory, chunk 0x304301f), 0x03043014/17/21-26/28/29/2A, 0x0305B000..0A..0D; replay 0x03093000 (header: map ident), 0x03093002, 0x03093007, 0x03093014; ghost 0x03092005/08/09/0A/0B (CP times count)/0C/0E/0F/10/12; skip marker 0x534B4950 "SKIP"; end marker 0xFACADE01 | reader.py:233-680 | verified |
| replay / ghost parsing (DeltaZero, measured) | map UID = replay header chunk 0x03093000 `map_ident[0]` (NOT ghost.uid, which differs per replay); control entry times offset 100000 (TMInterface InputEvents 100010) -> `shift` 1 tick; analog Steer is signed 24-bit, range exactly +-65536; keyboard right = +steer in TMInterface convention, replay analog sign -1; ghost samples every 100 ms equal HF trace rows (0.00 m); ghost rotation: `angle` is HALF angle raw/0xFFFF*pi, axis (cosH cosP, sinH cosP, sinP), H=raw/0x7FFF*pi, P=raw/0x7FFF*pi/2, quaternion (cos a, axis sin a); car forward/up = columns 2/1; error 3e-4 vs re-sim; trace sample holds input 10 ms before it (sample 0 = nothing pressed, take sample 1) | dz notes; dz\src\tmdriver\replay.py | verified |
| world coordinates | world = Gbx block coord * (32, 8, 32); MX x/z = Gbx x/z - 1; live P_MAP blocks == Gbx blocks + 32x32 StadiumGrass filler; block dir d: heading = -90 deg*d; start pose from start block `spawnLoc * blockLoc` float32 in game order (spawn.py bit-exact) | dz notes; tmnfc_port\README.md | verified |
| .tmcap (TMVS capture) | v1 (no sun colour), v2.., v3 for TM2 plan: colour, depth, matrices, sun; companions capture_NNN.hdr16 ("TMH1" w h RGBA16F), .mat (material per pixel, ids 1..255), .nrm16, .lit16; TM2 capture folder: trace.txt, hdr_t*.dds, engine_*.dds, scenev.bin, scenep.bin, shadow_cb.bin, tonemap_cb.bin, camera.txt, car.txt | tmvs notes; mp README | verified |
| TMNF-C snapshot formats | track snapshot (material-data section: 31 entries), vehicle snapshot `oracle/vehicles/LOLSPORT-Stadium.tmnfvehicle`, route snapshots, `trace_format.h` capture files, physics image 16,384 B | tmnfc | verified |
| TMInterface script / CSV | script `time command`; fvlab inputs CSV; to_script.py / script_to_csv.py convert | phys\scripts | verified |
| TMDriver wire protocol v12 | little-endian int32 type then payload: P_HELLO 1, P_MAP 2, P_STEP 3, P_UI 4, P_BENCH 5, P_PING 6, P_TREC 7, P_TEND 8, P_JOB 9; C_ACTION 10, C_SPEED 11, C_RESTART 12, C_STATUS 13, C_SIMONLY 14, C_SAVE 15, C_REWIND 16, C_MODE 17, C_BENCH 18, C_EXEC 19, C_DRAW 20, C_PLAY 21, C_WAIT 22, C_HOLD 23; host 127.0.0.1:8478; STEP carries race_time, flags, checkpoints, pos, rot rows, linear/angular speed, 4 wheels (damper, bits contact|sliding<<1|material<<8) order FL FR BR BL; action bits UP 1 DOWN 2 LEFT 4 RIGHT 8 STEER_ANALOG 16 GAS_ANALOG 32 RESPAWN 64 | dz\src\tmdriver\protocol.py | verified |
| TMX / TAS sources | tmtas.exchange replay API (ids 53, 128, 230, 4091 used by TMNF-C validation); trackmania.wiki pages; HF `traces` dataset (parts 0001..0309, 577,846 maps) used by DeltaZero | tmnfc\docs\VALIDATION.md; phys\docs\sources.md; dz | verified |
