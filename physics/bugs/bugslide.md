# Bugslide: the burnout/donut state machine

Back to the [catalogue](README.md). Force model: [model6.md](../model6.md). Applies to the TMNF/TMUF Stadium car (Model6). Exe: TMNF `TmForever.exe` build key `4d431494`, addresses are VAs.

Status: verified (decompile read and re-checked, or reproduced in simulation), inferred, unverified. Tags: [decompile], [sim], [game], [community].

## Summary

Holding gas + brake + one steer direction does not make the car "slide better". It switches the force model into a separate state machine (`car+0x69c`, TMInterface `SceneVehicleCar.Burnout`). The normal tyre model is replaced by a "donut" model built for low-speed spins: a pivot point beside the car and a pull towards it, with strength

`F = v_t^2 / d * exp((d - r0) - 0.2 * w)`

(`v_t` tangential speed around the pivot, `d` distance to the pivot, `r0` radius at entry, `w` speed towards the pivot). The force has no upper bound. The state is entered by looking only at the car's **forward** speed (3.0-20.83 m/s = 10.8-75 km/h), never at the total speed, so the spin model also runs at 200-1000 km/h when the car moves sideways. Forces are integrated with explicit Euler, so every sideways kick adds energy. Speed gain, sharp turns and "flings" all come from this one model. verified [decompile][sim].

The community describes the same technique as "gas, brake and a direction held; loses speed slowly at low speed, gives speed every tick above about 500 and bursts above about 750" ([community] https://www.trackmania.wiki/wiki/Bugslide; units are not stated on that page). The state machine and the numbers below are mine.

## States

| state | name | entry |
|---|---|---|
| 0 | normal | default |
| 1 | burnout | gas > 0.1, brake > 0.1, `v.z < 3.0 m/s`, upright (`R[1][1] > 0.75`); also **every time state 2 ends** |
| 2 | donut | gas > 0.1, brake > 0.1, 3.0 < `v.z` < 20.83 m/s, steering angle != 0, a wheel touches the ground, contact normal (car space) `y >= 0.75` |
| 3 | burnout end | from 1 after 500 ms, without ground contact, or brake without gas; to 0 after 150 ms |

verified [decompile] `CSceneVehicleCar::ComputeForcesModel6` 0x7c3e80 re-run in every `ComputeForces` call, [sim], tuning values [game].

## Execution window: how far the car must be turned

With `v.z = |v| * cos(theta)`, `theta` = angle between the car's heading and its direction of travel (travel in the car's ground plane). Turned too little: normal model. Turned too far (`v.z < 3`): state 1 (1 % side grip, no donut pull). verified (arithmetic on measured limits).

| total speed | heading off the travel direction | window width |
|---|---|---|
| 150 km/h | 60.0 - 85.9 deg | 25.9 deg |
| 200 km/h | 68.0 - 86.9 deg | 18.9 deg |
| 230 km/h | 71.0 - 87.3 deg | 16.3 deg |
| 300 km/h | 75.5 - 87.9 deg | 12.4 deg |
| 400 km/h | 79.2 - 88.5 deg | 9.3 deg |
| 500 km/h | 81.4 - 88.8 deg | 7.4 deg |
| 700 km/h | 83.8 - 89.1 deg | 5.3 deg |
| 1000 km/h | 85.7 - 89.4 deg | 3.7 deg |

The window narrows with speed, so the set-up is easiest at the lowest speed where the effect still pays; the middle of the window is safer than 90 deg. inferred (practical advice, not tested).

## What the donut does per call

1. Tangential speed `v_t` and distance `d` to the pivot are computed. The pivot lies along `n x v_local * sign(steer)` (perpendicular to the velocity), at `r0 = BurnoutRadius(v.z) + d0` (1-3 m plus a body distance).
2. If `|v_t| > 20.83 m/s` the state is set to the abort state, **but the code falls through** (0x7c42ef) and still applies the pull of this call.
3. If `d > r0` or `d < 20 m`: central pull as above with `t[0x278]` = 1.0, `t[0x27c]` = 0.2. Moving **away** from the pivot makes `w` negative and the factor larger (exp argument at 0x7c4372-0x7c439e).
4. Lateral correction `t[0x264] * (-sign(steer) * LatSpeed(r) - v_t)`, `t[0x264]` = 2, `LatSpeed` = 30 m/s.
5. Yaw/roll torques from `t[0x268..0x274]` and the donut/burnout rollover curves.
6. Any non-donut result at the end becomes state 1 with a fresh timer (0x7c64e8).

The per-wheel friction caps, Model6's top-speed force and the speedslide blend are skipped in state 2; the 1000 km/h clamp is not. verified [decompile][sim].

## Why it gains speed

- `CHmsDyna::IntegrateStep` is explicit Euler: `|v'|^2 = |v|^2 + (a*dt)^2` for a force perpendicular to the velocity. A huge sideways pull adds speed in every call without any engine. verified.
- The vehicle `ComputeForces` runs once per substep, `n = trunc(0.01*(|v|+|w|)/0.3) + 1` times per tick: steps at 108, 216, 324, 432 km/h without spin. More calls per tick mean more kicks per tick, so the gain grows with speed. Matches the community remark that it gives speed every tick above about 500 and bursts above about 750. verified (call count), inferred (link to the community numbers).
- `exp(d - r0)` grows if the car ends up farther from the pivot than at entry: 10 m too far is a factor of about 22,000. The "fling" or "yeet" the community describes is probably this. inferred; the collision-driven version is verified in [uberbug](uberbug.md).
- Between donut calls the car sits in state 1 where the per-wheel lateral force is scaled by `1 - 0.99*cos(pi*dt/1000 ms)`: 1 % right after a (re)entry. So the tyres barely fight the sideways motion. verified [decompile].

## The 1000 km/h clamp in this state

The clamp runs at the start of every call including in state 2, but the donut pull is added after it, so the speed at the end of a call can exceed 1000 km/h; the next call clamps again. A bugslide stays above 1000 km/h only if every call adds that much again ([noseboost](noseboost.md)). verified [decompile][sim].

## How to verify in TMInterface

1. Log `burnout_state` and the car-local `v.z` per tick on a bugslide replay: state 2 alternating with 1 while the three inputs are held, `v.z` inside 3-20.8 m/s whenever it switches to 2. ([state-layout.md](../state-layout.md))
2. Control: the same with `v.z` kept above 75 km/h (smaller angle) must stay in state 0 with no gain.
3. A per-call trace of a reconstruction (ForeverValidator build) shows `d`, `r0`, `w` directly; I used that for every number on the bug pages.
