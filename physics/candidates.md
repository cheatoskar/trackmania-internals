# Candidate bugs (unverified)

Back to the [catalogue](bugs/README.md). These come from reading the code or the public ports; **none of them has been confirmed in the running game** unless a line says so. Treat every entry as a lead. Background: [model6.md](model6.md), [bugslide](bugs/bugslide.md). Addresses are TMNF VAs (build key `4d431494`); ForeverValidator and TMNF-C file:line references are to the public MIT repos ([ForeverValidator](https://github.com/Skycrafter-dev/ForeverValidator), [TMNF-C](https://github.com/adonis-singh/TMNF-C)).

| id | candidate | basis | status |
|---|---|---|---|
| C1 | radial fling without a wall: the donut pull explodes when the car moves away from its pivot | mechanism confirmed for collisions ([uberbug](bugs/uberbug.md)); the collision-free version is open | mechanism verified [sim]; collision-free version unverified |
| C2 | "reverse ice": gas + brake while moving backwards keeps burnout state 1 | code | unverified |
| C3 | burnout glide: releasing steer out of a bugslide gives about 0.5 s of near grip-less glide | code | unverified |
| C4 | dirt bumps switched off by one wheel (front-left on road) | code, both ports | code verified, effect on landing bug unverified |
| C5 | speed-dependent call count (108/216/324/432 km/h) as cause of minimum speeds | measured call counts | counts verified [sim], causal role inferred |
| C6 | front-left wheel decides engine, brake, top speed | code + game | verified (see [model6.md](model6.md)); listed for completeness |
| C7 | solids tagged Water or GolfBall are passable for the car body | code | unverified |
| C8 | Model4 cars: front-left wheel decides all lateral grip | code | unverified |
| C9 | "uber clip": the substep count is fixed before the kick | code + measured travel per substep | unverified |
| C10 | turbo pads: crossing angle, last contact wins, reversing | ForeverValidator review | unverified |
| C11 | freewheel latch cleared only by a new checkpoint, finish or respawn | ForeverValidator review | unverified |

## C1: radial fling

The pull is `v_t^2/d * exp((d - r0) - 0.2*w)` (0x7c4372-0x7c439e). Only the tangential speed is limited (75 km/h); the speed **away** from the pivot is not and enters the exponent: moving away at 100 m/s would give `e^20`, about 5e8. The pull is applied even in the call that aborts the state. A set-up that keeps state 2 while the car moves away from its pivot, without a collision, should give an uberbug-sized kick on flat ground. Ways to get radial speed at entry: entering the donut while a bump or kicker pushes the car outward, or at a landing on a slope with the contact normal within 0.35 rad but the car sliding outward. Test: a per-call trace shows `d - r0` and `w` directly.

## C2: reverse ice

State 1 is entered every call while gas > 0.1, brake > 0.1, `v.z < 3 m/s` and the car is upright. Moving **backwards** at any speed satisfies `v.z < 3`. Each entry resets the burnout timer and the per-wheel lateral force is scaled by `1 - 0.99*cos(pi*dt/1000 ms)`, 1 % at dt = 0. So gas + brake while rolling backwards should leave the car almost free to slide sideways. Also: a 90 deg bugslide whose heading drifts slightly backwards (`v.z` just below 3 m/s) is in state 1, not 2; the switch is exactly at 10.8 km/h forward speed. Test: reverse down a straight and hold gas+brake, compare the side drift with and without brake.

## C3: burnout glide

When a donut call ends without staying in state 2 (steer released, sign flipped, limits), the state becomes 1 with a fresh timer (0x7c64e8). In the following normal calls the lateral tyre force ramps from 1 % to 100 % over 500 ms. Releasing only steer out of a bugslide should give about 0.5 s of near grip-less glide that keeps the sideways velocity. Test: log `burnout_state` (1 for about 500 ms, then 3) and lateral speed.

## C4: dirt bumps off by one wheel

`CreateFakeContacts` returns at the first touching wheel whose surface has no bump map (confirmed in both ports). If the front-left wheel (index 0) is on road or plastic, no wheel gets dirt bumps that tick. Candidate relation to the dirt landing bug; no test yet.

## C5: call count

Call count per tick `n = trunc(0.01*(|v|+|w|)/0.3) + 1` (measured: 1 at standstill, 5-6 at 530 km/h, once 32 calls under heavy spin). Everything that works per call gets one more application at 108 / 216 / 324 / 432 km/h; spin lowers the thresholds. Candidate explanation for the minimum speeds of the bugs and for wall exploits at those speeds.

## C7: water and golf-ball surfaces

`AbsorbContact` 0x7c3410 drops every contact with surface 13 (Water) or 23 (GolfBall): no impulse, no push-out, not a wheel contact. On island-style environments a solid tagged Water is passable for the car body and only `ApplyWaterForces` acts. Worth checking custom blocks with water material for clip-throughs.

## C8: Model4 side grip

`ComputeForcesModel4` 0x7fb5f0 computes the lateral force of front and rear axle with `GetLateralFriction` 0x7bf080, both with the car-wide material values (wheel 0's surface). Unlike Model6 there is no per-wheel pass with the wheel's own surface, so side grip of all four wheels would follow the front-left wheel. Which United car uses Model4 (`t[0x354] == 3`) is read from game data and not checked here. Model4 also has its own steer state (`car+0x640`, stored angle `+0x638`), not analysed.

## C9: uber clip

The number of substeps per tick is computed once from `|v|+|w|` at the start of the tick so that one substep is about 0.3 m of travel (ForeverValidator `physics_step.cpp` lines 100-117, `mesh_collision_queries.cpp` 108-111; TMNF-C `collision.c` line 960 agrees). A kick inside the tick does not raise it. Collision detection is a discrete overlap test per substep and mesh triangles are one-sided: a body centre already behind a triangle's plane gets no contact from it.

Measured travel per substep (|v| * dt) in traced ubers: 1.39 m (a 212 to 1000 km/h uber with only 2 substeps of 5 ms), 0.67 m, 0.65 m, noseboosts at most 0.40 m; design value 0.3 m. Hypothesis: below 108 km/h one substep covers the whole tick, so a 1000 km/h uber would move up to 2.78 m unchecked, enough to pass a thin wall in one step. In the three traced ubers the car's reference point crossed no map triangle during the long steps, so the clip itself remains a hypothesis; it would need an uber against thin geometry. (The 2-2.7 m steps seen at the finish ticks are logging after the race ended, not physics.)

## C10: turbo pads

ForeverValidator `scene_vehicle_car_feedback.cpp` 809-885: the boost scale is the pad's impulse scale times the pad's Z axis in car space, taken from the first wheel (index order) touching the pad. Direct pads overwrite the scale every substep, so the **last** contact decides; roulette pads fix it at the first contact. Crossing at 90 deg gives about zero boost but keeps the turbo state (forces the forward gear). Crossing backwards would give a negative scale (guess). Test: drive over one pad at 0, 45, 90 deg and backwards and compare the gain.

## C11: freewheel latch

The no-engine flag is cleared only by a **new** checkpoint, a counted finish or a respawn; an already taken checkpoint does not clear it (ForeverValidator `trackmania_race.cpp` 231-244). Probably known in part.

## Smaller leads, probably known

Race time = tick start of the crossing (10 ms resolution); the first running tick's inputs are wiped by the start reset; checkpoints in any order, no direction test; both digital steer keys give left; contact order at corners follows the world X coordinate; vertex contacts push out only 40-60 % of the depth.
