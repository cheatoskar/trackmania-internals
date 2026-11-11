# TMX (TrackMania Exchange) API: endpoints, filters, quirks

Exchanges: `tmnf.exchange`, `tmuf.exchange`, `nations.tm-exchange.com`, `sunrise.tm-exchange.com`, `original.tm-exchange.com` (TMF-era API, `/api/tracks`); ManiaPlanet era: `tm.mania.exchange`, `sm.mania.exchange`, `trackmania.exchange` (`/api/maps`). Docs: api2.mania.exchange; method list api.mania.exchange/api/methods.

Source keys (the author's own projects; file names and line numbers kept for provenance): `srv` = server controller (`TmxRules.cs`, `Presets/*.toml`, `IDEAS.md`); `web` = website TMX client (`tmx-api.ts`, `tmx-uid.ts`, docs); `hub` = TM Horizon launcher (`tmx.ts`, `tmx.rs`); `nc` = own Nostalgic Completionist fork; `plugin` = 100 TMX Bingo plugin (`worker.cpp`). `curl` = manual request on the given date.

## Endpoints

| Endpoint | Notes | Source | Status |
|---|---|---|---|
| `GET /api/tracks?fields=...&<filters>` | `fields` is REQUIRED (missing -> 400); answer `{More: bool, Results: []}` | web, srv | verified |
| `GET /api/replays?trackid=..|userid=..` | one of the two required; open (unfinished) map -> empty list (cheap "still open" check); `best=1&count=10&fields=ReplayTime,Position,ReplayAt,User.Name` = top 10; `Position` is zero-based | web docs; plugin:366 | verified |
| `/api/users`, `/api/trackpacks`, `/api/meta/tags`, `/api/meta/trackorders`, `/api/meta/maporders` | | web; hub | verified |
| `GET /trackgbx/<id>` | map file (filename in Content-Disposition, see encoding trap) | srv | verified |
| `HEAD /trackrandom?<filters>` | random pick = redirect, `Location: .../<trackId>`, no body needed | srv | verified |
| `GET /recordgbx/<replayId>` | replay file; `/replaygbx/`, `/replay/`, `/trackreplay/` are 404 | web fetchReplayFile | verified |
| `GET /trackplay/<id>` | 302 to ManiaCode `tmtp:///:tmx:download:tmnf?token=...`; only TMF registers `tmtp://` (TMN/TMS/TMO links do nothing) | web docs | verified |
| `/trackshow/<id>`, `/trackshow/<id>/image/0` (web) or `/image/1` (hub) | page / images; both indices used, meaning of 0 vs 1 unverified | web; hub | unverified (see contradictions) |
| `GET /api/trackpacks?id=<id>&fields=PackId,PackName,Creator.Name` | pack head; `TrackCount`, `Author.Name` -> 400 "field does not exist"; the maps come from `/api/tracks?packid=<id>` | hub tmx.rs (curl 2026-11-11) | verified |
| MX: `GET /api/mappacks?id=<id>&fields=MappackId,Name,Owner.Name`, maps via `/api/maps?mappackid=<id>`; map fields `MapId,Name,Uploader.Name,Medals.Author,Environment,HasThumbnail,MapUid` (`AuthorTime` -> 400 on MX) | tm.mania.exchange + trackmania.exchange, same shape | hub tmx.rs (curl 2026-11-11) | verified |
| Thumbnails: TMX `/trackshow/<id>/thumbnail` (same JPEG as `/image/0` and `/image/1` for the tested map, also on nations.tm-exchange.com), MX `/mapthumb/<id>`; `/tracks/thumbnail/<id>` is 404 | curl 2026-11-11 (tmnf 13775293, MX 267572) | verified |
| MX paging with `after=` on a pack gave an empty page (order unclear): stop when a page brings nothing new | curl 2026-11-11 | verified (observation) |

## Fields

| Fact | Source | Status |
|---|---|---|
| Dot paths work but are undocumented: `Uploader.Name`, `Uploader.UserId`, `WRReplay.ReplayId`, `User.Name`, `Track.TrackId` | srv IDEAS.md; web | verified |
| `fields=UId` (capital I) selects the map UID; `Uid` is rejected | web | verified |
| `Username`, `Author`, `AuthorLogin` -> 400 "field does not exist"; on replays `Login`, `PlayerLogin`, `TmLogin`, `Nickname` -> 400: TMX never exposes the TM login | web | verified |
| `Owner.Name` is not a trackpack field (400) | web | verified |
| Track fields seen: TrackId, TrackName, Tags[], Difficulty (0 Beginner .. 3 Lunatic), AuthorTime (ms), PrimaryType, AuthorBeatable, UnlimiterVersion, Awards, Environment, Mood, Routes, Length, UploadedAt, Uploader{UserId,Name}, WRReplay{ReplayId}, HasThumbnail, Authors[], UId | web; srv; hub | verified |
| Has-record check: `?id=<id>&fields=TrackId,WRReplay.ReplayId` -> `WRReplay` present | srv:439-464 | verified |
| `ReplayAt`/`UploadedAt` have no timezone suffix but are UTC: append `Z` | web docs; hub | verified |
| Result field `Environment` is 1-based (1 Snow .. 7 Stadium), while the TMUF filter `environment=` is 0-based (0 Snow, 1 Desert, 2 Rally, 3 Island, 4 Coast, 5 Bay, 6 Stadium) | srv Presets/100tmx_tmuf_snow.toml; hub tmx.ts | verified (2026-11-07) |

## Filters (and silent-ignore traps)

| Filter | Fact | Source | Status |
|---|---|---|---|
| `uid=<MapUid>` | works; `trackuid`, `uids` are silently ignored -> 200 + newest tracks | web | verified |
| `packid=<id>` | works; `trackpackid`, `inpack`, `pack`, `trackpack`, `packs`, `inpacks` silently ignored; nonexistent pack -> empty | web | verified |
| `id=a,b,c` | up to 100 ids per call | web | verified |
| `tag=<ids>` | use instead of `primarytype`: `/trackrandom` 404s for any nonzero `primarytype` though documented | srv Presets/short_lol_tmnf.toml | verified |
| `routes=` | listing all four `0,1,2,3` -> HTTP 500 (TMX bug): omit instead | srv presets | verified |
| `inhasrecord=0/1`, `inunlimiter=0`, `uploadedafter=<iso>`, `difficulty=3`, `authortimemax=45000`, `environment=`, `inauthortimebeaten`, `ETag=[Laps]` (exclude tag), `count` (max 1000) | used in presets | srv; nc | verified |
| `inunlimiter=0` | relies on uploader-declared `UnlimiterVersion`; not reliable | web docs | verified |
| Paging | `after=<TrackId of the LAST row of the page>` (following the max id loops forever); stop when a page brings nothing new | web paged(); srv:651-685 | verified |
| Orders | `order1=10` activity newest, `order1=26` awarded most recent, `order1=2` uploaded newest; maps: 40 awarded, 6 latest | srv; hub | verified |
| Tags (`/api/meta/tags`, 19 entries, 2026-08-16) | 0 Race, 1 Stunt, 2 Maze, 3 Offroad, 4 Multilap, 5 FullSpeed, 6 LOL, 7 Tech, 8 SpeedTech, 9 RPG, 10 PressForward, 11 Trial, 12 Grass, 13 Story, 14 Nascar, 15 Speedfun, 16 Endurance, 17 Altered Nadeo, 18 Transitional; same ids on nations.tm-exchange.com (2026-11-09) | srv TagNames | verified |
| `Routes`/Multilap tag vs the GBX | often disagree with GBX `LapRace`/`NbLaps` (6/6 Routes=1 maps had LapRace=false); check `nblaps` in the map header XML | srv IDEAS.md; nc | verified |
| Old games | for TMN/TMS/TMO LAN use only `PrimaryType=Race` maps ("only Race works properly with old LAN server") | nc | verified (claim) |

## Behaviour, rates, outages

| Fact | Source | Status |
|---|---|---|
| No documented rate limit found; clients here use >= 120 ms between requests per site, 5 retries with exponential backoff (1-30 s) on 429/5xx, other 4xx fatal, 45 s timeout | web tmx-api.ts | verified (client policy) |
| `/trackrandom` is uneven: 213 draws in 2 days gave 75 distinct maps from a 289-map pool | server analysis 2026-10-16 | verified (measurement) |
| TMX outages happen (HttpRequestException, 5xx, 429, 403, 408, timeouts): a controller should keep a local map-pool copy and a fallback to local `*.Challenge.Gbx` | srv; ServerSetup.cs | verified |
| A map UID is unique per upload; re-uploads keep the UID -> several hits | web | verified |
| UID lookup narrowing by environment: Stadium -> tmnf, tmn, tmuf; Island/Bay/Coast -> tms, tmuf; Desert/Snow/Rally -> tmo, tmuf | web tmx-uid.ts | verified |
| TMX sites send `X-Frame-Options: SAMEORIGIN` | hub README | verified |
| `account.mania.exchange` speaks OIDC, no dynamic client registration (client_id from MX staff) | web docs | verified |

## Replays and TMX acceptance

| Fact | Source | Status |
|---|---|---|
| TMX reads the header `validable` flag; client-saved online replays are refused, server-saved TMF replays accepted | srv README; own notes | verified |
| TMNF replay header `time` equals TMX `ReplayTime` exactly on every sample | web replay-header.ts | verified |
| TMX refuses a replay slower than your own record | web docs/replay-upload-plan.md | verified |
| TM only writes an autosave for a run that beats your own best on that map | plugin; web | verified |

## Archive CSVs

TMX exchange archive CSVs are headerless: 17 columns (TMO, incl. FirstResultSource), 16 (TMUF/TMN/TMS), 10 (old TMNF sheet). Column order: TrackId, FirstReplayAt, [FirstResultSource,] FirstResultId, FirstFinisherUserId, FirstFinisherName, FirstReplayTime, Tags, TagIds, Difficulty, AuthorTime, PrimaryType, AuthorBeatable, FirstReplayEarnedAuthorMedal, BestReplayTime, AuthorTimeHasBeenBeaten, StoredAuthorBeaten. Legacy TMNF sheet tag ids were offset by +2000. Source: web docs. Status: verified.

Rules for client mods: [../modding/tmx-rules-for-mods.md](../modding/tmx-rules-for-mods.md).
