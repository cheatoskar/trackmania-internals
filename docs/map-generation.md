# Map generation for TMNF Stadium (machine learning)

What I learned building a neural map generator for TrackMania Nations Forever: how a
Stadium map can be represented for a model, what worked and what failed, how to measure
generated maps, and how to write the result as a `.Challenge.Gbx`.

My generator is a private rebuild of donadigo's [TMTrackNN](https://github.com/donadigo/TMTrackNN)
(GPL-3.0) and is not published. Its map writer is derived from donadigo's GPL-3.0 code, so no
code from it is in this repository; this page keeps only the findings and the format facts.

Source key: `tmtracknn2` = my own generator experiments and their notes (unpublished); `dona` = donadigo's TMTrackNN; `[game]` = checked by driving or opening
the map in TMNF. Numbers are measured unless marked otherwise.

## Data

| Fact | Source | Status |
|---|---|---|
| A public corpus exists: the Hugging Face dataset [jeanmidev/trackmania-community-tracks-and-telemetry](https://huggingface.co/datasets/jeanmidev/trackmania-community-tracks-and-telemetry), CC BY 4.0, collected from the Mania Exchange sites. TMNF: ~617k tracks, block layouts in 310 Parquet parts (`part0000`..`part0309`, ~2,100 maps and ~15 MB each, ~4.5 GB), metadata `tracks_details_tmnf.parquet` (136 MB), winning-run traces (~87 MB per part, 18.9 GB) | tmtracknn2; HF API 2026-11-11 | verified |
| Parts are numbered from 0000, not 0001 (starting at 1 skips part 0 and 404s on 0310) | tmtracknn2 | verified |
| Use that dataset (plain HTTPS, no login) instead of crawling TMX for layouts. If you do call the TMX API, see [tmx/api.md](../tmx/api.md) and [modding/tmx-rules-for-mods.md](../modding/tmx-rules-for-mods.md): identify your client, respect rate limits, do not redistribute the maps | | recommendation |
| Block columns: `track_id, name, x, y, z, dir, block_id, position_block_trace_order`, entry/middle/exit speed. **No block flags** | tmtracknn2 | verified |
| `dir` is a string North/East/South/West = rotation 0/1/2/3 (97% of 100k blocks match the original files; the rest are ambiguous stacked borders). A few stray `4`/`6` rows exist | tmtracknn2 | verified |
| **Dataset x/z = `.Gbx` x/z − 1**, y identical (constant over 108 matched maps) | tmtracknn2 | verified |
| `position_block_trace_order` is the block *nearest* the car per replay sample, not a route; −1 means "not visited" (88% of rows). Ordered by it, only 48% of consecutive drivable blocks touch and 21% start on the start block | tmtracknn2 | verified |
| Traces: winning run every **100 ms** (physics runs at 10 ms): `x y z` in metres, `input_steer` (53% zero, 43% full lock), `input_gas`, `input_brake`. No orientation, so yaw in a drift cannot be recovered | tmtracknn2 | verified |
| 37% of maps have blocks outside 0..31 (border clips): drop the block, not the map. A few maps have impossible coordinates (x up to 198) | tmtracknn2 | verified |
| About 161k of 651k TMNF maps have at least one award; filtering on awards before processing cuts build time by ~75% and is a cheap quality filter | tmtracknn2 | verified |
| Metadata tag ids: 0 Normal, 1 Stunt, 2 Maze, 3 Offroad, 4 Laps, 5 Fullspeed, 6 LOL, 7 Tech, 8 SpeedTech, 9 RPG, 10 PressForward, 11 Trial, 12 Grass. `length_ms` is a length *class* (0 = 15 s, 1 = 30 s, 2 = 45 s, 3 = 1 min, ...) | tmtracknn2 | verified (ids decoded from name keywords) |
| Styles are unbalanced: Tech has ~20x the routes of RPG; Fullspeed, Offroad, RPG and Maze are data-starved | tmtracknn2 | verified |
| donadigo's `data/train_data.pkl`: 3,588 Tech routes ordered by the ghost replay (TMX filter tech, by awards, ~1 min). Small but clean: 99.7% start on a start line | dona; tmtracknn2 | verified |

## Representing a Stadium map

| Fact | Source | Status |
|---|---|---|
| The arena is a 32 × 32 grid of cells, y up to 31. **y = 0 is the terrain layer** (water, dirt, pool, dirt border), ground level is **y = 1**. Terrain is a heightmap: dirt hills stack | tmtracknn2 | verified |
| A block is `(block id, x, y, z, rotation 0-3, flags)`. Ids come from a name table (pygbx `STADIUM_BLOCKS`, ~370 names); unknown names must not be mapped to a default block (v1 silently turned them into straight roads) | tmtracknn2 | verified |
| Multi-cell footprints: pygbx `STADIUM_BLOCK_OFFSETS`. Rotation is **counter-clockwise about the origin, then re-anchored so the minimum corner sits at the block position** (clash rate 3.83% vs 5.38% clockwise, 7.05% unrotated) | tmtracknn2 | verified (empirical) |
| Rotating a whole map must re-anchor every footprint, not just rotate origins: rotating origins alone opened a one-cell gap at every non-square block (touching 94.2% -> 73-79%) and corrupted three quarters of an augmented corpus | tmtracknn2 | verified |
| Mirroring only works for blocks whose reflected footprint equals one of their own rotations; 82% of Tech routes contain a block that is not mirrorable. Augment 4x (rotations), not 8x | tmtracknn2; dona | verified |
| The offset table is approximate. 23 block types are stacked onto their own copies closer than it allows (e.g. `StadiumFabricPillarAir` is 3 cells in the table, 1 in practice). Real maps share 10.7% of occupied cells, so strict "one block per cell" vetoes common real placements | tmtracknn2 | verified |
| Name traps: `StadiumCircuitBase` is the circuit ground plate (~228 per map), as are Water, Pool, Grass, Dirt, DirtHill, DirtBorder. `StadiumRoadMainSlopeBase` contains "Base": never filter on that substring | tmtracknn2 | verified |
| The only start/finish blocks: `StadiumRoadMainStartLine`, `StadiumRoadMainFinishLine`, `StadiumRoadMainStartFinishLine`, `StadiumRoadDirtStartFinishLine`. Names like `StadiumGrassStartLine` do not exist. Wide finish lines are several finish blocks side by side | tmtracknn2 | verified |
| A map's track graph is genuinely disconnected: jumps, transitions and risky finishes need no physical connection. Unbridged, a typical map splits into ~9 components holding 43% of the track | tmtracknn2 | verified |
| Real maps average ~574 blocks, ~82% of them not driven on (ground plates, water, pillars, decoration) | tmtracknn2 | verified |
| `StadiumRoadMain` changes shape (straight/curve/T) from its neighbours through its flags; side-by-side roads stay separate | tmtracknn2; [game] | verified |

### Route order

| Approach | Result | Source |
|---|---|---|
| Greedy nearest-neighbour ordering (v1) | 31.6% of consecutive pairs 4+ cells apart | tmtracknn2 |
| Graph search from start to finish with gap bridging (v2) | 55% of maps reach a finish, but 28% of mined "transitions" were teleports from the bridging | tmtracknn2 |
| Ghost replay order (donadigo): blocks in the order the car touched them, a grass filler token where the car is over bare ground | clean; why his 3,000 maps beat 83,000 graph-searched routes | dona; tmtracknn2 |
| Walk the docking graph, rank neighbours by car position from the traces (v3) | 89.8% reach the finish, 90.8% docked, 13.3% decoration pieces in the route | tmtracknn2 |

Rule that worked: a block may be part of the route if the replay visited it *in that map*,
not by block name, because RPG and LOL maps drive on decoration blocks.

### Ports: connection as part of the representation (v3)

A **port** is `(canonical cell, face)` of a block type: a face the car crosses. Ports are
mined from real ordered routes, nothing is hand-written. A route step is a token
`(block, rotation relative to heading, entry port, exit port)` plus a gap
`(forward, lateral, up)`, `(0,0,0)` = docked. Position and rotation follow from the open
exit of the previous piece, so a docked step connects by construction and the
representation is rotation-invariant (no augmentation needed).

| Fact | Source | Status |
|---|---|---|
| Ports mined from 90% of donadigo's routes dock 99.7% of held-out touching steps (a block-pair table: 88.8%). The table has ~2k entries | tmtracknn2 | verified |
| The previous approach, predict `(block, rotation, offset)` and veto with a block-pair table, forbade 11.2% of held-out real steps; only 5.2% of held-out real routes were generatable at all | tmtracknn2 | verified |
| Encode/decode round trip exact for 3,459 of 3,491 donadigo routes, 0 mismatches | tmtracknn2 | verified |
| Gaps should only be drawn after "launch" pieces (blocks whose outgoing steps in real routes are gaps >= 30% of the time: ramps, pools, turbo ups). Free gap sampling gave 8.9 breaks per map vs 2.1 in real Tech maps; launch-only gave 0.3 | tmtracknn2 | verified |
| donadigo has no arena wall: he limits the route's own bounding box and centres the finished route. Clamping to 0..31 instead ran routes into the border (23 border cells -> 0 after porting his rule) | dona; tmtracknn2 | verified |
| Ground-only blocks (dirt roads, inflatables) must be excluded once the route leaves y = 1, or they float | dona; tmtracknn2 | verified |
| donadigo's `CONNECT_MAP` is defined but unused: a hand-written socket table is not needed | dona | verified |

### Macro planning

v1 had a "macro planner" (start -> straight -> tech -> jump -> finish segments) to stop the
model wandering off the 32 × 32 grid. It did not help because the model was never called:
the builder generated every map from random branches. What replaced it: conditioning on
real metadata (tags, difficulty, length class, awards, mood, a hashed name) as prefix
tokens with random dropout (classifier-free guidance), the bounding-box rule above, and a
length class whose per-tag median the sampler aims for. Laps maps (4% of routes) still
lack loop closing.

### Three stages, not one sequence

A route is a **chain** (position = offset from the previous piece); scenery is a **field**
(position = offset from the route piece it belongs to). One sequence cannot be both: a
whole-map model with absolute positions produced only 60-75% touching route steps. Split:

1. **Route:** decoder-only transformer over docked pieces.
2. **Structure:** blocks touching a route piece (27-neighbourhood) in the piece's frame:
   dirt hills under dirt roads, walls, pillars. About half of real "scenery" is this.
3. **Design:** the rest (on 3,000 real maps: 406 scenery blocks per map, 85 structure,
   321 design; half within 3 cells of the route). Masked discrete diffusion over the 32³
   grid with random-order unmasking; confidence-ordered unmasking put every block into the
   last round.

## What failed, and the general lessons

| Failure | Lesson |
|---|---|
| The decoder chose instead of the model, five times: first-fit picked small blocks (62% camera blocks), pooling all legal placements weighted blocks by how many cells they fit, EOS pooled with placements won because it paid one log-prob term instead of six, a `log(count)` prior turned the generator into a Markov chain, confidence-ordered unmasking | Draw from the model in the order it factorises; use tables and geometry only as a veto on that draw |
| A frequently firing veto biases the output | Whatever survives a common veto is selected for |
| 22x more data did not fix block-family switching; a 12M model had the best loss and the worst connectivity | Val loss and map quality disagree past a point; check the supervision before scaling |
| A transition table mined from extractor output was validated on extractor output (96.5% "coverage", 28% teleports) | Validation must not be circular |
| Unset style knobs defaulted to 0.3, which asked every map for a third of the real banked corners | A conditioning default is a claim about a normal map; use the corpus mean |
| Nucleus sampling (top_p 0.92) strangled generation to ~3 blocks per map | Do not cut the tail when legality is checked afterwards |
| Four sessions judged models on maps exported with wrong block flags | Write real maps through the same exporter first (control maps) |

## Quality metrics

Compare generated maps with **held-out real maps of the same style**, never in absolute
terms. Metrics used (all on the placed blocks):

| Metric | Meaning | Real Tech (example) |
|---|---|---|
| finished | route ends on a finish block | ~0.83-1.0 |
| docked / adjacent | consecutive pieces connect through ports / touch | 0.89-0.95 |
| breaks per map | non-docked steps | 2.1 |
| turn_rate | share of steps that change heading | 0.58-0.67 |
| family_switch | share of steps that change block family (RoadMain, Platform, Tube, Circuit ...) | 0.35-0.47 |
| repetition | longest run of one identical block | ~2.5-4.3 |
| variety | distinct blocks / route length | ~0.43 |
| elevation | vertical range of the route | ~5-7 |
| transition coverage | share of steps that occur in real maps | 0.965 |
| block_js | Jensen-Shannon divergence of the block mix | low |
| realism report | per tag: scenery share, top-type share, largest same-type cluster, floating share, overlap, bounding box, elevation; flag anything outside the real p5..p95 | real control maps pass |

The numbers vary between corpora and splits; the table shows the ranges seen, not
targets. Metrics catch broken maps; whether a map *feels* built by a person still needed
driving it.

## Writing the `.Challenge.Gbx`

The writer patches a template map (donadigo's `Template.Challenge.Gbx`) instead of
building a file from scratch. Container and LZO facts: [formats/gbx.md](../formats/gbx.md).

| Fact | Source | Status |
|---|---|---|
| Body chunk `0x0304301F` holds the block list: u32 count, then per block a lookback string (block name), u8 rotation, u8 x, u8 y, u8 z, u32 flags | tmtracknn2 writer; pygbx | verified (round trip) |
| Lookback strings: the first one in the stream is preceded by u32 version 3; a new string is written as `0x40000000` + u32 length + bytes, a repeat as an index | pygbx; tmtracknn2 | verified |
| Header chunk `0x03043002` holds the medal times (4 × i32 bronze, silver, gold, author; `-1` in the template); body chunk `0x0305B004` (challenge parameters) holds them again | tmtracknn2 writer | verified (game loads the result) |
| Header chunk `0x03043003` holds the map name (a length-prefixed string), so a longer name shifts the header: update that chunk's size in the header chunk table and `userDataSize`, then write the body sizes at the shifted offset | tmtracknn2 writer | verified |
| Header chunk `0x03043005` is the XML header (`<header type="challenge" version="TMc.6" exever="2.11.26"><ident uid name author/><desc envir="Stadium" mood type="Race" nblaps="0"/><times .../></header>`); rewrite it and its size together | tmtracknn2 writer | verified |
| UID: 27 random alphanumeric characters (same length as the template's, so no offsets move). The author name in the body is a fixed 8-byte lookback string in the template (`donadigo`); replacing it without moving offsets means padding or truncating to 8 bytes | tmtracknn2 writer | verified |
| donadigo's template contains a password chunk `0x03043029` (16-byte MD5, all zero, plus a non-zero CRC) that makes every derived map password protected with no password that matches. It is skippable (`SKIP` marker), so the writer removes it; `0x03043014` is the older plaintext form | tmtracknn2 writer; [game] | verified |
| The body is LZO1X-compressed (python-lzo, level 1, no header). An LZO-free writer could store a literal-only LZO stream, which the game also loads (see gbx.md) | tmtracknn2; gbx.md | verified / idea |
| Reader -> writer round trip of real maps returns identical block ids, positions and rotations (18/18 usable maps, up to 8,629 blocks) | tmtracknn2 | verified |

### Block flags

| Fact | Source | Status |
|---|---|---|
| Flags bits 0-11 select the shape variant (dirt hill slope, pool edge, tube road, RoadMain straight/curve/T), bit 12 ground vs air, 13-14 more state, 15 skin (then skin data follows, which a simple writer does not write: clear it), `0x100000` block parameters (clear it too) | tmtracknn2; [game] | verified |
| **The game renders what the flags say.** The right block with wrong flags shows holes in the ground or looks rotated and unconnected | [game] control test: same maps with original vs computed flags | verified |
| donadigo's `populate_flags` rule (RoadMain/dirt connections from the previous and next block in the *list*, ground bit at y = 1) reproduces the stored flags for 51-58% of blocks; pools and dirt hills 0% | tmtracknn2, 1,467 real maps | verified |
| A lookup on each block's physical neighbourhood in its own frame, mined from real files, with back-off levels (terrain height profile of the 8 surrounding columns, face neighbourhood with/without rotations, occupied faces, `(block, y==0, y==1, rests on terrain)`), reaches 83% held out | tmtracknn2 | verified |
| "Rests on terrain" matters: a road on a dirt hill is a ground block far above y = 1. Without it, 159 of 233 such blocks on an Offroad map came out as floating air pieces | tmtracknn2; [game] | verified |
| `DirtBorder` stays ~0.3 accurate: its sub-variant bits look random (rock variation), not shape | tmtracknn2 | inferred |

### Validation and author time

| Fact | Source | Status |
|---|---|---|
| A generated map has **no real author time**: nobody has driven it. The writer stores an *estimate* (0.75 s per drivable block, at least 18 s; gold ×1.08, silver ×1.2, bronze ×1.5) in the header, the XML and the challenge parameters | tmtracknn2 writer | verified |
| Whether the game treats such a map as validated was not checked. Treat it as unvalidated: drive it in the editor (validation records the real author time) before uploading it to TMX or a server | | unverified |
| Generated maps load in the editor and drive in TMNF | [game] | verified |

## Related

* [TMTrackNN](https://github.com/donadigo/TMTrackNN) and
  [pygbx](https://github.com/donadigo/pygbx) by donadigo (GPL-3.0).
* [docs/tools.md](tools.md) for GBX libraries (GBX.NET can also write maps).
