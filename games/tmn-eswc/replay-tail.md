# TMN ESWC: the ghost runs about 3 s past the finish

Exe: TmNationsESWC.exe unofficial 1.80 (`exever="0.1.7.9"`, SHA-256 983bd127..., timestamp 5ce7272f). Established 2026-11-12 from three online (Net-TimeAttack) replays, read-only parsing of the files. Status: verified = measured in the three files; inferred = reading of code or data, not tested; open = not explained.

## What is in the file

| Fact | Status |
|---|---|
| The input event list (chunk `0x2403F00D`) has `EventsDuration` equal to the race time (26710, 13410, 14000 ms) and its last event is at or before that time (25897, 13317, 13805 ms). Nothing is recorded after the finish | verified (3 files) |
| The ghost's sample data (chunk `0x0303F003` of the ghost node) holds one sample per 100 ms, 60 bytes each; the first u32 after the chunk data is the sample count. The counts are 298, 165 and 171, i.e. the ghost reaches 29700, 16400 and 17000 ms: **2.99, 2.99 and 3.00 s past the race time** | verified (3 files) |
| Sample offset 48 is a signed byte that follows the steering input (+127 = full right, -128 = full left) up to the finish, so it is used here as the steer value of the car | inferred (correlation with the input list, 0.6 at best on the full run; the exact meaning of the byte is not established) |
| After the finish the sampled car keeps moving: speed stays high, no braking. In the file whose last input state was "steer left + accelerate", the steer byte stays at -114..-122 for the whole 3 s and the speed falls from about 4677 to about 2630 (raw int16 at sample offset 18, unit not established), the car circles. Inputs that were pressed after the finish do not appear in the event list and have no visible effect | verified (1 file); inferred as the cause |
| In the two other files the last input state was "accelerate only", yet the steer byte reads +127 (full right) from 100-200 ms after the finish for the whole 3 s, while the speed value stays flat at about 4100-4240. A held steer input therefore does **not** explain this tail | open |
| The game's own `Autosaves` ghost of the same race has the same sample count (A-2: 173 samples in both), so the tail is game behaviour and not something a mod adds | reported by an earlier analysis, not re-run |
| Solo: the game cuts the throttle after the finish, so the same tail looks like coasting. Online the tail keeps the inputs from the finish line (full throttle in all three files) | solo part inferred, online part verified for throttle |

## When the recording stops

| Fact | Where | Status |
|---|---|---|
| Online (and LAN) the replay recording is not stopped by the finish line. It stops when the game makes the previous-race copy on restart: `RaceNet::StopReplayRecordAndKeepCopy()` copies app+0x144 into app+0x25C, which is the replay "Save previous replay" writes. That explains why the ghost keeps being sampled for about 3 s after the finish while the input list is already closed | 0x0074AB60, see [addresses.md](addresses.md) | verified (function and member roles); the link to the 3 s is inferred |
| Solo: the race end 0x006C5060 stops the recording of app+0x144 itself | addresses.md | verified |

Consequence for tools that read these replays: do not treat the ghost's last 3 s as driver input and do not compare the ghost length with the race time; use the header `best` or `EventsDuration`.

## Why these three files say validable="1"

[replay-validable-eswc.md](../../formats/replay-validable-eswc.md) and [gotchas](../../gotchas.md) say that online replays as the game writes them are `validable="0"` because the file holds one ghost per participant. These three files are different:

- The header says `validable="1"`, the ghost list (chunk `0x2403F004`) has count 1, and the race settings string in chunk `0x2403F00F` is still `Net-TimeAttack`. That is the "LAN, alone" row of the validable table: the flag follows the data in the file, not the mode.
- All three file names end in ` TMX.Replay.Gbx`. That is the name a replay-saving mod's TMX copy gives to a file it rewrote after an online save, keeping only the local player's ghost (the public eswc-autosave mod, `src/tmxcopy.cpp`, does this and recomputes the header flag from the new body). So these are not files as the game saved them; the game's own online save has several ghosts and `validable="0"`. Status: inferred from the file names and from the mod's source; the original multi-ghost files were not available to compare.
- The kept ghost node is copied as it is (inferred from the mod's source), so the 3 s tail belongs to the original ghost and is not an effect of the copy.

The gotcha stays correct for what the game writes itself; it was too absolute for files that went through a copy tool.
