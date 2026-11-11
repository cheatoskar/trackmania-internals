# Coast drag: releasing gas brakes the car, also in the air

Back to the [catalogue](README.md). Related: [uberbug](uberbug.md) (speed after an uber), [model6.md](../model6.md).

Status: verified in simulation and in code; **not yet checked in the running game** (the simulation is exact on my replays, so I expect the same). Tags: [sim], [decompile]/source, [community].

## Summary

Without gas input the car gets a braking force of `1.0 m/s^2 + 0.30/s * speed` against its velocity. The force does not check for ground contact, so it acts in the air too. With gas held it is zero. At 360 km/h releasing gas in the air costs about 110 km/h per second; after an uber at 900 km/h about 270 km/h per second (about 26 % of the speed per second). Holding gas in the air keeps the speed; releasing it is a brake that works in flight.

## Measured [sim, reference backend]

On a community map, all inputs in a window replaced by one held input:

| window | held | speed at the end |
|---|---|---|
| normal jump, 1.3 s airborne, no contact | gas | 359 to 363 km/h |
| same | nothing | 359 to 250 km/h |
| after a 909 km/h uber, 1.5 s airborne | gas | 909 to 839 km/h |
| same | nothing or brake only | 909 to 525 km/h |

Positions agree with the speeds, not only the speedometer (571.5 vs 592.8 m travelled in z over the same jump). Steering only changes the pitch; brake alone does nothing in the air.

Per-call force trace of the no-gas variant, airborne (no ground, body or wheel contact): the vehicle force is exactly anti-parallel to the velocity and fits `|F| = 1.00 + 0.300 * v`:

| v (m/s) | \|F\| (m/s^2) |
|---|---|
| 96.74 | 30.02 |
| 82.46 | 25.74 |
| 68.87 | 21.66 |

The gas variant has `F = (0, 0, 0)` on the same ticks. verified [sim].

## Where it comes from

ForeverValidator `src/engine/scene/scene_vehicle_car_feedback.cpp`, `CSceneVehicleCar::ApplyFrictionForces` ([community] https://github.com/Skycrafter-dev/ForeverValidator; the engine function is at 0x7bed10 in TMNF):

- it runs when the gas gate (the gas input) is below 1e-5 or the car is free-wheeling;
- force = `-lowSpeedFrictionMagnitude * v/|v|` plus, unless free-wheeling, `-lowSpeedLinearDamping * v`;
- the only early return is for the slip-response / geared-drive handling models in water; the Stadium car has no ground check here.

The values come from the car tuning in the replay or pack; the trace gives 1.0 and 0.30 for the Stadium car. The airborne air drag (`linearFluidFriction`, 0 grounded and 0.1 airborne) is a separate term and does not depend on gas. The TMNF-C Model6 drive code and its airborne drag do not show this force by themselves; it is in `ApplyFrictionForces`. verified (code of a validated port), engine-side address by name only.

## Consequences

- After an uber or any fast jump, gas held in the air keeps speed; releasing it for 0.5 s at 900 km/h throws away about 150 km/h.
- Releasing gas is the only real speed control in the air (brake alone does nothing there): it lowers the speed and steepens the drop without touching the rotation. In one measured case full gas flew 13 m over a ring checkpoint, no gas hit the ring at 335 km/h.
- The force scales with speed, so the effect is small on slow jumps and large at uber speeds.

## Open

- Game check pending (expected about 840 km/h after 1.5 s of gas at the speeds above).
- Known in the community? The effect on the ground (coasting) is obvious; that it is equally strong in the air is what I did not find written down. A web search in October 2026 found only player advice to release gas in the air to fix a nose-up attitude (rotation, not speed), for example a TMNF forum thread on forum.tomshw.it.
- Which input feeds `lowSpeedGateB` (used instead of gate A when `engine.useLowSpeedGateB` is set) is not traced.
