# fvlab: traces and a search tool on ForeverValidator

A patch series on [ForeverValidator](https://github.com/Skycrafter-dev/ForeverValidator) by Skycrafter-dev, the C++ reconstruction of the TMUF physics that reruns replays bit-exactly. The patches add:

1. opt-in CSV traces of the physics (`FV_TRACE`, `FV_TRACE_DONUT`, `FV_TRACE_IMPULSE`, `FV_TRACE_STEP`), off unless set;
2. running maxima of the donut pull (`tmphys_probe` in `scene_vehicle_car_handling.cpp`) for the search score;
3. `tools/fvlab.cpp`, a command-line probe and search tool (`fvlab` / `fvlab_dev` CMake targets), and `FV_IGNORE_GHOST` in the replay reader.

**Searches cheat.** Every objective that does not pin down *where* and *how fast* gets satisfied in a way you did not mean (crashing next to the target, flying under the road, a one-tick speed spike). Probe every "hit" before you believe it, then play it in the real game. Concepts, pitfalls and the verification routine: [tas/searching.md](../../tas/searching.md). Offline research only; see [tas/README.md](../../tas/README.md).

## Licence

ForeverValidator is MIT:

```
MIT License

Copyright (c) 2026 Skycrafter-dev contributors
```

(full text in the upstream `LICENSE`). These patches are a derived work under the same MIT licence, with that notice kept; the patches themselves are by cheatoskar. Nothing here is endorsed by the upstream author.

## Contents

| File | What |
|---|---|
| `patches/0001-*.patch` .. `0028-*.patch` | `git format-patch` of the 28 commits, in order |
| `fvlab.cpp` | `tools/fvlab.cpp` as it is after patch 0028, to read without applying anything |

Base: upstream commit `6a51fdfb80872424d40abb3f57f55e1a13f635f1` ("Fix segmented GPU winner inputs across horizon and promotion", 2026-10-05). Commit messages are unchanged. In patch 0028 a few code comments that named the maps and checkpoints of unfinished routes were reworded; code and line count are unchanged. Checked 2026-11-11: the series applies with `git am` on the base commit, and the result differs from the original branch only in those comments.

## Apply

```bash
git clone https://github.com/Skycrafter-dev/ForeverValidator
cd ForeverValidator
git checkout -b fvlab 6a51fdfb80872424d40abb3f57f55e1a13f635f1
git am /path/to/trackmania-internals/tools/fvlab/patches/*.patch
```

On a newer upstream commit the series may need small fixes; start from the base commit.

## Build (Windows, CPU only)

1. Run upstream's Windows build script once (see its README). It downloads a MinGW GCC toolchain and vcpkg; on a machine without the Vulkan SDK it stops at Vulkan, which is fine.
2. Configure with that toolchain, GPU backends off and the benchmark targets on (the `fvlab` targets sit in that block):
   ```
   cmake -S . -B build/windows-gcc-cpu -G Ninja -DCMAKE_BUILD_TYPE=Release
         -DCMAKE_CXX_COMPILER=<toolchain>/bin/x86_64-w64-mingw32-g++.exe
         -DCMAKE_TOOLCHAIN_FILE=<vcpkg>/scripts/buildsystems/vcpkg.cmake
         -DVCPKG_TARGET_TRIPLET=x64-mingw-static -DVCPKG_HOST_TRIPLET=x64-mingw-static
         -DFOREVERVALIDATOR_ENABLE_VULKAN=OFF -DFOREVERVALIDATOR_ENABLE_CUDA=OFF -DFOREVERVALIDATOR_ENABLE_HIP=OFF
         -DFOREVERVALIDATOR_BUILD_BENCHMARKS=ON
   cmake --build build/windows-gcc-cpu --target fvlab fvlab_dev
   ```
   With GCC 16 add `-DCMAKE_CXX_FLAGS=-mxsave` (the CPU-feature probe uses `_xgetbv`).
3. Copy `libgcc_s_seh-1.dll`, `libstdc++-6.dll` and `libwinpthread-1.dll` from the toolchain's `bin` next to the exe.

`fvlab_dev.exe` is the same tool as a second binary: Windows locks `fvlab.exe` while searches run, so you can rebuild the other one. For parallel searches with different builds, copy the exe to a new name per search.

The scripts in [scripts/tas/](../../scripts/tas/) find the exe through the environment variable `FVLAB_BIN` and the game's packs through `TM_PACKS`.

## Usage

```
fvlab PACKS REPLAY MODE [arguments]
```

`PACKS` is the `Packs` folder of a complete TrackMania United Forever install (TMNF alone is not enough). Times are replay times in ms (race start = 0, first input tick at 10 ms). Replay file names must be ASCII.

| Mode | Arguments | What it does |
|---|---|---|
| `inputs` | - | the replay's input events as CSV (`timeMs,action,kind,switch,analog`) |
| `probe` | T0 TICKS | state per tick from T0, columns `timeMs,kmh,px,py,pz,pitch,roll,wx_l,wy_l,wz_l,contacts,gas,brake,steer,cps` (pitch > 0 = nose down, contacts = 4 digits FL FR RL RR, 1 = touching, cps = checkpoints collected), each row followed by an indented `q,x,y,z,w` line with the rotation |
| `blocks` | - | placed blocks: name, purpose, world position and rotation (start, finish, checkpoints) |
| `scene` | X Y Z R | collision triangles near a point |
| `seam` | T0 FROM TO HORIZON | constant-steer sweep in a window |
| `keys` | FROM TO HORIZON | keyboard-only steering with one switch (human feasibility); `FVLAB_HOLD=1` holds gas and brake |
| `random` / `replay` | SEED STARTS TICKS GAIN / SEED START TICKS | random-input anomaly search |
| `opt` | SEED FROM TO SEGMENTS SECONDS OUT | earliest precise finish time (or box entry), local search on the steering in (FROM, TO) |
| `uber` | SEED FROM TO SECONDS OUT HORIZON X Y Z | search towards a target point; the objective switches below change the score |

Action ids in the input CSV: 1 Accelerate, 3 Brake, 4 Steer, 7 RaceRunning, 8 FinishLine, 9 Respawn; kind 1 = switch, 2 = analog (steer -65536..65536, negative = left). [scripts/physics/to_script.py](../../scripts/physics/to_script.py) and [script_to_csv.py](../../scripts/physics/script_to_csv.py) convert between this CSV and TMInterface scripts.

Verify any result with a fresh full run: `FVLAB_INPUTS=best.csv fvlab PACKS REPLAY opt 1 100 END 8 0 tmp.csv` prints the finish of the base run; `probe` with `FVLAB_INPUTS` shows the run tick by tick.

## Environment switches

### ForeverValidator (patches 0001, 0021)

| Variable | Effect |
|---|---|
| `FV_TRACE=<csv>` | one row per `ComputeForces` call: state, burnout phase, per-wheel contacts, materials and contact counts |
| `FV_TRACE_DONUT=<csv>` | the donut pull terms per call |
| `FV_TRACE_IMPULSE=<csv>` | every point impulse on the car |
| `FV_TRACE_STEP=<csv>` | the speed after integration and after the collision response per physics step |
| `FV_IGNORE_GHOST=1` | load a replay whose ghost state the reader rejects (TMInterface replays saved mid-run); only the inputs are used |

Trace analysis: [scripts/physics/](../../scripts/physics/) (`analyze_trace.py`, `run_report.py`, `find_kicks.py`, `multicontact.py`, `summarize_traces.py`).

### fvlab, general

| Variable | Effect |
|---|---|
| `FVLAB_OPTIMIZED=1` | optimized CPU backend (bit-identical, faster) instead of the reference backend |
| `FVLAB_INPUTS=<csv>` | replace the replay's inputs with this CSV before anything else |
| `FVLAB_RTA=1` | `uber`: human-playable inputs only |
| `FVLAB_FILL=1` | one steer event on every tick of the window (TMInterface's "fill missing steering input") |
| `FVLAB_CHANGES=n` | `uber`: 1..n changes per attempt (default 4) |
| `FVLAB_KICK_STALL=s` | `uber`: after s seconds without progress, restart from the best with 8-20 random changes |
| `FVLAB_HOLD=1` | `keys`: hold gas and brake |
| `FVLAB_STEER_MIN`, `FVLAB_STEER_MAX`, `FVLAB_STEER_STEP` | sweep range of the steering modes |
| `FVLAB_LEAKTEST` | memory-leak test of the search loop (development) |

### fvlab, objectives (`opt` and `uber`)

| Variable | Mode | Effect |
|---|---|---|
| `FVLAB_TRIGGER=x,y,z,sx,sy,sz` | `opt` | earliest entry into this box (TMInterface corner + size), sub-tick interpolated |
| `FVLAB_TRIGGER_SPEED=1` | `opt` | highest speed at the box entry instead |
| `FVLAB_TRIGGER_KMH_MS=w` | `opt` | entry time minus w ms per km/h at the entry |
| `FVLAB_TARGET=x,y,z` | `opt` | tie-breaker inside the finish tick |
| `FVLAB_NEED_CPS=n` | `uber`, `opt` | distance, nose landing or trigger box count only after n checkpoints |
| `FVLAB_SPEED_WEIGHT` | `uber` | weight of the peak speed (default 0.1; 0 = distance only) |
| `FVLAB_TARGET_MINKMH` | `uber` | +0.5 m per km/h below this speed at the target |
| `FVLAB_LAND_BOX=x0,y0,z0,x1,y1,z1` | `uber` | target is a landing in this box (2+ wheels down) |
| `FVLAB_GOAL_CPS=n`, `FVLAB_GOAL_KMH_MS=w` | `uber` | collecting the n-th checkpoint is the goal (fastest, or earliest with w ms per km/h) |
| `FVLAB_GOAL_NEAR=x,y,z,r`, `FVLAB_GOAL_RISING=v`, `FVLAB_GOAL_MINKMH=k`, `FVLAB_GOAL_MAXY=y` | `uber` | the goal checkpoint counts only near a point, while rising, fast enough, or coming from below |
| `FVLAB_STAGE=n,x,y,z,r,maxY,minKmh` | `uber` | the n-th checkpoint must be taken at that place and speed, else later checkpoints do not count |
| `FVLAB_VIA=x,y,z,sx,sy,sz`, `FVLAB_VIA_W`, `FVLAB_VIA_MINKMH`, `FVLAB_VIA_KMHW` | `uber` | soft guide box the car should pass through |
| `FVLAB_BOOST_BOX=...`, `FVLAB_BOOST_KMH`, `FVLAB_BOOST_BOX_W`, `FVLAB_BOOST_DIR=w`, `FVLAB_BOOST_FROM=ms` | `uber` | where the first tick at BOOST_KMH must be, its direction 100 ms later, from which time on |
| `FVLAB_OBJECTIVE=nose`, `FVLAB_LAND_MIN`, `FVLAB_NOSE_MINKMH`, `FVLAB_LAND_MINY`, `FVLAB_NOSE_LOSS`, `FVLAB_LOSS_MS`, `FVLAB_NOSE_ROLL`, `FVLAB_NOSE_SPIN`, `FVLAB_NOSE_BOX`, `FVLAB_NOSE_DIR`, `FVLAB_NOSE_NOLAND_KMH`, `FVLAB_NOSE_ONPITCH`, `FVLAB_NOSE_HOLD`, `FVLAB_GAIN_MS` | `uber` | nose landing and nose-ride objective with its penalties; `FVLAB_GAIN_MS` and the input window must reach well past the landing (1.5 s and more) |
| `FVLAB_OBJECTIVE=noselow`, `FVLAB_BOOST_GAIN`, `FVLAB_LIFT_MAX` | `uber` | lowest landing speed that still boosts |
| `FVLAB_OBJECTIVE=nosepos`, `FVLAB_EVAL_FROM/TO`, `FVLAB_ANGLE_MIN`, `FVLAB_POSE_WHEELS`, `FVLAB_POSE_PITCH`, `FVLAB_POSE_ROLL`, `FVLAB_POSE_YAW`, `FVLAB_POSE_MINKMH`, `FVLAB_POSE_SOFT`, `FVLAB_POSE_KMH_W`, `FVLAB_POSE_BOX` | `uber` | reach a pose (wheel states, pitch, roll, yaw, speed) in an evaluation window |
| `FVLAB_OBJECTIVE=chain`, `FVLAB_WAYPOINTS="x,z,r;..."`, `FVLAB_CHAIN_FROM`, `FVLAB_CHAIN_MINKMH`, `FVLAB_CHAIN_OFF_W`, `FVLAB_CHAIN_AIR_W` | `uber` | stay on the nose on the ground through waypoints |
| `FVLAB_EXP_WEIGHT`, `FVLAB_EXP_MINKMH`, `FVLAB_EXP_KEEP_MS` | `uber` | bonus for the near-uber exponent of the donut pull, only at speed and only if the speed holds |
| `FVLAB_SPIKE_FILTER=n`, `FVLAB_RAW_KMH_WEIGHT=w` | `uber` | speed = minimum over n ticks (one-tick contact spikes do not count); optional weight on the unfiltered peak |
| `FVLAB_WHEELIE_WEIGHT=w` | `uber` | reward a ride on the rear wheels (the uber set-up of several public TASes) |

The exact semantics and defaults are in the comment next to each `std::getenv` in [fvlab.cpp](fvlab.cpp). The search loop: `opt` cuts (FROM, TO) into SEGMENTS and changes 1-3 steer events per attempt, restoring the cached state at the segment start; every new best is re-run from the window start before it is accepted. `uber` mutates steer, gas and brake events and simulates to TO + HORIZON; score (lower is better) is the closest 3D distance to the target minus the speed and exponent terms, or the finish time minus 1e6 when it finishes.

## Pitfalls

- A replay saved without finishing validates only the start state.
- Restored mid-run snapshots can drift from a continuous run: verify with a full run.
- Window bounds must be on the 10 ms grid and must not contain structural events (RaceRunning, FinishLine, Respawn); nothing is placed on `FROM`.
- In the air, inputs change only the rotation, not the path of the centre of mass.
- All search processes share the CPU: one search at a time, at most cores - 1 workers.
