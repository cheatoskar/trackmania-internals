# data/

| File | Content | Origin | Status |
|---|---|---|---|
| symbols_tmnf.csv | 122 symbols (`va,class,method,mangled,kind`) of TMNF 4d431494: only the functions that the notes in this repo reference (by address or `Class::Method` name) | derived from Nadeo's `TmForever.map` for this build (a public copy is in [Delorean12DMC/tmnf-plugins](https://github.com/Delorean12DMC/tmnf-plugins)); the map file itself and the full symbol dump are not included | verified |
| symbols_tmuf.csv | the same symbols ported to TMUF 4b9e8add by masked byte match (`va_tmnf,class,method,va_tmuf,confidence,n_bytes,tier,sim256,size_tmnf`): 103 unique, 11 ambiguous, 6 none | own port (tm-physics-re `port_tmuf.py`, not included) | verified for `unique`; others need a manual check |
| model6_calls_consts.md | calls and float constants used by `CSceneVehicleCar::ComputeForcesModel6` and neighbours | [scripts/re/call_index.py](../scripts/re/call_index.py) (capstone, exe + map) | verified |
| reflection/*_classes.json | engine reflection tables (`CMwClassInfo` / member parameter tables) of TMNF (`tmnf`), TMN ESWC 1.80 unofficial (`eswc`) and TM Sunrise Win10 repack (`tms`): class id, class name, init/param table VA, and per member id, name, type and field offset | [scripts/re/classinfo.py](../scripts/re/classinfo.py), static read of the exe; raw table words removed | verified (static) |

Need a symbol that is not listed? Load the map for your own copy with `scripts/ghidra/ImportMsvcMap.java` or look it up with `scripts/re/map_lookup.py`.
