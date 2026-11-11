# Contributing

Corrections, new addresses and new findings are welcome as GitHub issues or pull requests.

## Every fact needs

1. **Game and exact build.** Game, version string *and* a way to identify the exe: PE timestamp or SHA-256 (see the table in the [README](README.md#coverage)). TMNF and TMUF share the version string `2.11.26`, so the version string alone is not enough.
2. **Address form.** Say whether it is a link-time VA (image base 0x00400000 for the 32-bit exes, 0x140000000 for ManiaPlanet x64) or an RVA. Never post a runtime address from a relocated process without converting it.
3. **Source.** How you know it: your own analysis (tool and method, e.g. "Ghidra xref from string X", "debugger breakpoint", "byte-compared port from TMNF"), or a public link (repo, wiki page, forum thread) with a short quote at most.
4. **Status.** `verified` (seen in code/data, measured, or re-checked), `inferred` (derived or guessed), or `unverified` (claim only). If your finding disagrees with an existing note, add a row to [contradictions.md](contradictions.md) instead of silently overwriting.

## Please do not submit

- Game files or parts of them: executables, DLLs, assets, shaders, maps, replays, `.map` symbol files, Ghidra projects, memory dumps, or decompiled functions longer than a few illustrative lines.
- Anything about logging in to or authenticating with Nadeo's master servers, impersonating a game client, account or session data, CD keys, or bypassing anti-cheat, replay validation, the Competition Patch, StarForce or other DRM.
- Cheats or anything that gives an advantage in online play or on leaderboards (see [modding/tmx-rules-for-mods.md](modding/tmx-rules-for-mods.md)).
- Personal data: real names, e-mail addresses, IPs, player logins, user ids, or local paths from your machine (use placeholders such as `<game dir>` or `<Documents>`).
- Bulk copies of third-party text. Link to it and quote briefly.

## Style

- Markdown tables, one fact per row, English.
- Keep addresses as `0x` + 8 hex digits (`0x0050F4B0`), offsets as `+0x1C`.
- Scripts go to `scripts/` with a header line naming origin and licence. New scripts are MIT (see [LICENSE-MIT](LICENSE-MIT)); text and data are CC BY 4.0 (see [LICENSE](LICENSE)). By contributing you agree to license your contribution under these terms.
