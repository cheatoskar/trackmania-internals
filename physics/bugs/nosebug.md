# Nosebug

Back to the [catalogue](README.md). Read [bugslide](bugslide.md) first: the nosebug is the bugslide state machine in a special car orientation. Chained nosebugs are the [noseboost](noseboost.md). Getting onto the nose: [edgebug](edgebug.md), [rammstein](rammstein.md), [bluebug](bluebug.md).

Status: verified (decompile + simulation), inferred, unverified. Tags: [decompile] (TMNF build key `4d431494`, VAs), [sim], [game], [community].

## What the community says

Car stands on its nose (front wheels down, body about 90 deg nose-up) while gas + brake + one steer direction are held; very sharp direction changes. The wiki says the car must be almost 90 deg nose-up with only the front wheels on the ground, that it does not happen below about 80 km/h (the car falls flat) and that above about 390 km/h it "counteracts ground friction with each tick of contact" ([community] https://www.trackmania.wiki/wiki/Nosebug, definition also at https://tmtas.exchange/faq). The 80 and 390 km/h numbers come from one wiki page without a cited test: unverified there, the 390 km/h figure is consistent with my measurements ([noseboost](noseboost.md)). The Miraheze "NoseBug (nb)" entry is an unrelated joke entry; ignore it.

## Mechanism

On its nose the car's forward axis points down, so almost none of the travel speed is along it. The donut entry test only looks at that forward speed (10.8-75 km/h), so the donut model switches on at **any** total speed. The pull then turns the velocity towards a pivot beside the car within one call, and explicit integration adds speed. That gives the near-instant 90 deg turns. verified [decompile][sim].

| condition | value | status |
|---|---|---|
| inputs | gas > 0.1, brake > 0.1, steering angle != 0, same direction throughout | verified [decompile] |
| car-local forward speed | 3.0 < `v.z` < 20.83 m/s | verified [decompile][game] |
| ground contact | at least one wheel; not in water; no turbo/freewheel flag | verified |
| contact normal | average normal of touching wheels in car space `y >= 0.75` | verified |
| stays active | normal within 0.35 rad of the entry normal; angle to the pivot within -45 / +100 deg | verified |

The contact-normal rule means the touching surface must be "under the wheels" from the car's point of view. On the nose that is true for the front wheels on a surface roughly perpendicular to the car's up axis: a nose-down car with its front wheels on flat ground. inferred (derived from the rule).

### Why the turn is instant

In the donut call the pull `v_t^2/d * exp(...)` points to a pivot beside the car. With `v_t` a large part of the travel speed and `d` of a few metres, `F*dt` per call is of the order of the speed itself: at 300 km/h `83^2/4` is about 1700 m/s^2, about 17 m/s per call, several calls per tick. The direction of travel swings by tens of degrees per tick. The abort at `|v_t| > 75 km/h` does not prevent the pull of that call; the state becomes 1 (1 % lateral grip) and the next call re-enters 2 if `v.z` is still inside the window. verified [decompile][sim].

## Pose: a good angle is not enough

Angle alone does not make a nose landing. Observations from exactly reproduced runs [sim]:

| case | outcome |
|---|---|
| landing on one front wheel at 83-88 deg with the pitch settled (pitch rate about 3-30 deg/s at touchdown), gas+brake held | holds the nose, speed does not drop in the first 300 ms |
| landing at 81 deg while still pitching forward at 130-150 deg/s | the nose passes vertical about 60 ms later, the body hits the ground, the car loses about 100 km/h within 0.1 s and is thrown off |
| landing at "perfect" 88-89 deg that is a **body hit** (one front wheel touches for one tick) | bounce, 453 to 373 km/h, tumble |
| nose reached by hitting the ground tail first | about -47 km/h and no boost |

A nose position is only a boost start if the pitch has settled at 83-88 deg before touchdown and the speed does not fall in the first 300 ms. A yaw/pitch/roll-only score (as in community bruteforce scripts) cannot tell these cases apart. verified [sim] (several cases), generalisation inferred.

## How a car gets onto the nose without a ramp [sim]

Reproduced set-ups on Nadeo campaign maps (map codes only, no inputs):

| set-up | speed | what happens |
|---|---|---|
| only the rear-left wheel lands on a raised road border, flat car, gas + full steer, brake off in the last 0.2 s of the flight | 411 km/h | the front-left wheel touches 0.08 s later; on that wheel the car tips forward 14 to 82 deg in 0.5 s |
| a ramm at the transition from a straight into a diagonal road block while sliding with gas+brake+full steer ([rammstein](rammstein.md)) | 414 km/h | spin kick, 0.9 s on one front wheel, pitch 4 to 80 deg |
| flat road, constant light steer, wheels lift until only the front-left wheel touches, then a 1.2 s flight in which the nose turns down | 484 km/h | lands at 78 deg, 455 km/h |
| the wheels leave a platform edge one by one, steer pulses, no brake | 422 km/h | 0.42 s air time, nose turns 0 to 63 deg, nose ride at 415 km/h |
| a slide over a road border on the rear wheels, then a low uber of 10 m height | 324 km/h | in the 0.75 s flight the nose turns 4 to 57 deg down |

The ride on the nose: with gas, brake most of the time and the steer flipping between full left and full right every few ticks, the car gains slowly (about +95 to +150 km/h per second) and kicks once it passes about 470-480 km/h. From about 390 km/h a nose ride reaches that in under a second. No boost started from below about 478 km/h in the reproduced community TAS, so a nose position at 390-480 km/h is useful. [sim] verified for these cases.

A slow "nose balance" also exists: the car stays on one front wheel at 285-295 km/h, pitch 80-88 deg, aligning its heading before it lets the rear down into a donut for an uber. It is not a boost. [sim]

## Which way the kick throws the car

In two cases on a community map the kick turned the travel direction by about 180 deg; in a Nadeo-map nose chain every kick turned it only 30-46 deg per tick (heading 95 to 263 deg over 3.3 s at 850-1000 km/h, 79-89 deg nose down, spin 6-18 rad/s around the car's long axis, gas on in 96 % of ticks, brake on in 51 %, steer sign flipping almost every tick). The turn is the instant turn around the pivot beside the car, so its size and side follow the steer sign and the time gas+brake+steer stay on together. Not a fixed rule. [sim] verified.

## Inputs needed (for real-time play) [sim]

A recorded 44-second Nadeo TAS had 23 noseboost episodes with 30-150 input events per second during the boost (mostly analog steering plus brake/gas taps). Replacing the steering by keyboard-style steering (full left / none / full right, one switch) on three of its boosts reproduced one of three (939 km/h with a switch inside a 3-tick window), only with the recorded brake/gas taps; holding gas+brake never gave a boost. The hard part is the approach (airborne at 600 km/h, 82-87 deg nose-down), not only the number of inputs. verified [sim] for these three cases.

## How to verify in TMInterface

Log `burnout_state` (2 on the turning tick), pitch near +-90 deg, only the front wheels with contact, `v.z` inside 3-20.8 m/s. Then check that speed gain only happens on ticks with state 2.
