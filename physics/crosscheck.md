# Cross-check of my Model6 notes against TMNF-C and ForeverValidator

Back to the [physics overview](tmnf-physics.md). My model: [model6.md](model6.md). Method: I read both public repos without building or running them and compared them with my own decompile notes (TMNF `TmForever.exe`, build key `4d431494`).

Credits and licences: **TMNF-C** by adonis-singh (https://github.com/adonis-singh/TMNF-C, MIT, a C/CUDA port of TMNF physics for `TmForever.exe`, abbreviated TC) and **ForeverValidator** by Skycrafter-dev (https://github.com/Skycrafter-dev/ForeverValidator, MIT, a C++ TMUF replay validator, FV). Both are cited here by file and line only; no code is copied, and statements about what the code does are my descriptions. `tmphysicsbridge` (https://github.com/Skycrafter-dev/tmphysicsbridge, TB) is a telemetry DLL from the same author as FV; I cite it only for offsets it reads.

Evidence strength: TC reproduces 7 recorded runs byte-exactly on CPU and CUDA, including an A01 noseboost (1782 ticks, 1422.3 km/h) and an A12 uberbug (1031 ticks), so the Model6 paths with wheel 0 and the donut are exercised; many single functions are still tagged "unvalidated" (no per-function trace) and its README says force models 4 and 5 are unvalidated. FV validates against the real TMUF game: 2169 Stadium replays, 0 mismatches after fixes (`docs/reference-determinism.md` lines 114, 127, 141). Not TMNF, but the same engine code path; both ports show the same quirks. Neither project documents any of these behaviours as a "bug"; they reproduce them. Verdict words: **agrees**, **disagrees** (my early notes were wrong), **new** (in the ports, missing from my early notes).

## Claim 1: the material blend uses wheel 0

| item | verdict | evidence |
|---|---|---|
| the loop tests wheel i for contact but reads wheel 0's material id | agrees (TC, FV) | TC `src/vehicle_contact.c` lines 830-831, with a comment at 812-813 stating the original routine indexes the material with wheel zero (function tagged unvalidated); FV `scene_vehicle_car_feedback.cpp` lines 217-219 |
| stale id while wheel 0 is airborne | agrees (implicit) | both read the id without a contact test on wheel 0 |
| result averaged over the contact count | agrees | TC 837-860, FV 231-238 |
| `[0]` top speed, `[1]` engine force, `[2]` brake cap, `[3]` lateral grip in the yaw-torque pass and slide detection | agrees | TC `vehicle_model6.c` line 1361 (soft force, not a clamp) and the drive and brake blocks; FV `scene_vehicle_car_legacy_models.cpp` ~1048 and ~1011, `scene_vehicle_car_handling.cpp` 641-643 |
| per-wheel lateral pass uses the wheel's own material | agrees | TC `model6_wheel_material` (`vehicle_model6.c` ~704); FV `ApplyModel6ContactWheel` (~930). The per-wheel cap additionally has a slope factor, the sliding scale `t[0xb0]`, the braking scale `t[0x244]` and a damper modulation, which my first notes lacked |
| wheel order 0/1 front, 2/3 rear; wheel 0 = front-left | agrees, settles my open item | FV `src/engine/game/vehicle_wheel_surface_id.h` lines 17-20 and `replay_vehicle_wheel_definition.h` line 34 (canonical order front-left, front-right, rear-right, rear-left) |
| no community write-up of the consequences | still true | neither repo comments on gameplay effects |

## Claim 2: the bump-contact loop exits early

| item | verdict | evidence |
|---|---|---|
| a return instead of a continue at the first touching wheel whose material has no bump bitmap | agrees | TC `src/vehicle_aux.c` 770-773 (function not tagged unvalidated); FV `scene_vehicle_car_wheels.cpp` 667-669 |
| a second whole-loop exit when the bitmap is not in system memory and cannot be regenerated | new | FV 672-674 |
| a pixel value 0 only skips that wheel | agrees | TC ~790, FV 688-690 |
| depth = pixel/255 * `v.z` * scale, capped | agrees, detail new | TC ~796-799, FV 645-650 and 694; uses the **signed** `v.z` and a one-sided cap, so reversing gives a negative "impact" |
| fake contact shape | detail | normal straight up, position at the wheel rest point, relative speed.y = -impact, peer material = the wheel's contact material (TC ~801-808, FV 698-710) |

## Claim 3: the burnout/donut state machine

State names: TC `drive_mode`, FV `burnoutPhase` (0 inactive, 1 timed spin, 2 circular drift, 3 exit fade). TB reads the state at `car+0x69c` (`tmphysicsbridge.c` line 913), confirming that offset, and also confirms the pivot at `+0x6e0`, base radius `+0x6ec`, target radius `+0x6f0`, direction `+0x708`, car-local speed `+0x70c`, iso `+0x6a4`, wheel array `+0x2e8` with stride 0x2fc, wheel normal accumulator `wheel+0x144`, and tuning `+0x278/+0x27c/+0x280/+0x284` (lines 822-888).

| item | verdict | evidence |
|---|---|---|
| state 1 entry: gas and brake above 0.1, `v.z < t[0x258]`, upright above 0.75 | agrees | TC `vehicle_model6.c` 871-880; FV `scene_vehicle_car_handling.cpp` 274-276 with gates at 262-273 |
| reverse entry to state 1 on a large force | agrees | TC ~1290-1320; FV `TryEnterReverseBurnout` 287-302 |
| state 2 entry: `t[0x258] < v.z < t[0x254]` | agrees | TC 881-887, FV 280-282. The lower bound is strict, so `v.z` exactly equal to `t[0x258]` enters neither state 1 nor 2 |
| steering test | agrees, with precision | the tested value is the steering **angle** (steer times an arcsine term), not the raw input: TC 887, FV 282 |
| contact normal y >= 0.75 | agrees | TC 948-955 (normal sum of contacting wheels, rotated into car space), FV `EnterCircularBurnout` 239-244 |
| needs ground contact | agrees | TC ~856, FV `TryEnterForwardBurnout` |
| pivot direction | **partly disagrees** | my first notes said "side direction of the car". Both ports build it as contact normal cross car-local velocity times the steer sign, normalised (TC ~918-935, FV 226-228), so the pivot is perpendicular to the **velocity**. Base distance from the car file's water box (FV `BuildBurnoutRadiusSeed` 184-195, TB ~856) |
| entry-angle gate within [-t[0x294], +t[0x290]] | new | TC ~960, FV 233-238 |
| state 2 skips the speed cap | agrees for `t[0x2c]`, **disagrees for the 1000 km/h clamp** | the model-level force cap is skipped (TC `model6_continue_donut` ~375-616, FV 825-842 and 969-973); the global clamp runs before the model dispatch (TC `vehicle_compute.c` line 470, FV `scene_vehicle_car_feedback.cpp` line 1010) and still applies |
| state 2 skips per-wheel lateral friction and the slide blend | agrees | wheels get the force-to-vehicle call and `is_sliding` = 0 (TC ~470-475, FV ~395-399) |
| the entry call still runs the full normal model | new | state 2 is set at the end of the contact-wheel pass (TC ~899), the skip starts with the next call |
| central pull `v_t^2/r * exp(...)`, applied only if `r > r0` or `r < t[0x280]` | agrees | TC 486-505, FV 379-392; the speed in the numerator is the **tangential** speed |
| abort limit `t[0x284]` | **disagrees (naming)** | it limits the tangential speed (TC 482, FV 376-378; TB calls it tangent speed max, line 887), not the radial speed |
| an aborted donut goes to state 0 | **disagrees** | both go to state 1 (timed spin) with a fresh start tick (TC 618-625, FV 831-835); only aborts inside the entry code leave state 0 |
| forces still applied on the abort call | new | abort conditions inside the force part only set the state (TC 476-497); early aborts for input, contact, water or the `+0x60c` flag return before any force |
| other abort conditions | agrees | gas or brake below 0.1, near-zero angle, steer sign change, lateral slow-down contact, body contact count, no ground material, water, `+0x60c`, normal drift above `t[0x28c]`, non-finite angle (TC 379-394 and 460-470, FV 312-322 and 339-351) |
| yaw torques `t[0x268..0x274]` and rollover curves | agrees | TC 540-616, FV ~395-440 |
| exit 1 to 3 after `t[0x298]` ticks, 3 to 0 after `t[0x2a8]` | agrees | TC 677-697 and 800-870, FV `AdvanceBurnoutPhases` 31-62; units are timer ticks (ms) |

## Claim 4: the speedslide blend

| item | verdict | evidence |
|---|---|---|
| accel = blend of slipping and normal curves by the slide factor | agrees | TC `vehicle_model6.c` 1192-1245, FV `ComputeGearedDriveForce` 621-628 and `ComputeSlipAccelerationBlend` (`handling.cpp` 544-555) |
| slide factor from the demand-to-cap excess divided by `t[0x200]`, clamped to [0, 1] | agrees | TC 1185-1195, FV 550-553 |
| extra gates | new | blend only if the car slid this tick and the total cap exceeds 1e-5 (TC 1180-1183, FV 547-549); the last-sliding tick is updated only when a wheel exceeds its car-wide cap in the torque pass; gear-shift mode zeroes the blended part (TC 1197, FV 629-632); reverse uses the rear-gear curve (TC 1205, FV 623-626) |
| the sums come from the yaw-torque pass with the car-wide `blend[3]`, without slope or damper factors | agrees | TC 1100-1135; the per-wheel pass (own material, with factors) only drives the wheel flags |
| per-sliding-wheel brake scale `t[0x240]` | detail new | TC 1252 |

## Claim 5: the 1000 km/h clamp and the substeps

| item | verdict | evidence |
|---|---|---|
| limit 277.7778 m/s | agrees, with a correction | TC `src/vec_env.h` line 176, FV `scene_vehicle_car.cpp` line 65. FV loads the limit from the car file's PhysicalParams chunk (`src/format/pack/default_vehicle_pack_archive.cpp` line 82, applied at `scene_vehicle_car.cpp` 371-375), so "no vehicle code writes it again" was wrong; TC reads it from the snapshot (`src/world.c` line 1115). Stadium measured 277.78 |
| clamp placement | agrees | TC `vehicle_compute.c` 466-470, FV `scene_vehicle_car_feedback.cpp` 1009-1010: friction forces, then the clamp, then the material values. Condition: limit squared below speed squared and above 1e-10; the clamped speed is written back |
| flag bit 2 set and bit 8 clear | agrees | TC ~435-448 |
| whole block skipped for block-speed bits 0x30 or a negative water-box x extent | agrees, one condition added | TC 424-433, FV 967-971 (the state is zeroed) |
| does `PhysicsStep2` run several substeps? | answered: yes | TC `src/physics.c` 134-183, FV `physics_step.cpp` 19-27 and 29-122. One initial full-dt pass for every dynamic body, then per body `min(1000, trunc(dt*(|v|+|w|)/substepLen) + 1)` substeps with `dt/substeps`, then a remainder step. The vehicle force callback is called from the corpus-force step (TC `physics.c` 121-124) |
| my premise "clamp once per tick" | wrong | the clamp is per `ComputeForces` call, i.e. per substep; only the last call's gain survives to the end of the tick. The reading of the code gives up to substeps+1 calls; my own per-call trace of ForeverValidator measured exactly `n` calls ([contradictions.md](../contradictions.md) row 24) |
| state side effects repeat | new | transitions, slip memory, turbo and tick timers run inside `ComputeForces`, so up to several times per tick with the same tick value |

## Claim 6: tuning numbers and the model per environment

| item | verdict | evidence |
|---|---|---|
| real Stadium curves and tuning values | absent in both repos | TC reads them from snapshots and a physics image generated from the user's own game copy (`docs/LOCAL_ASSETS.md`, assets not bundled); FV loads the vehicle tuning chunks from game files (`src/format/vehicle_tuning/*`). My numbers in [model6.md](model6.md) were read from the running game |
| FV default values | note | `replay_vehicle_tuning_definition.cpp` lines 4-108 holds constructor fallbacks (for example donut speeds 10 and 3, radius 20, tangent limit 10, burnout durations 1000 and 500, angle limit 0.35, engine speed norm 200/3.6) before the file chunks overwrite them. They are not game values |
| hard-coded constants | found | steer epsilon 1e-5, 0.75 upright and normal, 0.1 pedal gate, vector epsilon 1e-10 (TC `vehicle_model6.c` 10-16, FV `scene_vehicle_car_internal.h` 35-37); dirt slide gate 6 m/s, slow-down 0.1, ratio 1.5, scale 20 (same files, 51-56); reverse detection `|v.x| < 2` and `|v.z| < 2` (FV `handling.cpp` 156-182, TC 1007-1019) |
| model per environment | absent | model chosen per car by tuning `+0x354`: TC `vehicle_compute.c` line 483 and `vehicle_oldmodels.c` ~28-46, FV `scene_vehicle_car_feedback.cpp` 625-653 and `scene_vehicle_car_tuning_types.h` 16-20. Which car uses which value is read from game files. Indirect: TC's Stadium glitch replays exercise Model6; its README says models 4 and 5 are unvalidated and mentions seven environments |

## Leads from comments and unvalidated regions

1. TC `vehicle_contact.c` 812-813: the only place a port admits the wheel-0 oddity.
2. TC `vehicle_aux.c` 770-771: a material without a bump mask ends the whole pass.
3. TC `vehicle_model6.c` ~333-336 (`model6_add_material6_forces`): the normalisation of the dirt-slide speed vector writes only the x component at 0x007C50C7; FV mirrors it.
4. TC `vehicle_compute.c` ~460: two different slide flags with different stack arguments, easy to confuse.
5. TC `vehicle_model6.c` ~700 and `vehicle_aux.c` ~110: `car+0x5e4` is both the air-control lock and the airborne friction gate read at 0x007C3916 by `ApplyFrictionForces`: water contact disables air control and changes the friction.
6. TC `vehicle_oldmodels.c` ~43: only the model ids 0-5 exist.
7. FV `scene_vehicle_car_handling.cpp` ~375, `BuildBurnoutRadiusSeed`: literal `*0` terms are kept from the original x87 code; the donut base radius therefore depends on the car's water-box half extent z, a tuning input never meant for physics. Stadium value unchecked.
8. Dirt (id 6) slide: with all four wheels on Dirt and `v.z` > 6 m/s extra forces per slipping wheel (drag with brake; front/rear x and z forces with gas and no brake), applied once per contacting wheel, up to 4 times (TC 318-428 and 847-849, FV `scene_vehicle_car_legacy_models.cpp` 844-885). TC's condition uses a strict comparison of the lateral force scale, FV a non-strict one, so they differ at exactly 0.
9. State 3 forces `is_sliding` = 1 on all wheels each call (TC 696-700, FV `handling.cpp` 56-60).
10. The reverse-gear flag is updated only in state 0; any burnout state forces it to 0 (TC 1007-1034, FV `UpdateGearDirection` 156-182).
11. Model6 has a cross-wheel torque from the engine/body force (rollover axis `t[0x234..0x23c]`, TC 765-785, FV ~910-930).
12. Unvalidated in TC: `ApplyBurnoutDonut` in `vehicle_model6_burnout.c` (an older split copy of the donut continuation; the in-file `model6_continue_donut` is the live one), `ComputeVehicleGroundMaterialVals`, `ApplyFrictionForces`, `GetSlopeAdherence`, `IntegrateVehicle`, the curve evaluators, CHmsDyna state, most of `collision.c`; end-to-end it is still byte-exact on 7 runs. Collision response `collision_response.c` line 149 is marked as having no trace.
13. TC's README says its glitch comparisons cover two TAS runs plus two custom-map TAS, and not every glitch variation or arbitrarily long bugslides.
14. Divergences FV found against the real game (`docs/reference-determinism.md` 108-141): scene placement of clip components, one extra tick at the replay horizon, the freewheel state cleared before checkpoint acceptance; all fixed and none in the vehicle model, but they show where the order of operations mattered.

## Points where my early notes changed

Clamp is per call (not per tick); an aborted donut goes to state 1; only `t[0x2c]` is skipped in state 2 (the 1000 km/h clamp still runs); `t[0x284]` limits the tangential speed; the pivot is along `n x v_local * sign(steer)`; wheel 0 is front-left; `car+0x2e0` is loaded from the car file.

Tests this suggested (call count per tick with a breakpoint at 0x7c69e0 above 500 km/h; the state value right after a donut abort, expecting 1; all-four-wheels-on-dirt at 100 km/h while sliding; reading PhysicalParams for the limit and the water box) have not been run in the game. The call count and the abort target were later measured in simulation (see [model6.md](model6.md)).
