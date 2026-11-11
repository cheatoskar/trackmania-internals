# Lessons from TAS results

Generic lessons only; no inputs, no replay files. Nadeo maps are used as examples. Status labels as in
[README.md](README.md).

| lesson | example | status |
|---|---|---|
| A noseboost kicks the car to the 1000 km/h clamp from about 450 to 500 km/h; the kick turns the travel direction | Nadeo maps E02 and C02: the mechanism reproduced exactly in the simulation of a real TAS (details in [../physics/bugs/README.md](../physics/bugs/README.md)) | verified [sim] |
| A nose chain can hold 850 to 1000 km/h through a turn | A01: 3.3 s on the nose, about 170 degrees of turning | verified [sim] |
| Nose positions can start from flat road at a block transition after a spin kick | Nadeo D-0: about 414 km/h to a nose in 0.9 s, then 999 km/h about 0.5 s later | verified [sim] |
| Roof hits cost speed but give a nose landing | 994 to 551-614 km/h, pushed down | verified [sim] |
| A nose landing at about 405 km/h can still boost, if the input window runs long enough | Pro A-4: 1.25 s ride from 404 to 704 km/h, then the kick | verified [sim] |
| Searches fail more from objectives than from the search itself | every objective that did not pin down where and how fast was cheated; see [searching.md](searching.md) | verified |
| One bug that needs a hand-made set-up is often easier to find from a human drive than by search | Pro A-4: the first noseboost came from my own replay that already touched the obstacle | verified [sim] |
| Releasing gas in the air costs about 26 % speed per second | coast drag | verified [sim] |
| A bug checkpoint or bug finish can beat the intended route | seen in the game on a community map | verified [game] |
| Random inputs never produce more than 15 km/h gain per tick | about 4 100 random 2 s runs on 23 replays | verified [sim] |
| An uber into the finish can be found by search | a community map | verified [sim] |

## Aggregate insights from community TAS (no names)

I looked at the best known TAS of the Nadeo tracks (nose phases, speeds at boosts). The one number I rely on: in
that library, 134 of 248 nose phases begin after a spin kick, and very few TAS boost below about 478 km/h. I also
keep a list of TAS remakes with before/after times; I have not summarised it here and did not re-simulate it.

## Things I would not repeat

- Searching whole runs at once instead of sections with seeds.
- Letting a stuck search run for hours.
- Trusting a reported gain without a clean reference run and a game check.
- Rebuilding an early section for a few hundredths of a second and losing the set-up for a later trick.
