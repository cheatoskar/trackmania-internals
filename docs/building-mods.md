# Building a mod (with or without AI help)

A practical route from "I want to change something in the game" to a tested mod, for the classic games (TMNF, TMUF, ESWC, Sunrise) and, more briefly, ManiaPlanet/TM2. The addresses live in the per-game tables of this repository; this page is the process around them.

## 1. Pick the game and the exact exe

Addresses belong to one build. TMNF and TMUF share the exe name and version string.

1. Print the identity of your exe: `python scripts/re/pe_info.py <exe>` (PE timestamp, size, SHA-256).
2. Compare it with the [Coverage table in the README](../README.md#coverage). If your build is not listed, the addresses do not apply; you would have to find your own (step 3).
3. Use the matching tables: [TMNF/TMUF](../games/tmnf/engine-hooks.md), [ESWC](../games/tmn-eswc/addresses.md), [Sunrise](../games/tm-sunrise/README.md), [ManiaPlanet](../games/maniaplanet/engine.md).

## 2. Choose a hook route

| Route | Use when | Notes |
|---|---|---|
| TMLoader plugin | TMNF/TMUF, you want the normal ecosystem (profiles, other mods) | DLL must export `_DllMain@12`; start work in a thread, not in `DllMain`. The default profile relocates the exe, so rebase addresses. See [loaders-and-hooks.md](../modding/loaders-and-hooks.md). |
| dinput8 / d3d9 proxy | any 32-bit classic game without a loader; rendering or input mods | a DLL named like the system DLL, placed next to the exe, forwarding every export; loads your code on the first real call. Never import the proxied DLL by name from inside the proxy; load the system copy by full path. |
| ASI loader | quick experiments | an ASI loader masquerading as a game DLL; documented in the same page |
| Openplanet | TM2020 and the ManiaPlanet titles | a scripting platform ([docs](https://openplanet.dev/docs)); try it before any binary hook |
| d3d11 proxy | ManiaPlanet x64 | same pattern as d3d9, 64-bit; light coverage here |

Prefer, in this order: engine reflection by name ([reflection.md](../modding/reflection.md)), vtable slot or IAT patch, then inline hook. Fewer code patches mean fewer conflicts with other mods.

## 3. Find addresses

1. Look in the tables first; every row names build, address form (VA or RVA) and status.
2. For something new, use Ghidra: import a **copy** of the exe and analyse it headless ([tools/ghidra.md](../tools/ghidra.md), scripts in [scripts/ghidra/](../scripts/ghidra/)). Analysis of TMNF took about 8 minutes on a desktop CPU.
3. Search by strings, by reflection names ([data/reflection/](../data/README.md)) and by call sites ([scripts/re/](../scripts/re/)).
4. Verify before you trust: read the first 8-16 bytes of the function in the exe and compare them with what you expect (whole instructions, no relative operands in the part you copy). Write the bytes next to the address in your notes.
5. Relocation: `runtime = module_base - 0x400000 + VA` for 32-bit exes. Never use a VA directly.
6. Another build: do not copy addresses. Port by masked byte match (relocated operands and `call rel32` masked) and compare the result, see [games/tmuf/README.md](../games/tmuf/README.md).

### Ghidra with an MCP server

Optional, but useful for assistants and for fast manual queries. Setup in short (details and pitfalls in [tools/ghidra.md](../tools/ghidra.md)):

- Original GUI plugin: [GhidraMCP](https://github.com/LaurieWired/GhidraMCP) (open the project, start the server from the Tools menu). The [fork with a headless server](https://github.com/bethington/ghidra-mcp) serves a project without the GUI; [scripts/ghidra/mcp-headless.ps1](../scripts/ghidra/mcp-headless.ps1) is a launcher.
- Only one Ghidra process may hold a project; stop the headless server before opening the GUI.
- The server listens on the local machine only. Check it with `curl http://127.0.0.1:8089/check_connection`, then register it in your MCP client (a bridge script ships with the plugin).
- A server started from a background shell may exit at once (stdin closed); start it as a detached process.

## 4. Start from a template

[templates/](../templates/README.md) holds two mods that compile (MSVC, x86) and pass a self-test without a game:

- [tmloader-plugin](../templates/tmloader-plugin/) for TMNF/TMUF through TMLoader 2.12.0 (exports `_DllMain@12`, product folder with `description.yaml`);
- [dinput8-proxy](../templates/dinput8-proxy/) for ESWC 1.80 (unofficial build), Sunrise and TMNF/TMUF standalone (forwards all six dinput8 exports, chain-loads `dinput8.chain.dll`).

Both identify the exe by PE timestamp and file size against a table taken from the [Coverage table](../README.md#coverage), read one known prologue at its rebased address and refuse to do anything if it differs, log to a file next to the DLL, and install one harmless example hook (a frame counter on `IDirect3DDevice9::Present`). The shared [hook helpers](../templates/common/hook.h) cover vtable/IAT slots and inline hooks with an exact prologue check and a trampoline. Build with `templates\build.cmd` or CMake; install only into a test setup ([templates/README.md](../templates/README.md#install-for-a-test)).

Notes for your own inline hooks: copy whole instructions only, at least 5 bytes, with no relative operands (`call`/`jmp`/`jcc` rel); compare absolute operands against their rebased value (the SEH handler in `6a ff 68 <handler>`). Guard calls into the game with `__try/__except`. Start work on a thread, never in `DllMain`.

## 5. Test safely

- Work on a **test copy** of the game folder. Never patch the original, never write into a shared install.
- Do not test experimental hooks online. Keep the profile with the mod separate from the one you play with.
- Log what the mod does, and make "unknown exe" or "prologue mismatch" a logged no-op, never a crash.
- Hooks can crash the game at load; test the plain game first, then add one hook at a time.
- If you plan to publish a mod for the TMX community, read [tmx-rules-for-mods.md](../modding/tmx-rules-for-mods.md) first: driving aids, ghost lines and camera/MediaTracker changes are restricted even offline. Do not touch replay files or their validation state; this repository does not document ways around anti-cheat or replay validation.

## Using an AI coding assistant (e.g. Claude Code)

An assistant is good at the grind: reading decompiled functions, writing hook boilerplate, scripting Ghidra, comparing bytes. It is not an oracle for this game.

### Setup

1. Install Ghidra and import a copy of your exe ([tools/ghidra.md](../tools/ghidra.md)).
2. Optional: start a GhidraMCP server (above) and register it in the assistant's MCP configuration under any name (see below). Tools appear only in sessions started after registration.
3. Open the assistant in your mod's repository, with a checkout of this repository next to it (or added to the session as a second folder).
4. Give it an instruction file in your repo (for Claude Code: `CLAUDE.md`). Start from [templates/CLAUDE.md.template](../templates/CLAUDE.md.template): game and exact build (PE timestamp, hash), that addresses are VAs of that build, where the tables are, and the rules below.

#### Setup prompt

Copy this into a new session in an empty folder for your mod:

```text
Clone https://github.com/cheatoskar/trackmania-internals next to this folder (../trackmania-internals).
Read its README.md and templates/README.md. Then ask me which game I have and run
`python ../trackmania-internals/scripts/re/pe_info.py <path to my exe>` on the exe I name, and
compare timestamp, size and hash with the Coverage table; stop if the build is not listed.
Copy the templates folder into this folder and keep the matching template (tmloader-plugin for
TMNF/TMUF with TMLoader, dinput8-proxy otherwise) plus common/, tests/, build.cmd and CMakeLists.txt;
remove the other template and its lines in build.cmd and CMakeLists.txt. Create CLAUDE.md from
CLAUDE.md.template with my game and exact build filled in, build with build.cmd (x86) and run the
self-test.
Stop before installing anything: tell me which files to copy where, and I do it myself on a test copy.
Do not run the game.
```

#### Registering GhidraMCP

[GhidraMCP](https://github.com/LaurieWired/GhidraMCP) consists of a Ghidra plugin (an HTTP server on the local machine) and a Python bridge script that speaks MCP. Register the bridge in your client; paths and the port depend on the plugin version, so check its README. For Claude Code:

```bash
claude mcp add ghidra -- python <path-to>/bridge_mcp_ghidra.py --ghidra-server http://127.0.0.1:<port>/
```

or in a JSON MCP configuration:

```json
{
  "mcpServers": {
    "ghidra": {
      "command": "python",
      "args": ["<path-to>/bridge_mcp_ghidra.py", "--ghidra-server", "http://127.0.0.1:<port>/"]
    }
  }
}
```

Keep the server on `127.0.0.1`. Use project-local registration if your client supports it, so other projects do not see the Ghidra tools, and do not commit a configuration that contains your own paths.

The same pattern works for other domains: [trackmania-map-mcp](https://github.com/cheatoskar/trackmania-map-mcp) (experimental, my own) gives an assistant tools to read and edit TMF maps instead of letting it guess the GBX format. For physics and TAS questions, let it run the offline tools in [tools/fvlab/](../tools/fvlab/README.md) and [scripts/tas/](../scripts/tas/) rather than reasoning about the physics from memory; [DeltaZero](https://github.com/cheatoskar/DeltaZero) shows a headless-physics setup for driving AI.

### Bigger projects: one orchestrator, several helpers

For a mod with several parts (hooks, UI, tests, docs) one long chat gets slow and forgets details. What worked for me:

- **One orchestrator session** on the strongest model (in Claude Code: Opus). It keeps the plan, reads the reports and decides; it does little editing itself.
- **Helpers per bounded task** on a cheaper model (Sonnet; Haiku for tiny, exact edits): "find the race-end address in build X and verify the prologue", "write the settings page", "update the docs". Give each one the files to read, the goal, the checks to run and what not to touch. The orchestrator reviews every result; helpers report confidently even when wrong.
- **Parallel helpers only on separate files**, ideally each in its own git worktree, and merge through the orchestrator. Two helpers editing the same file is the usual way to lose work.
- **Several orchestrators** only for unrelated workstreams (e.g. one for the mod, one for a website), each with its own repository or worktree. They do not share memory: hand over through files in the repo (a plan, a changelog), not through chat.
- **Watch the cost and your machine.** Each helper is a full session; builds and analysis runs from several helpers at once can make the PC unusable. Bundle related tasks into one helper instead of spawning many.

### Good practices

- **One change at a time.** One hook, one test, one commit. Small steps make a crash attributable.
- **Verify every address yourself in Ghidra.** Open the function, read the first bytes, compare with the table and with the exe. Ask the assistant to show the bytes, then check them independently.
- **Make it cite sources.** Every address it proposes must name a table row, a reflection entry or a Ghidra query result; "I think it is at ..." is not enough.
- **Never let it run the game online.** Run the game yourself on the test copy, or let it run only offline test scripts. Do not give it any credentials.
- **Keep secrets out.** No tokens, keys, account names or personal paths in prompts, notes or committed files; check logs before pasting them.
- **Keep exes and assets out of the repo.** Do not let it commit game files, Ghidra projects or decompiled code.
- **Review diffs.** Read what it changed before running it; a mod DLL runs inside the game process.

### Limits

- It **hallucinates addresses, struct offsets and function names**, especially for a build it has not seen, and presents a plausible number as certain. Always check prologue bytes against the exe.
- It does not know which build you have unless you tell it; TMNF/TMUF confusion is easy.
- Decompiler output can be wrong (calling conventions, `thiscall`, stack arguments); a hook built on a misread signature crashes at runtime.
- It cannot judge TMX moderation or anti-cheat questions; follow the written rules.
- Long sessions drift; restate the build and the rules when a session gets long.
