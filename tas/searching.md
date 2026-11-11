# Searching: modes, objectives, cheats, verification

How my search tool (`fvlab`, see [tools.md](tools.md)) works, described as concepts you can reproduce in any
bruteforce tool. Times are replay times in ms (race start = 0, first input tick at 10 ms), tick = 10 ms.

## Modes

| mode | what it does |
|---|---|
| inputs / probe | print the replay's inputs; print the state per tick (speed, position, pitch/roll, wheel contacts, inputs) |
| blocks / scene | list placed blocks with world positions; list collision triangles around a point |
| earliest finish | local search on steering inside a window: cut the window into segments, change 1 to 3 steer events of the current best in one segment (value, time, insert, delete), restore the cached state at the segment start, simulate to the finish; stop an attempt as soon as it cannot beat the best. Score = sub-tick finish time. About 180 attempts per second per process on a 5.7 s run |
| trick search | mutates steer, gas and brake events in a window, simulates for a horizon and scores a configurable objective (below) |
| sweep / keys | constant-steer sweeps in a window (seam tests); keyboard-only steering (human playability) |
| random | random inputs from many start states, to look for anomalies; never produced more than 15 km/h gain per tick in about 4 100 random 2 s runs on 23 replays [sim] |

## Objectives (concepts)

Score, lower is better. Typical terms, combined per stage:

- **Finish time** (sub-tick estimate), with a small tie-breaker such as the distance one tick before the finish: an
  integer finish time has no gradient.
- **Distance to a target point**, minus a weighted peak speed (cap it at 1000 km/h, because post-kick spikes above it
  never move the car), plus a penalty per km/h below a minimum speed at the target.
- **Box entry**: earliest entry into a trigger box, or highest speed at the entry. Gate it on "n checkpoints taken".
- **Landing box**: the target is a landing with 2 or more wheels down inside a box; outside, distance to the box.
- **Nose objective**: first landing after a flight; nose angle, start of the nose bug, speed gained on the nose within a
  gain window; penalties for a slow landing, wrong height, spin, roll, speed lost after the landing (loss window as long as the
  gain window), direction error some ms after the landing, small reward for holding the nose.
- **Near-uber exponent**: the donut pull's exponent (see physics pages) rewards near-ubers before any speed shows up.
  Count it only above a minimum speed, and only if the car is not slower a moment later.
- **Pose target**: pitch, roll, yaw relative to the velocity, per-wheel states (touching, lifted, any), minimum speed.
  Soft penalties instead of hard gates give a gradient from a plain four-wheel drive.
- **Boost box / boost direction**: the first tick above a speed must be in a box; heading 100 ms later toward a target.
- **Checkpoints needed / reached**: distance and landing terms count only after n checkpoints.
- **Spike filter**: a tick's speed counts as the minimum over the last n ticks (n = 5 to 8 turns an uber that hangs on a wall into a
  held-speed gradient; n = 1 rewards one-tick spikes).
- **Stall kick**: after s seconds without progress a worker restarts from its best with 8 to 20 random changes.
- **Human playable**: keyboard steering only, at least 50 ms between events of one action.

## How searches cheat

My searches found a way around every objective that did not pin down *where* and *how fast*. Check every "hit" by
looking at the run.

- **Distance to a point** reached by crashing next to it and sliding on the side at 43 km/h, or by flying *under* an
  elevated road 8 m below the target. Fix: minimum speed at the target, target above the road surface, count only wheels
  on the surface (landing box). Even then a fast fly-under scored best on one map (500 km/h under the road).
- **Nose landing**: the first front contact while still rolling, a crash at 50 km/h, a landing on the ground far below
  the intended road. Fix: landing only after a flight, minimum landing speed, minimum height.
- **Gain on the nose** from a kick after the car fell back on its wheels (an uber). Fix: count gain only with rear
  wheels up and the nose more than 45 degrees down.
- **A box just behind a checkpoint line** reached by passing under or next to the checkpoint (a "gain" of 92 ms with the
  checkpoint not taken). Fix: gate every trigger box on the checkpoints needed.
- **A perfect-angle nose landing** where the body hits nose-first, one front wheel touches and the car bounces off
  (453 to 373 km/h, no boost). Fix: loss term after the landing.
- **Loss window shorter than gain window**: a clean ride to 430 km/h, then a crash just after the loss window.
  Same with a boost: 479 to 1000 km/h at the end of a 1.0 s loss window, 360 km/h 0.1 s later. Make the loss window at
  least as long as the gain window and past the boost.
- **Crash before the landing** that bounces the car over a border, then a slow landing. Fix: minimum landing speed near
  what the stage needs.
- **Window that starts after the landing**: the nose objective never sees the landing and scores every run as "no landing".
- **Boost direction** aimed with the nose objective never turned toward the next checkpoint; the flick right after the
  boost sets the direction, so free the car afterwards.
- **One-tick speed spikes** in a crash (251 to 774 to 264 km/h at an inflatable) count as "speed at the checkpoint" or
  pass a speed gate. Fix: spike filter.
- **Near-uber exponent also rises when the car nearly stands** (crashes, crawling contacts). On one Nadeo map every
  exponent up to 5.94 came from ticks below 80 km/h, and searches chased them for hours. Gate by speed and require the speed to be
  held; the exponent marks hard wheel corrections, not uber set-ups.
- **Landing box around an obstacle that also holds the approach path**: the search dropped the uber and scored the
  approach at 290 km/h. Check the start lines: the uber line must score its landing speed, a line without it must not.
- **A checkpoint search that flies over the checkpoint.**
- **A direction target behind the car** or **a box hit on the wrong side**.
- **A pose target on a flat straight** cheated by braking (see [workflow.md](workflow.md)).
- **A trigger box for a landing that is tall**: crossed in the air, and the search lands somewhere else. Keep it at ground level.
- **Time targets with an integer finish** have no gradient: use a tie-breaker.
- A window that ends before the landing cannot press the brake that the nose bug needs.
- A nose hold reward weighted too high made the search trade speed for hold time (513 to 446 km/h); as a small
  tie-breaker next to the gain it worked.

Rule: always print the state of the best run tick by tick before reporting a result.

## Pitfalls of the search machinery

- **Snapshot drift**: restored mid-run snapshots can differ from a continuous run. Re-run every new best from the window start
  before accepting it.
- **Event on the window start**: the state captured at the window start has already applied that tick. An event placed
  exactly on it was ignored by the search but applied by a fresh run and the game (a -8.7 reported became +182.6 after
  reload). Place nothing on the first tick, keep a base event exactly on the last tick, keep window bounds on the 10 ms grid,
  and keep structural events (race running, finish, respawn) out of windows.
- A replay saved without finishing validates only the start state; with a finish the whole ghost is compared even though the simulated car
  never finishes.
- Finish-time estimates have no sub-tick information from the replay itself; the tool estimates it.
- In the air, inputs change only rotation, not the path of the centre of mass.
- Do not run two searches at once.

## Verifying a hit tick by tick

1. Run the candidate input list from scratch against the same replay container with the optimized search backend switched off
   (a "reference run"), and compare its finish or box-entry time with the search's report.
2. Print the state per tick around the interesting moment plus 0.5 s past the input window. Look at speed, pitch, wheel
   contacts and the inputs. A run of +15 to 30 km/h steps at 450 to 520 km/h is a boost building up; one tick of 774 km/h
   is a spike.
3. Check the checkpoint counter at the end, and the geometry (is the car really where the box is, on the right side?).
4. Compare with a reference run of your own base line: the same state print on both, tick by tick, to see where they diverge.
5. Play the script in the real game with TMInterface ([tools.md](tools.md)). The simulation is a reconstruction.

## Switch cheat-sheet (concept to name)

My fvlab patches read their settings from environment variables with an `FVLAB_` prefix (target point, minimum speed,
objective, nose minimum speed, landing box, loss and gain windows, spike filter, exponent minimum speed, checkpoints
needed, pose wheels/pitch/roll/yaw, direction target, stall kick, human-playable). The concepts above are the real content;
the exact names and a table of them are in [tools/fvlab/README.md](../tools/fvlab/README.md).
