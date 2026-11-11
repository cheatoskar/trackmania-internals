# TMX moderation rules that constrain client mods

What the TMX (TrackMania Exchange) moderators said about client-side features, as relayed to the author. Read this before building any driving aid, overlay or replay tool. All entries: status = verified (moderator statements as relayed, 2026-10-24/25), source = own notes.

| Feature | Verdict | Date |
|---|---|---|
| Showing checkpoints/triggers on a minimap or in the world ("ray mode") | forbidden, even offline | 2026-10-24 |
| Ideal line / brake line / drawing the ghost's path / adherence % | forbidden (TMInterface-class; TMI replays may not be uploaded) | 2026-10-24 |
| Disabling custom cams or MediaTracker clips | forbidden | 2026-10-24 |
| Pausing the game outside the replay editor (photo freeze), even after finish | counts as cheating | 2026-10-24 |
| Plain minimap (no highlights) while racing | first allowed, then forbidden in race; allowed in replay editor and in exported video frames only | 2026-10-25 |
| Split times vs PB / WR ghost (deltas when the game registers a CP) | allowed online | 2026-10-24 |
| OverlapFix action-block priority (boosters/CPs drawn above overlapping blocks) | accepted by the mod author, open question to moderators | 2026-10-24 |
| A runtime "am I online" check as gate | not enough; offline-only features need a startup-locked session (precedent: TMInterface) | 2026-10-24 |

My own rule (not a moderator statement): never edit replay files or their `validable` flag to change their validation status; that is forgery.

Design rule derived: any driving aid sits behind a setting read only at game start; inert all session when off (own notes, status inferred as best practice).
