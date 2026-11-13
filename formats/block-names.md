# Block names per game and environment

Lists of block names that appear in `.Challenge.Gbx` maps (and in the challenge embedded in `.Replay.Gbx` files), split by game and environment, with counts. Status of every Nadeo-map row: **verified (Nadeo map)**.

## Files

One CSV per game and environment in [data/blocks/](../data/blocks/), named `<game>-<environment>.csv`. Columns:

| Column | Meaning |
|---|---|
| `block` | Block name as stored in the map's block list (lookback string) |
| `origin` | `nadeo` = the block appears in at least one map whose author login is `Nadeo`; `community-only` = it appears only in community maps |
| `maps_nadeo`, `placements_nadeo` | Distinct maps (deduplicated by map UID) and total placements in Nadeo maps |
| `maps_community`, `placements_community` | The same for all other maps. Only counts, no map identification |
| `ground_placements`, `air_placements` | Placements with and without flag bit 12 (`0x1000`, "ground"), over all maps |
| `skinned_placements` | Placements with flag bit 15 (`0x8000`, skin/advertisement data follows) |
| `distinct_flag_variants` | Number of distinct values of flag bits 0-11 (shape variant) seen for this name |
| `first_exever`, `last_exever` | Lowest and highest `exever` from the map header among maps that use the block (numeric order, over all maps) |
| `status` | `verified (Nadeo map)` or `community map only (may be a hidden game block placed with an editor extension, or a name written by an external tool)` |

Rows are sorted with Nadeo-verified names first.

## Why two origins

A block name in a map is only a string. The game does not check it when a map is saved by other tools, so a community map can carry names that are not part of the shipped game:

- editor extensions such as TMUnlimiter can place blocks that the stock editor hides (other environments' blocks in Stadium, unreleased or internal blocks);
- external GBX editors and generators can write any name, including names that never existed;
- maps converted from other games can keep foreign environment names.

Therefore the base list is the `nadeo` rows. `community-only` rows are a lead, not a fact about the shipped game: a name may be a hidden game block that really exists, or a string that no game version defines. One visible effect: the shared Stadium list has several hundred community-only names, many of them blocks of other environments (for example Alpine or Island names inside Stadium maps).

## Summary

Maps are unique by map UID. "Blocks" are distinct names.

| Game | Environment | Blocks in Nadeo maps | Community-only blocks | Nadeo maps | Other maps | CSV |
|---|---|---|---|---|---|---|
| Highlands (class 0x03043000, unclassified) | Highlands | 0 | 361 | 0 | 65 | [highlands-unclassified-highlands.csv](../data/blocks/highlands-unclassified-highlands.csv) |
| TM Nations ESWC / Nations (legacy format) | Stadium | 206 | 8 | 145 | 323 | [nations-eswc-stadium.csv](../data/blocks/nations-eswc-stadium.csv) |
| TM Original (legacy format) | Alpine | 123 | 0 | 24 | 2 | [original-alpine.csv](../data/blocks/original-alpine.csv) |
| TM Original (legacy format) | Rally | 124 | 0 | 25 | 0 | [original-rally.csv](../data/blocks/original-rally.csv) |
| TM Original (legacy format) | Speed | 103 | 0 | 29 | 0 | [original-speed.csv](../data/blocks/original-speed.csv) |
| TM Sunrise (legacy format) | Bay | 174 | 0 | 30 | 2 | [sunrise-bay.csv](../data/blocks/sunrise-bay.csv) |
| TM Sunrise (legacy format) | Coast | 172 | 1 | 22 | 1 | [sunrise-coast.csv](../data/blocks/sunrise-coast.csv) |
| TM Sunrise (legacy format) | Island | 188 | 10 | 27 | 3 | [sunrise-island.csv](../data/blocks/sunrise-island.csv) |
| TMNF / TMUF (shared Stadium) | Stadium | 269 | 596 | 203 | 38566 | [tmnf-tmuf-stadium.csv](../data/blocks/tmnf-tmuf-stadium.csv) |
| TMUF (other environments) | Alpine | 72 | 96 | 7 | 24 | [tmuf-alpine.csv](../data/blocks/tmuf-alpine.csv) |
| TMUF (other environments) | Bay | 108 | 80 | 11 | 23 | [tmuf-bay.csv](../data/blocks/tmuf-bay.csv) |
| TMUF (other environments) | Coast | 84 | 103 | 7 | 23 | [tmuf-coast.csv](../data/blocks/tmuf-coast.csv) |
| TMUF (other environments) | Island | 107 | 102 | 8 | 27 | [tmuf-island.csv](../data/blocks/tmuf-island.csv) |
| TMUF (other environments) | Rally | 94 | 68 | 6 | 23 | [tmuf-rally.csv](../data/blocks/tmuf-rally.csv) |
| TMUF (other environments) | Speed | 86 | 52 | 9 | 25 | [tmuf-speed.csv](../data/blocks/tmuf-speed.csv) |

The Highlands maps use the `0x03043000` class with environment `Highlands`; no Nadeo-authored map of it was found, so all its names are community-only.

## Method

1. List every `*.Challenge.Gbx` and `*.Replay.Gbx` in the scanned locations.
2. Read the header XML only and take the map UID (`<ident uid>` for maps, `<challenge uid>` for replays). Keep one file per UID; challenge files are preferred over replays.
3. Replays: decompress the replay body (LZO1X) and take the embedded `GBX` challenge that starts in it. Challenge files are used as they are.
4. Parse the map: header XML (`version`, `exever`, `<ident author>`, `<desc envir>`), then decompress the body and read the block-list chunk (`0x0304301F` for class `0x03043000`, `0x2400301F` for the legacy class `0x24003000`): lookback strings, `u8 direction, x, y, z`, `u32 flags`; for flag bit 15 the author lookback string and the skin node (a node index already seen earlier is a back-reference with no data) follow. See [gbx.md](gbx.md).
5. A map counts as Nadeo when the header author login is `Nadeo` (case-insensitive).
6. Game from the class id and environment: class `0x03043000` with Stadium = TMNF/TMUF (the same files, not separable by header), with Island/Bay/Coast/Alpine/Rally/Speed = TMUF; class `0x24003000` with Stadium = Nations ESWC / Nations, Island/Bay/Coast = Sunrise, Alpine/Rally/Speed = Original.
7. Aggregate per block name: maps using it, placements, flag bits, `exever` range.

All files were treated as untrusted data and parsed read-only with a pure-Python LZO1X decompressor. The repository contains only the aggregated names and counts, never game files, maps or replays.

## Sources

Scanned: 351,996 map and replay files in total (about 40,000 unique maps after deduplication by UID): 328,395 in a folder of downloaded maps and replays, 18,343 in the user's TrackMania documents folder, 4,179 in local game copies and 1,079 in the game installs (read only). Files by game (maps and replays, only counting the files whose map could be parsed):

| Game | Challenge files | Replay files |
|---|---|---|
| TMNF / TMUF Stadium | 43,017 | 301,780 |
| TMUF other environments | 452 | 414 |
| Nations ESWC / Nations | 1,566 | 493 |
| Sunrise | 324 | 387 |
| Original | 290 | 253 |
| Highlands | 130 | 132 |

About 2,540 files (about 520 unique maps) could not be parsed (decompression error, a block-list layout not handled, a block name with characters outside letters, digits and `_-. `) and are not counted. Another 220 files had no readable header.

## Coverage

- A few community-only names that look like personal handles or free-form labels were left out of the CSVs (8 names).
- Only blocks that are used in the maps on the scanning PC. **Blocks that were never placed are missing**, so the lists are lower bounds, not the full block set of a game.
- Nadeo-authored maps are few outside TMNF/TMUF Stadium (single digits to a few dozen per environment), so the Nadeo lists of the TMUF environments and of Nations/Sunrise/Original are the least complete.
- Flag bits are checked on TMNF/TMUF maps. For the legacy class (`0x24003000`) the same bit positions are assumed, not verified.
- The TMNF and TMUF Stadium lists cannot be told apart by file header; the TMUF-only Stadium blocks are inside the shared list.
- ManiaPlanet / TM2020 `.Map.Gbx` files use a different format and are not included.

## How to extend

Scanner scripts are not part of this repository yet. To add data: parse more maps with the steps above, keep the same columns, merge by block name (add map and placement counts, but count each map UID once across the whole data set), and keep the origin rule: a name is `nadeo` only if a Nadeo-authored map contains it.
