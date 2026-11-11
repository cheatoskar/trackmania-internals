# TMNF-C: headless TMNF physics, how it maps to the game, and using it on Windows

Back to the [physics overview](tmnf-physics.md). Addresses it hooks and annotates: [../games/tmnf/tmnfc-annotations.md](../games/tmnf/tmnfc-annotations.md). Where my own Model6 notes agree or disagree with it: [crosscheck.md](crosscheck.md).

**Credit:** [TMNF-C](https://github.com/adonis-singh/TMNF-C) by adonis-singh, MIT licence (copyright 2026 adonis-singh). It is a C (and CUDA) reimplementation of TrackMania Nations Forever physics with vector environments for reinforcement learning. Everything about its internals below is described from reading its source and docs at commit `eb6be04` (2026-09-13); short quotes are marked as such, no code is copied. The Windows port and the measurements are mine, published in [DeltaZero `tmnfc_port/`](https://github.com/cheatoskar/DeltaZero/tree/main/tmnfc_port) (MIT, with TMNF-C's licence file kept).

Source keys: `tmnfc` = TMNF-C at `eb6be04` (file names are paths in that repo); `dz` = [DeltaZero](https://github.com/cheatoskar/DeltaZero) (public commits linked per row); `dz-notes` = DeltaZero's working notes (unpublished); `own` = my captures and runs. Status: **verified** = seen in code/data or measured; **inferred**; **unverified** = prose only.

## What it is and what it targets

| fact | value | source | status |
|---|---|---|---|
| target exe | `TmForever.exe` 2.11.26 Nations, SHA-256 `3847cf9f...` (the TMNF reference build `4d431494`); every snapshot file carries this exe hash | tmnfc README; `src/track.h` header field `exe_sha256` | verified |
| scope | Model6 (Stadium) validated end to end; force models 3/4/5 ported, "Force models 4 and 5 remain unvalidated" (README quote); tested on 212 Nations and United campaign tracks across the seven car environments | tmnfc README | verified (doc) |
| tick | fixed 100 Hz, 10 ms; one `PhysicsStep2` call per tick (`tick_ms is normally 10`, comment at 0x00549c90) | tmnfc `src/physics.h` | verified |
| platforms | upstream: Linux, C11 + CMake, optional CUDA (tested compute capability 8.6 and 12.0); capture side through Wine + TMInterface | tmnfc README, docs/LOCAL_ASSETS.md | verified (doc) |
| game data | none in the repo: a 16,384-byte "physics image" (from `TestMaterialHeight.tga` in the user's own packs, hash-checked) is compiled in; track, vehicle and route snapshots are generated locally; a library built with the image contains derived game data and must not be redistributed | tmnfc docs/LOCAL_ASSETS.md, docs/DISTRIBUTION.md | verified (doc) |

## How it is matched against the real exe

| fact | value | source | status |
|---|---|---|---|
| golden traces | an in-game tracer DLL inline-hooks 33 game functions (list with prologue bytes in [tmnfc-annotations.md](../games/tmnf/tmnfc-annotations.md)) and writes, per call, the raw bytes of the `this` object and referenced arguments at entry and the output buffers at return; the replay harness feeds the recorded inputs to the C function and compares the outputs byte for byte | tmnfc `src/trace_format.h`, `oracle/tracer/` | verified |
| per-function status | functions without their own golden trace are tagged `UNVALIDATED` in the source; end-to-end runs are still byte-exact through them | tmnfc `src/*.c` comments | verified |
| tracer loading (upstream) | a local mod loaded through Ultimate ASI Loader, which takes the name `binkw32.dll` (the original kept as `binkw32Hooked.dll`); configured by environment variables (`TMNF_TRACE_DIR`, `TMNF_TRACE_TARGETS`, `TMNF_TRACK_SHA256` ...) | tmnfc docs/LOCAL_ASSETS.md, `oracle/tracer/tracer.c` | verified |
| PhysicsStep2 site | the tracer expects an existing `e9` jump at 0x00549c90 (clean bytes `55 8b ec 83 e4 f8`): TMInterface hooks there first | tmnfc `oracle/tracer/targets.c` | verified (bytes) / inferred (who hooks) |
| end-to-end check | game runs driven through TMInterface 2.2.1 and Linesight's `Python_Link.as` socket plugin; the native per-tick capture is compared with the simulation after a documented normalisation | tmnfc `oracle/`, `tools/normalize_tick_capture.py` | verified |
| normalisation rule | only memory the game never wrote since `CSceneVehicleCar::VehicleReset` is copied before the compare (heap leftovers): car +0x5f8/+0x5fc turbo start/end tick and +0x608 roulette value (written only by `EnableTurbo` 0x007BCF90) until the first turbo; the upper 16 bits of the wheel contact material +0x128 (every game load/store is 16-bit); the wheel's item-space surface position +0x130..+0x13b until the first contact with a surface. Physical state is never normalised | tmnfc `tools/normalize_tick_capture.py` docstring | verified |
| published validation | 7 local capture replays byte-exact on CPU and CUDA; TAS replays (tmtas.exchange ids 53, 128, 230, 4091) match every captured tick: A01 noseboost 1,782 ticks reaching 1,422.3 km/h, A12 uberbug 1,031 ticks (312.6 -> 485.8 km/h between ticks 656 and 657), two bugslide maps 1,002 and 742 ticks | tmnfc docs/VALIDATION.md | verified (doc) |
| determinism in the game | the same inputs run twice in the game give byte-identical captures (`oracle/determinism_test.py`; my two vehicle-probe and two track-probe runs, 16,680 bytes each, identical) | tmnfc; own | verified (measured) |

## Floating point: why it can be bit-exact at all

| fact | value | source | status |
|---|---|---|---|
| x87 precision control | the game's CRT sets x87 precision to 53 bits, but **Direct3D device creation reprograms the control word to 24 bits** before physics runs (no `D3DCREATE_FPU_PRESERVE`). Under PC=24 every x87 result is rounded to a float significand, so it equals a correctly rounded float32 operation (only the exponent range is wider, never reached by car physics) | tmnfc `src/tmnf_fp.h` header comment ("CORRECTED MODEL (from golden traces)") | verified (by the traces) |
| consequence for a port | do each operation in float (or in double and round once); never accumulate in double; keep the exact operation order of the assembly, because the decompiler's order is not reliable; float->int is truncation; `fsqrt` under PC=24 = float sqrt | tmnfc `src/tmnf_fp.h` | verified |
| compiler flags | `-ffp-contract=off -fno-fast-math -fno-math-errno` (no FMA contraction), SLP vectorisation off for speed (not correctness) | tmnfc `CMakeLists.txt` | verified |
| self-test | `tests/x87_fp_identity.c` checks sqrt for every non-negative float bit pattern and + - * / over all exponent pairs; it passes on the Windows build too | tmnfc; own | verified |
| hint for other tools | anything that reads physics in the running game, or a mod that creates its own D3D device in the game process, must not change the x87 control word on the physics thread (inferred from the above) | - | inferred |

## Snapshot and capture formats

All little-endian, 8-byte magic, header with an endian marker `0x12345678` and SHA-256 fields.

| file | magic | content | source | status |
|---|---|---|---|---|
| `<VA>_<name>.bin` trace | `TMNFTRC1` | function VA, record count; per record seq, entry buffers and return buffers, each `{tag, original address, length, bytes}`; by-value scalars as 4-byte buffers with address 0 | tmnfc `src/trace_format.h` | verified |
| `.tmnftrack` | `TMNFTRK1` (version 3) | header 0x150 bytes, 12 sections (static collision entries, surfaces, meshes, trees, materials, corpus isometries, water map header + cells at CHmsZone+0x154 ...), exe and track SHA-256, payload SHA-256; pointers stored as file offsets and fixed up on load, then the mapping is made read-only | tmnfc `src/track.h` | verified |
| `.tmnfvehicle` | `TMNFM6G1` (version 4) | Model6 vehicle graph: tuning, wheels, curves, gearbox arrays, collision tree, bound to one track hash; the tuning lookup is `CSceneVehicleTunings`, keyed by the active tuning-array index at +0x24 (not by surface) | tmnfc `src/world.c`; own capture reports | verified |
| `.tmnfroute` | `TMNFROU1` | header 0xE0, 5 sections: start pose, checkpoints, finishes (alternatives; checkpoints + finishes <= 64), reference line | tmnfc `src/route.h` | verified |
| other tracer outputs | `TMNFBP01` broadphase, `TMNFDET1` detection, `TMNFRSP1` collision response traces | tmnfc `oracle/tracer/*_trace.h` | verified |
| per-tick native capture | 1,668-byte record per tick (race time, quaternion, rotation, position, ... wheels at 356 + wheel*328) | tmnfc `tests/replay_tick.c`; dz `compat/resim_check.py` | verified |
| input schedule | `TMNFRaceInputs`, 0x48 bytes per tick | dz `compat/resim_check.py` | verified |

Stadium car collision tree as captured (identical in three captures on three maps): 8 ellipsoids below one root, 4 body shapes (material 0) and 4 wheels (material 9) with half extents 0.182 x 0.364 x 0.364 m, front wheels at x = +-0.863, y 0.1525, z 1.782, rear wheels at x = +-0.885, z -1.2055 (car-local metres). 30 tuning objects, active key 29; 605 curves and 120 gearbox arrays captured. Source: own capture reports from `oracle/dump_vehicle_snapshot.py`; status verified (measured). United cars use sphere wheels and nested body nodes (tmnfc `src/world.c` comment).

## Track, vehicle and route: what can be built without the game

| fact | value | source | status |
|---|---|---|---|
| track | `tools/build_track` builds the collision snapshot offline from the `.Challenge.Gbx` and the user's packs (needs .NET 10 and GBX.NET from its own NuGet feed) | tmnfc | verified |
| vehicle | upstream captures it per track from the running game; **one capture drives every map**: re-bind the snapshot's track hash to the new map, respawn the car at the new start (`World_Respawn`) and step one tick without input; that gives the new map's race-time-10 state exactly (lolsport capture: Lucky Jump 5/5 and S81 Minis 4/4 runs exact) | [dz 130a848](https://github.com/cheatoskar/DeltaZero/commit/130a84895ab9bd413ce1cf01604e61f0efb3fa58), dz `tmnfc_port/README.md` | verified (measured) |
| start pose | `CGameCtnBlock::GetSpawnLoc` of the start block: the block info's spawn location (ground or air variant) times the block's mobil location, float32 in the game's operation order; bit-exact against the game on every map checked | dz `compat/spawn.py` | verified |
| route | checkpoints and finishes offline: each trigger is the mobil its block's mobil links to (`objectLinks`), box = the trigger tree's surface box, placed at `relativeLocation x block location`; finish times equal the replays' on 25 maps (73/73 runs) | [dz 7111f24](https://github.com/cheatoskar/DeltaZero/commit/7111f24d88e4c0c1102da0f048f6c95e2c51031a) | verified |
| input delay | the game applies an input one tick later; a virtual instance must model it | [dz d07475f](https://github.com/cheatoskar/DeltaZero/commit/d07475f6797a4339247c967a7960637c99d26b65) | verified |
| respawn | respawn to the last checkpoint; with no respawnable checkpoint passed (`CTrackManiaPlayerInfo+0x2e8` clear) the game's `CTrackManiaRace1P::SmallRespawn` (0x00472700 -> RespawnPlayer 0x0047C0D0 -> RespawnPlayerVehicle 0x0047BF00) restarts the race instead (`CGameRace::SetStatus(3)`); the DLL's race layer does the same | tmnfc `src/race.h`; dz 7111f24 | verified |

## Where it diverges from the game (measured on TMX maps)

Method: TMX maps with the most awards, each replay's inputs re-simulated, a run counts as exact when the ghost stays within 1 mm and the finish time matches.

| finding | value | source | status |
|---|---|---|---|
| overall | ~55-60 % of replays exact over the first ~150 maps; a map is usually all or nothing. Over a night: 14,081 maps, 176,106 replays, 99,936 exact (57 %) | dz `tmnfc_port/README.md`; dz-notes | verified (measured) |
| maps with any exact run | 96 % of their runs exact, analog steering included | dz-notes | verified (measured) |
| cause 1: builder order | the offline builder emits the same meshes and corpora as the game but in another order for stacked blocks in one x/z column; the BVH code equals the game's (no sort), so the corpus **input** order differs. Example: ESL-Hockolicious (TMX 414041) diverges at the first concrete contact (3.53 s) with either vehicle, and is 5/5 exact on a track dumped from the game. Most maps are permuted too and still exact. Tried and reverted: ReplaceByLastAt, x87-wide compare, surface-first | dz-notes; own `track_check` (7/7 dumps: same counts, not identical) | verified (measured) / open (rule) |
| cause 2: physics gap | not exact even on the in-game track with the map's own vehicle: the simulated car jumps 0.1-0.7 m **ahead** of the ghost within one 100 ms sample (the real car loses speed suddenly). Maps with dirt blocks fail 75-86 % (base rate 42 %), fabric and inflatable blocks ~60 %; at the deviation 35 % are in a StadiumDirt column, wheels often on grass near y 9-10. Example: Light Force (TMX 175621), landing at ~300 km/h near y = 9 | dz-notes; own `classify` run (3 maps builder, 7 physics) | verified (measured) / open (cause) |
| not the cause | display quality (`TMNF_QUALITY=high` fixed 0/40 failing maps; 3/14 in-game dumps identical with high vs low), map mood, age, type | dz-notes | verified (measured) |
| display quality matters for building | some decorations are collidable only at one display quality (`collidableCond` in the builder's scene code), so build for the quality the replay's driver used (default low) | [dz cedd2ed](https://github.com/cheatoskar/DeltaZero/commit/cedd2ed746cf2163440448cf4f28c19068b252ed) | verified (code) |
| skipped replays | respawns ~3 % before the route layer existed, analog gas ~0.5 % | dz-notes | verified (measured) |

## Maps the exact builder rejects, and the lenient mode

The strict builder in `tools/build_track` stops on anything it cannot reproduce exactly. On random TMX maps 38/40 built; over a night 132 maps were rejected. The rejections and what the lenient mode (`TMNF_LENIENT=1`, part of the Windows patch) does instead:

| rejection (builder message) | lenient behaviour | source | status |
|---|---|---|---|
| `file ground flag X but IsBlockOnGround=Y` | keep the map file's ground flag and its unit infos | dz `tmnfc-windows.patch` (`scene.py`) | verified |
| flat or frontier block with no matching zone (`no terrain block under ...`) | place it like a classic block | same | verified |
| field unit missing (block outside the field) | skip the unit | same | verified |
| `water face at y=..., not 8` (raised water) | leave it out of the water grid (stays a plain surface) | same (`build_track.py`) | verified |

Result: 85 of 132 rejected maps build, 381 of their 949 replays exact. Every run on a lenient track is still checked against the replay's ghost, so a wrong guess only costs that map ([dz 130a848](https://github.com/cheatoskar/DeltaZero/commit/130a84895ab9bd413ce1cf01604e61f0efb3fa58)). Status verified (measured).

## What it is good for

| use | notes | source | status |
|---|---|---|---|
| headless re-simulation | no game client: ~120k ticks/s per core (the game: ~2.5k ticks/s); loading a track ~1-2 s, so keep one process per map | dz `tmnfc_port/README.md`, dz-notes | verified (measured) |
| replay checks | inputs + ghost + finish time decide "exact"; a quick way to sort replays a simulator can reproduce from those it cannot | [dz c9cd03c](https://github.com/cheatoskar/DeltaZero/commit/c9cd03c6ab334a85dca1426d38badce46ed83c74) | verified |
| AI training | upstream: per-track RL agents beat the author medal on four tracks; DeltaZero: imitation data from exact runs and batched PPO in one TMNF-C world (~4,600 ticks/s with 64 cars, the PPO update was the bottleneck), save/rewind of any car state | tmnfc README; [dz d07475f](https://github.com/cheatoskar/DeltaZero/commit/d07475f6797a4339247c967a7960637c99d26b65), dz-notes | verified |
| TAS checks | a candidate input schedule can be checked offline before it is tried in the game; the published TAS validations show that bug mechanics (noseboost, uberbug, bugslide) are reproduced on those runs. Treat a hit on a map of the "physics gap" kind with care and confirm in the game | tmnfc docs/VALIDATION.md; own | verified / inferred |
| RE reference | its annotated functions and constants are a second opinion for decompile work ([crosscheck.md](crosscheck.md)) | tmnfc | verified |

## Pitfalls on Windows (from the port)

| pitfall | fix | source | status |
|---|---|---|---|
| upstream is Linux-only (mmap, futex, CPU affinity, `__builtin_cpu_supports`) | force-included shims: mmap reads the file, futex -> `WaitOnAddress`, affinity ignored, cpuid + xgetbv instead of `__builtin_cpu_supports` (zig's Windows runtime lacks libgcc's `__cpu_model`) | dz `tmnfc_port/compat/` | verified |
| toolchain | `zig cc` from `pip install ziglang`: x86_64 for the physics DLL, `-target x86-windows-gnu` for the 32-bit tracer and memory reader; no Visual Studio, no admin rights | same | verified |
| `src/world.c` frees two `aligned_alloc` blocks with `free()` | on Windows `aligned_alloc` maps to `_aligned_malloc`, which needs `_aligned_free`; the patch routes both frees through a macro | dz `tmnfc-windows.patch` | verified |
| no `MSG_WAITALL` on Windows sockets | loop `recv` until the length is read (TMInterface client) | same | verified |
| tracer needs environment variables in the game process | a `TMNFTracer.cfg` (`NAME=VALUE` lines) next to the DLL as fallback, so the DLL can be injected into an already running game with `CreateRemoteThread(LoadLibraryA)`; the game folder stays untouched | same; `compat/inject.c` | verified |
| ASLR | the normal TMLoader 2.12.0 profile relocates the exe and breaks the fixed-address memory reader; use a profile with `2.12.0-compat` (same `.text` as Nations 2.11.26, base 0x400000) | dz `tmnfc_port/README.md` | verified |
| NuGet | the track builder's `NuGet.config` clears all sources; add nuget.org next to the GBX.NET feed | dz patch | verified |
| spawn check | worlds respawned at another map's start can move during their spawn tick, so the vector environment's spawn check has to be skippable; exactness is checked against the ghosts instead | dz patch (`src/vec_env.c`) | verified |
| long paths | keep the clone at a short path (MAX_PATH broke a scratch-folder build) | dz-notes | verified |
| disk | built tracks are 3-26 MB each; a cache of every built track reached ~300 GB in two nights: delete each track after its map, rebuilding takes ~2 s | [dz f8f785f](https://github.com/cheatoskar/DeltaZero/commit/f8f785ff097c481b1f2fd68d6e07643a49970b11), dz-notes | verified |
| CPU features | the DLL picks AVX2 / AVX-512 at run time, so one build runs on machines without AVX-512 | dz-notes | verified |
| Wine capture lane | custom maps may crash in `CHmsPackLightMap::InitBitmapsAndRaster` after shader compile errors: install native `d3dx9_31` in the prefix and let the first shadow calculation finish | tmnfc docs/LOCAL_ASSETS.md | verified (doc) |
| unattended capture | each game restart for a capture needs the offline/login and shadow dialogs clicked; plan capture sessions with a person at the PC, one game session for many maps (load maps with the `map` console command) | dz-notes | verified |
