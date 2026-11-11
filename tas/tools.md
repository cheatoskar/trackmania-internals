# TAS tools and how they fit together

Apart from my patch series on ForeverValidator in [tools/fvlab/](../tools/fvlab/README.md) (MIT, upstream notice kept),
no third-party code is copied into this repository. Every tool below is linked with its author and licence; read the
licence in the tool's own repository before reusing code.

## Overview

```
 map + replay ──> ForeverValidator (reconstructed physics, replays bit-exactly)
                      │   + my patches: traces and a search tool (fvlab)
                      v
              input list (text, TMInterface syntax)  <──> ForeverTAS (GUI bruteforce on the same physics)
                      │
                      v
              TMInterface in the real game: play the script, check it, save a replay
                      │
                      v
              PhysProbe (my plugin): log the real game's per-tick state to compare with the simulation
```

## ForeverValidator

- Author: Skycrafter-dev and contributors. https://github.com/Skycrafter-dev/ForeverValidator
- Licence: MIT (copyright line "Skycrafter-dev contributors").
- A C++ reconstruction of the TMUF physics that reruns a replay and compares it with the recorded ghost. It reproduces
  real replays bit-exactly, which is what makes offline search trustworthy.
- Needs the `Packs` directory of a complete TrackMania United Forever install; a TMNF install alone is not sufficient.
  It supports the seven vanilla environments and cars, not TMUnlimiter or custom physics. TMNF has only the Stadium
  environment; a TMNF Stadium map should load against the TMUF packs, but I did not test that and the equality of
  TMNF and TMUF Stadium physics is unchecked [inferred]. Compare the result in the game.
- Pitfall: a replay saved without finishing validates only the start state ([searching.md](searching.md)).

## My patches on ForeverValidator ("fvlab")

- Derived work of ForeverValidator, so MIT with the upstream notice kept. They sit on one upstream commit as a patch
  series: [tools/fvlab/](../tools/fvlab/README.md) (patches, how to apply and build, every switch). The scripts that
  drive it are in [scripts/tas/](../scripts/tas/), the trace analysis in [scripts/physics/](../scripts/physics/).
- What they add: (1) opt-in per-physics-call CSV traces (state, burnout phase, donut terms, wheel impulses), off unless
  an environment variable is set; (2) maxima of the donut pull exposed to the search; (3) a command-line search and
  experiment tool, `fvlab`, with the modes and objectives of [searching.md](searching.md).
- Build: CPU only on Windows (upstream build script once, then configure without Vulkan and CUDA). GPU is not set up in
  my tooling; it would help only plain finish-time searches, and GPU winners need a CPU re-check.
- Traces answer questions like "how many force calls per tick", "when is the donut active", "which wheel collected how many
  contacts". They need the same ASCII file names for replays.

## ForeverTAS

- Author: Skycrafter-dev. https://github.com/Skycrafter-dev/ForeverTAS , MIT (licence file in the repository).
- A desktop bruteforce/TAS client on top of ForeverValidator (Qt GUI, CPU and GPU backends). Facts below come from its README and docs, not
  from heavy use [inferred].
- Input scripts use the TMInterface text syntax, input-only subset: `0.00 press up`, `1.26 steer 19381`,
  `2.00-3.00 press down`; times are multiples of 10 ms; steering in [-65536, 65536], negative = left. It can extract inputs
  from a replay into a script.
- Custom targets and conditions are documented in https://github.com/Skycrafter-dev/ForeverTAS/blob/main/docs/CONDITIONS.md :
  `min EXPR`, `max EXPR`, `target VALUE EXPR`; conditions are boolean lines; symbols include speed, local velocity, yaw/pitch/roll,
  sliding, gear, and per-wheel ground contact, sliding and surface.
- At the time of my notes the burnout state was not an available symbol, so a "donut" condition has to be approximated
  (sliding flag, lateral velocity) and then checked in the game with PhysProbe.
- Results are text only (a copy-ready script). There is no replay export: play the script in TMInterface and save the replay in the game.

## TMInterface

- Author: donadigo. https://donadigo.com/tminterface , source of the public parts: https://github.com/donadigo/TMInterfacePublic
- Closed-source, free to use; follow its own terms. It runs input scripts in the real game, extracts inputs
  (`dump_inputs`), and offers an AngelScript plugin API (https://donadigo.com/tminterface/plugins/api/all).
- Script syntax: `time command`, time in seconds or ms, ranges (`2.00-3.00 press down`), `#` comments. Docs:
  https://donadigo.com/tminterface/first-script , https://donadigo.com/tminterface/script-syntax ,
  https://donadigo.com/tminterface/input-extraction
- A Python client (https://github.com/donadigo/TMInterfaceClientPython) can read and write game state from outside the game.
  Licence: GPL-3.0 (per its repository).
- Use TMInterface for offline research and for checking a simulation result in the game. It is not meant for online play.

## Community bruteforce plugins (ideas only)

I read these to learn what the community scores; nothing was copied.

- Nosepos+ and others by Shweetz: https://github.com/Shweetz/TMInterfaceAngelScript . Nosepos+ scores an angle to a target pose
  (pitch error + roll error + yaw error, with "accept any yaw" reducing the yaw error by 90 degrees) and then a second goal
  (point, speed, time, hold). No licence stated: ideas only.
- RammFinder and others by Sai-Moen: https://github.com/Sai-Moen/TMInterface-AS-SaiMoen . RammFinder uses per-wheel states
  (grounded, lifted, any) and scores a wheel height. Check its licence before reusing code.
- Guides: igntuL (with Skycrafter), "TrackMania TAS Tutorial" https://docs.google.com/document/d/1iXvjL-ZqHgD6Xk4_NgKsWOl-o1f-0KKsnD3NeTEqsAI/edit
  (search strategy: short independent sections, changes per attempt, gradients); lukalyc and others, "Trackmania: Learn to Noseboost"
  https://docs.google.com/document/d/1Qu0x6Q37OsrfSxjGeym7bncoxQLtnIP_7Buek2Inv-Y/edit (noseboost facts, countersteer, evaluation windows).
  The guides belong to their authors; read the originals. I paraphrase only a few numbers.

## PhysProbe (my own plugin)

- Source: [../scripts/tas/PhysProbe/PhysProbe.as](../scripts/tas/PhysProbe/PhysProbe.as), MIT, entirely my own code.
- A TMInterface 2 AngelScript plugin. It logs one CSV line per physics tick: race time, speed, car-local speed, yaw/pitch/roll,
  inputs, burnout state (0 none, 1 burnout, 2 donut, 3 after), sliding and lateral contact flags, and per wheel ground contact,
  contact material, sliding and damper. It works in normal play, replay validation and bruteforce.
- Status: written against the API docs and **not run in the game yet** [unverified]. Treat the first run as the test.
- The plugin API has no file IO. Lines are buffered and written with `CommandList::Save`, which writes into
  `<Documents>\TMInterface\Scripts\`.
- Install: copy `PhysProbe.as` to `<Documents>\TMInterface\Plugins\`, restart TMInterface, and enable it under Settings > Plugins.
  Variables: `physprobe_enabled`, `physprobe_file`, `physprobe_burnout_only`, `physprobe_console`, `physprobe_flush_ticks`,
  `physprobe_max_lines`.
- Open points for the first run: whether the file keeps the `.csv` extension; whether the simulation callback fires every tick
  during validation; bruteforce logs every candidate into one buffer, so use the burnout-only switch or disable the plugin.

## Validating a replay with the probe

1. Put the replay into the game's replays folder, then Replays > Edit a replay > Launch > Validate (greyed out if
   unfinished or online).
2. The probe logs with `ctx=0` during validation and writes the file at the end.
3. For a real-game log, extract the inputs (`dump_inputs name.txt`), `load` them and drive the map for a `ctx=1` log.
4. Compare the two logs tick by tick with the simulation's trace.
