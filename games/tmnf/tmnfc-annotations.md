# TMNF addresses annotated by TMNF-C and its tracer

TMNF-C (github.com/adonis-singh/TMNF-C, MIT) is a C reimplementation of TMNF physics, byte-exact on validation replays. Its tracer hooks the real game (TMNF 2.11.26, PE timestamp 4d431494) at the addresses below. Rows tagged UNVALIDATED in TMNF-C have no per-function golden trace (end-to-end still byte-exact). Note: the tracer expects `e9` (an existing jump) at PhysicsStep2 0x00549c90, whose clean bytes are `55 8b ec 83 e4 f8` - another tool (TMInterface) is presumably hooked there first (inferred).

Source keys (short names for own project sources, not part of this repo): `tmvs` = [TM Vibrant Shaders](https://github.com/cheatoskar/TM-Vibrant-Shaders) mod source; `tmvs-mp` = its ManiaPlanet port; `tmvs[X]` = feature branch X of it; `tmvs-notes` = its notes; `phys` = own physics RE notes; `tmnfc` = TMNF-C (third-party MIT reimplementation, github.com/adonis-singh/TMNF-C); `dz` = [DeltaZero](https://github.com/cheatoskar/DeltaZero); `xr` = TrackManiaForeverOpenXR (upstream jiink/TrackManiaForeverOpenXR; own fork [TrackManiaForeverOpenXR-TMVS](https://github.com/cheatoskar/TrackManiaForeverOpenXR-TMVS)); `tm-re` = own RE tools. Status: verified = seen in code/data or measured; inferred; unverified = prose only.

## TMNF-C oracle tracer targets (inline hook sites, TMNF 2.11.26)

| VA | name | prologue len | prologue bytes |
|---|---|---|---|
| 008e3120 | GmQuat_Normalize | 7 | 51 56 8b f1 d9 46 04 |
| 008e0c60 | GmMat3_SetTranspose | 6 | 8b 44 24 04 d9 00 |
| 008e09f0 | GmMat3_Mult | 6 | 8b 44 24 04 d9 00 |
| 0045bba0 | GmVec3_SetMultIso4 | 8 | 8b 44 24 08 8b 54 24 04 |
| 004574a0 | GmVec3_SetMultMat3 | 8 | 8b 54 24 08 8b 44 24 04 |
| 0045bd40 | GmVec3_MultTranspose | 5 | 83 ec 0c 8b 01 |
| 00533510 | CHmsDyna_IntegrateStep | 6 | 83 ec 40 53 55 56 |
| 00532d40 | CHmsDyna_CopyStateToTemp | 9 | 56 8b c1 8b b0 2c 03 00 00 |
| 005341c0 | CHmsDyna_AddLocalTorque | 7 | 83 ec 0c 8b 54 24 10 |
| 00535d50 | CHmsDyna_ComputeSynthetizedReplacement | 10 | 83 ec 1c 53 8d 99 30 03 00 00 |
| 00549c90 | CHmsZoneDynamic_PhysicsStep2 | 5 | e9 |
| 007bc8b0 | CSceneVehicleCar_UpdateTurbo | 7 | 83 b9 00 06 00 00 00 |
| 007c1060 | CSceneVehicleCarWheelRealTimeState_Integrate | 9 | 83 ec 10 d9 05 64 ef b9 00 |
| 007c0ec0 | CSceneVehicleCar_WheelUpdateSpeedFromVehicleSpeed | 5 | 56 8b 74 24 08 |
| 008f46c0 | GmSpringFloat_Integrate | 6 | d9 41 0c d8 61 08 |
| 007bd090 | SDynaMath_ComputeImpulse | 8 | 83 ec 18 56 8b 74 24 30 |
| 007bd3f0 | CSceneVehicleCar_WheelIntegrate | 6 | 83 ec 0c 53 56 57 |
| 007be310 | CSceneVehicleCar_AddVehicleCentralForce | 6 | 56 57 8b 7c 24 0c |
| 007be2c0 | CSceneVehicleCar_AddVehicleForce | 5 | 8b 44 24 08 56 |
| 007be390 | CSceneVehicleCar_AddVehicleImpulse | 9 | 83 ec 3c 53 8b d9 8b 4b 28 |
| 007bd700 | CSceneVehicleCar_EngineIntegrate | 7 | 83 ec 14 d9 44 24 18 |
| 007c11d0 | CSceneVehicleCar_WheelAbsorbContact | 7 | 83 ec 38 53 55 56 57 |
| 007c1810 | CSceneVehicleCar_WheelAddForceToVehicle | 5 | 83 ec 24 53 55 |
| 007c3410 | CSceneVehicleCar_AbsorbContact | 9 | 83 ec 30 56 57 8b 7c 24 3c |
| 007c3e80 | CSceneVehicleCar_ComputeForcesModel6 | 6 | 81 ec 30 01 00 00 |
| 007fa770 | CSceneVehicleCar_ComputeForcesModel3 | 6 | 83 ec 6c 53 55 56 |
| 007fb5f0 | CSceneVehicleCar_ComputeForcesModel4 | 6 | 81 ec 84 00 00 00 |
| 007fc170 | CSceneVehicleCar_ComputeForcesModel5 | 6 | 81 ec 90 00 00 00 |
| 00547e00 | GmCollision_Neg | 5 | d9 41 0c d9 e0 |
| 00548bf0 | CHmsZoneDynamic_SolveImpulse | 6 | 81 ec 8c 00 00 00 |
| 005497c0 | CHmsZoneDynamic_ComputeCollisionResponse | 6 | 81 ec d8 00 00 00 |
| 007b39f0 | internal_CSceneMobilAbsorbContact | 7 | 8b 44 24 04 8b 48 40 |
| 0047cba0 | internal_CTrackManiaRaceTriggerAbsorbContact | 7 | 57 8b f9 83 7f 04 00 |

(33 targets)

## TMNF-C tracer.c mid-function capture sites

| VA | tag |
|---|---|
| 0054825b | force-field |
| 005483b7 | vehicle-force |
| 007c12aa | wheel-contact-surface-iso |
| 007c12fc | wheel-contact-car-iso |
| 007c2928 | water-car-iso |
| 007c298b | water-zone-environment |
| 007c3be4 | integrate-vehicle-callback |
| 007c3cb5 | fake-contact-car-iso |
| 007c3e9b | model6-car-iso |
| 007c73fe | compute-forces-post |
| 007c97b5 | water-splash-zombie |
| 00844e1a | bitmap-mipmap-image |
| 00845861 | bitmap-regenerate-image |
| 0092494d | mw-release-child |
| 0092497d | mw-release-self |
| 0093e5d2 | command-buffer-node-iso |

## TMNF-C src comments naming TMNF addresses (function annotated at that VA)

| VA | description (as commented) | TMNF-C file |
|---|---|---|
| 00457540 | Uses the float sign bit directly, including signed zero/NaN. | gm.c |
| 004575b0 | The clamp precedes the 0x009C1D90 __CIasin dispatcher. | gm.c |
| 00472700 | SmallRespawn restarts the race instead of respawning. | race.h |
| 0047bf00 | CTrackManiaRace::RespawnPlayerVehicle, reached from the Enter | vehicle_respawn.h |
| 0047c0d0 | RespawnPlayer -> 0x0047BF00 RespawnPlayerVehicle), or NULL | race.h |
| 0047c330 | OnCheckpoint: a block whose info has +0x120 set keeps | race.c |
| 0047cba0 | CTrackManiaRaceTriggerAbsorbContact::AbsorbContact). The | race.h |
| 0047dcd0 | OnInputEvent runs before the tick's | vec_env.c |
| 0047e400 | InternalOnCheckpoint) | race.c |
| 00480820 | OnFinishLine -> 0x0047E400 InternalOnCheckpoint | race.c |
| 004831f0 | ResetPlayer clears the | race.c |
| 004fe500 | Maps timestamped digital/analog race inputs to car controls. | vehicle.c |
| 004ff950 | GmMap2<unsigned char>::IsInside and 0x004FFAC0 GetValue share | vehicle_aux.c |
| 005334e0 | live.pos += d | hms_dyna.c |
| 00533510 | one integration substep: out = integrate(in, dt) | hms_dyna.c |
| 005338f0 | CHmsDyna::SetLocation: state B first, then the live copy. | vehicle_respawn.c |
| 00533a70 | Adds a world-space force and its moment about the world-space | hms_dyna.c |
| 00533b80 | UNVALIDATED. | hms_dyna.c |
| 00533be0 | UNVALIDATED. | hms_dyna.c |
| 00533c10 | impulse J at world point `at`: linear + angular | hms_dyna.c |
| 00533d50 | central linear impulse: linVel += J/mass | hms_dyna.c |
| 00533dd0 | velocity at world point `at` = linVel + angVel x (at - worldCOM) | hms_dyna.c |
| 00533f60 | UNVALIDATED. | hms_dyna.c |
| 00534030 | UNVALIDATED. | hms_dyna.c |
| 005340b0 | UNVALIDATED. | hms_dyna.c |
| 00534120 | UNVALIDATED. | hms_dyna.c |
| 00534140 | UNVALIDATED. | hms_dyna.c |
| 005341c0 | UNVALIDATED. | hms_dyna.c |
| 00534300 | UNVALIDATED. | hms_dyna.c |
| 00534370 | UNVALIDATED. | hms_dyna.c |
| 005343e0 | UNVALIDATED. | hms_dyna.c |
| 00534400 | UNVALIDATED. | hms_dyna.c |
| 00534470 | UNVALIDATED. | hms_dyna.c |
| 00534cd0 | CHmsDyna::CHmsStateDyna::Reset: velocities, accumulators and | vehicle_respawn.c |
| 00535cb0 | CHmsDyna::Reset. | vehicle_respawn.c |
| 00535d00 | integrate live state one step, then clear the replacement buffer. | hms_dyna.c |
| 00535d50 | reduce accumulated replacement vectors into one clamped vector. | hms_dyna.c |
| 00535fc0 | append a penetration-correction vector and mark dirty. | hms_dyna.c |
| 00536400 | apply the synthesized replacement to the live position. | hms_dyna.c |
| 00536da0 | Returns the indexed 0x4c-byte collision record. | fastbuffer.c |
| 00537150 | Computes surface collision and remaps material indices. | collision.c |
| 00537370 | Grows capacity with the game's 1.5x policy. | fastbuffer.c |
| 00537400 | Appends one uninitialized collision record. | fastbuffer.c |
| 00537530 | Tests overlap of two center/half-extent AABBs. | collision.h |
| 00537e80 | Computes squared speeds for one non-static group. | collision.c |
| 00537f30 | Fills pairwise collision-enable tables for one group. | collision.c |
| 00538090 | Initializes a collision buffer with capacity 50. | collision.c |
| 00538100 | Merges a per-sphere buffer into the destination. | collision.c |
| 00539880 | Initializes a mergeable per-sphere collision buffer. | collision.c |
| 0053a0e0 | Prepares all five collision groups. | collision.c |
| 0053a120 | Collides a tree against the current static collision tree. | collision.c |
| 0053a3d0 | Collides tree two's root/subtree against tree one's root. | collision.c |
| 0053a660 | Collides tree one's root/subtree against tree two's root. | collision.c |
| 0053a8f0 | Recursively collides two located plug trees. | collision.c |
| 0053afb0 | Builds root pairs and collides two corpora. | collision.c |
| 0053b1c0 | Detects all enabled collisions for one corpus. | collision.c |
| 0053d340 | CHmsItem::ResetDynamicState -> 0x005474E0 CHmsCorpus::Reset -> | vehicle_respawn.c |
| 005477b0 | CHmsCorpus::SetLocation -> | vehicle_respawn.h |
| 00547c80 | Orders physical collisions for response processing. | collision.c |
| 00547de0 | Sorts records with the game's MSVC quicksort implementation. | fastbuffer.c |
| 00547e00 | Negates collision orientation and swaps surface materials. | collision_response.c |
| 00548bf0 | Resolves replacement, contact callbacks, friction, and impulse. | collision_response.c |
| 005497c0 | Sorts and dispatches all detected physical collisions. | collision_response.c |
| 00549c90 | tick_ms is normally 10 for TMNF's fixed 100 Hz simulation. | physics.h |
| 0055f3b0 | The position argument is intentionally unused: a uniform field | physics.c |
| 00586200 | for the two auxiliary tuning curves. | vehicle_aux.c |
| 005914c0 | recomputes | vehicle_curve.h |
| 00597780 | copy | gm.h |
| 007b2e00 | CSceneMobil::SetLocation(spawn) -> 0x0053D550 | vehicle_respawn.h |
| 007bc800 | VehicleBlockSpeed2Set(0) | vehicle_respawn.c |
| 007bc9a0 | SEngine::Reset | vehicle_respawn.c |
| 007bca20 | CSceneVehicleCar::SSimulationWheel::SState::Reset. Bytes | vehicle_respawn.c |
| 007bd090 | Computes one contact impulse from effective point mass. | vehicle.c |
| 007bd2a0 | CSceneVehicleCar::WheelReset. | vehicle_respawn.c |
| 007bd700 | Integrates the engine and gearbox state; the model-5 body is | vehicle.c |
| 007bdbc3 | jumps to the same unclamped active-throttle store | vehicle.c |
| 007be00b | The Steer01..Steer05 engine (tuning +0x354 != 5): a speed | vehicle.c |
| 007be2c0 | Adds a local force at a local point on the vehicle. | vehicle.c |
| 007be310 | Adds a local central force to the vehicle rigid body. | vehicle.c |
| 007be360 | Adds a local torque to the vehicle rigid body. | vehicle.c |
| 007be690 | CSceneVehicleCar::AddVehicleImpulse(GmVec3 const&). | vehicle_aux.c |
| 007c0320 | CSceneVehicleCar::VehicleReset and its base 0x007CB6B0 | vehicle_respawn.c |
| 007c0ec0 | Updates angular wheel speed from contact and driver inputs. | vehicle.c |
| 007c1060 | Integrates one wheel's angular and contact blend state. | vehicle.c |
| 007c1810 | Applies one wheel's suspension force to the vehicle. | vehicle.c |
| 007c2910 | through | track.h |
| 007c2bd9 | Both curve lookups start from the | vehicle_aux.c |
| 007c3916 | by ApplyFrictionForces. | vehicle_model6.c |
| 007c3a5a | divides by 0x00B36AB8 (180). | vehicle_aux.c |
| 007c3c00 | CreateFakeContacts descriptor: material +0x24 mask image | vehicle_contact.h |
| 007c3e80 | call after replacing the game `this` pointer with the composed | vehicle_compute.h |
| 007c3ed8 | stores car +0x5e4, which is both the air-control lock and | vehicle_model6.c |
| 007c50c7 | stores only the x component. | vehicle_model6.c |
| 007c69e0 | tests +0x354 for 3, 4, 5 in that order; the else branch | vehicle_compute.c |
| 007c6a00 | Not double 0.1. | vehicle_model6.c |
| 007c6f0f | when a wheel has contact (+0x124) and is active (+0x0); | vehicle_compute.c |
| 007c8aa0 | UpdateSurface moves the | vehicle_respawn.c |
| 007c93a0 | SSurfaceHandler::Reset: location = source, then the wheel | vehicle_respawn.c |
| 007c97a0 | CSceneVehicle::WaterSplash (car +0x1f8, +0x204..+0x20c, | vehicle_aux.c |
| 007cb6b0 | CSceneVehicle::VehicleReset calls 0x0053D340 | vehicle_respawn.h |
| 007f3f40 | CSceneVehicleCarTuning::GetWaterFrictionFromSpeed: speed times | vehicle_aux.c |
| 007fa770 | CSceneVehicleCar::ComputeForcesModel3 (994 instructions, 521 | vehicle_model3.c |
| 007fb5f0 | CSceneVehicleCar::ComputeForcesModel4 (879 instructions, 437 | vehicle_model4.c |
| 007fc170 | CSceneVehicleCar::ComputeForcesModel5 (1,150 instructions, 583 | vehicle_model5.c |
| 007fc5d2 | Both | vehicle_oldmodels.c |
| 007fc67c | Skipped while car +0x60c is set. Compares against | vehicle_oldmodels.c |
| 007fcf08 | Both limits are rounded products; the reverse one | vehicle_oldmodels.c |
| 0087d9a0 | Combines two material restitution coefficients. | collision_response.c |
| 008e0640 | copy | gm.h |
| 008e08a0 | this = A, then Mult(B): the same products and sums. | gm.h |
| 008e0b10 | build rotation matrix from quaternion (x,y,z,w) | gm.h |
| 008e0c60 | this = src^T | gm.h |
| 008e0d70 | schedule used by IntegrateVehicle. | vehicle_aux.c |
| 008e2570 | UNVALIDATED. | gm.h |
| 008e3120 | normalize quaternion in place | gm.h |
| 008e34c0 | quaternion (scalar first, as the dyna state stores it) from a | gm.h |
| 008e5230 | Transforms an AABB by an affine isometry. | collision.h |
| 008e7dc0 | Signed angle around the global Y axis. UNVALIDATED. | gm.c |
| 008e8890 | Dispatches a geometry pair and fixes reversed output. | collision.c |
| 008ea2d0 | Computes sphere contacts against a triangle mesh. | collision.c |
| 008eadc0 | Computes ellipsoid contacts against a triangle mesh. | collision.c |
| 008f46c0 | Integrates a scalar damped spring by one simulation step. | vehicle.c |
| 008f49d0 | Computes sphere/sphere overlap and contact data. | collision.c |
| 008f5200 | Tests an oriented box against a triangle mesh. | collision.c |
| 009c1c40 | __CIacos. DAT_00D7AFF8 is zero in the live image, so the | gm.c |
| 00af4a00 | loads the double | collision.c |
| 00b2c060 | float -1.0, 0x00D0AC60 float 1e-10 (static .data initialiser, | vehicle_oldmodels.h |
| 00b2c178 | times the | vehicle_model4.c |
| 00b313b8 | double 0.5, 0x00B2C178 double 0.0, 0x00B36110 float | vehicle_oldmodels.h |
| 00b362c0 | float 0.1 as double, 0x00BA38FC float 1e-5, | vehicle_oldmodels.h |
| 00b574fc | is -1e-05f. | vehicle_aux.c |
| 00d67530 | out of the file image instead | collision.c |

(132 entries)
