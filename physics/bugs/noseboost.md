# Noseboost

Back to the [catalogue](README.md). Needs [nosebug](nosebug.md) (the nose position and the donut entry) and [bugslide](bugslide.md). Why speed stops at 1000 km/h: [model6.md](../model6.md#speed-clamp-the-1000-kmh-cap).

Status: verified (reproduced in simulation on exactly matching replays), inferred, unverified. Tags: [sim], [decompile], [community].

## What the community says

Several nosebugs chained quickly, each contact tick adding speed once above a threshold of about 390 km/h; found in September 2021 by TAS players; some simple ones may be possible in real time ([community] https://www.trackmania.wiki/wiki/Noseboost). The community maximum cited is 1422.3 km/h in a TMNF-C reproduction of a 17.610 s A01-Race noseboost run of 1782 ticks (TMNF-C `docs/VALIDATION.md`, https://github.com/adonis-singh/TMNF-C). Videos: https://www.youtube.com/watch?v=kJnAQuCRESY (first noseboost on a Nadeo track), https://www.youtube.com/watch?v=JnCHQvmzb-o.

## What one noseboost tick looks like [sim]

On the nose the car travels along its own **up axis** (car-local `v.y` 150-240 m/s), the forward axis points into the ground (`v.z` 3-20 m/s, inside the donut window) and only one front wheel touches. With 6-11 force calls per tick the state alternates every call:

| call | state | what happens |
|---|---|---|
| k | 1 to 2 | donut entered: pivot at `r0` = 5.27 m from the car, perpendicular to the velocity |
| k+1 | 2 to 1 | the car has moved one substep, the donut pulls, then aborts (`\|v_t\| >> 75 km/h`) |
| k+2 | 1 to 2 | entered again |

So the car gets one centripetal kick every second call. Example from a reproduced Nadeo TAS: 681, 710, 758, 873 km/h inside one tick, then 998 and the clamp in the next. verified [sim].

## Where the exponential comes from: the substep itself

At entry the pivot is exactly perpendicular to the velocity (`v_r` = 0, `d` = `r0`). One substep later the car has moved `s = |v|*dt` in a straight line, so it is slightly **farther** from the pivot and moving **away** from it:

```
d - r0 ~ s^2 / (2*r0)           measured 0.007-0.008 m (negligible)
v_r    ~ -|v_t| * s / r0        measured v_r/|v_t| = -0.047 ... -0.070 (median -0.053)
exponent = (d - r0) - 0.2*v_r ~ 0.2 * |v_t| * s / r0
```

`s` is about constant (0.25-0.37 m) because the substep count grows with speed, so the exponent grows **linearly with speed** and the pull grows like `v^2 * e^(0.01*v)`:

| `\|v_t\|` (m/s) | exponent (median) | factor e^exponent |
|---|---|---|
| 100-150 | 1.4-1.5 | about 4 |
| 150-200 | 1.7-1.8 | about 6 |
| 200-250 | 2.3-2.4 | about 10 |
| 250-300 | 2.9 | about 18 |

Gain per kick (speed after minus before one centripetal kick), 249 kicks in two reproduced Nadeo TAS:

| speed | kicks | median gain | max gain |
|---|---|---|---|
| 400-500 km/h | 22 | +2.6 km/h | +5 |
| 500-600 km/h | 79 | +7.4 km/h | +20 |
| 600-700 km/h | 54 | +24 km/h | +92 |
| 700-800 km/h | 35 | +56 km/h | +246 |
| 800-900 km/h | 22 | +108 km/h | +193 |
| 900+ km/h | 37 | clamp | |

Every 100 km/h roughly doubles the gain per kick, and the number of kicks per tick also rises with speed. That positive feedback is the noseboost: slow at 400-500 km/h, then 600 to 1000 km/h in a few ticks. No kick with the car on its nose was seen below 400 km/h in these runs; the gain per kick (a few km/h) does not beat the losses of the front-wheel contact below that. This matches the community figure of about 390 km/h. verified [sim] (numbers), threshold interpretation inferred.

Per-call speed gain, derived: `|v'|^2 - |v|^2 = (a*dt)^2` (explicit Euler, force perpendicular to velocity), and with `a` proportional to `v^2` the gain grows with about `v^4 * dt^2`.

## Why it stops at 1000 km/h

The clamp to 277.78 m/s runs at the start of every force call. After the integration of a kick the speed can be far higher for a moment (2716 km/h after integration in one tick; 45 substeps above 1000 km/h in one TAS), but position uses the speed from *before* the kick and the next call clamps first. So the car's centre never travels faster than 1000 km/h because of a kick. Speedometer values above 1000 (the 1422 km/h reproduction) are the momentary post-kick velocity. The car's reference point moved up to 1043 km/h while spinning at 28 rad/s; I think the reference point is about 0.5 m off the centre of mass. verified [sim] / inferred (reference-point reason). Whether checkpoints and the finish use that reference point is open.

## Reaching the threshold: sub-ticks and substep counts

The call count per tick steps up at 108, 216, 324, 432 km/h without spin; on the nose the car usually spins, which adds calls and moves the steps lower. 378 km/h (the 324-432 step with spin) sits just below the 390 km/h community threshold. Whether the call-count step is *the* reason for the threshold is inferred, not proven.

## Chains and approach speed [sim]

- A nose chain keeps the car at 850-1000 km/h for seconds; every 0.2-0.6 s a re-boost tops the speed up to the clamp (see the numbers on [nosebug](nosebug.md)).
- A "nose ride" from about 390 km/h to the 470-480 km/h kick point gains about +95 to +150 km/h per second.
- Searches on a community map found the boost needs gas **and** brake held at the landing and the steering swinging between full left and right; releasing the brake at the wrong time kept the car slow. Holding the nose gives almost nothing until about 0.5 s later; letting the nose fall and rolling down a slope gives speed at once but ends the nose position. verified [sim] on that map; general rule inferred.
- After the boost the car is thrown roughly against its direction on the nose; flicking up out of the nose costs 1000 to 500 km/h in 0.1 s, a bounce off a road border turned the car 42 deg for 809 to 539 km/h, which is cheaper. [sim]

## Minimum speed in the searches I ran [sim]

On one reproduced set-up the lowest start speed that still gave a boost was 450 km/h (search from 475 km/h down). No reproduced community TAS boosted from below about 478 km/h on its own; the nose ride gets there from about 390 km/h. These are search results on specific set-ups, not a rule from the code.
