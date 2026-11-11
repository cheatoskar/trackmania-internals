# Offline TAS research on TMNF / TMUF

This folder collects what I learned while building tool-assisted runs (TAS) of TrackMania Nations Forever (TMNF)
and United Forever (TMUF) on top of a bit-exact physics simulation. It is knowledge, not a download: no input
scripts, no replays.

## TAS runs are tool-assisted

Everything here is about **offline, tool-assisted** play. A TAS run is produced by frame-by-frame input search
(10 ms ticks) and is not a human run. It must never be submitted to a leaderboard or to TMX as a real run, and it
must not be presented as one. I do not cover replay editing, validation tricks or removing TAS traces, and I will
not add them. Use TAS categories where they exist (for example the TAS exchange: https://tmtas.exchange/faq) and
label every run as a TAS.

## Pages

| page | what it answers |
|---|---|
| [workflow.md](workflow.md) | How to TAS a map from scratch: preparation, stage order, short-window searches, trick table, hygiene |
| [searching.md](searching.md) | Search modes and objectives as concepts, how searches cheat, how to verify a hit tick by tick |
| [tools.md](tools.md) | ForeverValidator, my patches, ForeverTAS, TMInterface and its Python client, PhysProbe: how they fit, credit and licence |
| [results.md](results.md) | Generic lessons from my results, with Nadeo maps as examples |

Physics background lives elsewhere in this repository: the bug mechanisms are in
[../physics/bugs/README.md](../physics/bugs/README.md) and the force model in [../physics/model6.md](../physics/model6.md).
My own TMInterface plugin is in [../scripts/tas/PhysProbe/](../scripts/tas/PhysProbe/).

## Status labels

- **verified**: reproduced and checked (the simulation matches a real replay exactly, or an in-game test).
- **inferred**: follows from code or data but was not run end to end.
- **unverified**: a guess or a plan.

Source types: **[sim]** = measured in the simulation on a real replay, **[game]** = in-game test,
**[code]** = read from the game binary, **[community]** = a community source with a link.

## What offline TAS research needs

1. A simulation that replays real replays bit-exactly (ForeverValidator is one).
2. A search tool that mutates inputs inside a short window and scores the result.
3. TMInterface to play a found script in the real game, which is the only final check.
4. Patience with objectives: most of the work is choosing a score that the search cannot cheat.

A search can be driven by hand, by shell scripts, or by an AI assistant calling the tools; the method is the same.
