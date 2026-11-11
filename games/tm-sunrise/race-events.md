# Sunrise race events (TmSunrise.exe Win10 repack, PE timestamp 5cc8f0cf)

Status: **static, not run** (2026-11-11). Found by aligning the CTrackManiaRace / CTrackManiaRace1P vtables of
ESWC 1.80 and Sunrise (`scripts/re/racevt.py`, `scripts/re/vtalign.py`) and checking the call structure.
Hook only behind a prologue check (bytes below); if they differ, keep the event off.

## Vtables

| | ESWC 1.80 | Sunrise |
|---|---|---|
| CTrackManiaRace class info | 0xcae564 | 0xbcc59c |
| CTrackManiaRace vtable | 0xaf1b70 (111 slots) | 0xa4c7f8 (95 slots) |
| CTrackManiaRace1P class info | 0xcae3ac | 0xbcc4bc |
| CTrackManiaRace1P vtable | 0xaf1390 (113 slots) | 0xa4c438 (97 slots) |

From slot 98 on, Sunrise slot = ESWC slot - 16 (anchors: the empty `ret` at ESWC 104 / Sunrise 88, and
identical functions at ESWC 110 / Sunrise 94, similarity 1.00).

## Functions

All thiscall on the race object, `ret 4` (one argument: the player) unless noted.

| Event | ESWC (slot) | Sunrise (slot) | Sunrise prologue (16 bytes) | Evidence |
|---|---|---|---|---|
| CTrackManiaRace::OnStartLine | 0x6d9be0 (102) | 0x7ec550 (86) | `8b 44 24 04 56 8b 70 34 8b 86 28 02 00 00 85 c0` | alignment; same shape, ret 4 |
| CTrackManiaRace::OnFinishLine | 0x6d9f50 (103) | 0x7ec7a0 (87) | `8b 44 24 04 53 55 56 33 db 3b c3 57 8b f9 0f 84` | Race1P::OnFinishLine calls it first, as in ESWC |
| CTrackManiaRace::OnCheckpoint | 0x6d9c80 (105) | 0x7ec5b0 (89) | `83 ec 38 53 55 8b 6c 24 44 56 8b f1 8b 9e e8 02` | alignment; same shape (stack 0x38 vs 0x40) |
| CTrackManiaRace::ResetPlayer | 0x6d9670 (101) | 0x7ebfb0 (85) | `8b 44 24 04 83 ec 08 53 55 8b 68 34 56 57 8b d9` | identical prologue, `ret 0xc` in both |
| CTrackManiaRace1P::OnFinishLine | 0x6c4f30 (103) | 0x7dd680 (87) | `56 8b 74 24 08 57 56 8b f9 e8 12 f1 00 00 8b 76` | calls base 0x7ec7a0, then the same helper sequence as ESWC |
| CTrackManiaRace1P solo race end | 0x6c5060 (104) | 0x7dd7b0 (88) | `83 ec 08 53 55 56 57 8b f1 e8 b2 cc e7 ff 8b 9e` | same shape and call pattern |
| GetLocalPlayerInfo | 0x71c730 | 0x82aeb0 | | called at the same places in both Race1P::OnFinishLine |
| race quit/next helper (`push 2; call`) | 0x71e040 | 0x82c600 | | same position in Race1P::OnFinishLine |

RespawnPlayer (ESWC slot 87) has no reliable Sunrise match yet (best similarity 0.47); poll for respawns instead.

Call-pattern pairs used for the check (ESWC → Sunrise): 0x4955a0 → 0x64f9a0, 0x71b350 → 0x838e00,
0x71c730 → 0x82aeb0, 0x71e040 → 0x82c600, 0x6daa40 → 0x7ecd80.
