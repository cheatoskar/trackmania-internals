# Uberbug

Back to the [catalogue](README.md). Needs [bugslide](bugslide.md) (donut state) and [model6.md](../model6.md).

Status: verified (reproduced in simulation on exactly matching replays, decompile), inferred, unverified. Tags: [sim], [decompile], [community].

## What the community says

A sudden large speed gain on a collision while bugsliding. The trackmania.wiki says it can occur on a collision during a bugslide, that the minimum speed "has yet to be tested", lists the lowest TAS set-up speeds as 201 (A06-Obstacle) and 216 (D07-Race), unit not stated, and calls the mechanism "still unknown" ([community] https://www.trackmania.wiki/wiki/Uberbug). The Miraheze list asks for a speed above 243 "units", a nose-down landing of roughly -10 to -30 deg and a grass slide towards a wall (https://tmnf.miraheze.org/wiki/UberBug). TMNF-C reproduces an A12-Speed uberbug (10.100 s, 1031 ticks) with the speed jumping 312.6 to 485.8 km/h between two consecutive ticks ([community] https://github.com/adonis-singh/TMNF-C, `docs/VALIDATION.md`): +173 km/h in one tick cannot be friction.

## Mechanism (verified in simulation)

The uberbug is a bugslide (donut state) disturbed by a collision. A border hit rotates the car's velocity so that it moves **away from the pivot** while the state is active. The `-0.2*w` term in the donut exponent then multiplies the pull (`w` negative when moving away). In my measured cases 97 % of the exponent came from the radial speed and only 3 % from the radius error. The pull is applied in the same call in which the state ends. verified [sim][decompile].

Per-call measurements of one reproduced TAS uber (tick 8910, gas+brake+steer held, car sliding sideways at 384 km/h):

| substep | what happens | speed |
|---|---|---|
| 0 | state 1, forward speed 24 m/s (outside the donut window) | 384 km/h |
| 1 | front-left wheel hits a Rubber border (surface 9); 2 wheel impulses; forward speed drops to 8 m/s: **donut entered** | 336 km/h |
| 2 | 6 more wheel impulses turn the velocity: the car now moves away from the pivot | 281 km/h |
| 3 | radius 5.42 m (target 5.27), `v_t` = 67.9 m/s, `v_r` = -24.4 m/s, exponent (5.42-5.27) + 0.2*24.4 = 5.03, factor 154, pull = 67.9^2/5.42 * 154 = 130,600 m/s^2 | 78 to 311 m/s in one 2.5 ms substep; 962 km/h at the next call |

### Geometry

At entry the pivot lay at car-local (-0.49, 1.52, +5.42) m: about 5.4 m **in front of the nose** (pivot direction = contact normal x velocity x steer sign; sliding sideways to the left, "up x left" points along the car's forward axis). The front-left wheel, the leading corner, hit the border and its impulses pushed the car **backwards**: forward speed +8 to -17 m/s, i.e. 24 m/s away from the pivot.

### Same pattern in more cases [sim]

| case | forward speed before to after the border hit | `v_r` (m/s) | exponent | factor | after integration |
|---|---|---|---|---|---|
| TAS uber, tick 8910 | +8.0 to -17.0 m/s | -24.4 | 5.03 | 154 | 311 m/s, clamped to 1000 km/h |
| C02 TAS, tick 28790 | +20.0 to -10.6 m/s | -24.6 | 5.05 | 156 | 392 m/s (212 to 1000 km/h in one call) |
| E02 TAS, tick 10010 | to -3.4 m/s | -20.4 | 4.23 | 69 | 276 to 696 km/h |
| search result on a community map | +17.6 to -16.3 m/s | | | | pull 141,800 m/s^2, clamp at 1000 km/h |

In all four the leading front wheel hits a border or kerb (Rubber/Asphalt/Grass), the car is pushed backwards along its own axis, away from the pivot ahead of the nose, and the pull fires in the call that ends the donut. The fourth case shows the car does not need a long wall ride: it arrived tumbling sideways out of a nose landing and a single front wheel caught a 0.55-0.7 m kerb from the grass side (contact normal in car space (-0.58, 0.22, -0.78), almost horizontal and pointing backwards). verified [sim] (reproduced with the reference backend and traced per call).

## Conditions from the code

| condition | value | status |
|---|---|---|
| inputs | gas + brake + steer (same sign as at entry) | verified [decompile] |
| forward speed of the car | 3.0 < `v.z` < 20.83 m/s, so the car travels nearly sideways (at least 71 deg off at 230 km/h) | verified |
| wheels | at least one wheel on the ground, average contact normal "up" in car space | verified |
| stays active | contact normal within 0.35 rad of entry; `d > r0` or `d < 20 m` | verified |
| the collision | must be a **wheel ground contact**: `\|n.x\| < 0.707` in car space | verified [decompile] |
| no abort flags | no body contact (`car+0x5d8`) and no steep wheel contact (`car+0x5dc`) in the previous call; flags are cleared at the end of `ComputeForces` (0x7c7895 / 0x7c78ad) | verified |

Three collision cases in `AbsorbContact` (0x7c3410):

1. body contact (not on a wheel, or normal y <= 0.2): sets `+0x5d8`, the next call aborts the donut **before** the pull; no uber.
2. wheel contact tilted > 45 deg sideways (`WheelAbsorbContact` 0x7c11d0): sets `+0x5dc`, bouncy impulse (restitution 0.5), also aborts; a rammstein-like hop instead ([rammstein](rammstein.md)).
3. wheel contact tilted < 45 deg sideways: counts as ground; no flag; the suspension push-out moves the car along the border normal; at 230+ km/h into a raised face the push-out is large. The uber happens here.

## Derived rules (inferred, from the numbers above)

1. Slide sideways with gas + brake + steer so that the car's own forward speed falls into 10.8-75 km/h; the pivot is then a few metres in front of the nose (behind it with the opposite steer sign).
2. The **front corner on the sliding side** must hit a border/raised edge so that the hit pushes the car backwards along its own axis. A hit that pushes sideways or forwards gives no uber: only the speed component away from the pivot enters the exponent (factor `e^(0.2*v_away)`: 10 m/s gives 7x, 20 m/s 55x, 24 m/s 121x).
3. The hit must count as a wheel contact with a mostly upward normal in car space (a slight tilt so the wheel meets the top of the border); a steep side hit ends the donut first.
4. Keep the steer direction through the hit (changing sign ends the donut).

The kick scales with the square of the speed around the pivot, so a faster slide gives a much bigger uber, but the heading window narrows ([bugslide](bugslide.md)).

## Minimum speed

The community's lowest TAS set-ups (201 and 216 km/h) fit two code facts (inferred): the pull scales with `v_t^2`, so below about 200 km/h the kick is too small to beat the collision loss; and the call count without spin steps from 2 to 3 at **216 km/h**, giving the pull another chance after the collision inside the same tick. Not proven.

## After the uber: gas in the air keeps the speed

This is [coast drag](coast-drag.md): without gas the car brakes by `1.0 + 0.30*speed` in flight too. After a 909 km/h uber on a community map, replacing all inputs for 1.9 s of flight by one held input gave 885 / 861 / 839 km/h at 0.5 / 1.0 / 1.5 s with gas against 757 / 630 / 525 km/h with nothing, with brake only, or with steer only (steer only changes the pitch). verified [sim].

## How to verify

Log `burnout_state` on an uber replay: state 2 on the tick before the jump, `has_any_lateral_contact` = 1, `v.z` in 3-20.8 m/s, then the speed jump. Control test: the same without brake (no state 2) gives no uber. The exact `d - r0` needs a per-call trace.
