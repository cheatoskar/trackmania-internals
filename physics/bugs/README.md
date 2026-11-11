# Physics bugs and glitch mechanics: catalogue (TMNF/TMUF Stadium car)

Part of the [physics overview](../tmnf-physics.md). Force model: [model6.md](../model6.md). Layout: [state-layout.md](../state-layout.md). Unconfirmed ideas: [candidates.md](../candidates.md). Where my model agrees with the public ports: [crosscheck.md](../crosscheck.md).

Status words: **verified** (decompile read and re-checked, or reproduced in simulation on a replay that the simulation reproduces exactly, or measured in the running game), **inferred**, **unverified**. Source tags: **[decompile]** function + address (TMNF `TmForever.exe`, build key `4d431494`, VA), **[sim]** ForeverValidator / TMNF-C reproduction, **[game]**, **[community]** with name and link. "Cause" is the engine-side reason; community names are listed because people look for them.

## Catalogue

| mechanic | what you see | cause (short) | conditions | status | page |
|---|---|---|---|---|---|
| bugslide / grass slide | car turns much sharper than normal, can be perpendicular to its velocity | gas+brake+steer switches Model6 into a burnout/donut state machine with a pull towards a pivot | gas > 0.1, brake > 0.1, steer; car-local forward speed 10.8-75 km/h | verified [decompile][sim] | [bugslide](bugslide.md) |
| nosebug | instant 90 deg turns while on the nose | the donut entry only tests forward speed along the car axis, which is small on the nose at any total speed | bugslide inputs, car about 80-90 deg nose down, front wheel(s) touching | verified [sim] | [nosebug](nosebug.md) |
| noseboost | chain of nosebugs gains speed up to 1000 km/h | donut re-entered every second substep; explicit Euler plus an exponential factor that doubles the gain per kick every 100 km/h | nose position at about 390-480 km/h or more | verified [sim] | [noseboost](noseboost.md) |
| uberbug | one-tick speed gain of hundreds of km/h on a collision | a border hit pushes the car away from the donut pivot; the `-0.2*v_r` term in the exponent multiplies the pull | sliding sideways with gas+brake+steer at about 200+ km/h, leading front wheel hits a border | verified [sim] | [uberbug](uberbug.md) |
| bluebug | hop or kick at a block seam | many contact points of one wheel each fire an impulse (restitution 0.8) in the same pass | any seam between blocks, speed into the seam matters | verified [sim] | [bluebug](bluebug.md) |
| rammstein | wheel lifted by a raised road border | wheel contact tilted > 45 deg sideways is not a ground contact but a bouncy impulse at the wheel | border/edge face with sideways-facing normal | verified [decompile], example [sim] | [rammstein](rammstein.md) |
| coast drag | releasing gas brakes the car, also in the air | `ApplyFrictionForces` applies `1.0 + 0.30*speed` against the velocity when no gas is held, no ground check | gas below 1e-5 | verified [sim], code found; not checked in game | [coast-drag](coast-drag.md) |
| edgebug | car pitches nose down after leaving an edge | rear wheel kicked by the edge while leaving it | rear wheel touches the edge as the car leaves | verified [sim] (measured examples) | [edgebug](edgebug.md) |
| wheel-0 surface bug | engine, brake, top speed and slide grip follow the front-left wheel | `ComputeVehicleGroundMaterialVals` always reads wheel 0's surface | any wheel in contact; stale while wheel 0 airborne | verified [decompile][sim][game] | [model6.md](../model6.md#wheel-0-rule-surface-bug) |
| bump-map early exit | dirt bumps vanish on later wheels | `CreateFakeContacts` returns at the first touching wheel on a flat surface | an earlier wheel on a surface without bump map | verified [decompile][sim] | [model6.md](../model6.md#bump-contacts-fake-contacts) |
| impulse speed kill | speed set to zero | one wheel impulse raising `|v|^2` by more than 10000 sets velocity 0 | very large impulse | verified in code, never observed in 22 replays | [bluebug](bluebug.md#also-seen) |
| water / golf-ball contacts ignored | solids tagged Water pass through the car body | `AbsorbContact` drops surfaces 13 and 23 | water or golf-ball material on a solid | verified [decompile], not observed | [candidates](../candidates.md) |
| speedslide | speed gain by sliding at high speed (steering only) | slipping acceleration (constant 10) beats the normal curve above 201 / 401 km/h | grass/dirt from 201 km/h, road from 401 km/h | verified [decompile][game] | [below](#speedslide-community-technique-explained) |
| landing bug | speed loss or hop on landing | cause not found; community reports only | after landing, dirt prone | unverified [community] | [below](#landing-bug-and-other-community-entries) |
| checkpoint / finish handling | race time and freewheel state at checkpoints | see below | | inferred (port source review) | [below](#checkpoint-and-finish-handling) |

## Speedslide (community technique, explained)

The community describes a speedslide (also "speeddrift", "SD") as gaining speed by steering at an angle at high speed, needing about 400 km/h on road and about 200 km/h on dirt/grass/plastic; an "autoslide" is said to start by itself above about 598 km/h on flat road ([community] https://www.trackmania.wiki/wiki/Speedslide, https://www.trackmania.wiki/wiki/Bugslide). The only quantitative earlier analysis I know is the TM-AlgoCrack write-up by Tomashu (https://github.com/TomashuTTTT7/TM-AlgoCrack): constant sliding drive, normal drive in steps of 16 / 11 / 7 / 5.5 / 1 (0-100, 101-200, 201-400, 401-800, 801+ km/h), a side-friction curve and material multipliers 1.0 / 0.25 (dirt) / 0.15 (grass). I measured the same numbers in the running game and found the code path.

| fact | status |
|---|---|
| Normal acceleration steps 16 / 11 / 7 / 5.5 / 1 and slipping acceleration 10 (constant) read from the game | verified [game] |
| Blend `s = clamp(((sum\|d\|-sum c)/sum c)/t[0x200], 0, 1)`, accel = `s*Slipping + (1-s)*Normal`, only if the car slid this tick | verified [decompile][sim] |
| Gain above 201 km/h = 10 - 7 = 3, above 401 km/h = 10 - 5.5 = 4.5 | verified (arithmetic on measured values) |
| Road needs a large angle because lateral drag (20 * v_lat above the cap of 60-75) eats a gain of 3; only 4.5 pays. Dirt/grass grip 0.25/0.15 slides at tiny angles | inferred, consistent with the thresholds |
| Material grip comes from the **front-left wheel** only for the slide detection (wheel-0 rule), not an average over wheels as the community plugin assumes | verified [decompile]; effect on the community plugin's estimate inferred |
| Extra yaw term `\|angvel_y\| * 29.8759` in the community plugin | unverified, constant not found in the engine |

Community tooling: SD Railgun by SaiMoen maintains speedslides in TMInterface (https://sai-moen.github.io/TMInterface-AS-SaiMoen/distributions/incremental.html).

## Landing bug and other community entries

These are community claims; I could not trace a cause for any of them in the code. Short quotes only; follow the links.

| entry | claim | status |
|---|---|---|
| landing bug | after landing the car may lose speed or jump; dirt is "notoriously prone" in TMNF ([community] https://www.trackmania.wiki/wiki/Surfaces); the Miraheze bug list gives about a 55 % chance of losing 60 % of the speed ([community] https://tmnf.miraheze.org/wiki/UberBug). Air-brake in the air is said to reduce it | unverified; dirt-bump early exit ([model6.md](../model6.md#bump-contacts-fake-contacts)) is a possible contributor (inferred) |
| edge bug | the car "jumps" at sharp block edges (Miraheze list) | see [edgebug](edgebug.md) for the measured effect |
| wallbang / glitch through walls | car passes blocks; banned in a competition patch | unverified; candidate cause in [candidates.md](../candidates.md) (substep length) |
| spider bug, starbug, mud/water bug, colour-booster bug, bug finish | listed on the Miraheze page; mixed quality, partly jokes | unverified |
| TPSlide | bugslide variant on Island, not Stadium | out of scope |
| rules | noseboost and uberbug are excluded from the "Vanilla" TAS category ([community] https://tmtas.exchange/faq) | community |

## Checkpoint and finish handling

From reading the ForeverValidator and TMNF-C sources (not from my own decompile), unverified in game:

- Race time is the tick start of the crossing, 10 ms resolution.
- Checkpoints may be taken in any order; there is no direction test.
- The no-engine (freewheel) flag is cleared only by a **new** checkpoint, a counted finish or a respawn; a checkpoint already taken does not clear it.
- The inputs of the first running tick are wiped by the start reset; if both digital steer keys are held, left wins.
- Contact order at block corners follows the world X coordinate; vertex contacts push out only 40-60 % of the penetration.
- After a race ends the trace of a simulator may keep logging; step lengths in those ticks are not physics.

A "ring" or checkpoint-trigger bug in the sense of passing a checkpoint without driving through it is **not** documented in my sources; I only saw cars flying through ring checkpoints at high speed (the substep-length question is in [candidates.md](../candidates.md)).

## Reproducibility facts

- The simulation is deterministic at 100 Hz. ForeverValidator reproduced 2169 Stadium replays with 0 mismatches; TMNF-C reproduces 7 recorded runs byte-exactly on CPU and CUDA ([community] https://github.com/Skycrafter-dev/ForeverValidator, https://github.com/adonis-singh/TMNF-C). verified.
- Random inputs never changed the speed by more than 15 km/h in a single tick (about 4100 random 2 s runs from 23 start states). [sim] verified. Large per-tick jumps need one of the mechanics above.
- Per-call numbers for the bugs were taken from a per-call trace build of ForeverValidator on replays it reproduces exactly.

## Community tooling

TMInterface by donadigo (https://donadigo.com/tminterface/) exposes the state fields used here. Community bruteforce scripts such as "Nosepos+" (nose-position scoring) score a candidate by yaw/pitch/roll against a target nose position; they read wheel contact and gear only through condition checks. A yaw/pitch score cannot tell a settled nose landing from a body hit (see [nosebug](nosebug.md)).
