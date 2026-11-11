# TAS workflow: how I build a run on a new map

Playbook from several maps (Nadeo tracks and community maps), written generically. Order matters more than any
single trick. Status words as in [README.md](README.md). All numbers are [sim] unless marked.

## 1. Before the first search (about 30 minutes, saves hours)

1. **Start with the start trick** (the standard "super start" input pattern, see
   https://www.trackmania.wiki/wiki/Category:Tricks). Drive the base run with it from the first try. Added later it
   gains only a few milliseconds and breaks every trick after it, because the line is chaotic.
2. **Use a long container replay.** Your own drive should reach the finish, or at least the last section. Any input
   list can then be run on it; a short container cuts every later stage off.
3. **Check that the simulation replays your drive exactly** (a reference run that prints the finish time). Anything
   that matches your own drive is then as good as game-confirmed.
4. **Scan the map**: block names and positions (start, checkpoints, finish) and the collision triangles around every
   place where a trick could happen (kerbs, borders, roads overhead, objects in the flight path). Write heights and
   boxes into a route note. I lost hours to "a sculpture" that was really the underside of a road.
5. **Mark TMInterface boxes** with their meaning: checkpoint triggers, the finish box, and for a contact box whether
   the car should touch it from below or from above. Compare the finish box with the simulation's finish early.
6. **Keep one route table**: checkpoint order, positions, timeline of your drive (state every 250 ms).

## 2. Order of work: section by section, forward

Each stage is seeded with the best line of the previous stage.

1. **Get there**: distance to a point (a gradient). Never score a checkpoint that the start line cannot reach yet.
2. **Set-up**: the contact that fixes the pose (a border, a kerb, a road underside). In the air the pose is fixed at
   the last contact; inputs in flight barely matter and change only rotation, not the path of the centre of mass.
3. **Trick**: nose landing, noseboost, uber, flick (section 4).
4. **Through the checkpoint**: checkpoint time and speed together.
5. **Finish**: finish time with a horizon to the finish.

If a later section needs more speed than the line can give, go back one section and build the trick there, not
further down. Rebuilding an early section for a slightly earlier checkpoint once lost the nose landing that a later
boost needed; the boost was worth far more than the saved 0.2 s.

## 3. Staged short-window searches

- Windows of **1 to 5 s**: most 1.5 to 4 s, noseboost and bug-through 0.5 to 1.5 s. Short windows keep the search
  space small and make each stage independent. Experienced community TASers report the same (igntuL's guide, link in
  [tools.md](tools.md)).
- **Changes per attempt** depend on the task: turns 2 to 5, tricks 5 to 8, ubers 8 to 15, noseboosts 10 to 20 (short
  windows only) [community].
- **Several start lines per stage**: the first accepted improvement decides the basin. Many lines in parallel beat
  one clever search; "a good base run bruteforced for 5 minutes is better than a bad base run for 10 hours"
  [community].
- **Rounds of 5 to 10 minutes**; if the score has not moved after two rounds, the objective or window is wrong. Look
  at the run instead of adding rounds.
- Let the input window start 0.1 to 0.2 s **before** the evaluation start (steering has a turn-rate lag) and, for
  nose tricks, let evaluation and inputs run at least 1.5 s past the nose landing (notes below).
- Score a section up to a point where the next part already depends on it, not at the checkpoint itself. Examples
  from one Nadeo map (Pro A-4): a checkpoint search found a line 30 ms earlier that was too wide and hit pillars
  behind it; a take-off box gave a line 30 ms earlier at the edge that landed at -13 degrees and crashed, while a box
  behind the landing gave a level landing.
- Run the human-playability constraint (keyboard-only steering with minimum gaps between events) **last**; it
  shrinks the search space a lot.

## 4. Trick table (generic)

All [sim] on Nadeo TAS replays and my own searches; mechanisms in
[../physics/bugs/README.md](../physics/bugs/README.md).

| trick | needs | inputs | observed |
|---|---|---|---|
| **noseboost** | front wheels on flat ground, pitch 83 to 88 degrees and settled (< ~30 deg/s), roll < 3 degrees, about 478 km/h or more | gas + brake + steer (one sign) with small steer wiggles; landing window 0.1 to 0.3 s before to 0.5 to 0.8 s after | 476 to 997 km/h on a road, 444 to 1000 km/h in 0.7 s; of 25 attempts at 480 to 500 km/h, 16 reached 1000 with one kick |
| **boost direction** | none | the kick turns the travel direction; the side follows the steer sign | about 180 degrees after a long ride, 30 to 60 degrees after a short one |
| **nose chain** | a spinning car on its nose (6 to 18 rad/s around the long axis) | gas almost always, brake toggled, steer sign flipped nearly every tick | one Nadeo TAS: 3.3 s at 850 to 1000 km/h with a turn of about 170 degrees |
| **nose position from a border** | landing or driving with only one rear wheel on a raised border | gas + full steer, brake from half-way | 411 km/h to nose at 82 degrees in 0.6 s |
| **nose ride to the boost** | nose position at about 390 km/h or more | gas, brake most of the time, steer full left/right every few ticks | +95 to 150 km/h per second until about 470 to 480, then one kick to 1000 |
| **nose position from flat road** | a block transition or edge while sliding with gas + brake + steer | spin kick, then 0.7 to 0.9 s on one front wheel with gas held | in the Nadeo TAS library I analysed, 134 of 248 nose phases began after a spin kick |
| **kerb noseboost** | leading front wheel into a kerb or border while in the burnout state | short window around the contact | 276 to 999 km/h at a 0.6 m road kerb |
| **roof hit** | an overhead surface sloping down in the flight direction | an uber steep enough to reach it, aimed at the underside | 994 to 551-614 km/h, pushed down, then a nose landing |
| **flick** | high speed and one wheel or the body touching an edge or the ground | gas + brake + full steer during the contact | e.g. 642 to 697 km/h with a 60 degree turn |
| **gas in the air** | none | keep gas on in every flight | releasing it costs about 90 km/h in 0.4 s (coast drag) |
| **bug checkpoint / finish** | a trigger that reaches out of its block sideways or from below | fly or bounce into that part of the trigger; map the real trigger first, my own box can be too small | worked on a community map and the game counted it |

Notes on noseboost (community and my tests):

- Gains start at about 391 km/h; low-speed noseboosts can get stuck around 440 km/h ("gear" in the community guide,
  see [tools.md](tools.md)). My nose searches therefore demand 450 km/h or more at the landing.
- A nose landing at about 405 km/h needs a long ride (about 1.25 s, 404 to 704 km/h) before the kick to 1000. If the
  evaluation or input window ends earlier, a line 0.05 s from the boost looks "stuck" and the car lifts off.
- Do not airbrake into a nose position; countersteer (community advice).

## 5. Search recipes that worked

Per stage, with the concepts explained in [searching.md](searching.md):

| stage | objective | window |
|---|---|---|
| get to a point | distance to a target point, speed weight 0, minimum speed at the target | 1 to 2 s before |
| nose landing | nose landing after a flight, minimum landing speed, penalties for spin and roll, loss measured as long as the gain | from the last contact before the flight |
| noseboost at a landing | as above with a direction target ahead of the car | 0.05 s before the landing to 0.3 s after, extended per the notes above |
| checkpoint with speed | previous checkpoints required, checkpoint plus speed | from before the previous contact |
| finish | all checkpoints required, target = finish, horizon to the finish | from the last trick |

**Pose then boost.** A single search from flat driving straight to a noseboost does not work: the boost score is
zero until the car is on its nose, so there is no gradient. Chain two stages: first a pose target (wheel states such
as "rear wheels lifted", pitch about 82, a speed floor), then a nose search from the first nose tick, seeded with the
pose result. Community plugins use the same idea (Nosepos+ and RammFinder, see [tools.md](tools.md)).

## 6. Hygiene (each line cost me a round)

- **Verify every hit** with a state print and a clean reference run ([searching.md](searching.md)).
- **Stop a stuck search** after two rounds and change objective or window.
- **One search at a time.** Searches use all CPU cores; a second one just slows both. Kill the wrapper script first,
  then the worker processes, then count what is left running: killing only the workers lets the wrapper start the
  next round.
- Never put a score target where the car cannot go straight (behind walls, under a road).
- A pose target with soft wheel penalties on a flat straight is cheated by braking (the nose dives, rear wheels lift
  for a few ticks). Use a height target above the road first (a hop needs a real contact), then the pose.
- A search that cannot add start lines needs a restart; restart right after a round, not in the middle.
- Shell pitfall: PowerShell reads `"$Var: text"` as a drive-scoped variable; write `"${Var}: text"`.

## 7. Hand-over and game check

Export the best input list to the TMInterface script syntax (`0.00 press up`, `0.16 steer 65536`; time = event time
minus race start minus 10 ms). Start the script with a set speed of 10 and drop to speed 1 about 1 to 2 s before
the new part. Cut the script after the part you want to watch and end with `steer 0`. Play it in the real game the
same day: a game check of the new part and of the finish beats any amount of simulation. The simulation is a
reconstruction and can differ from the game.

## 8. Time budget (one map, two days)

A first drive of 53.5 s became about 33.9 s in simulation. Rough costs: corner polish 30 min; a roof hit to a
checkpoint with 580 km/h 2 h (wrong target first); noseboost plus flick 1 h once a line existed; the finish from the
flick one 10-minute round. What cost the most: objectives the search could cheat, and searches I let run too long.
