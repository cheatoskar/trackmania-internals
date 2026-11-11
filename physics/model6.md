# Model6: the Stadium car force model (TMNF/TMUF)

Part of the [physics overview](tmnf-physics.md). Exe: TMNF `TmForever.exe`, build key `4d431494` (md5 prefix `e22ff952`). Addresses are virtual addresses (VA, image base 0x400000), not file offsets; `t[0x..]` and `car+0x..` are byte offsets inside the tuning object / car object (see [state-layout.md](state-layout.md)). TMUF addresses differ per function (no constant delta, see [contradictions.md](../contradictions.md) row 6).

Status words: **verified** = read from the decompile and re-checked, or reproduced in simulation on an exactly matching replay, or read from the running game; **inferred** = my reading, not tested; **unverified** = claimed somewhere, not checked. Source tags: **[decompile]** function + address, **[sim]** ForeverValidator / TMNF-C reproduction, **[game]** read from the running game, **[community]** with name and link.

Related pages: [bugs catalogue](bugs/README.md), [candidates](candidates.md), [cross-check with the public ports](crosscheck.md).

## Units and tick

| fact | value | source | status |
|---|---|---|---|
| physics tick | 10 ms (100 Hz); race time in ms; the game re-simulates inputs at this rate to validate replays | [community] tmtas.exchange FAQ (https://tmtas.exchange/faq); TMNF-C docs | verified |
| engine speed unit | m/s; km/h = m/s x 3.6 | [decompile] every tuning curve getter multiplies its m/s argument by 3.6 | verified |
| curve keys | km/h of the car-local forward speed (`v.z`) | [decompile] tuning getters 0x7f3c70-0x7f4130 | verified |
| car-local axes | `v.x` lateral, `v.y` up, `v.z` forward | [sim] | verified |
| inputs | gas `car+0x50`, brake `car+0x54`, steer `car+0x58`; "pressed" means > 0.1; the steering angle used by the model is `-steer * asin(1 / (|v.z| * t[0x70] + t[0x6c]))` | [decompile] ComputeForces 0x7c69e0 | verified |

## Call chain per tick

1. The physics step (`CHmsZoneDynamic::PhysicsStep2` 0x549c90) splits the 10 ms tick into substeps for a moving body: `n = min(1000, trunc(dt * (|v| + |w|) / L) + 1)`, `dt` = 0.01 s, `|w|` = angular speed in rad/s, `L` = dyna params[12] = **0.3 m** [game]. [decompile] [sim] verified.
2. Each substep runs `ComputeCorpusForces` (damping, then the vehicle callback `CSceneVehicleCar::ComputeForces(dt/n)`), integration, collision detection and response, `DoPostCollisionDynamic`. One additional full-`dt` pass happens first for bodies in the dynamic set. [decompile] [sim] verified.
3. Without spin the count steps up at 108, 216, 324, 432 km/h (every +30 m/s, `trunc`). Spin lowers these speeds. Measured with a per-call trace: the vehicle `ComputeForces` ran `n` times per tick (1 at standstill, 5-6 at 530 km/h, one tick with 32 calls under heavy spin). [sim] verified. Reading the code alone suggested `n+1` calls; see [contradictions.md](../contradictions.md) row 24.
4. Everything inside `ComputeForces` (clamp, state transitions, timers, turbo) therefore runs several times per tick with the same tick number. A donut entry in call k can already be continued in call k+1 of the same tick. [decompile] [sim] verified.

`ComputeForces` (0x7c69e0), in order: `CreateFakeContacts` (0x7c3c00), `IntegrateVehicle`, `ApplyFrictionForces` (0x7bed10), **speed clamp**, `ComputeVehicleGroundMaterialVals` (0x7c2800), `GetSlopeAdherence`, steering angle, then dispatch on tuning `t[0x354]`: 3 = Model4, 4 = Model5, 5 = Model6, else Model3. The Stadium car has `t[0x354]` = 5 [game]. Models 4 and 5 are unvalidated in TMNF-C. The whole force block is skipped (state zeroed) when the block-speed bits `0x30` of the car flags are set (start countdown) or the water box x extent is negative. [decompile] [sim] verified.

Integration is explicit Euler (`CHmsDyna::IntegrateStep`): position uses the old velocity, velocity uses the force. A force perpendicular to the velocity therefore lengthens it: `|v'|^2 = |v|^2 + (a*dt)^2`. This is the energy source behind every bugslide-family bug. [decompile] [sim] verified.

## Speed clamp (the "1000 km/h cap")

| fact | value | source | status |
|---|---|---|---|
| limit | `car+0x2e0` = 277.7778 m/s (1000 km/h); constructor default (constant at 0xb9f208), ForeverValidator loads it from the car file's PhysicalParams chunk; measured 277.78 for Stadium | [decompile] [game] [sim] | verified |
| placement | at the start of **every** `ComputeForces` call (after `ApplyFrictionForces`, before the material values): if `|v|^2 > max^2` then `v *= max/|v|`; also runs while the car is in the donut state | [decompile] [sim] | verified |
| condition | only if flag bit 2 of `car+0x2f4` is set and bit 8 clear; constructor sets bits 1, 2, 4 | [decompile] | verified |
| consequence | speeds above 1000 km/h exist only as the gain of the last call(s) of a tick; the next call clamps first. Positions use the pre-kick speed, so the car centre does not travel faster than 1000 km/h because of a kick. Post-kick velocities up to 2716 km/h were measured; TMNF-C reports 1422.3 km/h on a noseboost capture | [sim] | verified |
| car reference point | up to 1043 km/h (2.898 m per tick) seen while spinning at 28 rad/s with |v| = 997 km/h; probably the reference point is offset from the centre of mass | [sim] | inferred |

The community wiki lists per-surface maxima (grass 500, dirt 900, road 1000 km/h; https://www.trackmania.wiki/wiki/Surfaces). These match the measured top-speed blend of the surfaces below (0.5, 0.9, 1.0 times the 1000 km/h tuning value) but are a force-side soft limit, not the clamp. [community] + [game], inferred.

## Surfaces

`EPlugSurfaceMaterialId` name table at 0xd05500 (31 entries); the same 31 names are in TMInterface's `PlugSurfaceMaterialId` ([decompile] + [community], verified): 0 Concrete, 1 Pavement, 2 Grass, 3 Ice, 4 Metal, 5 Sand, 6 Dirt, 7 Turbo, 8 DirtRoad, 9 Rubber, 10 SlidingRubber, 11 Test, 12 Rock, 13 Water, 14 Wood, 15 Danger, 16 Asphalt, 17 WetDirtRoad, 18 WetAsphalt, 19 WetPavement, 20 WetGrass, 21 Snow, 22 ResonantMetal, 23 GolfBall, 24 GolfWall, 25 GolfGround, 26 Turbo2, 27 Bumper, 28 NotCollidable, 29 FreeWheeling, 30 TurboRoulette. Friction/restitution pairs of the collision response are read from a table at 0xd6eec0 (31 x two floats, filled at runtime).

Per-surface blend `[topspeed, accel, brake, grip]`, read from the running game (only values other than 1; verified [game]):

| surface | topspeed | accel | brake | grip |
|---|---|---|---|---|
| Grass | 0.5 | 0.4 | 0.05 | 0.15 |
| Ice | 1 | 0.1 | 0 | 0 |
| Sand | 0.2 | 0.4 | 1 | 0.5 |
| Dirt | 0.9 | 0.9 | 0.4 | 0.25 |
| DirtRoad | 0.7 | 1 | 1 | 0.5 |
| WetDirtRoad | 0.9 | 1 | 1 | 1 |
| WetPavement | 1 | 1 | 0.85 | 0.85 |
| WetGrass | 0.1 | 0.5 | 1 | 0.5 |
| Snow | 0.2 | 0.6 | 0.1 | 0.1 |

All other surfaces (Asphalt, Concrete, Pavement, Metal, Wood, Turbo, ...) are 1/1/1/1.

### Wheel-0 rule (surface bug)

`ComputeVehicleGroundMaterialVals` (0x7c2800) loops over the wheels and checks wheel i for ground contact, but always reads the surface id of **wheel 0** (the push of constant 0 at 0x7c2856). The car-wide blend is therefore wheel 0's surface as soon as any wheel touches the ground, and while wheel 0 is airborne its last (stale) surface id is used. Wheel 0 is the **front-left** wheel. [decompile] [sim] [game], verified; TMNF-C and ForeverValidator agree. I found no community write-up of this effect.

| blend value | used for |
|---|---|
| `[0]` | top-speed force limit: `t[0x2c]` forward, `t[0x30]` reverse, times blend[0] |
| `[1]` | engine force multiplier (gas, and brake when reversing) |
| `[2]` | brake force cap |
| `[3]` | lateral grip in the yaw-torque pass and in the slide detection that drives the speedslide blend |

The per-wheel lateral-force pass uses each wheel's **own** surface. So engine, brake, top speed and the slide/accel blend follow the front-left wheel, side force is per wheel. Practical consequence: front-left on road with the other three wheels on grass keeps road values for engine and top speed; front-left on grass slows the whole car. Verified in code; the in-game effect on mixed surfaces was not separately tested (inferred).

For Model4 cars the lateral force of both axles uses the car-wide blend, so the front-left wheel decides side grip of all four wheels. [decompile] ComputeForcesModel4 0x7fb5f0, unverified in game (see [candidates](candidates.md)).

### Bump contacts (fake contacts)

`CreateFakeContacts` (0x7c3c00) adds bump contacts from a per-material height bitmap: depth = pixel/255 * `v.z` * scale, capped. The loop visits wheels in order, but for the first touching wheel whose material has **no** bump bitmap it returns (`JZ 0x7c3e69` at 0x7c3c83) instead of continuing. Later wheels then get no bumps that tick. ForeverValidator has a second early return when the bitmap is not in system memory and cannot be regenerated. The depth uses the signed `v.z`, so reversing gives a negative impact. [decompile] [sim], verified (both ports agree). Relevance to dirt landing bugs: inferred.

## Normal driving branch (not in state 1/2)

Short list of what matters for the bugs; values for the Stadium car were read from the game [game]:

| item | value |
|---|---|
| acceleration curve `t[0x34]` (steps) | 16 (below 101 km/h), 11 (101-201), 7 (201-401), 5.5 (401-801), 1 (above 801) |
| slipping acceleration `t[0x1e0]*t[0x1e4]` | constant 10 |
| max side friction `t[0xac]` (linear between keys) | 80 (0-100 km/h), 75 (200), 67 (300), 60 (400), 55 (500) |
| lateral demand factor `t[0xa4]` | 40 (demand = 20 * v_lat per axle) |
| dirt branch (all four wheels on Dirt, `v.z` > 6 m/s) | brake adds drag `-0.1*v`; gas without brake adds forces at sliding front/rear wheels scaled by `|v_lat dir|*20+1`, gas and `1/(|v.z|+1)^2`; applied once per contacting wheel |
| burnout end (state 3) | forces `is_sliding = 1` on all wheels for `t[0x2a8]` ticks |

Speedslide blend [decompile] [sim], verified: per wheel `d = -t[0xa4]*0.5*v_lat_axle`, cap `c = GetMaxSideFrictionFromSpeed(v.z) * blend[3]`; `s = clamp(((sum|d| - sum c)/sum c) / t[0x200], 0, 1)`; engine accel = `s*Slipping(v.z) + (1-s)*Accel(v.z)`. Only applied if the car slid this tick (`tick == last_sliding_tick`, total cap > 1e-5); gear-shift mode zeroes the blended part; reverse uses the rear-gear curve. See [speedslide in the catalogue](bugs/README.md#speedslide-community-technique-explained).

## Burnout and donut state machine (`car+0x69c`)

Evaluated in every `ComputeForces` call. Names are mine; TMNF-C calls it `drive_mode`, ForeverValidator `burnoutPhase`. All verified [decompile] [sim] unless marked.

| state | name | entry |
|---|---|---|
| 0 | normal | default |
| 1 | burnout (timed spin) | gas > 0.1, brake > 0.1, `v.z < t[0x258]` (3.0 m/s) and upright (`R[1][1] > 0.75`); or reversing with enough force; **every time state 2 ends** (fresh timer) |
| 2 | donut (circular drift) | gas > 0.1, brake > 0.1, `t[0x258] < v.z < t[0x254]` (3.0 to 20.83 m/s = 10.8 to 75 km/h), the steering **angle** > 1e-5, any wheel in contact, average contact normal of the touching wheels in car space `y >= 0.75`, entry angle inside `[-t[0x294], +t[0x290]]` (-45 deg to +100 deg) |
| 3 | burnout end | from 1 after `t[0x298]` ticks (500), or brake without gas, or no ground contact; back to 0 after `t[0x2a8]` ticks (150) |

The entry test uses only the car-local forward speed `v.z`, never the total speed. At 230 km/h total speed the car is in the window when its heading is 71-87 deg off the travel direction, at 400 km/h 79-88.5 deg (window width shrinks with speed: 150 km/h 25.9 deg, 300 km/h 12.4 deg, 1000 km/h 3.7 deg). [sim] verified.

State 2 details:

- Pivot = car position + `(BurnoutRadius(v.z) + d0) * normalize(n x v_local) * sign(steer)`, stored at `car+0x6e0`. `n` is the average contact normal, `v_local` the car-local velocity: the pivot is perpendicular to the **velocity**, not to the car side. `BurnoutRadius` goes 1 m (standstill) to 3 m (10 km/h and above) [game]. `d0` is a base distance read from the car file's water box (a quirk: ForeverValidator keeps literal `*0` terms from the original code). [decompile] [sim] verified, Stadium `d0` value unchecked.
- Forces per call: central pull `F = (v_t^2 / d) * exp((d - r0)*t[0x278] - w*t[0x27c])` towards the pivot (`t[0x278]` = 1.0, `t[0x27c]` = 0.2; `v_t` tangential speed, `d` distance to the pivot, `r0` radius at entry, `w` speed towards the pivot); only applied if `d > r0` or `d < t[0x280]` (20 m); lateral correction `t[0x264] * (-sign * LatSpeed(r) - v_t)` (`t[0x264]` = 2, `LatSpeed` = 30 m/s); yaw/roll torques from `t[0x268..0x274]` and the donut/burnout rollover curves.
- Skipped in state 2: the per-wheel lateral friction, Model6's own top-speed force and the speedslide blend. The 1000 km/h clamp still runs.
- Abort conditions: gas or brake below 0.1, steering angle ~0, steer sign flipped, `|v_t| > t[0x284]` (20.83 m/s; tangential, not radial), body contact flag `car+0x5d8` or steep-wheel-contact flag `car+0x5dc` from the previous call, no ground, water, turbo/freewheel flag `car+0x60c`, contact normal drifting more than `t[0x28c]` (0.35 rad), angle outside the window. A donut that ends goes to **state 1** with a fresh timer, not to 0.
- **The pull of the aborting call is still applied** when the abort comes from the speed test (the code writes the new state and falls through at 0x7c42ef). This is why a donut entered at 300 km/h still delivers a huge sideways kick.
- Entry tick: state 2 is set at the end of the contact-wheel pass; the normal model still runs in that call, the skip starts with the next call.
- In state 1, per-wheel lateral force is scaled by `1 - 0.99*cos(pi*dt/1000 ms)`, i.e. 1 % grip right after a (re)entry, rising to 100 % over 500 ms (code 0x7c4dff-0x7c4e5e). verified [decompile]; the grip-less glide it should cause is untested in game.

## Contact handling

| fact | source | status |
|---|---|---|
| `AbsorbContact` 0x7c3410: contacts with Water (13) or GolfBall (23) are ignored completely (no impulse, no push-out) | [decompile] | verified |
| a contact belongs to a wheel if it hits a wheel surface tree and the normal y > 0.2; then `WheelAbsorbContact` handles it, no rigid body impulse | [decompile] | verified |
| Model6 body contact with normal y < -0.75 (car on its roof): replacement pushed onto the normal only | [decompile] | verified; effect on roof sliding inferred |
| body contact with `t[0x350] == 2` and approach speed: own impulse via `SDynaMath::ComputeImpulse`; restitution/friction `t[0x178]/t[0x174]` for Metal, `t[0x17c]/t[0x170]` otherwise | [decompile] | verified |
| `WheelAbsorbContact` 0x7c11d0: wheel "stands" if `|n.x| < sin 45 deg = 0.707` in car space (constant at 0xb5b8d8); then suspension; otherwise flag `car+0x5dc`, surface id not updated, bouncy impulse (restitution 0.5) at the wheel | [decompile] | verified |
| ground-contact branch of `WheelAbsorbContact`: for every contact point of a wheel still moving into the surface, an impulse with restitution 0.8 (`t[0x184]` = -0.8) is applied at once | [decompile] | verified; see [bluebug](bugs/bluebug.md) |
| `AddVehicleImpulse` 0x7be390: an impulse raising `|v|^2` by more than `t[0x150]` = 10000 sets the linear velocity to 0 | [decompile] | verified in code; never fired in 22 replays |
| turbo pads: boost scale = impulse scale * pad Z axis in car space, from the first wheel touching the pad; direct pads: last contact wins | ForeverValidator source review | unverified in game |
| freewheel flag is cleared only by a new checkpoint, a counted finish or a respawn | ForeverValidator source review | unverified in game |

## Tuning table of the Stadium car (read from the running game, TMNF)

The car holds 30 tunings (old versions are kept for replays); the active one is the last (index 29). [game] verified.

| field | value | meaning |
|---|---|---|
| `t[0x2c]` / `t[0x30]` | 277.78 / 138.61 m/s | top speed forward / reverse (times blend[0]) |
| `t[0x254]` / `t[0x258]` | 20.83 / 3.0 m/s | donut entry window on car-local `v.z` |
| `t[0x25c]` | 1 m (0) - 2 (5) - 3 m (10 km/h+) | burnout radius curve |
| `t[0x260]` | 30 (for r >= 1/3.6) | lateral target speed in the donut |
| `t[0x264]` | 2 | donut lateral force scale |
| `t[0x278]` / `t[0x27c]` | 1.0 / 0.2 | donut exponent factors |
| `t[0x280]` / `t[0x284]` | 20 / 20.83 | donut radius limit / tangential speed limit |
| `t[0x290]` / `t[0x294]` | 1.745 / 0.785 rad | donut angle window |
| `t[0x298]` / `t[0x2a8]` | 500 / 150 ticks | burnout duration / burnout-end duration |

Wheels, car-local [game]: wheel 0 (+0.863, 0.352, +1.782) steer, wheel 1 (-0.863, ., +1.782) steer, wheel 2 (-0.885, ., -1.206), wheel 3 (+0.885, ., -1.206); order front-left, front-right, rear-right, rear-left (also ForeverValidator `vehicle_wheel_surface_id.h:17-20`). Hard-coded constants: steer epsilon 1e-5, upright/normal 0.75, pedal gate 0.1, vector epsilon 1e-10, dirt slide speed gate 6 m/s, reverse-gear detection `|v.x| < 2` and `|v.z| < 2`.

The full byte-offset to field-name table of the Model6 tuning object is in TMNF-C `src/vehicle_model6.h` (https://github.com/adonis-singh/TMNF-C); the names there match the offsets used on this page.

## What the numbers explain

- **Speedslide thresholds.** Sliding blends the engine force towards the constant 10; normal acceleration is 11 below 201 km/h, 7 from 201, 5.5 from 401. Sliding pays above 201 km/h (+3) and more above 401 km/h (+4.5). On road (grip 1) a slide needs `20*v_lat` above the side-friction cap (60-75), which costs lateral drag, so only the +4.5 above 401 km/h is enough. On dirt/grass (grip 0.25/0.15) a tiny angle already slides, so it works from 201 km/h. Matches the community thresholds 201 (grass/dirt) / 401 (road). [game]+[decompile], verified.
- **Donut window and bug heading.** See the window table above; this is the "land at about 90 deg" (uberbug) and "on the nose" (nosebug) condition.
- **Call count.** The effects that run per call (donut pull, impulses, state changes) get one more application per tick at 108/216/324/432 km/h; spin lowers the thresholds. Whether this is the cause of the minimum speeds of the bugs is inferred, not proven.
