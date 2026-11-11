# Car and vehicle state layout (TMNF/TMUF)

Part of the [physics overview](tmnf-physics.md); force model in [model6.md](model6.md).

**How to read offsets.** Exe: TMNF `TmForever.exe`, build key `4d431494`. Function addresses elsewhere in these pages are **virtual addresses (VA)**, image base 0x400000 (RVA = VA - 0x400000). Offsets on this page are **byte offsets inside an object or buffer**, never addresses. Two families:

- **TMInterface buffer offsets** (decimal, from the `tminterface` Python package 1.0.2 `structs.py`/`constants.py` by donadigo, https://github.com/donadigo/TMInterfaceClientPython, and the AngelScript API https://donadigo.com/tminterface/plugins/api/all). They are offsets in TMInterface's copies of the engine structs (`SimStateData`).
- **Engine object offsets** (hex, `car+0x...`, `wheel+0x...`, `t[0x...]`) seen in the decompile. Where both are known they are identical (for example `max_linear_speed` = 736 = 0x2e0, `burnout_state` = 1692 = 0x69c, wheel `has_ground_contact` 292 = 0x124). For the region 744-760 the two sources disagree (see the note after the car table), so do not assume identity there.

Status words as in [model6.md](model6.md): verified (decompile or measured, re-checked), inferred, unverified.

## Save state (`SimStateData`, 8 buffers)

`version u32 @0`, `context_mode u32` (0 = simulation, 1 = run), `flags u32`, `timers[53] i32` (212 bytes), then the buffers. [community] verified against the package.

| buffer | size (bytes) | flag bit |
|---|---|---|
| timers | 212 | 0x01 (`timers[1]` is the simulation time) |
| dyna (`CHmsDyna`) | 1424 | 0x02 |
| scene_mobil (`CSceneVehicleCar`) | 2168 | 0x04 |
| simulation_wheels (4 x 764) | 3056 | 0x08 |
| plug_solid | 68 | 0x10 |
| cmd_buffer_core | 264 | 0x20 |
| input state (10 x CachedInput) | 120 | 0x40 |
| player_info | 952 | 0x80 |

## Dyna (rigid body)

Three 180-byte states inside the dyna buffer: `previous_state @268`, `current_state @448`, `temp_state @628`. Fields relative to a state: quaternion 0 (16 B), rotation 3x3 16 (36 B), position 52, **linear_speed 64** (m/s; km/h = norm * 3.6), add_linear_speed 76, angular_speed 88, force 100, torque 112, inverse_inertia_tensor 124, not_tweaked_linear_speed 164. [community] verified. The substep length `L` = 0.3 m is `params[12]` of the dyna (read from the running game).

## Car (`CSceneVehicleCar`, offsets in the 2168-byte buffer = engine offsets)

| offset (dec / hex) | field | note | status |
|---|---|---|---|
| 80 / 0x50, 84 / 0x54, 88 / 0x58 | input gas, brake, steer (float) | set by `CSceneVehicle::VehicleInput*Set`; the model's own steer value used for the angle is a processed copy (`car+0x5e8`) | verified |
| 736 / 0x2e0 | max_linear_speed | 277.78 m/s, see [model6.md](model6.md) | verified |
| 740 | gearbox_state | | community |
| 744 | block_flags (TMInterface name) | engine side reads car flags at `car+0x2f4` (756) and a wheel table at `car+0x2e8` (744); the two namings overlap, see the note below | unresolved |
| 0x2f4 | integration flags: bit 2 = run force block, bit 8 = zero x/z speed, bits 0x10/0x20 = block-speed (start countdown) | | verified |
| 0x354 (tuning) | handling model id (3 Model4, 4 Model5, 5 Model6) | tuning object, not car | verified |
| 748 / 916 / 1084 / 1252 | prev_sync / sync / async / prev_async `SceneVehicleCarState` (168 B each) | | community |
| 1436 | engine (48 B): max_rpm +0, braking_factor +20, clamped_rpm +24, actual_rpm +28, slide_factor +32, rear_gear +40, gear +44 | | community |
| 1500 | has_any_lateral_contact (bool) | used by community scripts as "hit a wall" | community |
| 1508 | water_forces_applied | | community |
| 1524 | turbo_boost_factor | also turbo_type 1536, last_turbo_time 1532, roulette_value 1544 | community |
| 1548 | is_freewheeling | | community |
| 1576 | is_sliding (vehicle-level, separate from the wheel flag) | | community |
| 1660 | wheel_contact_absorb_counter | | community |
| **1692 / 0x69c** | **burnout_state** (0 normal, 1 burnout, 2 donut, 3 burnout end) | confirmed by the decompile and by an independent telemetry DLL that reads it at this offset | verified |
| 0x6a4 | iso (orientation) saved for the donut | | verified |
| 0x6e0 | donut pivot (vec3); radius base 0x6ec, target radius 0x6f0, direction (steer sign) 0x708, car-local speed 0x70c, timers 0x6f4 / 0x6f8 | | verified |
| 0x5d8 / 0x5dc | flags: body contact / steep wheel contact in the previous call (both abort the donut) | cleared at the end of `ComputeForces` | verified |
| 0x5e4 | air-control lock and airborne friction gate (one field, two roles; water contact sets it) | | verified (port source comment) |
| 0x60c | flag that aborts the donut and disables the dirt gas branch | | verified |
| 0x68 / 0x6c / 0x70 | material manager; ground-id table; steer constants (`t[0x6c]`, `t[0x70]` are tuning values) | | verified (partly) |
| 1804 | current_local_speed (vec3) | car-local velocity | community |
| 2072 | total_central_force_added (vec3) | | community |
| 2116 | is_rubber_ball | | community |

Note on 744-760 (unresolved, [contradictions.md](../contradictions.md) row 25): TMInterface puts `block_flags` at 744 and the first `SceneVehicleCarState` at 748; the decompile reads the integration flags at `car+0x2f4` (756) and a telemetry DLL for the same engine reads a wheel array from `car+0x2e8`. The offsets 0x50-0x58, 0x2e0 and 0x69c agree in both. Treat 0x2e8-0x2f4 as engine-side only and 744-760 as TMInterface-side only until checked in the debugger.

`SceneVehicleCarState` (relative): 0 speed_forward, 4 speed_sideward, 8 input_steer, 12 input_gas, 16 input_brake, 20 is_turbo, 128 rpm, 136 gearbox_state. [community]

## Wheel (`SimulationWheel`, 764 bytes each)

| offset | field | note |
|---|---|---|
| 4 | steerable (bool) | |
| 12 | surface_handler (100 B) | rotation 3x3 at +52, position at +88 |
| 168 | offset_from_vehicle (vec3) | wheel position relative to the car |
| 180 | real_time_state (168 B) | below |
| 352 | contact_relative_local_distance (vec3) | |
| 364 / 464 / 564 / 664 | prev_sync / sync / unnamed / async wheel states | opaque 100-byte blocks in the package |

`RealTimeState` (relative to wheel offset 180; absolute in brackets): damper_absorb 0 [180]; **has_ground_contact 112 [292 = 0x124]**; **contact_material_id 116 [296 = 0x128]**; **is_sliding 120 [300 = 0x12c]**; relative_rotz_axis 124 [304]; nb_ground_contacts 140 [320]; normal accumulator for the donut check at wheel+0x144 (engine side). Wheel order in index 0..3: front-left, front-right, rear-right, rear-left. [community] + [decompile], verified. The package exposes no "slipping coefficient"; it is not in any named field (unverified whether it hides in the opaque blocks).

The surface id enum is not defined in `tminterface` 1.0.2; the names are in the game exe, see [model6.md](model6.md#surfaces).

## Player info (952 bytes)

Offsets from the package declaration order: team 576, prev_race_time 680, race_start_time 684, race_time 688, race_best_time 692, lap_start_time 696, lap_time 700, lap_best_time 704, nb_completed 712, cur_checkpoint 728, race_state 788, cur_lap_cp_count 816, cur_cp_count 820, cur_lap 824, race_finished 828, display_speed 832, countdown_time 916. [community] inferred (computed from declaration order, not read in game).

## Turbo, gears, rpm

`is_turbo` per car-state at +20, `turbo_boost_factor` 1524, `turbo_type` 1536; `engine.gear` 1480 (1436+44), `engine.rear_gear` 1476, `engine.actual_rpm` 1464, `engine.clamped_rpm` 1460, `engine.max_rpm` 1436, `engine.slide_factor` 1468, `engine.braking_factor` 1456; car-state copies hold `rpm` at 128 and `gearbox_state` at 136. [community] verified against the package.

## Using the layout for bug analysis

A TMInterface plugin that logs, per tick, `burnout_state`, the car-local velocity, the four wheels' `has_ground_contact`/`contact_material_id` and the pitch is enough to recognise every bugslide-family mechanic in a replay: [bugslide](bugs/bugslide.md) shows state 2 alternating with 1 while gas+brake+steer are held and `v.z` inside 3.0-20.8 m/s at each switch to 2. Which community scripts use these fields (nose-position bruteforce, air-time scripts that read `has_any_lateral_contact`) is listed in [community scoring](bugs/README.md#community-tooling).
