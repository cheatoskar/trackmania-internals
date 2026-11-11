# Script inventory

## Copied into this repo (`scripts/`)

Each file starts with a one-line description. All are the author's own code (MIT, see [../LICENSE-MIT](../LICENSE-MIT)). Python needs `pefile`/`capstone` where imported.

| File | Purpose | Origin |
|---|---|---|
| ghidra/ImportMsvcMap.java | pre-script: MSVC `.map` publics -> labels/functions | own Ghidra workspace |
| ghidra/analyze.ps1 | headless import + analysis of TMNF (with map) and TMUF | own Ghidra workspace |
| ghidra/mcp-headless.ps1 | GhidraMCP headless server on 127.0.0.1:8089 | own Ghidra workspace |
| ghidra/Dec.java | read-only decompile of addresses (`x:` xrefs, `r:` data refs) | own Ghidra workspace |
| ghidra/XrefQuery.java | xrefs for strings / addresses / data values | own ESWC RE workspace |
| ghidra/ExportDecompilation.java | decompiled C + asm export | own ESWC RE workspace |
| ghidra/DumpRange.java, FindDisp.java, ConfigureAnalysis.java | listing dump, displacement search, slow analyzers off | own ESWC RE workspace |
| re/port.py | TMNF -> TMUF address port (masked byte search) | own Ghidra workspace |
| re/pe.py, findvt.py, findbytes.py | PE/capstone helpers, vtable and byte search | own ESWC RE workspace |
| re/call_index.py | calls + float constants of functions (exe + map); reference only, needs unpublished helper modules | own physics RE project |
| re/classinfo.py, mwmethod.py, fnmatch.py, racevt.py, vtalign.py, callers.py, immref.py, strref.py, tmfwd.py | reflection table dump (`data/reflection`), cross-build function matching, vtable alignment, xref/string/immediate helpers | own RE workspace |
| re/pe_info.py | size, SHA-256, PE timestamp, image base, ASLR flag of exes | written for this repo |
| re/map_lookup.py | VA -> symbol (or name search) in TmForever.map | written for this repo |
| gbx/gbxhdr.py, hdr.py, ghosts.py, evdur.py, chunks.py | GBX header/ghost/chunk inspection | own ESWC RE workspace |
| gbx/lzo1x.ts | non-GPL LZO1X decompressor | own website code |
| physics/dump_tuning.py | read car tuning/material/wheel data from the running game | own physics RE project |
| physics/to_script.py, script_to_csv.py | fvlab CSV <-> TMInterface script | own physics RE project |

## Worth knowing, not copied

| Project | Purpose | Why not copied |
|---|---|---|
| eswc-autosave (`src/{autosave,tmxcopy,online,loader}.cpp`) | ESWC hooks, C++ single-ghost converter, TLS 1.0 fix, CRT-free dinput8 loader | published separately; read there |
| tm-physics-re (`map_symbols.py`, `port_tmuf.py`, trace helpers) | produced `data/symbols_*.csv`, more RE/trace helpers | project-specific, hard-coded paths |
| ForeverValidator search/probe patches (fvlab) | offline physics search/probe tool | big; upstream MIT project |
| TMNF-C (`tools/ghidra_scripts/*.java`, tracer) | map import, vtable export, inline-hook tracer | third-party (MIT); read there |
| TrackManiaForeverOpenXR (`src/large_address_aware.cpp`) | safe LAA flag patch with backup | third-party; read there |
