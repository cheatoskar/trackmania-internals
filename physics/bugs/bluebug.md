# Bluebug: stacked wheel impulses at a block seam

Back to the [catalogue](README.md). Related: [rammstein](rammstein.md) (border faces), [edgebug](edgebug.md), [model6.md](../model6.md#contact-handling).

Status: verified (decompile + simulation on an exactly reproduced replay, 235/235 states, deviation 0), inferred, unverified. Tags: [decompile] (TMNF build key `4d431494`, VAs), [sim].

## What happens [sim]

Example: a reproduced Nadeo-map A01 TAS at an internal tick 9570, 249 km/h, all four wheels on Asphalt (surface 16), gas + steer, no brake. In one collision pass of substep 1 the **rear-left wheel** (contact point car-local (0.878, 0.00, -1.126)) gets **11 point impulses in a row**, all upwards, each about 14 % smaller than the one before:

```
jy: 1.005 0.864 0.743 0.639 0.550 0.473 0.406 0.349 0.332 ...   (sum about 5.4 m/s per unit mass)
```

The wheel's contact counter shows 14 contact samples in that call, average normal (0.041, 0.982, -0.184). Result in one 3.3 ms substep: car-space vertical speed -0.25 to +5.35 m/s, pitch rate +2.23 rad/s (nose down), roll rate +3.35 rad/s; afterwards only the front-right wheel stays on the ground and the car hops. verified [sim].

## Why [decompile]

`CSceneVehicleCar::WheelAbsorbContact` 0x7c11d0, ground-contact branch: for **every** contact of a wheel whose point is still moving into the surface it computes `SDynaMath::ComputeImpulse` with restitution 0.8 (`t[0x184]` = -0.8, non-metal) and applies it immediately (`AddVehicleImpulse` 0x7be390 changes the velocity at once). At a seam the wheel touches the collision mesh of both blocks with many contact points at nearly the same place. Each contact sees the velocity left by the previous impulse, which is still "approaching" for that contact's normal, so it fires again; the impulses form a geometric series instead of one bounce. One contact gives a hop of `(1 + 0.8) * v_in`; `n` stacked contacts give up to about `1/(1 - 0.86)` = 7 times that. verified [decompile]; the factor 7 is my arithmetic from the measured ratio.

The kick is proportional to how fast the wheel moves **into** the seam edge (normal speed at the contact), not to the edge height. Tiny seams on platform blocks are enough. inferred from one measured case plus the mechanism.

## Repeatability [sim]

A sandbox sweep from the same state with the steering of the last 20-80 ms replaced by constant values (776 variants):

| result | share |
|---|---|
| the original rear-left stack (2.18 rad/s nose-down, 3.16 rad/s roll, same size every time) | 6 % |
| a different stack at a front wheel (1.38 rad/s nose-**up**) | 63 % |
| nothing | 30 % |
| a double rear-wheel stack | none |

So stacking at seams is common, but which wheel gets it depends on sub-centimetre position. verified [sim] for this set-up.

## Idea for a nose position (not tested)

One rear wheel stacking gives nose-down pitch **and** roll. For a clean nose-down rotation both rear wheels would have to stack in the same substep (cross the seam squarely so both touch it together; roll cancels, pitch adds). From 2.23 rad/s with one wheel, two wheels would give about 4-4.5 rad/s, about 0.35-0.4 s to 90 deg nose-down. unverified; needs a simulator sweep over angle, speed and compression.

## Also seen

The impulse speed-kill rule (`AddVehicleImpulse` 0x7be390: if one impulse raises `|v|^2` by more than `t[0x150]` = 10000, the linear velocity is set to 0) did not fire in any of 22 replays. verified in code, never observed.
