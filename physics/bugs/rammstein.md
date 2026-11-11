# Rammstein: a wheel on a raised road border or edge

Back to the [catalogue](README.md). Related: [nosebug](nosebug.md) (ramm to nose position), [uberbug](uberbug.md), [bluebug](bluebug.md).

Status: verified (decompile; example reproduced in simulation), inferred, unverified. Tags: [decompile] (TMNF build key `4d431494`, VAs), [sim], [community].

## What the community says

Road blocks have a slightly raised border (usually black or light blue). A wheel travelling on it collides and the wheel is lifted. Usually harmful; landing on it often causes a landing bug. An exception is a campaign A03-Race record that uses it to lift a front wheel before a jump so the nose dips on landing ([community] https://www.trackmania.wiki/wiki/Rammstein, https://www.trackmania.wiki/wiki/Surfaces). The wiki says nothing about seams between two road blocks. The Miraheze "RandsteinBug" entry is a separate joke-like entry.

## The wheel contact test [decompile]

`CSceneVehicleCar::WheelAbsorbContact` 0x7c11d0 decides per contact whether a wheel "stands" on it:

```
ground_contact = |n.x| < sin(45 deg) = 0.707     (n = contact normal in car space; constant at 0xb5b8d8)
```

Only the **sideways** component of the normal is tested. Then:

- ground contact: the normal is added to the wheel's normal sum (used by the donut entry check), the wheel's surface id is updated (for wheel 0 this is the car's engine/grip surface), and the contact is handled as suspension, no rigid impulse;
- no ground contact (normal tilted more than 45 deg sideways): car-wide flag `+0x5dc` ("lateral wheel contact") is set, the contact point is stored, the surface id is **not** updated, and if the wheel moves into the surface a rigid impulse with restitution 0.5 (`t[0x17c]` = -0.5; Metal `t[0x178]`) is applied **at the wheel**.

verified [decompile].

## Consequences

| statement | status |
|---|---|
| A raised border's edge and the seam between two blocks can present sideways-facing faces; a wheel touching them gets no ground contact but a bouncy impulse at the wheel position, which kicks the wheel up and rolls the car: the rammstein lift | inferred from the code; the example below is a measured instance |
| Because only the sideways tilt is tested, a **front-facing** step (normal pointing backwards in car space) still counts as ground contact at any angle: the wheel "stands" on a vertical face in front of it and takes over that face's surface | inferred (code), not observed |
| `+0x5dc` set by such a contact is one of the donut abort flags, so a rammstein during a bugslide stops the donut for that call | verified [decompile] |

## Example: ramm to nose position on flat road [sim]

A reproduced Nadeo-map D-0 TAS (6.94 s, 70/70 states, deviation 0):

| phase | what happens | speed |
|---|---|---|
| start to 3.7 s | start and three boosters, flat | up to 420 km/h |
| 3.67-3.83 s | sliding into the transition from the straight to a diagonal road block: wheels lose and regain contact, gas + brake + full steer from 3.78 s | 411-422 km/h |
| 3.85 s | **ramm kick**: all four wheels leave the road; angular velocity jumps from (0, -0.1, -0.2) to (3.6, 0, 4.9) rad/s with no speed change | 414 km/h |
| 3.87-4.75 s | the car rises on **one front wheel** with gas held, brake from 4.25 s; pitch 4 to 80 deg | 415-429 km/h |
| 4.79-5.30 s | nose ride, pitch 80-82 deg, steer sign changes almost every tick | 420 to 672 km/h |
| 5.32 s | noseboost ([noseboost](noseboost.md)) | 733 to 999 km/h |

A ramm at a block transition can turn flat driving at about 420 km/h into a nose position in 0.9 s, on one front wheel, without a jump. The same speed (about 420-440 km/h) is what a booster chain gives on most maps. verified [sim] for this case.

## How to verify

Log the wheel states on a rammstein clip: on the hit tick the wheel on the border shows `has_ground_contact` = 0, its `contact_material_id` does not change, the car's angular speed jumps, and `burnout_state` drops from 2 if a bugslide was running.
